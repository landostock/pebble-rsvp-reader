import assert from "node:assert/strict";
import { mkdtemp, rm } from "node:fs/promises";
import os from "node:os";
import path from "node:path";
import test from "node:test";
import { deserializeReaderPackage } from "../../shared/src/reader-package.js";
import { createEpubFixture } from "../../shared/test/zip-fixture.js";
import { LibraryService } from "../src/library-service.js";
import { LocalObjectStore } from "../src/object-store.js";

async function withService(callback) {
  const directory = await mkdtemp(path.join(os.tmpdir(), "pebble-rsvp-test-"));
  try {
    const service = new LibraryService(new LocalObjectStore(directory));
    await callback(service);
  } finally {
    await rm(directory, { recursive: true, force: true });
  }
}

test("normalizes progress fields in libraries created before progress summaries", async () => {
  const directory = await mkdtemp(path.join(os.tmpdir(), "pebble-rsvp-legacy-library-test-"));
  try {
    const store = new LocalObjectStore(directory);
    await store.putJson("library/library.json", {
      library_revision: 7,
      updated_at: "2026-08-29T12:00:00.000Z",
      progress_updated_at: "null",
      books: [{ book_id: "33333333-4444-4555-8666-777777777777", title: "Legacy" }]
    });

    const library = await new LibraryService(store).readLibrary();

    assert.equal(library.library_revision, 7);
    assert.equal(library.progress_revision, 0);
    assert.equal(library.progress_updated_at, null);
    assert.equal(library.books[0].progress_token_index, 0);
    assert.equal(library.books[0].progress_percentage, 0);
    assert.equal(library.books[0].progress_updated_at, null);
  } finally {
    await rm(directory, { recursive: true, force: true });
  }
});

test("publishes processed EPUB objects and updates library last", async () => {
  await withService(async (service) => {
    const result = await service.publishBookFromEpub({
      sourceBytes: createEpubFixture(),
      sourceName: "fixture.epub",
      bookId: "33333333-4444-4555-8666-777777777777",
      now: new Date("2026-08-29T12:00:00Z")
    });
    const library = await service.readLibrary();
    const readerPackage = deserializeReaderPackage(await service.getReaderPackageBytes(result.metadata.book_id));

    assert.equal(library.library_revision, 1);
    assert.equal(library.books.length, 1);
    assert.equal(library.books[0].title, "Spine Test");
    assert.equal(result.metadata.processing_state, "ready");
    assert.equal(readerPackage.chapters[0].title, "Zweites Kapitel");
    assert.match(result.metadata.content_hash, /^[0-9a-f]{64}$/u);
  });
});

test("publishes EPUB cover images when the source declares one", async () => {
  await withService(async (service) => {
    const coverBytes = Buffer.from("published cover", "utf8");
    const result = await service.publishBookFromEpub({
      sourceBytes: createEpubFixture({ cover: "properties", coverBytes }),
      sourceName: "fixture.epub",
      bookId: "77777777-8888-4999-aaaa-bbbbbbbbbbbb",
      now: new Date("2026-08-29T12:30:00Z")
    });
    const library = await service.readLibrary();
    const cover = await service.getBookCover(result.metadata.book_id);

    assert.equal(result.metadata.cover_revision, 1);
    assert.equal(library.books[0].cover_revision, 1);
    assert.equal(cover.contentType, "image/jpeg");
    assert.deepEqual(cover.bytes, coverBytes);
  });
});

test("metadata edits do not change content revision or content hash", async () => {
  await withService(async (service) => {
    const published = await service.publishBookFromEpub({
      sourceBytes: createEpubFixture(),
      bookId: "44444444-5555-4666-8777-888888888888"
    });
    await service.writeProgress(published.metadata.book_id, {
      book_id: published.metadata.book_id,
      book_revision: 1,
      chapter_id: 0,
      paragraph_id: 0,
      sentence_id: 0,
      token_index: 2,
      timestamp: "2026-08-29T12:00:00.000Z",
      session_id: "metadata-test"
    });
    const updated = await service.updateBookMetadata(
      published.metadata.book_id,
      {
        title: "Corrected Title",
        authors: ["Corrected Author"]
      },
      new Date("2026-08-29T13:00:00Z")
    );
    const library = await service.readLibrary();

    assert.equal(updated.title, "Corrected Title");
    assert.deepEqual(updated.authors, ["Corrected Author"]);
    assert.equal(updated.revision, published.metadata.revision);
    assert.equal(updated.metadata_revision, published.metadata.metadata_revision + 1);
    assert.equal(updated.content_hash, published.metadata.content_hash);
    assert.equal(library.books[0].progress_token_index, 2);
  });
});

test("rejects oversized or structurally invalid metadata edits", async () => {
  await withService(async (service) => {
    const published = await service.publishBookFromEpub({
      sourceBytes: createEpubFixture(),
      bookId: "45454545-5555-4666-8777-888888888888"
    });

    await assert.rejects(
      () => service.updateBookMetadata(published.metadata.book_id, { title: "x".repeat(513) }),
      /invalid book metadata/u
    );
    await assert.rejects(
      () => service.updateBookMetadata(published.metadata.book_id, { authors: "not-an-array" }),
      /invalid book metadata/u
    );
  });
});

test("cover uploads update cover metadata without changing reader content", async () => {
  await withService(async (service) => {
    const published = await service.publishBookFromEpub({
      sourceBytes: createEpubFixture(),
      bookId: "66666666-7777-4888-9999-aaaaaaaaaaaa"
    });
    await service.writeProgress(published.metadata.book_id, {
      book_id: published.metadata.book_id,
      book_revision: 1,
      chapter_id: 0,
      paragraph_id: 0,
      sentence_id: 0,
      token_index: 2,
      timestamp: "2026-08-29T14:00:00.000Z",
      session_id: "cover-test"
    });
    const coverBytes = Buffer.from("fake png bytes", "utf8");
    const updated = await service.updateBookCover(
      published.metadata.book_id,
      {
        bytes: coverBytes,
        contentType: "image/png",
        now: new Date("2026-08-29T15:00:00Z")
      }
    );
    const cover = await service.getBookCover(published.metadata.book_id);
    const library = await service.readLibrary();

    assert.equal(updated.cover_revision, published.metadata.cover_revision + 1);
    assert.equal(updated.metadata_revision, published.metadata.metadata_revision + 1);
    assert.equal(updated.revision, published.metadata.revision);
    assert.equal(updated.content_hash, published.metadata.content_hash);
    assert.equal(cover.contentType, "image/png");
    assert.deepEqual(cover.bytes, coverBytes);
    assert.equal(library.books[0].cover_revision, updated.cover_revision);
    assert.equal(library.books[0].progress_token_index, 2);
  });
});

test("extracts a cover from the stored source EPUB over an existing manual cover", async () => {
  await withService(async (service) => {
    const coverBytes = Buffer.from("stored source cover", "utf8");
    const published = await service.publishBookFromEpub({
      sourceBytes: createEpubFixture({ cover: "metadata", coverBytes }),
      bookId: "88888888-9999-4aaa-bbbb-cccccccccccc"
    });

    await service.updateBookCover(published.metadata.book_id, {
      bytes: Buffer.from("manual cover", "utf8"),
      contentType: "image/png",
      now: new Date("2026-08-29T15:30:00Z")
    });
    const extracted = await service.extractBookCoverFromSource(published.metadata.book_id, {
      now: new Date("2026-08-29T16:00:00Z")
    });
    const cover = await service.getBookCover(published.metadata.book_id);

    assert.equal(extracted.cover_revision, 3);
    assert.equal(extracted.metadata_revision, 3);
    assert.equal(extracted.content_hash, published.metadata.content_hash);
    assert.equal(cover.contentType, "image/jpeg");
    assert.deepEqual(cover.bytes, coverBytes);
  });
});

test("deletes a book and its stored progress", async () => {
  await withService(async (service) => {
    const published = await service.publishBookFromEpub({
      sourceBytes: createEpubFixture(),
      bookId: "99999999-aaaa-4bbb-8ccc-dddddddddddd"
    });
    await service.writeProgress(published.metadata.book_id, {
      book_id: published.metadata.book_id,
      book_revision: 1,
      chapter_id: 0,
      paragraph_id: 0,
      sentence_id: 0,
      token_index: 2,
      timestamp: "2026-08-29T17:00:00.000Z",
      session_id: "delete-test"
    });

    const deleted = await service.deleteBook(published.metadata.book_id, new Date("2026-08-29T17:30:00Z"));
    const library = await service.readLibrary();

    assert.equal(deleted.deleted, true);
    assert.equal(library.library_revision, 2);
    assert.equal(library.progress_revision, 1);
    assert.equal(library.books.length, 0);
    assert.equal(await service.readProgress(published.metadata.book_id), null);
    await assert.rejects(() => service.getBookMetadata(published.metadata.book_id));
  });
});

test("stores and retrieves canonical progress", async () => {
  await withService(async (service) => {
    const published = await service.publishBookFromEpub({
      sourceBytes: createEpubFixture(),
      bookId: "55555555-6666-4777-8888-999999999999"
    });
    const progress = {
      book_id: published.metadata.book_id,
      book_revision: 1,
      chapter_id: 0,
      paragraph_id: 2,
      sentence_id: 3,
      token_index: 12,
      timestamp: "2026-08-29T14:00:00.000Z",
      session_id: "test-session"
    };

    const stored = await service.writeProgress(progress.book_id, progress);
    assert.equal(stored.token_index, 12);
    assert.equal(stored.percentage,
      Math.round((12 * 100) / (published.metadata.token_count - 1)));
    assert.deepEqual(await service.readProgress(progress.book_id), stored);
  });
});

test("publishes progress summaries and rejects stale updates", async () => {
  await withService(async (service) => {
    const published = await service.publishBookFromEpub({
      sourceBytes: createEpubFixture(),
      bookId: "aaaaaaaa-bbbb-4ccc-8ddd-eeeeeeeeeeee"
    });
    const newer = await service.writeProgress(published.metadata.book_id, {
      book_id: published.metadata.book_id,
      book_revision: 1,
      chapter_id: 0,
      paragraph_id: 0,
      sentence_id: 0,
      token_index: 2,
      timestamp: "2026-08-30T12:00:00.000Z",
      session_id: "watch"
    });
    const stale = await service.writeProgress(published.metadata.book_id, {
      ...newer,
      token_index: 1,
      timestamp: "2026-08-30T11:00:00.000Z"
    });
    const rewound = await service.writeProgress(published.metadata.book_id, {
      ...newer,
      token_index: 1,
      furthest_token_index: 1,
      timestamp: "2026-08-30T13:00:00.000Z"
    });
    const library = await service.readLibrary();

    assert.equal(stale.token_index, 2);
    assert.equal(rewound.token_index, 1);
    assert.equal(rewound.furthest_token_index, 2);
    assert.equal(library.progress_revision, 2);
    assert.equal(library.books[0].progress_token_index, 1);
    assert.equal(library.books[0].progress_furthest_token_index, 2);
    assert.equal(library.books[0].progress_percentage, rewound.percentage);
  });
});

test("resets progress with a tombstone that rejects stale device updates", async () => {
  await withService(async (service) => {
    const bookId = "12345678-1234-4567-89ab-123456789abc";
    const published = await service.publishBookFromEpub({ sourceBytes: createEpubFixture(), bookId });
    await service.writeProgress(bookId, {
      book_id: bookId,
      book_revision: published.metadata.revision,
      token_index: 2,
      furthest_token_index: 2,
      timestamp: "2026-09-01T12:00:00.000Z",
      session_id: "watch"
    });

    const reset = await service.resetProgress(bookId, new Date("2026-09-01T13:00:00.000Z"));
    const stale = await service.writeProgress(bookId, {
      book_id: bookId,
      book_revision: published.metadata.revision,
      token_index: 2,
      furthest_token_index: 2,
      timestamp: "2026-09-01T12:30:00.000Z",
      session_id: "stale-watch"
    });
    const library = await service.readLibrary();

    assert.equal(reset.reset, true);
    assert.equal(stale.reset, true);
    assert.equal(stale.token_index, 0);
    assert.equal(stale.furthest_token_index, 0);
    assert.equal(library.books[0].progress_token_index, 0);
    assert.equal(library.books[0].progress_furthest_token_index, 0);
    assert.equal(library.books[0].progress_reset, true);
    assert.equal(library.progress_revision, 2);
  });
});

test("stores normalized account settings with English and recently read defaults", async () => {
  await withService(async (service) => {
    const defaults = await service.readAccountSettings();
    assert.equal(defaults.language, "en");
    assert.equal(defaults.library_sort, "last_read");

    const updated = await service.writeAccountSettings(
      { language: "de", library_sort: "author", display_theme: "dark" },
      new Date("2026-08-31T12:00:00.000Z")
    );
    assert.equal(updated.revision, 1);
    assert.equal(updated.language, "de");
    assert.equal(updated.library_sort, "author");
    assert.equal(updated.display_theme, "dark");
  });
});

test("replaces invalid legacy progress timestamps with server time", async () => {
  await withService(async (service) => {
    const bookId = "bbbbbbbb-cccc-4ddd-8eee-ffffffffffff";
    await service.publishBookFromEpub({ sourceBytes: createEpubFixture(), bookId });
    const stored = await service.writeProgress(bookId, {
      book_id: bookId,
      book_revision: 1,
      token_index: 12,
      timestamp: "null",
      session_id: "legacy"
    }, new Date("2026-08-31T12:30:00.000Z"));

    assert.equal(stored.timestamp, "2026-08-31T12:30:00.000Z");
  });
});

test("clamps future progress timestamps so later watch updates remain possible", async () => {
  await withService(async (service) => {
    const bookId = "cccccccc-dddd-4eee-8fff-000000000001";
    await service.publishBookFromEpub({ sourceBytes: createEpubFixture(), bookId });
    const now = new Date("2026-08-31T12:30:00.000Z");
    const stored = await service.writeProgress(bookId, {
      book_id: bookId,
      book_revision: 1,
      token_index: 1,
      timestamp: "2099-01-01T00:00:00.000Z",
      session_id: "clock-skew"
    }, now);

    assert.equal(stored.timestamp, now.toISOString());
  });
});
