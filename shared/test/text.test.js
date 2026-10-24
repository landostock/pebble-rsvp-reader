import assert from "node:assert/strict";
import test from "node:test";
import { normalizeParagraphs, normalizeReaderText } from "../src/unicode.js";

test("decodes HTML entities and preserves German punctuation", () => {
  assert.equal(normalizeReaderText("Er&nbsp;sagte: &bdquo;Hallo!&ldquo;"), "Er sagte: „Hallo!“");
});

test("normalizes NFC without spelling correction", () => {
  const decomposed = "gro\u0308ßer und a\u0308ußerst";
  assert.equal(normalizeReaderText(decomposed), "größer und äußerst");
});

test("removes layout soft hyphens", () => {
  assert.equal(normalizeReaderText("Stra\u00adßenverkehrsordnung"), "Straßenverkehrsordnung");
});

test("reconstructs confident line-break hyphenation but preserves lexical hyphens", () => {
  const input = "Donau-\ndampfschifffahrt\n\nE-\nMail Jean-\nPaul Nord-\nSüd";
  assert.deepEqual(normalizeParagraphs(input), [
    "Donaudampfschifffahrt",
    "E-Mail Jean-Paul Nord-Süd"
  ]);
});
