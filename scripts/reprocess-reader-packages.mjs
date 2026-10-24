import { createHash } from "node:crypto";
import { readFile, rename, writeFile } from "node:fs/promises";
import path from "node:path";
import { processEpubToReaderPackage } from "../shared/src/epub.js";
import { serializeReaderPackage } from "../shared/src/reader-package.js";

const dataRoot = path.resolve(process.argv[2] ?? "server/.data");
const libraryPath = path.join(dataRoot, "library/library.json");
const library = JSON.parse(await readFile(libraryPath, "utf8"));
const timestamp = new Date().toISOString();

async function writeJsonAtomic(filePath, value) {
  const temporary = `${filePath}.tmp`;
  await writeFile(temporary, `${JSON.stringify(value, null, 2)}\n`);
  await rename(temporary, filePath);
}

function tokenDisplays(readerPackage) {
  return readerPackage.tokens.map((token) => String(token.display ?? "").normalize("NFC"));
}

function buildTokenIndexMap(oldPackage, newPackage) {
  const oldTokens = tokenDisplays(oldPackage);
  const newTokens = tokenDisplays(newPackage);
  const mapping = new Int32Array(oldTokens.length);
  let oldIndex = 0;
  let newIndex = 0;

  while (oldIndex < oldTokens.length && newIndex < newTokens.length) {
    if (oldTokens[oldIndex] === newTokens[newIndex]) {
      mapping[oldIndex++] = newIndex++;
      continue;
    }

    let resync = null;
    for (let oldSkip = 0; oldSkip <= 16 && oldIndex + oldSkip < oldTokens.length; oldSkip += 1) {
      for (let newSkip = 0; newSkip <= 16 && newIndex + newSkip < newTokens.length; newSkip += 1) {
        if (oldSkip === 0 && newSkip === 0) continue;
        const oldCandidate = oldIndex + oldSkip;
        const newCandidate = newIndex + newSkip;
        if (oldTokens[oldCandidate] === newTokens[newCandidate] &&
            oldTokens[oldCandidate + 1] === newTokens[newCandidate + 1]) {
          resync = { oldSkip, newSkip };
          break;
        }
      }
      if (resync) break;
    }

    if (!resync) {
      const remainingOld = oldTokens.length - oldIndex;
      const remainingNew = newTokens.length - newIndex;
      while (oldIndex < oldTokens.length) {
        const relative = oldIndex - (oldTokens.length - remainingOld);
        mapping[oldIndex++] = Math.min(
          newTokens.length - 1,
          newIndex + Math.floor((relative * Math.max(1, remainingNew)) / Math.max(1, remainingOld))
        );
      }
      break;
    }

    const changedOld = Math.max(1, resync.oldSkip);
    const changedNew = Math.max(1, resync.newSkip);
    for (let offset = 0; offset < resync.oldSkip; offset += 1) {
      mapping[oldIndex + offset] = Math.min(
        newTokens.length - 1,
        newIndex + Math.floor((offset * changedNew) / changedOld)
      );
    }
    oldIndex += resync.oldSkip;
    newIndex += resync.newSkip;
  }

  while (oldIndex < oldTokens.length) {
    mapping[oldIndex++] = Math.max(0, newTokens.length - 1);
  }
  return mapping;
}

function remapIndex(mapping, value) {
  const oldIndex = Math.max(0, Math.min(mapping.length - 1, Math.trunc(Number(value) || 0)));
  return mapping.length > 0 ? mapping[oldIndex] : 0;
}

for (const book of library.books) {
  const prefix = path.join(dataRoot, "books", book.book_id);
  const source = await readFile(path.join(prefix, "source.epub"));
  const metadataPath = path.join(prefix, "metadata.json");
  const metadata = JSON.parse(await readFile(metadataPath, "utf8"));
  const oldReaderPackage = JSON.parse(await readFile(path.join(prefix, "reader-package.bin"), "utf8"));
  const revision = Number(book.revision ?? metadata.revision ?? 1) + 1;
  const processed = processEpubToReaderPackage(source, {
    bookId: book.book_id,
    revision,
    sourceName: metadata.source_name ?? "book.epub"
  });

  const indexMap = buildTokenIndexMap(oldReaderPackage, processed.readerPackage);
  processed.readerPackage.metadata.title = book.title;
  processed.readerPackage.metadata.authors = book.authors;
  processed.readerPackage.metadata.language = book.language;
  const bytes = serializeReaderPackage(processed.readerPackage);
  const contentHash = createHash("sha256").update(bytes).digest("hex");

  await writeFile(path.join(prefix, "reader-package.bin.tmp"), bytes);
  await rename(path.join(prefix, "reader-package.bin.tmp"), path.join(prefix, "reader-package.bin"));
  await writeJsonAtomic(path.join(prefix, "reader-manifest.json"), {
    reader_package_version: processed.readerPackage.reader_package_version,
    book_id: book.book_id,
    revision,
    content_hash: contentHash,
    token_count: processed.readerPackage.tokens.length,
    chapter_count: processed.readerPackage.chapters.length
  });
  await writeJsonAtomic(metadataPath, {
    ...metadata,
    revision,
    content_hash: contentHash,
    chapter_count: processed.readerPackage.chapters.length,
    updated_at: timestamp
  });

  const progressPath = path.join(dataRoot, "progress", `${book.book_id}.json`);
  try {
    const progress = JSON.parse(await readFile(progressPath, "utf8"));
    const tokenIndex = remapIndex(indexMap, progress.token_index);
    const furthestTokenIndex = remapIndex(indexMap, progress.furthest_token_index ?? progress.token_index);
    await writeJsonAtomic(progressPath, {
      ...progress,
      book_revision: revision,
      token_index: tokenIndex,
      furthest_token_index: furthestTokenIndex,
      percentage: processed.readerPackage.tokens.length > 1
        ? Math.round((furthestTokenIndex * 1000) / (processed.readerPackage.tokens.length - 1)) / 10
        : 100
    });
    book.progress_token_index = tokenIndex;
    book.progress_furthest_token_index = furthestTokenIndex;
    book.progress_percentage = processed.readerPackage.tokens.length > 1
      ? Math.round((furthestTokenIndex * 1000) / (processed.readerPackage.tokens.length - 1)) / 10
      : 100;
  } catch (error) {
    if (error.code !== "ENOENT") throw error;
  }

  book.revision = revision;
  book.content_hash = contentHash;
  book.token_count = processed.readerPackage.tokens.length;
  book.chapter_count = processed.readerPackage.chapters.length;
  book.updated_at = timestamp;
}

library.library_revision = Number(library.library_revision ?? 0) + 1;
library.updated_at = timestamp;
await writeJsonAtomic(libraryPath, library);
console.log(`Reprocessed ${library.books.length} books and migrated token positions.`);
