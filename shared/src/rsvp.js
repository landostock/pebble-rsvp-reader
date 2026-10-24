const LONG_WORD_AFTER_CHARS = 6;
const LONG_WORD_PERCENT_PER_CHAR = 6;
const VERY_LONG_WORD_AFTER_CHARS = 10;
const VERY_LONG_WORD_PERCENT_PER_CHAR = 9;
const ULTRA_LONG_WORD_AFTER_CHARS = 14;
const ULTRA_LONG_WORD_PERCENT_PER_CHAR = 12;
const LONG_WORD_MAX_PERCENT = 170;
const COMPOUND_JOINER_PERCENT = 14;
const LONG_COMPOUND_WORD_PERCENT = 18;
const TECHNICAL_CONNECTOR_PERCENT = 8;
const SYLLABLE_BONUS_AFTER_COUNT = 2;
const SYLLABLE_BONUS_PERCENT_PER_GROUP = 10;
const SYLLABLE_BONUS_MAX_PERCENT = 50;
const ALL_CAPS_COMPLEXITY_PERCENT = 14;
const MIXED_TOKEN_COMPLEXITY_PERCENT = 22;
const NUMERIC_TOKEN_COMPLEXITY_PERCENT = 10;
const DENSE_CONNECTOR_COMPLEXITY_PERCENT = 12;
const COMPLEX_WORD_MAX_PERCENT = 85;
const COMMA_PAUSE_PERCENT = 45;
const DASH_PAUSE_PERCENT = 60;
const CLAUSE_PAUSE_PERCENT = 80;
const ELLIPSIS_PAUSE_PERCENT = 135;
const SENTENCE_PAUSE_PERCENT = 135;
const STRONG_SENTENCE_PAUSE_PERCENT = 150;

const WORD_CHARACTER = /[\p{L}\p{N}]/u;
const LETTER = /\p{L}/u;
const DIGIT = /\p{N}/u;
const UPPERCASE = /\p{Lu}/u;
const LOWERCASE = /\p{Ll}/u;

const IGNORED_TRAILING = new Set([
  "\"",
  "'",
  ")",
  "]",
  "}",
  "’",
  "”",
  "»",
  "‹",
  "›",
  "“",
  "„",
  "‚",
  "«"
]);

const KNOWN_ABBREVIATIONS = new Set([
  "mr.",
  "mrs.",
  "ms.",
  "dr.",
  "prof.",
  "sr.",
  "jr.",
  "st.",
  "vs.",
  "etc.",
  "e.g.",
  "i.e.",
  "cf.",
  "no.",
  "fig.",
  "eq.",
  "inc.",
  "ltd.",
  "co.",
  "dept.",
  "mt.",
  "ft.",
  "z.b.",
  "bzw.",
  "bspw.",
  "ca.",
  "u.a.",
  "nr."
]);

export const DEFAULT_READING_SETTINGS = Object.freeze({
  wpm: 300,
  pacing: Object.freeze({
    longWordDelayMs: 80,
    complexWordDelayMs: 70,
    numericDelayMs: 110,
    weakPunctuationDelayMs: 120,
    sentencePunctuationDelayMs: 180,
    paragraphDelayMs: 220
  })
});

function isWordCharacter(value) {
  return WORD_CHARACTER.test(value);
}

function isLetter(value) {
  return LETTER.test(value);
}

function isDigit(value) {
  return DIGIT.test(value);
}

function isUppercase(value) {
  return UPPERCASE.test(value);
}

function isLowercase(value) {
  return LOWERCASE.test(value);
}

function isVowel(value) {
  const base = value.normalize("NFD").replace(/\p{M}/gu, "").toLocaleLowerCase();
  return base === "a" || base === "e" || base === "i" || base === "o" || base === "u" || base === "y";
}

function codePointCount(value, predicate) {
  let count = 0;
  for (const codePoint of Array.from(value)) {
    if (predicate(codePoint)) {
      count += 1;
    }
  }
  return count;
}

function connectorCount(value, predicate) {
  const codePoints = Array.from(value);
  let count = 0;
  for (let index = 1; index + 1 < codePoints.length; index += 1) {
    const previous = codePoints[index - 1];
    const current = codePoints[index];
    const next = codePoints[index + 1];
    if (predicate(current) && isWordCharacter(previous) && isWordCharacter(next)) {
      count += 1;
    }
  }
  return count;
}

function isSegmentSeparator(value) {
  return value === "-" || value === "/" || value === "_";
}

function isTechnicalConnector(value) {
  return value === "-" || value === "/" || value === "_" || value === "." || value === "+" || value === "\\";
}

function meaningfulCodePoints(value) {
  const codePoints = Array.from(value);
  while (codePoints.length > 0 && IGNORED_TRAILING.has(codePoints[codePoints.length - 1])) {
    codePoints.pop();
  }
  return codePoints;
}

export function trailingRhythmCharacter(value) {
  const codePoints = meaningfulCodePoints(value);
  if (codePoints.length === 0) {
    return "";
  }

  const trailing = codePoints[codePoints.length - 1];
  switch (trailing) {
    case "、":
    case "，":
      return ",";
    case "。":
      return ".";
    case "！":
      return "!";
    case "：":
      return ":";
    case "；":
      return ";";
    case "？":
      return "?";
    case "–":
    case "—":
      return "-";
    default:
      return trailing;
  }
}

function trailingRepeatedCharacterCount(value, target) {
  const codePoints = meaningfulCodePoints(value);
  let count = 0;
  for (let index = codePoints.length - 1; index >= 0; index -= 1) {
    if (codePoints[index] !== target) {
      break;
    }
    count += 1;
  }
  return count;
}

function endsWithEllipsis(value) {
  return trailingRhythmCharacter(value) === "…" || trailingRepeatedCharacterCount(value, ".") >= 3;
}

export function startsWithLowercaseLetter(value) {
  for (const codePoint of Array.from(value)) {
    if (isLowercase(codePoint)) {
      return true;
    }
    if (isLetter(codePoint)) {
      return false;
    }
  }
  return false;
}

function isDottedInitialism(value) {
  const codePoints = meaningfulCodePoints(value);
  if (codePoints.length <= 1) {
    return false;
  }

  let letterCount = 0;
  let expectLetter = true;
  for (const codePoint of codePoints) {
    if (expectLetter) {
      if (!isLetter(codePoint)) {
        return false;
      }
      letterCount += 1;
      expectLetter = false;
    } else if (codePoint === ".") {
      expectLetter = true;
    } else {
      return false;
    }
  }

  return expectLetter && letterCount >= 2;
}

export function looksLikeAbbreviation(value, nextWordStartsLowercase = false) {
  const comparable = meaningfulCodePoints(value).join("").toLocaleLowerCase();
  if (KNOWN_ABBREVIATIONS.has(comparable)) {
    return true;
  }
  if (!comparable.endsWith(".")) {
    return false;
  }
  if (isDottedInitialism(comparable)) {
    return true;
  }

  const readableLength = codePointCount(comparable, isWordCharacter);
  if (readableLength <= 2) {
    return true;
  }
  return nextWordStartsLowercase && readableLength <= 4;
}

function approximateSyllableGroupCount(value) {
  let groups = 0;
  let letterCount = 0;
  let previousWasVowel = false;
  let previousLetter = "";
  let lastLetter = "";

  for (const codePoint of Array.from(value)) {
    if (!isLetter(codePoint)) {
      previousWasVowel = false;
      continue;
    }

    letterCount += 1;
    const lowered = codePoint.toLocaleLowerCase();
    previousLetter = lastLetter;
    lastLetter = lowered;

    const vowel = isVowel(lowered);
    if (vowel && !previousWasVowel) {
      groups += 1;
    }
    previousWasVowel = vowel;
  }

  if (groups > 1 && letterCount > 3 && lastLetter === "e" && previousLetter !== "l" && previousLetter !== "y") {
    groups -= 1;
  }
  return groups === 0 && letterCount > 0 ? 1 : groups;
}

export function focusOrdinal(length) {
  if (length <= 1) {
    return 0;
  }
  if (length <= 5) {
    return 1;
  }
  if (length <= 9) {
    return 2;
  }
  if (length <= 13) {
    return 3;
  }
  return 4;
}

export function focusCharacterIndex(value) {
  const codePoints = Array.from(value);
  const wordCharacterPositions = [];
  for (let index = 0; index < codePoints.length; index += 1) {
    if (isWordCharacter(codePoints[index])) {
      wordCharacterPositions.push(index);
    }
  }
  if (wordCharacterPositions.length === 0) {
    return codePoints.length === 0 ? -1 : 0;
  }

  return wordCharacterPositions[Math.min(focusOrdinal(wordCharacterPositions.length), wordCharacterPositions.length - 1)];
}

export function lengthBonusPercentForToken(value) {
  const readableLength = codePointCount(value, isWordCharacter);
  if (readableLength === 0) {
    return 0;
  }

  let bonus = 0;
  if (readableLength > LONG_WORD_AFTER_CHARS) {
    bonus += (readableLength - LONG_WORD_AFTER_CHARS) * LONG_WORD_PERCENT_PER_CHAR;
  }
  if (readableLength > VERY_LONG_WORD_AFTER_CHARS) {
    bonus += (readableLength - VERY_LONG_WORD_AFTER_CHARS) * VERY_LONG_WORD_PERCENT_PER_CHAR;
  }
  if (readableLength > ULTRA_LONG_WORD_AFTER_CHARS) {
    bonus += (readableLength - ULTRA_LONG_WORD_AFTER_CHARS) * ULTRA_LONG_WORD_PERCENT_PER_CHAR;
  }

  const joinerCount = connectorCount(value, isSegmentSeparator);
  if (joinerCount > 0) {
    bonus += joinerCount * COMPOUND_JOINER_PERCENT;
    if (readableLength >= VERY_LONG_WORD_AFTER_CHARS) {
      bonus += LONG_COMPOUND_WORD_PERCENT;
    }
  }

  const technicalConnectorCount = connectorCount(value, isTechnicalConnector);
  if (technicalConnectorCount > joinerCount) {
    bonus += (technicalConnectorCount - joinerCount) * TECHNICAL_CONNECTOR_PERCENT;
  }

  return Math.min(LONG_WORD_MAX_PERCENT, bonus);
}

export function complexityBonusPercentForToken(value) {
  let bonus = 0;
  const syllableGroups = approximateSyllableGroupCount(value);
  if (syllableGroups > SYLLABLE_BONUS_AFTER_COUNT) {
    bonus += Math.min(
      SYLLABLE_BONUS_MAX_PERCENT,
      (syllableGroups - SYLLABLE_BONUS_AFTER_COUNT) * SYLLABLE_BONUS_PERCENT_PER_GROUP
    );
  }

  const letterCount = codePointCount(value, isLetter);
  const digitCount = codePointCount(value, isDigit);
  const uppercaseCount = codePointCount(value, isUppercase);
  if (letterCount > 0 && digitCount > 0) {
    bonus += MIXED_TOKEN_COMPLEXITY_PERCENT;
  } else if (digitCount >= 3) {
    bonus += NUMERIC_TOKEN_COMPLEXITY_PERCENT;
  }

  if (uppercaseCount >= 2 && uppercaseCount === letterCount) {
    bonus += ALL_CAPS_COMPLEXITY_PERCENT;
  }

  const technicalConnectorCount = connectorCount(value, isTechnicalConnector);
  if (technicalConnectorCount >= 2) {
    bonus += (technicalConnectorCount - 1) * DENSE_CONNECTOR_COMPLEXITY_PERCENT;
  }

  return Math.min(COMPLEX_WORD_MAX_PERCENT, bonus);
}

export function punctuationPause(value, nextWordStartsLowercase = false) {
  if (endsWithEllipsis(value)) {
    return { kind: "sentence", percent: ELLIPSIS_PAUSE_PERCENT };
  }

  switch (trailingRhythmCharacter(value)) {
    case ",":
      return { kind: "weak", percent: COMMA_PAUSE_PERCENT };
    case "-":
      return { kind: "weak", percent: DASH_PAUSE_PERCENT };
    case ";":
    case ":":
      return { kind: "weak", percent: CLAUSE_PAUSE_PERCENT };
    case ".":
      return looksLikeAbbreviation(value, nextWordStartsLowercase)
        ? { kind: "none", percent: 0 }
        : { kind: "sentence", percent: SENTENCE_PAUSE_PERCENT };
    case "!":
    case "?":
      return { kind: "sentence", percent: STRONG_SENTENCE_PAUSE_PERCENT };
    default:
      return { kind: "none", percent: 0 };
  }
}

function scaledDelay(percent, delayMs) {
  return Math.round((percent * delayMs) / 100);
}

export function numericDelayPercentForToken(value) {
  const digitCount = codePointCount(value, isDigit);
  if (digitCount < 2) {
    return 0;
  }

  // Long numbers are decoded digit by digit and need more time than words of
  // the same visual width. Separators add another small grouping cost.
  const separatorCount = codePointCount(value, (codePoint) =>
    codePoint === "." || codePoint === "," || codePoint === ":" || codePoint === "/"
  );
  return Math.min(190, 65 + (digitCount - 2) * 14 + separatorCount * 12);
}

export function durationForToken(token, nextToken = undefined, settings = DEFAULT_READING_SETTINGS) {
  const display = typeof token === "string" ? token : token.display;
  if (!display) {
    return 0;
  }

  const effectiveSettings = {
    ...DEFAULT_READING_SETTINGS,
    ...settings,
    pacing: { ...DEFAULT_READING_SETTINGS.pacing, ...(settings?.pacing ?? {}) }
  };
  const baseInterval = Math.round(60000 / effectiveSettings.wpm);
  const nextDisplay = typeof nextToken === "string" ? nextToken : nextToken?.display;
  const punctuation = punctuationPause(display, startsWithLowercaseLetter(nextDisplay ?? ""));
  const punctuationDelay =
    punctuation.kind === "sentence"
      ? effectiveSettings.pacing.sentencePunctuationDelayMs
      : effectiveSettings.pacing.weakPunctuationDelayMs;

  let total = baseInterval;
  total += scaledDelay(lengthBonusPercentForToken(display), effectiveSettings.pacing.longWordDelayMs);
  total += scaledDelay(complexityBonusPercentForToken(display), effectiveSettings.pacing.complexWordDelayMs);
  total += scaledDelay(numericDelayPercentForToken(display), effectiveSettings.pacing.numericDelayMs);
  total += scaledDelay(punctuation.percent, punctuationDelay);
  if (typeof token !== "string" && token.paragraph_end) {
    total += effectiveSettings.pacing.paragraphDelayMs;
  }
  return total;
}

export function tokenEndsSentence(token, nextToken = undefined) {
  const display = typeof token === "string" ? token : token.display;
  const nextDisplay = typeof nextToken === "string" ? nextToken : nextToken?.display;
  const pause = punctuationPause(display ?? "", startsWithLowercaseLetter(nextDisplay ?? ""));
  return pause.kind === "sentence";
}

export function adjustWpm(currentWpm, delta) {
  if (delta === 0) {
    return currentWpm;
  }
  const lowWpmMax = 100;
  const lowStep = 10;
  const highStep = 25;
  let next = currentWpm;
  if (delta > 0) {
    next += next < lowWpmMax ? lowStep : highStep;
    if (next > lowWpmMax && currentWpm < lowWpmMax) {
      next = lowWpmMax;
    }
  } else {
    next -= next <= lowWpmMax ? lowStep : highStep;
    if (next < lowWpmMax && currentWpm > lowWpmMax) {
      next = lowWpmMax;
    }
  }
  return Math.max(50, Math.min(900, next));
}
