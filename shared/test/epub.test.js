import assert from "node:assert/strict";
import test from "node:test";
import { deserializeReaderPackage } from "../src/reader-package.js";
import {
  extractEpubCover,
  normalizeEpubChapterTitles,
  parseEpub,
  processEpubToReaderPackage
} from "../src/epub.js";
import { createEpubFixture } from "./zip-fixture.js";

const BOOK_ID = "22222222-3333-4444-8555-666666666666";

test("extracts EPUB metadata and rejects encrypted EPUBs", () => {
  const parsed = parseEpub(createEpubFixture(), {
    bookId: BOOK_ID,
    sourceName: "spine-test.epub"
  });

  assert.equal(parsed.metadata.title, "Spine Test");
  assert.deepEqual(parsed.metadata.authors, ["Fixture Author"]);
  assert.equal(parsed.metadata.language, "de");
  assert.equal(parsed.opfPath, "OEBPS/content.opf");
  assert.match(parsed.sourceHash, /^[0-9a-f]{64}$/u);

  assert.throws(
    () => parseEpub(createEpubFixture({ encrypted: true }), { bookId: BOOK_ID }),
    /encrypted EPUBs are not supported/u
  );
});

test("extracts EPUB 3 manifest cover images", () => {
  const coverBytes = Buffer.from("epub3 cover", "utf8");
  const parsed = parseEpub(createEpubFixture({ cover: "properties", coverBytes }), {
    bookId: BOOK_ID,
    sourceName: "spine-test.epub"
  });

  assert.equal(parsed.cover.contentType, "image/jpeg");
  assert.equal(parsed.cover.path, "OEBPS/images/cover.jpg");
  assert.equal(parsed.cover.source, "manifest-properties");
  assert.deepEqual(parsed.cover.bytes, coverBytes);
});

test("extracts EPUB 2 metadata cover images", () => {
  const coverBytes = Buffer.from("epub2 cover", "utf8");
  const parsed = parseEpub(createEpubFixture({ cover: "metadata", coverBytes }), {
    bookId: BOOK_ID,
    sourceName: "spine-test.epub"
  });

  assert.equal(parsed.cover.contentType, "image/jpeg");
  assert.equal(parsed.cover.path, "OEBPS/images/cover.jpg");
  assert.equal(parsed.cover.source, "metadata-cover");
  assert.deepEqual(parsed.cover.bytes, coverBytes);
});

test("extracts EPUB guide cover pages", () => {
  const coverBytes = Buffer.from("guide cover", "utf8");
  const parsed = parseEpub(createEpubFixture({ cover: "guide-page", coverBytes }), {
    bookId: BOOK_ID,
    sourceName: "spine-test.epub"
  });

  assert.equal(parsed.cover.contentType, "image/jpeg");
  assert.equal(parsed.cover.path, "OEBPS/images/frontispiece.jpg");
  assert.equal(parsed.cover.source, "guide-page");
  assert.deepEqual(parsed.cover.bytes, coverBytes);
});

test("falls back to cover-like image filenames", () => {
  const coverBytes = Buffer.from("filename cover", "utf8");
  const parsed = parseEpub(createEpubFixture({ cover: true, coverBytes }), {
    bookId: BOOK_ID,
    sourceName: "spine-test.epub"
  });

  assert.equal(parsed.cover.contentType, "image/jpeg");
  assert.equal(parsed.cover.source, "filename");
  assert.deepEqual(parsed.cover.bytes, coverBytes);
});

test("extracts covers without building reader content", () => {
  const coverBytes = Buffer.from("cover only", "utf8");
  const cover = extractEpubCover(createEpubFixture({ cover: "metadata", coverBytes }));

  assert.equal(cover.contentType, "image/jpeg");
  assert.equal(cover.path, "OEBPS/images/cover.jpg");
  assert.deepEqual(cover.bytes, coverBytes);
});

test("honors OPF spine order instead of ZIP or filename order", () => {
  const result = processEpubToReaderPackage(createEpubFixture(), {
    bookId: BOOK_ID,
    sourceName: "spine-test.epub"
  });

  const displays = result.readerPackage.tokens.map((token) => token.display);
  const secondChapterIndex = displays.indexOf("Er");
  const firstChapterIndex = displays.indexOf("Da");

  assert.ok(secondChapterIndex >= 0);
  assert.ok(firstChapterIndex >= 0);
  assert.ok(secondChapterIndex < firstChapterIndex);
  assert.deepEqual(
    result.readerPackage.chapters.map((chapter) => chapter.title),
    ["Zweites Kapitel", "Erstes Kapitel"]
  );
});

test("preserves German text fidelity while removing non-reading HTML", () => {
  const result = processEpubToReaderPackage(createEpubFixture(), {
    bookId: BOOK_ID,
    sourceName: "spine-test.epub"
  });
  const text = result.readerPackage.tokens.map((token) => token.display).join(" ");

  assert.match(text, /Er sagte: „Hallo!“/u);
  assert.match(text, /Donaudampfschifffahrtsgesellschaft/u);
  assert.match(text, /Straßenverkehrsordnung\./u);
  assert.match(text, /Da war’s plötzlich still\./u);
  assert.match(text, /Abbildung: Der stille Platz\./u);
  assert.match(text, /E-Mail, Jean-Paul und Nord-Süd-Verbindung/u);
  assert.match(text, /Schon komisch, die Welt ist schön\./u);
  assert.doesNotMatch(text, /\bd ie\b/u);
  assert.doesNotMatch(text, /Table of contents/u);
  assert.doesNotMatch(text, /font-family/u);
});

test("serializes processed reader packages that can be read back", () => {
  const result = processEpubToReaderPackage(createEpubFixture(), {
    bookId: BOOK_ID,
    revision: 4,
    sourceName: "spine-test.epub"
  });
  const readerPackage = deserializeReaderPackage(result.readerPackageBytes);

  assert.equal(readerPackage.metadata.book_id, BOOK_ID);
  assert.equal(readerPackage.metadata.revision, 4);
  assert.equal(readerPackage.tokens[0].chapter_id, 0);
  assert.equal(readerPackage.tokens.at(-1).chapter_id, 1);
});

test("replaces implementation filenames without renumbering authored chapter headings", () => {
  const longText = Array.from({ length: 130 }, (_, index) => `Wort${index}`).join(" ");
  const normalized = normalizeEpubChapterTitles([
    { title: "part0001", paragraphs: ["Titelblatt"] },
    { title: "part0005", paragraphs: [longText] },
    { title: "part0006", paragraphs: [longText] },
    { title: "14", paragraphs: [longText] },
    { title: "part0035", paragraphs: ["Danksagung an alle Beteiligten."] }
  ], "de");

  assert.deepEqual(normalized.map((chapter) => chapter.title), [
    "Vorspann", "Vorspann", "Vorspann", "14", "Danksagung"
  ]);
});

test("numbers generic body files by reading order and leaves trailing files as appendices", () => {
  const longText = Array.from({ length: 600 }, (_, index) => `Wort${index}`).join(" ");
  const normalized = normalizeEpubChapterTitles([
    { title: "part0001", paragraphs: ["Titelblatt"] },
    { title: "part0005", paragraphs: [longText] },
    { title: "part0006", paragraphs: [longText] },
    { title: "part0035", paragraphs: ["Kurzer Nachspann"] }
  ], "de");

  assert.deepEqual(normalized.map((chapter) => chapter.title), [
    "Vorspann", "Kapitel 1", "Kapitel 2", "Anhang"
  ]);
});

test("recognizes time-like implementation labels and front-matter text", () => {
  const longText = Array.from({ length: 130 }, (_, index) => `Wort${index}`).join(" ");
  const normalized = normalizeEpubChapterTitles([
    { title: "14:16", paragraphs: ["Copyright ISBN 1234 Alle Rechte vorbehalten."] },
    { title: "part0002", paragraphs: [longText] },
    { title: "part0003", paragraphs: [longText] }
  ], "de");

  assert.deepEqual(normalized.map((chapter) => chapter.title), ["Vorspann", "Kapitel 1", "Kapitel 2"]);
  assert.equal(normalized[0].showHeading, false);
  assert.equal(normalized[1].showHeading, true);
});

test("does not confuse zero-padded page labels with authored numeric chapters", () => {
  const body = Array.from({ length: 130 }, (_, index) => `Wort${index}`).join(" ");
  const normalized = normalizeEpubChapterTitles([
    { title: "002", paragraphs: ["Eine Zusammenfassung vor dem eigentlichen Buch."] },
    { title: "006", paragraphs: ["Copyright"] },
    { title: "1", paragraphs: [body] },
    { title: "2", paragraphs: [body] }
  ], "de");

  assert.deepEqual(normalized.map((chapter) => chapter.title), [
    "Vorspann", "Vorspann", "1", "2"
  ]);
});
