import assert from "node:assert/strict";
import test from "node:test";
import {
  buildReaderPackageFromChapters,
  buildReaderPackageFromText,
  contextForToken,
  deserializeReaderPackage,
  progressForToken,
  rewindSentence,
  serializeReaderPackage
} from "../src/reader-package.js";

const BOOK_ID = "11111111-2222-4333-8444-555555555555";

test("builds reader package tokens with paragraph and sentence indexes", () => {
  const readerPackage = buildReaderPackageFromText({
    bookId: BOOK_ID,
    title: "German Sample",
    authors: ["Fixture"],
    language: "de",
    text: "„Warum?“, fragte er. Da war’s plötzlich still.\n\nStraßenverkehrsordnung."
  });

  assert.equal(readerPackage.reader_package_version, 1);
  assert.equal(readerPackage.tokens[0].display, "„Warum?“,");
  assert.equal(readerPackage.tokens.at(-1).display, "Straßenverkehrsordnung.");
  assert.equal(readerPackage.paragraphs.length, 2);
  assert.ok(readerPackage.sentences.length >= 2);
});

test("merges isolated punctuation into neighboring reader tokens", () => {
  const readerPackage = buildReaderPackageFromText({
    bookId: BOOK_ID,
    text: "Hallo ! ( Welt ) — Danach."
  });

  assert.deepEqual(readerPackage.tokens.map((token) => token.display), [
    "Hallo!", "(Welt)—", "Danach."
  ]);
});

test("includes opted-in chapter headings as paced reader structure", () => {
  const readerPackage = buildReaderPackageFromChapters({
    bookId: BOOK_ID,
    chapters: [{ title: "Kapitel 7", paragraphs: ["Der Text beginnt."], showHeading: true }]
  });

  assert.deepEqual(readerPackage.tokens.slice(0, 2).map((token) => token.display), ["Kapitel", "7"]);
  assert.equal(readerPackage.tokens[0].chapter_heading, true);
  assert.ok(readerPackage.tokens[1].duration_ms_at_300wpm > readerPackage.tokens[2].duration_ms_at_300wpm);
});

test("rewinds to sentence starts rather than subtracting fixed words", () => {
  const readerPackage = buildReaderPackageFromText({
    bookId: BOOK_ID,
    text: "Er fragte leise. Da war’s plötzlich still. Danach ging er."
  });

  const daIndex = readerPackage.tokens.findIndex((token) => token.display === "Da");
  const stillIndex = readerPackage.tokens.findIndex((token) => token.display === "still.");

  assert.equal(rewindSentence(readerPackage, stillIndex), daIndex);
  assert.equal(rewindSentence(readerPackage, daIndex), 0);
});

test("records a recommended first-reading position without changing token order", () => {
  const readerPackage = buildReaderPackageFromChapters({
    bookId: BOOK_ID,
    chapters: [
      { title: "Copyright", paragraphs: ["Rechtliche Hinweise."] },
      { title: "Kapitel 1", paragraphs: ["Hier beginnt die Geschichte."], recommendedStart: true }
    ]
  });

  assert.equal(readerPackage.metadata.recommended_start_token_index, 2);
  assert.deepEqual(readerPackage.tokens.map((token) => token.display), [
    "Rechtliche", "Hinweise.", "Hier", "beginnt", "die", "Geschichte."
  ]);
});

test("serializes and rejects unknown reader package versions", () => {
  const readerPackage = buildReaderPackageFromText({
    bookId: BOOK_ID,
    text: "Ein kurzer Satz."
  });
  const serialized = serializeReaderPackage(readerPackage);
  assert.deepEqual(deserializeReaderPackage(serialized), readerPackage);

  const incompatible = Buffer.from(JSON.stringify({ ...readerPackage, reader_package_version: 999 }), "utf8");
  assert.throws(() => deserializeReaderPackage(incompatible), /unsupported reader package version/u);
});

test("derives canonical progress and paragraph context", () => {
  const readerPackage = buildReaderPackageFromText({
    bookId: BOOK_ID,
    revision: 2,
    text: "Ein kurzer Satz.\n\nNoch einer."
  });
  const progress = progressForToken(readerPackage, 3, new Date("2026-08-29T12:00:00Z"));
  const context = contextForToken(readerPackage, 3);

  assert.equal(progress.book_id, BOOK_ID);
  assert.equal(progress.book_revision, 2);
  assert.equal(progress.token_index, 3);
  assert.equal(context.text, "Noch einer.");
  assert.equal(context.local_token_index, 0);
});
