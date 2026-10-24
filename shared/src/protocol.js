export const PROTOCOL_VERSION = 1;
export const PROTOCOL_MAGIC = "PRSP";
export const HEADER_LENGTH = 46;
export const DEFAULT_APP_MESSAGE_PAYLOAD_BYTES = 160;
const MAX_ACTIVE_TRANSFERS = 8;
const MAX_TRANSFER_SEQUENCE_COUNT = 512;
const MAX_TRANSFER_BYTES = 256 * 1024;
const MAX_SEEN_PACKETS = 2_048;

const MESSAGE_TYPE_TO_CODE = new Map([
  ["data", 1],
  ["ack", 2],
  ["nack", 3],
  ["request", 4],
  ["progress", 5],
  ["settings", 6]
]);

const MESSAGE_CODE_TO_TYPE = new Map(Array.from(MESSAGE_TYPE_TO_CODE, ([key, value]) => [value, key]));

const CRC32_TABLE = new Uint32Array(256);
for (let index = 0; index < CRC32_TABLE.length; index += 1) {
  let value = index;
  for (let bit = 0; bit < 8; bit += 1) {
    value = value & 1 ? 0xedb88320 ^ (value >>> 1) : value >>> 1;
  }
  CRC32_TABLE[index] = value >>> 0;
}

function assertUInt(value, name, max) {
  if (!Number.isInteger(value) || value < 0 || value > max) {
    throw new Error(`${name} must be an integer between 0 and ${max}`);
  }
}

export function crc32(bytes) {
  const buffer = Buffer.from(bytes);
  let crc = 0xffffffff;
  for (const byte of buffer) {
    crc = CRC32_TABLE[(crc ^ byte) & 0xff] ^ (crc >>> 8);
  }
  return (crc ^ 0xffffffff) >>> 0;
}

export function parseUuid(uuid) {
  const normalized = String(uuid).toLowerCase();
  if (!/^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/u.test(normalized)) {
    throw new Error(`invalid UUID: ${uuid}`);
  }
  return Buffer.from(normalized.replace(/-/gu, ""), "hex");
}

export function formatUuid(bytes) {
  const hex = Buffer.from(bytes).toString("hex");
  if (hex.length !== 32) {
    throw new Error("UUID must be 16 bytes");
  }
  return `${hex.slice(0, 8)}-${hex.slice(8, 12)}-${hex.slice(12, 16)}-${hex.slice(16, 20)}-${hex.slice(20)}`;
}

export function encodePacket({
  protocolVersion = PROTOCOL_VERSION,
  messageType,
  requestId,
  bookId = "00000000-0000-0000-0000-000000000000",
  bookRevision = 0,
  chunkId = 0,
  sequenceNumber = 0,
  sequenceCount = 1,
  flags = 0,
  payload = Buffer.alloc(0)
}) {
  if (protocolVersion !== PROTOCOL_VERSION) {
    throw new Error(`unsupported protocol version: ${protocolVersion}`);
  }
  const messageCode = MESSAGE_TYPE_TO_CODE.get(messageType);
  if (!messageCode) {
    throw new Error(`unknown message type: ${messageType}`);
  }

  const payloadBytes = Buffer.from(payload);
  assertUInt(flags, "flags", 0xff);
  assertUInt(requestId, "requestId", 0xffffffff);
  assertUInt(bookRevision, "bookRevision", 0xffffffff);
  assertUInt(chunkId, "chunkId", 0xffffffff);
  assertUInt(sequenceNumber, "sequenceNumber", 0xffff);
  assertUInt(sequenceCount, "sequenceCount", 0xffff);
  assertUInt(payloadBytes.length, "payload length", 0xffff);
  if (sequenceCount === 0 || sequenceNumber >= sequenceCount) {
    throw new Error("sequenceNumber must be less than sequenceCount");
  }

  const packet = Buffer.alloc(HEADER_LENGTH + payloadBytes.length);
  packet.write(PROTOCOL_MAGIC, 0, "ascii");
  packet.writeUInt8(protocolVersion, 4);
  packet.writeUInt8(messageCode, 5);
  packet.writeUInt8(flags, 6);
  packet.writeUInt8(HEADER_LENGTH, 7);
  packet.writeUInt32BE(requestId, 8);
  packet.writeUInt32BE(bookRevision, 12);
  packet.writeUInt32BE(chunkId, 16);
  packet.writeUInt16BE(sequenceNumber, 20);
  packet.writeUInt16BE(sequenceCount, 22);
  packet.writeUInt16BE(payloadBytes.length, 24);
  packet.writeUInt32BE(crc32(payloadBytes), 26);
  parseUuid(bookId).copy(packet, 30);
  payloadBytes.copy(packet, HEADER_LENGTH);
  return packet;
}

export function encodeAckPacket(packet, payload = Buffer.alloc(0)) {
  return encodePacket({
    messageType: "ack",
    requestId: packet.requestId,
    bookId: packet.bookId,
    bookRevision: packet.bookRevision,
    chunkId: packet.chunkId,
    sequenceNumber: packet.sequenceNumber,
    sequenceCount: packet.sequenceCount,
    payload
  });
}

export function encodeNackPacket(packet, reason = "") {
  return encodePacket({
    messageType: "nack",
    requestId: packet.requestId,
    bookId: packet.bookId,
    bookRevision: packet.bookRevision,
    chunkId: packet.chunkId,
    sequenceNumber: packet.sequenceNumber,
    sequenceCount: packet.sequenceCount,
    payload: Buffer.from(reason, "utf8")
  });
}

export function splitPayloadIntoPackets({
  requestId,
  bookId,
  bookRevision = 0,
  chunkId = 0,
  payload,
  maxPayloadLength = DEFAULT_APP_MESSAGE_PAYLOAD_BYTES
}) {
  const payloadBytes = Buffer.from(payload);
  assertUInt(maxPayloadLength, "maxPayloadLength", 0xffff);
  if (maxPayloadLength < 1) {
    throw new Error("maxPayloadLength must be at least 1");
  }

  const sequenceCount = Math.max(1, Math.ceil(payloadBytes.length / maxPayloadLength));
  assertUInt(sequenceCount, "sequenceCount", 0xffff);

  const packets = [];
  for (let sequenceNumber = 0; sequenceNumber < sequenceCount; sequenceNumber += 1) {
    const start = sequenceNumber * maxPayloadLength;
    const end = Math.min(start + maxPayloadLength, payloadBytes.length);
    packets.push(
      encodePacket({
        messageType: "data",
        requestId,
        bookId,
        bookRevision,
        chunkId,
        sequenceNumber,
        sequenceCount,
        payload: payloadBytes.subarray(start, end)
      })
    );
  }
  return packets;
}

export function decodePacket(bytes) {
  const packet = Buffer.from(bytes);
  if (packet.length < HEADER_LENGTH) {
    throw new Error("packet is shorter than the protocol header");
  }
  if (packet.toString("ascii", 0, 4) !== PROTOCOL_MAGIC) {
    throw new Error("packet magic mismatch");
  }
  const protocolVersion = packet.readUInt8(4);
  if (protocolVersion !== PROTOCOL_VERSION) {
    throw new Error(`unsupported protocol version: ${protocolVersion}`);
  }
  const messageType = MESSAGE_CODE_TO_TYPE.get(packet.readUInt8(5));
  if (!messageType) {
    throw new Error("unknown message type code");
  }
  const headerLength = packet.readUInt8(7);
  if (headerLength !== HEADER_LENGTH) {
    throw new Error(`unsupported header length: ${headerLength}`);
  }

  const payloadLength = packet.readUInt16BE(24);
  if (packet.length !== HEADER_LENGTH + payloadLength) {
    throw new Error("packet payload length mismatch");
  }
  const payload = packet.subarray(HEADER_LENGTH);
  const expectedChecksum = packet.readUInt32BE(26);
  const actualChecksum = crc32(payload);
  if (expectedChecksum !== actualChecksum) {
    throw new Error("packet checksum mismatch");
  }

  const sequenceNumber = packet.readUInt16BE(20);
  const sequenceCount = packet.readUInt16BE(22);
  if (sequenceCount === 0 || sequenceNumber >= sequenceCount) {
    throw new Error("invalid packet sequence fields");
  }

  return {
    protocolVersion,
    messageType,
    flags: packet.readUInt8(6),
    requestId: packet.readUInt32BE(8),
    bookRevision: packet.readUInt32BE(12),
    chunkId: packet.readUInt32BE(16),
    sequenceNumber,
    sequenceCount,
    payloadLength,
    checksum: expectedChecksum,
    bookId: formatUuid(packet.subarray(30, 46)),
    payload
  };
}

function transferKey(packet) {
  return `${packet.requestId}:${packet.bookId}:${packet.bookRevision}:${packet.chunkId}`;
}

function packetKey(packet) {
  return `${transferKey(packet)}:${packet.sequenceNumber}`;
}

export class TransferReceiver {
  #seenPackets = new Set();
  #transfers = new Map();

  receive(bytes) {
    const packet = decodePacket(bytes);
    if (packet.messageType !== "data") {
      return { status: "control", packet };
    }
    if (packet.sequenceCount > MAX_TRANSFER_SEQUENCE_COUNT) {
      return {
        status: "nack",
        reason: "transfer_too_large",
        packet,
        response: encodeNackPacket(packet, "transfer_too_large")
      };
    }

    const seenKey = packetKey(packet);
    if (this.#seenPackets.has(seenKey)) {
      return { status: "duplicate", packet, response: encodeAckPacket(packet) };
    }

    const key = transferKey(packet);
    if (!this.#transfers.has(key) && this.#transfers.size >= MAX_ACTIVE_TRANSFERS) {
      return {
        status: "nack",
        reason: "too_many_transfers",
        packet,
        response: encodeNackPacket(packet, "too_many_transfers")
      };
    }
    const transfer =
      this.#transfers.get(key) ??
      {
        nextSequence: 0,
        sequenceCount: packet.sequenceCount,
        byteCount: 0,
        parts: []
      };

    if (packet.sequenceCount !== transfer.sequenceCount) {
      return {
        status: "nack",
        reason: "sequence_count_changed",
        packet,
        response: encodeNackPacket(packet, "sequence_count_changed")
      };
    }

    if (packet.sequenceNumber !== transfer.nextSequence) {
      return {
        status: "nack",
        reason: "out_of_order",
        expectedSequence: transfer.nextSequence,
        packet,
        response: encodeNackPacket(packet, `out_of_order:${transfer.nextSequence}`)
      };
    }
    if (transfer.byteCount + packet.payload.length > MAX_TRANSFER_BYTES) {
      this.#transfers.delete(key);
      return {
        status: "nack",
        reason: "transfer_too_large",
        packet,
        response: encodeNackPacket(packet, "transfer_too_large")
      };
    }

    transfer.parts.push(packet.payload);
    transfer.byteCount += packet.payload.length;
    transfer.nextSequence += 1;
    this.#seenPackets.add(seenKey);
    while (this.#seenPackets.size > MAX_SEEN_PACKETS) {
      this.#seenPackets.delete(this.#seenPackets.values().next().value);
    }
    this.#transfers.set(key, transfer);

    if (transfer.nextSequence === transfer.sequenceCount) {
      this.#transfers.delete(key);
      return { status: "complete", packet, payload: Buffer.concat(transfer.parts), response: encodeAckPacket(packet) };
    }
    return { status: "ack", packet, expectedSequence: transfer.nextSequence, response: encodeAckPacket(packet) };
  }
}
