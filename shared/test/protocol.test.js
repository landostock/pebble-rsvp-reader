import assert from "node:assert/strict";
import test from "node:test";
import {
  TransferReceiver,
  decodePacket,
  encodeAckPacket,
  encodeNackPacket,
  encodePacket,
  splitPayloadIntoPackets
} from "../src/protocol.js";

const BOOK_ID = "aaaaaaaa-bbbb-4ccc-8ddd-eeeeeeeeeeee";

test("encodes and decodes protocol packets", () => {
  const encoded = encodePacket({
    messageType: "data",
    requestId: 42,
    bookId: BOOK_ID,
    bookRevision: 7,
    chunkId: 3,
    sequenceNumber: 0,
    sequenceCount: 1,
    payload: Buffer.from("Hallo", "utf8")
  });
  const decoded = decodePacket(encoded);

  assert.equal(decoded.protocolVersion, 1);
  assert.equal(decoded.messageType, "data");
  assert.equal(decoded.requestId, 42);
  assert.equal(decoded.bookId, BOOK_ID);
  assert.equal(decoded.payload.toString("utf8"), "Hallo");
});

test("encodes and decodes reader settings packets", () => {
  const encoded = encodePacket({
    messageType: "settings",
    requestId: 43,
    payload: Buffer.from([1, 1, 2])
  });

  const decoded = decodePacket(encoded);
  assert.equal(decoded.messageType, "settings");
  assert.deepEqual(decoded.payload, Buffer.from([1, 1, 2]));
});

test("bounds fragmented inbound transfers", () => {
  const receiver = new TransferReceiver();
  const oversized = encodePacket({
    messageType: "data",
    requestId: 44,
    sequenceNumber: 0,
    sequenceCount: 513
  });

  const result = receiver.receive(oversized);
  assert.equal(result.status, "nack");
  assert.equal(result.reason, "transfer_too_large");
});

test("rejects corrupt packet payloads", () => {
  const encoded = encodePacket({
    messageType: "data",
    requestId: 1,
    bookId: BOOK_ID,
    payload: Buffer.from("abc", "utf8")
  });
  encoded[encoded.length - 1] ^= 0xff;
  assert.throws(() => decodePacket(encoded), /checksum mismatch/u);
});

test("rejects unsupported protocol versions", () => {
  const encoded = encodePacket({
    messageType: "ack",
    requestId: 1,
    bookId: BOOK_ID
  });
  encoded[4] = 99;
  assert.throws(() => decodePacket(encoded), /unsupported protocol version/u);
});

test("detects duplicate and out-of-order transfer packets", () => {
  const first = encodePacket({
    messageType: "data",
    requestId: 9,
    bookId: BOOK_ID,
    chunkId: 12,
    sequenceNumber: 0,
    sequenceCount: 2,
    payload: Buffer.from("Hel", "utf8")
  });
  const second = encodePacket({
    messageType: "data",
    requestId: 9,
    bookId: BOOK_ID,
    chunkId: 12,
    sequenceNumber: 1,
    sequenceCount: 2,
    payload: Buffer.from("lo", "utf8")
  });
  const receiver = new TransferReceiver();

  assert.equal(receiver.receive(second).status, "nack");
  assert.equal(receiver.receive(first).status, "ack");
  assert.equal(receiver.receive(first).status, "duplicate");

  const complete = receiver.receive(second);
  assert.equal(complete.status, "complete");
  assert.equal(complete.payload.toString("utf8"), "Hello");
});

test("creates ACK and NACK control packets for transfer responses", () => {
  const packet = decodePacket(
    encodePacket({
      messageType: "data",
      requestId: 11,
      bookId: BOOK_ID,
      bookRevision: 2,
      chunkId: 5,
      sequenceNumber: 1,
      sequenceCount: 3,
      payload: Buffer.from("part", "utf8")
    })
  );

  const ack = decodePacket(encodeAckPacket(packet));
  const nack = decodePacket(encodeNackPacket(packet, "retry"));

  assert.equal(ack.messageType, "ack");
  assert.equal(ack.requestId, packet.requestId);
  assert.equal(ack.sequenceNumber, packet.sequenceNumber);
  assert.equal(nack.messageType, "nack");
  assert.equal(nack.payload.toString("utf8"), "retry");
});

test("splits payloads into deterministic sequence packets", () => {
  const payload = Buffer.from("abcdefghijklmnopqrstuvwxyz", "utf8");
  const packets = splitPayloadIntoPackets({
    requestId: 77,
    bookId: BOOK_ID,
    bookRevision: 1,
    chunkId: 2,
    payload,
    maxPayloadLength: 10
  });
  const receiver = new TransferReceiver();

  assert.equal(packets.length, 3);
  assert.deepEqual(
    packets.map((packet) => decodePacket(packet).sequenceNumber),
    [0, 1, 2]
  );

  let result;
  for (const packet of packets) {
    result = receiver.receive(packet);
  }
  assert.equal(result.status, "complete");
  assert.equal(result.payload.toString("utf8"), payload.toString("utf8"));
});
