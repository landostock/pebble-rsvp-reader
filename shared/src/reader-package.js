import { durationForToken, focusCharacterIndex, tokenEndsSentence } from "./rsvp.js";
import { normalizeParagraphs } from "./unicode.js";

export const READER_PACKAGE_VERSION = 1;

function tokenizeParagraph(paragraph) {
  const rawTokens = paragraph.split(/\s+/u).filter(Boolean);
  const tokens = [];
  let pendingPrefix = "";

  for (const rawToken of rawTokens) {
    const punctuationOnly = !/[\p{L}\p{N}]/u.test(rawToken);
    if (!punctuationOnly) {
      tokens.push(`${pendingPrefix}${rawToken}`);
      pendingPrefix = "";
      continue;
    }

    const opensFollowingToken = /^[([{«‹„“‚‘]+$/u.test(rawToken);
    if (opensFollowingToken || tokens.length === 0) {
      pendingPrefix += rawToken;
    } else {
      tokens[tokens.length - 1] += rawToken;
    }
  }

  if (pendingPrefix && tokens.length > 0) {
    tokens[tokens.length - 1] += pendingPrefix;
  }
  return tokens;
}

function sameHeadingText(left, right) {
  const normalize = (value) => String(value ?? "")
    .normalize("NFKC")
    .toLocaleLowerCase()
    .replace(/[^\p{L}\p{N}]+/gu, " ")
    .trim();
  return normalize(left) !== "" && normalize(left) === normalize(right);
}

function percentage(tokenIndex, tokenCount) {
  if (tokenCount <= 0) {
    return 0;
  }
  return Math.round((Math.min(tokenIndex, tokenCount) / tokenCount) * 10000) / 100;
}

export function buildReaderPackageFromText({
  bookId,
  revision = 1,
  title = "Untitled",
  authors = [],
  language = "und",
  chapterTitle = title,
  text,
  settings
}) {
  return buildReaderPackageFromChapters({
    bookId,
    revision,
    title,
    authors,
    language,
    chapters: [
      {
        title: chapterTitle,
        text
      }
    ],
    settings
  });
}

export function buildReaderPackageFromChapters({
  bookId,
  revision = 1,
  title = "Untitled",
  authors = [],
  language = "und",
  chapters,
  settings
}) {
  if (!bookId) {
    throw new Error("bookId is required");
  }
  if (!Array.isArray(chapters) || chapters.length === 0) {
    throw new Error("at least one chapter is required");
  }

  const tokens = [];
  const chapterIndex = [];
  const paragraphIndex = [];
  const sentenceIndex = [];
  let recommendedStartTokenIndex = 0;
  let hasRecommendedStart = false;
  let sentenceId = 0;
  let paragraphId = 0;

  for (let chapterId = 0; chapterId < chapters.length; chapterId += 1) {
    const chapter = chapters[chapterId];
    const chapterStart = tokens.length;
    chapterIndex.push({
      chapter_id: chapterId,
      title: chapter.title || `Chapter ${chapterId + 1}`,
      token_start: chapterStart
    });
    if (chapter.recommendedStart && !hasRecommendedStart) {
      recommendedStartTokenIndex = chapterStart;
      hasRecommendedStart = true;
    }

    const sourceParagraphs = Array.isArray(chapter.paragraphs)
      ? [...chapter.paragraphs]
      : normalizeParagraphs(chapter.text ?? "");
    const includeHeading = chapter.showHeading && chapter.title &&
      !sameHeadingText(chapter.title, sourceParagraphs[0]);
    const paragraphs = includeHeading ? [chapter.title, ...sourceParagraphs] : sourceParagraphs;
    for (const paragraph of paragraphs) {
      paragraphIndex.push({
        paragraph_id: paragraphId,
        chapter_id: chapterId,
        token_start: tokens.length
      });

      const paragraphTokens = tokenizeParagraph(paragraph);
      for (let localIndex = 0; localIndex < paragraphTokens.length; localIndex += 1) {
        const display = paragraphTokens[localIndex];
        const nextDisplay = paragraphTokens[localIndex + 1];
        if (sentenceIndex.length === sentenceId) {
          sentenceIndex.push({
            sentence_id: sentenceId,
            chapter_id: chapterId,
            paragraph_id: paragraphId,
            token_start: tokens.length
          });
        }

        const token = {
          token_index: tokens.length,
          display,
          chapter_id: chapterId,
          paragraph_id: paragraphId,
          sentence_id: sentenceId,
          focus_char_index: focusCharacterIndex(display),
          paragraph_end: localIndex + 1 === paragraphTokens.length
        };
        token.duration_ms_at_300wpm = durationForToken(token, nextDisplay, settings);
        if (includeHeading && paragraph === chapter.title) {
          token.chapter_heading = true;
          token.duration_ms_at_300wpm += localIndex + 1 === paragraphTokens.length ? 650 : 300;
        }
        tokens.push(token);

        if (tokenEndsSentence(display, nextDisplay) && localIndex + 1 < paragraphTokens.length) {
          sentenceIndex[sentenceId].token_end = tokens.length;
          sentenceId += 1;
        }
      }

      if (tokens.length > paragraphIndex[paragraphIndex.length - 1].token_start) {
        paragraphIndex[paragraphIndex.length - 1].token_end = tokens.length;
        if (sentenceIndex[sentenceId] && sentenceIndex[sentenceId].token_end === undefined) {
          sentenceIndex[sentenceId].token_end = tokens.length;
        }
        sentenceId += 1;
        paragraphId += 1;
      } else {
        paragraphIndex.pop();
      }
    }

    chapterIndex[chapterIndex.length - 1].token_end = tokens.length;
    if (chapterIndex[chapterIndex.length - 1].token_end === chapterStart) {
      chapterIndex.pop();
    }
  }

  return {
    reader_package_version: READER_PACKAGE_VERSION,
    created_by: "@pebble-rsvp/shared",
    metadata: {
      book_id: bookId,
      revision,
      title,
      authors,
      language,
      recommended_start_token_index: recommendedStartTokenIndex
    },
    chapters: chapterIndex,
    paragraphs: paragraphIndex,
    sentences: sentenceIndex.filter((entry) => entry.token_end !== undefined && entry.token_end > entry.token_start),
    tokens
  };
}

export function serializeReaderPackage(readerPackage) {
  if (readerPackage?.reader_package_version !== READER_PACKAGE_VERSION) {
    throw new Error(`unsupported reader package version: ${readerPackage?.reader_package_version}`);
  }
  return Buffer.from(JSON.stringify(readerPackage), "utf8");
}

export function deserializeReaderPackage(bytes) {
  const readerPackage = JSON.parse(Buffer.from(bytes).toString("utf8"));
  if (readerPackage.reader_package_version !== READER_PACKAGE_VERSION) {
    throw new Error(`unsupported reader package version: ${readerPackage.reader_package_version}`);
  }
  if (!Array.isArray(readerPackage.tokens)) {
    throw new Error("reader package tokens are missing");
  }
  return readerPackage;
}

export function sentenceStartAtOrBefore(readerPackage, tokenIndex) {
  if (!readerPackage.tokens.length) {
    return 0;
  }
  const bounded = Math.max(0, Math.min(tokenIndex, readerPackage.tokens.length - 1));
  let candidate = 0;
  for (const sentence of readerPackage.sentences) {
    if (sentence.token_start > bounded) {
      break;
    }
    candidate = sentence.token_start;
  }
  return candidate;
}

export function rewindSentence(readerPackage, tokenIndex) {
  if (!readerPackage.tokens.length) {
    return 0;
  }
  const currentStart = sentenceStartAtOrBefore(readerPackage, tokenIndex);
  if (currentStart === tokenIndex && tokenIndex > 0) {
    return sentenceStartAtOrBefore(readerPackage, tokenIndex - 1);
  }
  return currentStart;
}

export function progressForToken(readerPackage, tokenIndex, now = new Date()) {
  const token = readerPackage.tokens[Math.max(0, Math.min(tokenIndex, readerPackage.tokens.length - 1))];
  return {
    book_id: readerPackage.metadata.book_id,
    book_revision: readerPackage.metadata.revision,
    chapter_id: token?.chapter_id ?? 0,
    paragraph_id: token?.paragraph_id ?? 0,
    sentence_id: token?.sentence_id ?? 0,
    token_index: token?.token_index ?? 0,
    percentage: percentage(tokenIndex, readerPackage.tokens.length),
    timestamp: now.toISOString()
  };
}

export function contextForToken(readerPackage, tokenIndex) {
  const token = readerPackage.tokens[Math.max(0, Math.min(tokenIndex, readerPackage.tokens.length - 1))];
  const paragraph = readerPackage.paragraphs.find((entry) => entry.paragraph_id === token.paragraph_id);
  if (!paragraph) {
    return { text: "", current_token_index: tokenIndex, local_token_index: 0 };
  }

  const paragraphTokens = readerPackage.tokens.slice(paragraph.token_start, paragraph.token_end);
  return {
    text: paragraphTokens.map((entry) => entry.display).join(" "),
    current_token_index: token.token_index,
    local_token_index: token.token_index - paragraph.token_start,
    token_start: paragraph.token_start,
    token_end: paragraph.token_end
  };
}
