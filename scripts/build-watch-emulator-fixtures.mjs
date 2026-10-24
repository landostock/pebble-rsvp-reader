import fs from "node:fs/promises";
import path from "node:path";
import { splitPayloadIntoPackets } from "../shared/src/protocol.js";

const root = path.resolve(import.meta.dirname, "..");
const output = process.argv[2] ?? "/tmp/pebble-rsvp-watch-fixtures";
const library = JSON.parse(await fs.readFile(path.join(root, "server/.data/library/library.json"), "utf8"));
const books = library.books.slice(0, 12);
if (books.length === 0) throw new Error("the local library is empty");

function truncated(value, maxBytes) {
  let result = "";
  for (const character of String(value ?? "")) {
    if (Buffer.byteLength(result + character, "utf8") > maxBytes) break;
    result += character;
  }
  return Buffer.from(result, "utf8");
}

function putText(buffer, offset, value, maxBytes) {
  const bytes = truncated(value, maxBytes);
  buffer[offset] = bytes.length;
  bytes.copy(buffer, offset + 1);
}

function uuidBytes(uuid) {
  return Buffer.from(uuid.replaceAll("-", ""), "hex");
}

function libraryPayload() {
  const entryBytes = 364;
  const payload = Buffer.alloc(12 + books.length * entryBytes);
  payload.write("LIBR", 0, "ascii");
  payload[4] = 1;
  payload[5] = books.length;
  payload.writeUInt32BE(library.books.length, 8);
  books.forEach((book, index) => {
    const offset = 12 + index * entryBytes;
    uuidBytes(book.book_id).copy(payload, offset);
    payload.writeUInt32BE(book.revision, offset + 16);
    putText(payload, offset + 20, book.title, 48);
    putText(payload, offset + 69, (book.authors ?? []).join(", "), 24);
    payload.writeUInt32BE(book.progress_token_index ?? 0, offset + 94);
    payload.writeUInt32BE(book.token_count ?? 0, offset + 98);
    payload[offset + 102] = Math.max(0, Math.min(100, book.progress_percentage ?? 0));
  });
  return payload;
}

const selected = books[0];
const readerPackage = JSON.parse(
  await fs.readFile(path.join(root, `server/.data/books/${selected.book_id}/reader-package.bin`), "utf8"),
);

function readerPayload(start = 0) {
  const tokens = readerPackage.tokens.slice(start, start + 64);
  const records = tokens.map((token, index) => {
    const display = truncated(token.display, 64);
    const record = Buffer.alloc(4 + display.length);
    const previous = start + index > 0 ? readerPackage.tokens[start + index - 1] : null;
    const focus = Number.isInteger(token.focus_char_index) && token.focus_char_index >= 0 && token.focus_char_index <= 15
      ? 0x80 | (token.focus_char_index << 3)
      : 0;
    record[0] = display.length;
    record[1] = (token.paragraph_end ? 1 : 0) |
      (!previous || previous.chapter_id !== token.chapter_id ? 2 : 0) |
      (!previous || previous.sentence_id !== token.sentence_id ? 4 : 0) |
      focus;
    record.writeUInt16BE(Math.max(0, Math.min(0xffff, token.duration_ms_at_300wpm ?? 200)), 2);
    display.copy(record, 4);
    return record;
  });
  const payload = Buffer.alloc(49 + records.reduce((total, record) => total + record.length, 0));
  payload.write("RWND", 0, "ascii");
  payload[4] = 2;
  payload.writeUInt16BE(tokens.length, 6);
  payload.writeUInt32BE(start, 8);
  payload.writeUInt32BE(readerPackage.tokens.length, 12);
  putText(payload, 16, selected.title, 32);
  let offset = 49;
  records.forEach((record) => {
    record.copy(payload, offset);
    offset += record.length;
  });
  return payload;
}

function chapterPayload() {
  const chapters = readerPackage.chapters.slice(0, 64);
  const payload = Buffer.alloc(8 + chapters.length * 29);
  payload.write("CHAP", 0, "ascii");
  payload[4] = 1;
  payload[5] = chapters.length;
  payload.writeUInt16BE(readerPackage.chapters.length, 6);
  chapters.forEach((chapter, index) => {
    const offset = 8 + index * 29;
    payload.writeUInt32BE(chapter.token_start, offset);
    putText(payload, offset + 4, chapter.title, 24);
  });
  return payload;
}

async function writePackets(name, payload, book = null, requestId = 1, chunkId = requestId) {
  const directory = path.join(output, name);
  await fs.rm(directory, { recursive: true, force: true });
  await fs.mkdir(directory, { recursive: true });
  const packets = splitPayloadIntoPackets({
    requestId,
    bookId: book?.book_id ?? "00000000-0000-0000-0000-000000000000",
    bookRevision: book?.revision ?? 0,
    chunkId,
    payload,
  });
  await Promise.all(packets.map((packet, index) =>
    fs.writeFile(path.join(directory, `${String(index).padStart(2, "0")}.bin`), packet),
  ));
  return packets.length;
}

await fs.mkdir(output, { recursive: true });
const counts = {
  library: await writePackets("library", libraryPayload(), null, 1),
  reader: await writePackets("reader", readerPayload(), selected, 2, 0),
  readerNext: await writePackets("reader-next", readerPayload(64), selected, 4, 64),
  chapters: await writePackets("chapters", chapterPayload(), selected, 3, 0),
};
console.log(JSON.stringify({ output, selected: selected.title, counts }, null, 2));
