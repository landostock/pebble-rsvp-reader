import { createHash, randomBytes, randomUUID, timingSafeEqual } from "node:crypto";
import { extractEpubCover, processEpubToReaderPackage } from "../../shared/src/epub.js";

const LIBRARY_KEY = "library/library.json";
const ACCOUNT_SETTINGS_KEY = "account/settings.json";
const DEVICE_TOKENS_KEY = "account/device-tokens.json";
const COVER_BYTES_KEY = "cover.bin";
const COVER_MANIFEST_KEY = "cover.json";
const ALLOWED_COVER_TYPES = new Set(["image/png", "image/jpeg", "image/webp", "image/gif"]);
const ALLOWED_LANGUAGES = new Set(["en", "de"]);
const ALLOWED_LIBRARY_SORTS = new Set(["last_read", "title", "author", "recent", "progress"]);
const ALLOWED_LIBRARY_VIEWS = new Set(["grid", "list"]);
const ALLOWED_DISPLAY_THEMES = new Set(["system", "light", "dark"]);
const ALLOWED_LIBRARY_DENSITIES = new Set(["comfortable", "compact"]);
const MAX_BOOK_TITLE_CHARACTERS = 512;
const MAX_BOOK_AUTHOR_CHARACTERS = 256;
const MAX_BOOK_AUTHORS = 16;
const MAX_BOOK_LANGUAGE_CHARACTERS = 32;

function sha256(bytes) {
  return createHash("sha256").update(bytes).digest("hex");
}

function nowIso(now = new Date()) {
  return now.toISOString();
}

function defaultLibrary() {
  return {
    library_revision: 0,
    progress_revision: 0,
    updated_at: null,
    progress_updated_at: null,
    books: []
  };
}

function defaultDeviceTokens() {
  return {
    revision: 0,
    updated_at: null,
    tokens: []
  };
}

function defaultAccountSettings() {
  return {
    revision: 0,
    updated_at: null,
    language: "en",
    library_sort: "last_read",
    library_view: "grid",
    display_theme: "system",
    library_density: "comfortable",
    storage_limit_bytes: 2 * 1024 * 1024 * 1024,
    storage_warning_bytes: Math.floor(1.5 * 1024 * 1024 * 1024)
  };
}

function validTimestamp(value) {
  return typeof value === "string" && Number.isFinite(Date.parse(value)) ? value : null;
}

function enumSetting(value, allowed, fallback) {
  return typeof value === "string" && allowed.has(value) ? value : fallback;
}

function normalizeAccountSettings(stored = {}) {
  const defaults = defaultAccountSettings();
  return {
    revision: Math.max(0, Math.trunc(Number(stored.revision) || 0)),
    updated_at: validTimestamp(stored.updated_at),
    language: enumSetting(stored.language, ALLOWED_LANGUAGES, defaults.language),
    library_sort: enumSetting(stored.library_sort, ALLOWED_LIBRARY_SORTS, defaults.library_sort),
    library_view: enumSetting(stored.library_view, ALLOWED_LIBRARY_VIEWS, defaults.library_view),
    display_theme: enumSetting(stored.display_theme, ALLOWED_DISPLAY_THEMES, defaults.display_theme),
    library_density: enumSetting(stored.library_density, ALLOWED_LIBRARY_DENSITIES, defaults.library_density),
    storage_limit_bytes: Math.max(0, Math.trunc(Number(stored.storage_limit_bytes) || defaults.storage_limit_bytes)),
    storage_warning_bytes: Math.max(0, Math.trunc(Number(stored.storage_warning_bytes) || defaults.storage_warning_bytes))
  };
}

function publicDeviceToken(record) {
  return {
    token_id: record.token_id,
    name: record.name,
    created_at: record.created_at
  };
}

function deviceTokenHash(token) {
  return createHash("sha256").update(String(token), "utf8").digest();
}

function publicBookMetadata(metadata, progressSource = metadata) {
  return {
    book_id: metadata.book_id,
    revision: metadata.revision,
    metadata_revision: metadata.metadata_revision,
    title: metadata.title,
    authors: metadata.authors,
    language: metadata.language,
    cover_revision: metadata.cover_revision,
    content_hash: metadata.content_hash,
    source_hash: metadata.source_hash,
    created_at: metadata.created_at,
    updated_at: metadata.updated_at,
    token_count: metadata.token_count,
    chapter_count: metadata.chapter_count,
    processing_state: metadata.processing_state,
    progress_token_index: progressSource.progress_token_index ?? 0,
    progress_furthest_token_index:
      progressSource.progress_furthest_token_index ?? progressSource.progress_token_index ?? 0,
    progress_percentage: progressSource.progress_percentage ?? 0,
    progress_updated_at: progressSource.progress_updated_at ?? null,
    progress_reset: Boolean(progressSource.progress_reset)
  };
}

function normalizeCoverType(contentType) {
  const normalized = String(contentType ?? "").split(";")[0].trim().toLowerCase();
  if (!ALLOWED_COVER_TYPES.has(normalized)) {
    throw new Error("cover content type must be image/png, image/jpeg, image/webp, or image/gif");
  }
  return normalized;
}

function characterCount(value) {
  return Array.from(value).length;
}

function validateMetadataText(value, field, maximum, { allowEmpty = false } = {}) {
  if (typeof value !== "string") {
    throw new Error(`invalid book metadata: ${field} must be text`);
  }
  const normalized = value.trim().normalize("NFC");
  if ((!allowEmpty && !normalized) || characterCount(normalized) > maximum) {
    throw new Error(`invalid book metadata: ${field} must contain at most ${maximum} characters`);
  }
  return normalized;
}

function truncateMetadataText(value, maximum, fallback = "") {
  const normalized = String(value ?? "").trim().normalize("NFC");
  return Array.from(normalized).slice(0, maximum).join("") || fallback;
}

async function putCoverObjects(store, prefix, { bytes, contentType, timestamp, source = "manual", sourcePath = "" }) {
  const cover = Buffer.from(bytes);
  if (cover.length === 0) {
    throw new Error("cover image is empty");
  }
  const normalizedContentType = normalizeCoverType(contentType);

  await store.putBytes(`${prefix}/${COVER_BYTES_KEY}`, cover);
  await store.putJson(`${prefix}/${COVER_MANIFEST_KEY}`, {
    content_type: normalizedContentType,
    byte_length: cover.length,
    sha256: sha256(cover),
    source,
    source_path: sourcePath,
    updated_at: timestamp
  });
}

export class LibraryService {
  constructor(store) {
    this.store = store;
  }

  async readLibrary() {
    const stored = await this.store.getJson(LIBRARY_KEY, defaultLibrary());
    return {
      ...defaultLibrary(),
      ...stored,
      updated_at: validTimestamp(stored.updated_at),
      progress_updated_at: validTimestamp(stored.progress_updated_at),
      books: Array.isArray(stored.books)
        ? stored.books.map((book) => ({
            ...book,
            progress_token_index: book.progress_token_index ?? 0,
            progress_furthest_token_index:
              book.progress_furthest_token_index ?? book.progress_token_index ?? 0,
            progress_percentage: book.progress_percentage ?? 0,
            progress_updated_at: validTimestamp(book.progress_updated_at),
            progress_reset: Boolean(book.progress_reset)
          }))
        : []
    };
  }

  async diagnostics() {
    const library = await this.readLibrary();
    return {
      backend_version: "0.1.0",
      protocol_version: 1,
      reader_package_version: 1,
      tigris_storage_usage: await this.store.approximateBytes(),
      library_revision: library.library_revision,
      book_count: library.books.length
    };
  }

  async listDeviceTokens() {
    const collection = await this.store.getJson(DEVICE_TOKENS_KEY, defaultDeviceTokens());
    return {
      revision: collection.revision,
      updated_at: collection.updated_at,
      tokens: collection.tokens.map(publicDeviceToken)
    };
  }

  async createDeviceToken(name, now = new Date()) {
    const normalizedName = String(name ?? "").trim();
    if (!normalizedName || normalizedName.length > 80) {
      throw new Error("device token name must contain between 1 and 80 characters");
    }

    const collection = await this.store.getJson(DEVICE_TOKENS_KEY, defaultDeviceTokens());
    const tokenId = randomUUID();
    const token = `prsp_${tokenId}.${randomBytes(32).toString("base64url")}`;
    const timestamp = nowIso(now);
    const record = {
      token_id: tokenId,
      name: normalizedName,
      token_hash: deviceTokenHash(token).toString("hex"),
      created_at: timestamp
    };
    await this.store.putJson(DEVICE_TOKENS_KEY, {
      revision: collection.revision + 1,
      updated_at: timestamp,
      tokens: [...collection.tokens, record]
    });
    return {
      ...publicDeviceToken(record),
      token
    };
  }

  async revokeDeviceToken(tokenId, now = new Date()) {
    const collection = await this.store.getJson(DEVICE_TOKENS_KEY, defaultDeviceTokens());
    if (!collection.tokens.some((record) => record.token_id === tokenId)) {
      throw new Error("device token not found");
    }
    const timestamp = nowIso(now);
    await this.store.putJson(DEVICE_TOKENS_KEY, {
      revision: collection.revision + 1,
      updated_at: timestamp,
      tokens: collection.tokens.filter((record) => record.token_id !== tokenId)
    });
    return {
      token_id: tokenId,
      revoked: true,
      updated_at: timestamp
    };
  }

  async authenticateDeviceToken(token) {
    const match = String(token ?? "").match(/^prsp_([0-9a-f-]{36})\.[A-Za-z0-9_-]{43}$/u);
    if (!match) {
      return null;
    }
    const collection = await this.store.getJson(DEVICE_TOKENS_KEY, defaultDeviceTokens());
    const record = collection.tokens.find((candidate) => candidate.token_id === match[1]);
    if (!record) {
      return null;
    }
    const actual = deviceTokenHash(token);
    const expected = Buffer.from(record.token_hash, "hex");
    return expected.length === actual.length && timingSafeEqual(expected, actual)
      ? publicDeviceToken(record)
      : null;
  }

  async publishBookFromEpub({ sourceBytes, sourceName = "book.epub", now = new Date(), bookId = randomUUID() }) {
    const source = Buffer.from(sourceBytes);
    const revision = 1;
    const processed = processEpubToReaderPackage(source, {
      bookId,
      revision,
      sourceName
    });

    const timestamp = nowIso(now);
    const metadata = {
      book_id: bookId,
      revision,
      metadata_revision: 1,
      title: truncateMetadataText(processed.metadata.title, MAX_BOOK_TITLE_CHARACTERS, "Untitled"),
      authors: (Array.isArray(processed.metadata.authors) ? processed.metadata.authors : [])
        .slice(0, MAX_BOOK_AUTHORS)
        .map((author) => truncateMetadataText(author, MAX_BOOK_AUTHOR_CHARACTERS))
        .filter(Boolean),
      language: truncateMetadataText(processed.metadata.language, MAX_BOOK_LANGUAGE_CHARACTERS, "und"),
      cover_revision: processed.cover ? 1 : 0,
      content_hash: sha256(processed.readerPackageBytes),
      source_hash: processed.sourceHash,
      source_name: sourceName,
      created_at: timestamp,
      updated_at: timestamp,
      token_count: processed.readerPackage.tokens.length,
      chapter_count: processed.readerPackage.chapters.length,
      processing_state: "ready"
    };

    const prefix = `books/${bookId}`;
    await this.store.putBytes(`${prefix}/source.epub`, source);
    await this.store.putBytes(`${prefix}/reader-package.bin`, processed.readerPackageBytes);
    await this.store.putJson(`${prefix}/reader-manifest.json`, {
      reader_package_version: processed.readerPackage.reader_package_version,
      book_id: bookId,
      revision,
      content_hash: metadata.content_hash,
      token_count: metadata.token_count,
      chapter_count: metadata.chapter_count
    });
    if (processed.cover) {
      await putCoverObjects(this.store, prefix, {
        bytes: processed.cover.bytes,
        contentType: processed.cover.contentType,
        timestamp,
        source: `epub-${processed.cover.source}`,
        sourcePath: processed.cover.path
      });
    }
    await this.store.putJson(`${prefix}/metadata.json`, metadata);

    const library = await this.readLibrary();
    const nextLibrary = {
      ...library,
      library_revision: library.library_revision + 1,
      updated_at: timestamp,
      books: [publicBookMetadata(metadata), ...library.books.filter((book) => book.book_id !== bookId)]
    };
    await this.store.putJson(LIBRARY_KEY, nextLibrary);

    return {
      metadata: publicBookMetadata(metadata),
      readerPackage: processed.readerPackage
    };
  }

  async getBookMetadata(bookId) {
    return this.store.getJson(`books/${bookId}/metadata.json`);
  }

  async deleteBook(bookId, now = new Date()) {
    const library = await this.readLibrary();
    const existing = library.books.find((book) => book.book_id === bookId);
    if (!existing) {
      throw new Error("book not found");
    }

    await this.store.delete(`books/${bookId}`);
    await this.store.delete(`progress/${bookId}.json`);

    const timestamp = nowIso(now);
    await this.store.putJson(LIBRARY_KEY, {
      ...library,
      library_revision: library.library_revision + 1,
      updated_at: timestamp,
      books: library.books.filter((book) => book.book_id !== bookId)
    });

    return {
      book_id: bookId,
      deleted: true,
      updated_at: timestamp
    };
  }

  async getReaderPackageBytes(bookId) {
    return this.store.getBytes(`books/${bookId}/reader-package.bin`);
  }

  async updateBookMetadata(bookId, patch, now = new Date()) {
    const existing = await this.getBookMetadata(bookId);
    const title = Object.hasOwn(patch, "title")
      ? validateMetadataText(patch.title, "title", MAX_BOOK_TITLE_CHARACTERS)
      : existing.title;
    let authors = existing.authors;
    if (Object.hasOwn(patch, "authors")) {
      if (!Array.isArray(patch.authors) || patch.authors.length > MAX_BOOK_AUTHORS) {
        throw new Error(`invalid book metadata: authors must contain at most ${MAX_BOOK_AUTHORS} entries`);
      }
      authors = patch.authors.map((author) =>
        validateMetadataText(author, "author", MAX_BOOK_AUTHOR_CHARACTERS, { allowEmpty: true }))
        .filter(Boolean);
    }
    const language = Object.hasOwn(patch, "language")
      ? validateMetadataText(patch.language, "language", MAX_BOOK_LANGUAGE_CHARACTERS)
      : existing.language;
    const updated = {
      ...existing,
      title,
      authors,
      language,
      metadata_revision: existing.metadata_revision + 1,
      updated_at: nowIso(now)
    };
    await this.store.putJson(`books/${bookId}/metadata.json`, updated);

    const library = await this.readLibrary();
    await this.store.putJson(LIBRARY_KEY, {
      ...library,
      library_revision: library.library_revision + 1,
      updated_at: updated.updated_at,
      books: library.books.map((book) => (book.book_id === bookId ? publicBookMetadata(updated, book) : book))
    });

    return publicBookMetadata(updated);
  }

  async updateBookCover(bookId, { bytes, contentType, now = new Date() }) {
    return this.#storeBookCover(bookId, {
      bytes,
      contentType,
      now,
      source: "manual"
    });
  }

  async extractBookCoverFromSource(bookId, { now = new Date() } = {}) {
    const cover = extractEpubCover(await this.store.getBytes(`books/${bookId}/source.epub`));
    if (!cover) {
      throw new Error("cover not found in source EPUB");
    }
    return this.#storeBookCover(bookId, {
      bytes: cover.bytes,
      contentType: cover.contentType,
      now,
      source: `epub-${cover.source}`,
      sourcePath: cover.path
    });
  }

  async #storeBookCover(bookId, { bytes, contentType, now = new Date(), source = "manual", sourcePath = "" }) {
    const existing = await this.getBookMetadata(bookId);
    const timestamp = nowIso(now);
    const updated = {
      ...existing,
      cover_revision: existing.cover_revision + 1,
      metadata_revision: existing.metadata_revision + 1,
      updated_at: timestamp
    };
    const prefix = `books/${bookId}`;
    await putCoverObjects(this.store, prefix, {
      bytes,
      contentType,
      timestamp,
      source,
      sourcePath
    });
    await this.store.putJson(`${prefix}/metadata.json`, updated);

    const library = await this.readLibrary();
    await this.store.putJson(LIBRARY_KEY, {
      ...library,
      library_revision: library.library_revision + 1,
      updated_at: updated.updated_at,
      books: library.books.map((book) => (book.book_id === bookId ? publicBookMetadata(updated, book) : book))
    });

    return publicBookMetadata(updated);
  }

  async getBookCover(bookId) {
    const manifest = await this.store.getJson(`books/${bookId}/${COVER_MANIFEST_KEY}`, null);
    if (!manifest) {
      throw new Error("cover not found");
    }
    return {
      bytes: await this.store.getBytes(`books/${bookId}/${COVER_BYTES_KEY}`),
      contentType: manifest.content_type
    };
  }

  async readProgress(bookId) {
    return this.store.getJson(`progress/${bookId}.json`, null);
  }

  async writeProgress(bookId, progress, now = new Date()) {
    if (progress.book_id !== bookId) {
      throw new Error("progress book_id does not match path");
    }
    const existing = await this.readProgress(bookId);
    const suppliedTimestamp = validTimestamp(progress.timestamp);
    const suppliedTime = Date.parse(suppliedTimestamp ?? "");
    const latestAcceptedTime = now.getTime() + 5 * 60 * 1000;
    const canonicalTimestamp = Number.isFinite(suppliedTime) && suppliedTime <= latestAcceptedTime
      ? suppliedTimestamp
      : nowIso(now);
    const incomingTimestamp = Date.parse(canonicalTimestamp);
    const existingTimestamp = Date.parse(existing?.timestamp ?? "");
    if (Number.isFinite(existingTimestamp) && incomingTimestamp < existingTimestamp) {
      return existing;
    }

    const library = await this.readLibrary();
    const book = library.books.find((candidate) => candidate.book_id === bookId);
    if (!book) {
      throw new Error("book not found");
    }
    if (progress.book_revision !== book.revision) {
      throw new Error("progress book_revision does not match library revision");
    }

    const tokenIndex = Math.max(0, Math.trunc(Number(progress.token_index) || 0));
    const tokenCount = Math.max(0, Math.trunc(Number(book.token_count) || 0));
    const canonicalIndex = tokenCount > 0 ? Math.min(tokenIndex, tokenCount - 1) : tokenIndex;
    if (Number.isFinite(existingTimestamp) && incomingTimestamp === existingTimestamp &&
        canonicalIndex < Math.max(0, Math.trunc(Number(existing?.token_index) || 0))) {
      return existing;
    }
    const existingFurthest = Math.max(
      0,
      Math.trunc(Number(existing?.furthest_token_index ?? existing?.token_index) || 0),
      Math.trunc(Number(book.progress_furthest_token_index ?? book.progress_token_index) || 0)
    );
    const suppliedFurthest = Math.max(0, Math.trunc(Number(progress.furthest_token_index) || 0));
    const furthestIndex = tokenCount > 0
      ? Math.min(Math.max(canonicalIndex, suppliedFurthest, existingFurthest), tokenCount - 1)
      : Math.max(canonicalIndex, suppliedFurthest, existingFurthest);
    const percentage = tokenCount <= 1
      ? 0
      : Math.min(100, Math.max(0, Math.round((canonicalIndex * 100) / (tokenCount - 1))));
    const canonical = {
      book_id: bookId,
      book_revision: book.revision,
      chapter_id: Math.max(0, Math.trunc(Number(progress.chapter_id) || 0)),
      paragraph_id: Math.max(0, Math.trunc(Number(progress.paragraph_id) || 0)),
      sentence_id: Math.max(0, Math.trunc(Number(progress.sentence_id) || 0)),
      token_index: canonicalIndex,
      furthest_token_index: furthestIndex,
      percentage,
      timestamp: canonicalTimestamp,
      session_id: String(progress.session_id ?? "").slice(0, 128),
      reset: false
    };
    await this.store.putJson(`progress/${bookId}.json`, canonical);

    if (book) {
      await this.store.putJson(LIBRARY_KEY, {
        ...library,
        progress_revision: (library.progress_revision ?? 0) + 1,
        progress_updated_at: canonical.timestamp,
        books: library.books.map((candidate) => candidate.book_id === bookId
          ? {
              ...candidate,
              progress_token_index: canonical.token_index,
              progress_furthest_token_index: canonical.furthest_token_index,
              progress_percentage: canonical.percentage,
              progress_updated_at: canonical.timestamp,
              progress_reset: false
            }
          : candidate)
      });
    }
    return canonical;
  }

  async resetProgress(bookId, now = new Date()) {
    const library = await this.readLibrary();
    const book = library.books.find((candidate) => candidate.book_id === bookId);
    if (!book) {
      throw new Error("book not found");
    }

    const timestamp = nowIso(now);
    const canonical = {
      book_id: bookId,
      book_revision: book.revision,
      chapter_id: 0,
      paragraph_id: 0,
      sentence_id: 0,
      token_index: 0,
      furthest_token_index: 0,
      percentage: 0,
      timestamp,
      session_id: "progress-reset",
      reset: true
    };
    await this.store.putJson(`progress/${bookId}.json`, canonical);
    await this.store.putJson(LIBRARY_KEY, {
      ...library,
      progress_revision: (library.progress_revision ?? 0) + 1,
      progress_updated_at: timestamp,
      books: library.books.map((candidate) => candidate.book_id === bookId
        ? {
            ...candidate,
            progress_token_index: 0,
            progress_furthest_token_index: 0,
            progress_percentage: 0,
            progress_updated_at: timestamp,
            progress_reset: true
          }
        : candidate)
    });
    return canonical;
  }

  async readAccountSettings() {
    return normalizeAccountSettings(await this.store.getJson(ACCOUNT_SETTINGS_KEY, defaultAccountSettings()));
  }

  async writeAccountSettings(patch, now = new Date()) {
    const existing = await this.readAccountSettings();
    const candidate = { ...existing };
    const enumFields = [
      ["language", ALLOWED_LANGUAGES],
      ["library_sort", ALLOWED_LIBRARY_SORTS],
      ["library_view", ALLOWED_LIBRARY_VIEWS],
      ["display_theme", ALLOWED_DISPLAY_THEMES],
      ["library_density", ALLOWED_LIBRARY_DENSITIES]
    ];
    for (const [field, allowed] of enumFields) {
      if (Object.hasOwn(patch, field)) {
        if (typeof patch[field] !== "string" || !allowed.has(patch[field])) {
          throw new Error(`invalid account setting: ${field}`);
        }
        candidate[field] = patch[field];
      }
    }
    for (const field of ["storage_limit_bytes", "storage_warning_bytes"]) {
      if (Object.hasOwn(patch, field)) {
        const value = Math.trunc(Number(patch[field]));
        if (!Number.isFinite(value) || value < 0) {
          throw new Error(`invalid account setting: ${field}`);
        }
        candidate[field] = value;
      }
    }
    const updated = normalizeAccountSettings({
      ...candidate,
      revision: existing.revision + 1,
      updated_at: nowIso(now)
    });
    await this.store.putJson(ACCOUNT_SETTINGS_KEY, updated);
    return updated;
  }

  async writeReaderSettings(patch, now = new Date()) {
    const readerPatch = {};
    for (const field of ["language", "library_sort"]) {
      if (Object.hasOwn(patch, field)) {
        readerPatch[field] = patch[field];
      }
    }
    if (Object.keys(readerPatch).length === 0) {
      throw new Error("reader settings require language or library_sort");
    }
    return this.writeAccountSettings(readerPatch, now);
  }
}
