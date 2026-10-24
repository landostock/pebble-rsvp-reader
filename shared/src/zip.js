import { inflateRawSync } from "node:zlib";

const EOCD_SIGNATURE = 0x06054b50;
const CENTRAL_DIRECTORY_SIGNATURE = 0x02014b50;
const LOCAL_FILE_SIGNATURE = 0x04034b50;
const MAX_ENTRY_COUNT = 10_000;
const MAX_ENTRY_BYTES = 32 * 1024 * 1024;
const MAX_ARCHIVE_BYTES = 256 * 1024 * 1024;
const MAX_COMPRESSION_RATIO = 1_000;

function normalizeZipPath(value) {
  return String(value).replace(/\\/gu, "/").replace(/^\/+/u, "");
}

function findEndOfCentralDirectory(buffer) {
  const minimum = 22;
  const maxComment = 0xffff;
  const start = Math.max(0, buffer.length - minimum - maxComment);
  for (let offset = buffer.length - minimum; offset >= start; offset -= 1) {
    if (buffer.readUInt32LE(offset) === EOCD_SIGNATURE) {
      return offset;
    }
  }
  throw new Error("ZIP end of central directory not found");
}

export class ZipArchive {
  #buffer;
  #entries = new Map();

  constructor(bytes) {
    this.#buffer = Buffer.from(bytes);
    this.#readCentralDirectory();
  }

  entryNames() {
    return Array.from(this.#entries.keys()).sort();
  }

  hasEntry(name) {
    return this.#entries.has(normalizeZipPath(name));
  }

  readEntry(name) {
    const entry = this.#entries.get(normalizeZipPath(name));
    if (!entry) {
      throw new Error(`ZIP entry not found: ${name}`);
    }

    const localOffset = entry.localHeaderOffset;
    if (localOffset + 30 > this.#buffer.length) {
      throw new Error(`ZIP local header is outside archive for ${entry.name}`);
    }
    if (this.#buffer.readUInt32LE(localOffset) !== LOCAL_FILE_SIGNATURE) {
      throw new Error(`invalid ZIP local header for ${entry.name}`);
    }
    const localNameLength = this.#buffer.readUInt16LE(localOffset + 26);
    const localExtraLength = this.#buffer.readUInt16LE(localOffset + 28);
    const dataStart = localOffset + 30 + localNameLength + localExtraLength;
    if (dataStart > this.#buffer.length || entry.compressedSize > this.#buffer.length - dataStart) {
      throw new Error(`ZIP entry data extends beyond archive for ${entry.name}`);
    }
    const compressed = this.#buffer.subarray(dataStart, dataStart + entry.compressedSize);

    let output;
    switch (entry.compressionMethod) {
      case 0:
        output = Buffer.from(compressed);
        break;
      case 8:
        output = inflateRawSync(compressed, { maxOutputLength: MAX_ENTRY_BYTES });
        break;
      default:
        throw new Error(`unsupported ZIP compression method ${entry.compressionMethod} for ${entry.name}`);
    }
    if (output.length !== entry.uncompressedSize) {
      throw new Error(`ZIP entry size mismatch for ${entry.name}`);
    }
    return output;
  }

  readTextEntry(name) {
    return this.readEntry(name).toString("utf8");
  }

  #readCentralDirectory() {
    const eocd = findEndOfCentralDirectory(this.#buffer);
    const entryCount = this.#buffer.readUInt16LE(eocd + 10);
    const centralDirectorySize = this.#buffer.readUInt32LE(eocd + 12);
    const centralDirectoryOffset = this.#buffer.readUInt32LE(eocd + 16);
    if (centralDirectoryOffset + centralDirectorySize > this.#buffer.length) {
      throw new Error("ZIP central directory extends beyond archive");
    }
    if (entryCount > MAX_ENTRY_COUNT) {
      throw new Error("ZIP archive contains too many entries");
    }

    let offset = centralDirectoryOffset;
    let totalUncompressedSize = 0;
    for (let index = 0; index < entryCount; index += 1) {
      if (offset + 46 > centralDirectoryOffset + centralDirectorySize || offset + 46 > this.#buffer.length) {
        throw new Error("ZIP central directory entry is truncated");
      }
      if (this.#buffer.readUInt32LE(offset) !== CENTRAL_DIRECTORY_SIGNATURE) {
        throw new Error("invalid ZIP central directory entry");
      }

      const compressionMethod = this.#buffer.readUInt16LE(offset + 10);
      const generalPurposeFlags = this.#buffer.readUInt16LE(offset + 8);
      const compressedSize = this.#buffer.readUInt32LE(offset + 20);
      const uncompressedSize = this.#buffer.readUInt32LE(offset + 24);
      const fileNameLength = this.#buffer.readUInt16LE(offset + 28);
      const extraLength = this.#buffer.readUInt16LE(offset + 30);
      const commentLength = this.#buffer.readUInt16LE(offset + 32);
      const localHeaderOffset = this.#buffer.readUInt32LE(offset + 42);
      const entryEnd = offset + 46 + fileNameLength + extraLength + commentLength;
      if (entryEnd > centralDirectoryOffset + centralDirectorySize || entryEnd > this.#buffer.length) {
        throw new Error("ZIP central directory entry fields are truncated");
      }
      const name = normalizeZipPath(this.#buffer.subarray(offset + 46, offset + 46 + fileNameLength).toString("utf8"));

      if ((generalPurposeFlags & 1) !== 0) {
        throw new Error(`encrypted ZIP entry is not supported: ${name}`);
      }
      if (!name || this.#entries.has(name)) {
        throw new Error(`invalid or duplicate ZIP entry name: ${name}`);
      }
      if (localHeaderOffset + 30 > this.#buffer.length) {
        throw new Error(`ZIP local header offset is outside archive: ${name}`);
      }
      if (uncompressedSize > MAX_ENTRY_BYTES) {
        throw new Error(`ZIP entry exceeds uncompressed size limit: ${name}`);
      }
      if (uncompressedSize > 0 && compressedSize === 0) {
        throw new Error(`ZIP entry has invalid compressed size: ${name}`);
      }
      if (compressedSize > 0 && uncompressedSize / compressedSize > MAX_COMPRESSION_RATIO) {
        throw new Error(`ZIP entry exceeds compression ratio limit: ${name}`);
      }
      totalUncompressedSize += uncompressedSize;
      if (totalUncompressedSize > MAX_ARCHIVE_BYTES) {
        throw new Error("ZIP archive exceeds total uncompressed size limit");
      }

      this.#entries.set(name, {
        name,
        compressionMethod,
        compressedSize,
        uncompressedSize,
        localHeaderOffset
      });

      offset = entryEnd;
    }
  }
}
