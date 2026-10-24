import assert from "node:assert/strict";
import test from "node:test";
import {
  adjustWpm,
  durationForToken,
  focusCharacterIndex,
  looksLikeAbbreviation,
  tokenEndsSentence,
  trailingRhythmCharacter
} from "../src/rsvp.js";

test("calculates Unicode-aware focus positions", () => {
  assert.equal(focusCharacterIndex("a"), 0);
  assert.equal(focusCharacterIndex("word"), 1);
  assert.equal(focusCharacterIndex("größer"), 2);
  assert.equal(focusCharacterIndex("Donaudampfschifffahrtsgesellschaft"), 4);
});

test("detects punctuation through German closing quotes", () => {
  assert.equal(trailingRhythmCharacter("„Warum?“"), "?");
  assert.equal(trailingRhythmCharacter("„Warum?“,"), ",");
});

test("does not treat common abbreviations as sentence endings", () => {
  assert.equal(looksLikeAbbreviation("Dr.", false), true);
  assert.equal(tokenEndsSentence("Dr.", "Müller"), false);
  assert.equal(tokenEndsSentence("still.", undefined), true);
});

test("adds deterministic pacing for long words and sentence punctuation", () => {
  const base = durationForToken("kurz", "weiter");
  const comma = durationForToken("kurz,", "weiter");
  const sentence = durationForToken("kurz.", "Weiter");
  const compound = durationForToken("Donaudampfschifffahrtsgesellschaft", "weiter");
  const number = durationForToken("20261021", "weiter");

  assert.equal(base, 200);
  assert.ok(comma > base);
  assert.ok(sentence > comma);
  assert.ok(compound > base);
  assert.ok(number > durationForToken("abcdefgh", "weiter"));
  assert.equal(durationForToken("kurz.", "Weiter"), sentence);
});

test("uses RSVP Nano style WPM steps", () => {
  assert.equal(adjustWpm(90, 1), 100);
  assert.equal(adjustWpm(100, 1), 125);
  assert.equal(adjustWpm(125, -1), 100);
});
