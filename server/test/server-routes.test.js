import assert from "node:assert/strict";
import { createServer } from "node:http";
import { mkdtemp, rm } from "node:fs/promises";
import os from "node:os";
import path from "node:path";
import test from "node:test";
import { createEpubFixture } from "../../shared/test/zip-fixture.js";
import { hashPassword } from "../src/auth.js";
import { LibraryService } from "../src/library-service.js";
import { LocalObjectStore } from "../src/object-store.js";
import { createApp } from "../src/server.js";

async function withHttpApp(callback) {
  const directory = await mkdtemp(path.join(os.tmpdir(), "pebble-rsvp-route-test-"));
  const service = new LibraryService(new LocalObjectStore(directory));
  const server = createServer(createApp({ service, devNoAuth: true }));
  await new Promise((resolve) => server.listen(0, "127.0.0.1", resolve));
  const { port } = server.address();
  try {
    await callback(`http://127.0.0.1:${port}`);
  } finally {
    await new Promise((resolve) => server.close(resolve));
    await rm(directory, { recursive: true, force: true });
  }
}

async function withPasswordHttpApp(callback) {
  const directory = await mkdtemp(path.join(os.tmpdir(), "pebble-rsvp-auth-route-test-"));
  const service = new LibraryService(new LocalObjectStore(directory));
  const adminPasswordHash = await hashPassword("correct horse battery staple", { salt: "route-test-salt" });
  const server = createServer(createApp({
    service,
    adminPasswordHash,
    sessionSecret: "route-test-session-secret",
    devNoAuth: false
  }));
  await new Promise((resolve) => server.listen(0, "127.0.0.1", resolve));
  const { port } = server.address();
  const baseUrl = `http://127.0.0.1:${port}`;
  try {
    const login = await fetch(`${baseUrl}/api/session`, {
      method: "POST",
      headers: { "content-type": "application/json" },
      body: JSON.stringify({ password: "correct horse battery staple" })
    });
    const session = await login.json();
    const cookie = login.headers.get("set-cookie").split(";", 1)[0];
    await callback({
      baseUrl,
      adminHeaders: {
        cookie,
        "x-csrf-token": session.csrf
      }
    });
  } finally {
    await new Promise((resolve) => server.close(resolve));
    await rm(directory, { recursive: true, force: true });
  }
}

test("stores and serves a custom book cover", async () => {
  await withHttpApp(async (baseUrl) => {
    const published = await fetch(`${baseUrl}/api/books?filename=fixture.epub`, {
      method: "POST",
      headers: { "content-type": "application/epub+zip" },
      body: createEpubFixture()
    }).then((response) => response.json());

    const coverBytes = Buffer.from("fake image bytes", "utf8");
    const updated = await fetch(`${baseUrl}/api/books/${published.book_id}/cover`, {
      method: "POST",
      headers: { "content-type": "image/webp" },
      body: coverBytes
    }).then((response) => response.json());
    const cover = await fetch(`${baseUrl}/api/books/${published.book_id}/cover`);

    assert.equal(updated.cover_revision, 1);
    assert.equal(cover.status, 200);
    assert.equal(cover.headers.get("content-type"), "image/webp");
    assert.deepEqual(Buffer.from(await cover.arrayBuffer()), coverBytes);
  });
});

test("serves automatically extracted EPUB cover images", async () => {
  await withHttpApp(async (baseUrl) => {
    const coverBytes = Buffer.from("embedded cover bytes", "utf8");
    const published = await fetch(`${baseUrl}/api/books?filename=fixture.epub`, {
      method: "POST",
      headers: { "content-type": "application/epub+zip" },
      body: createEpubFixture({ cover: "metadata", coverBytes })
    }).then((response) => response.json());
    const cover = await fetch(`${baseUrl}/api/books/${published.book_id}/cover`);

    assert.equal(published.cover_revision, 1);
    assert.equal(cover.status, 200);
    assert.equal(cover.headers.get("content-type"), "image/jpeg");
    assert.deepEqual(Buffer.from(await cover.arrayBuffer()), coverBytes);
  });
});

test("extracts cover from the stored EPUB source through the API", async () => {
  await withHttpApp(async (baseUrl) => {
    const coverBytes = Buffer.from("source cover bytes", "utf8");
    const published = await fetch(`${baseUrl}/api/books?filename=fixture.epub`, {
      method: "POST",
      headers: { "content-type": "application/epub+zip" },
      body: createEpubFixture({ cover: "guide-page", coverBytes })
    }).then((response) => response.json());

    await fetch(`${baseUrl}/api/books/${published.book_id}/cover`, {
      method: "POST",
      headers: { "content-type": "image/png" },
      body: Buffer.from("manual cover", "utf8")
    });

    const extracted = await fetch(`${baseUrl}/api/books/${published.book_id}/cover/extract`, {
      method: "POST"
    }).then((response) => response.json());
    const cover = await fetch(`${baseUrl}/api/books/${published.book_id}/cover`);

    assert.equal(extracted.cover_revision, 3);
    assert.equal(cover.status, 200);
    assert.equal(cover.headers.get("content-type"), "image/jpeg");
    assert.deepEqual(Buffer.from(await cover.arrayBuffer()), coverBytes);
  });
});

test("deletes a book through the API", async () => {
  await withHttpApp(async (baseUrl) => {
    const published = await fetch(`${baseUrl}/api/books?filename=fixture.epub`, {
      method: "POST",
      headers: { "content-type": "application/epub+zip" },
      body: createEpubFixture()
    }).then((response) => response.json());

    const deleted = await fetch(`${baseUrl}/api/books/${published.book_id}`, {
      method: "DELETE"
    }).then((response) => response.json());
    const library = await fetch(`${baseUrl}/api/library`).then((response) => response.json());
    const metadata = await fetch(`${baseUrl}/api/books/${published.book_id}/metadata`);

    assert.equal(deleted.deleted, true);
    assert.equal(deleted.book_id, published.book_id);
    assert.equal(library.books.length, 0);
    assert.equal(metadata.status, 404);
  });
});

test("marks a book as unread through the progress API", async () => {
  await withHttpApp(async (baseUrl) => {
    const published = await fetch(`${baseUrl}/api/books?filename=fixture.epub`, {
      method: "POST",
      headers: { "content-type": "application/epub+zip" },
      body: createEpubFixture()
    }).then((response) => response.json());
    await fetch(`${baseUrl}/api/progress/${published.book_id}`, {
      method: "PUT",
      headers: { "content-type": "application/json" },
      body: JSON.stringify({
        book_id: published.book_id,
        book_revision: published.revision,
        token_index: 2,
        timestamp: "2026-09-01T12:00:00.000Z"
      })
    });

    const response = await fetch(`${baseUrl}/api/progress/${published.book_id}`, { method: "DELETE" });
    const reset = await response.json();
    const library = await fetch(`${baseUrl}/api/library`).then((value) => value.json());

    assert.equal(response.status, 200);
    assert.equal(reset.reset, true);
    assert.equal(reset.token_index, 0);
    assert.equal(library.books[0].progress_reset, true);
    assert.equal(library.books[0].progress_furthest_token_index, 0);
  });
});

test("returns 304 when the library revision has not changed", async () => {
  await withHttpApp(async (baseUrl) => {
    const first = await fetch(`${baseUrl}/api/library`);
    assert.equal(first.status, 200);
    assert.equal(first.headers.get("etag"), '"library-0-progress-0-settings-0"');

    const unchanged = await fetch(`${baseUrl}/api/library`, {
      headers: { "if-none-match": first.headers.get("etag") }
    });
    assert.equal(unchanged.status, 304);
    assert.equal(await unchanged.text(), "");

    const settings = await fetch(`${baseUrl}/api/settings`, {
      method: "PUT",
      headers: { "content-type": "application/json" },
      body: JSON.stringify({ language: "de", library_sort: "title" })
    }).then((response) => response.json());
    assert.equal(settings.language, "de");
    assert.equal(settings.library_sort, "title");

    const settingsChanged = await fetch(`${baseUrl}/api/library`, {
      headers: { "if-none-match": first.headers.get("etag") }
    });
    assert.equal(settingsChanged.status, 200);
    assert.equal(settingsChanged.headers.get("etag"), '"library-0-progress-0-settings-1"');

    await fetch(`${baseUrl}/api/books?filename=fixture.epub`, {
      method: "POST",
      headers: { "content-type": "application/epub+zip" },
      body: createEpubFixture()
    });
    const changed = await fetch(`${baseUrl}/api/library`, {
      headers: { "if-none-match": first.headers.get("etag") }
    });
    assert.equal(changed.status, 200);
    assert.equal(changed.headers.get("etag"), '"library-1-progress-0-settings-1"');
  });
});

test("creates revocable device tokens with read and progress scope only", async () => {
  await withPasswordHttpApp(async ({ baseUrl, adminHeaders }) => {
    const publishedResponse = await fetch(`${baseUrl}/api/books?filename=fixture.epub`, {
      method: "POST",
      headers: {
        ...adminHeaders,
        "content-type": "application/epub+zip"
      },
      body: createEpubFixture()
    });
    assert.equal(publishedResponse.status, 201);
    const published = await publishedResponse.json();

    const createdResponse = await fetch(`${baseUrl}/api/device-tokens`, {
      method: "POST",
      headers: {
        ...adminHeaders,
        "content-type": "application/json"
      },
      body: JSON.stringify({ name: "Pixel 10" })
    });
    assert.equal(createdResponse.status, 201);
    const created = await createdResponse.json();
    assert.match(created.token, /^prsp_[0-9a-f-]{36}\.[A-Za-z0-9_-]{43}$/u);

    const listed = await fetch(`${baseUrl}/api/device-tokens`, {
      headers: adminHeaders
    }).then((response) => response.json());
    assert.deepEqual(listed.tokens, [{
      token_id: created.token_id,
      name: "Pixel 10",
      created_at: created.created_at
    }]);
    assert.equal(JSON.stringify(listed).includes(created.token), false);
    assert.equal(JSON.stringify(listed).includes("token_hash"), false);

    const deviceHeaders = { authorization: `Bearer ${created.token}` };
    const libraryResponse = await fetch(`${baseUrl}/api/library`, { headers: deviceHeaders });
    assert.equal(libraryResponse.status, 200);
    assert.equal((await libraryResponse.json()).books[0].book_id, published.book_id);

    const settingsResponse = await fetch(`${baseUrl}/api/settings`, {
      method: "PUT",
      headers: { ...deviceHeaders, "content-type": "application/json" },
      body: JSON.stringify({
        language: "de",
        library_sort: "author",
        display_theme: "dark",
        storage_limit_bytes: 1
      })
    });
    assert.equal(settingsResponse.status, 200);
    const readerSettings = await settingsResponse.json();
    assert.equal(readerSettings.language, "de");
    assert.equal(readerSettings.library_sort, "author");
    assert.equal(readerSettings.display_theme, "system");
    assert.equal(readerSettings.storage_limit_bytes, 2 * 1024 * 1024 * 1024);

    const invalidSettings = await fetch(`${baseUrl}/api/settings`, {
      method: "PUT",
      headers: { ...deviceHeaders, "content-type": "application/json" },
      body: JSON.stringify({ language: "invalid" })
    });
    assert.equal(invalidSettings.status, 400);

    const diagnostics = await fetch(`${baseUrl}/api/diagnostics`, { headers: deviceHeaders });
    assert.equal(diagnostics.status, 403);

    const progress = {
      book_id: published.book_id,
      book_revision: 1,
      chapter_id: 0,
      paragraph_id: 0,
      sentence_id: 0,
      token_index: 2,
      timestamp: "2026-08-30T12:00:00.000Z",
      session_id: "pixel-10-test"
    };
    const progressResponse = await fetch(`${baseUrl}/api/progress/${published.book_id}`, {
      method: "PUT",
      headers: {
        ...deviceHeaders,
        "content-type": "application/json"
      },
      body: JSON.stringify(progress)
    });
    assert.equal(progressResponse.status, 200);

    const unknownProgress = await fetch(`${baseUrl}/api/progress/00000000-0000-4000-8000-000000000099`, {
      method: "PUT",
      headers: {
        ...deviceHeaders,
        "content-type": "application/json"
      },
      body: JSON.stringify({
        ...progress,
        book_id: "00000000-0000-4000-8000-000000000099"
      })
    });
    assert.equal(unknownProgress.status, 404);

    const forbidden = await fetch(`${baseUrl}/api/books/${published.book_id}`, {
      method: "DELETE",
      headers: deviceHeaders
    });
    assert.equal(forbidden.status, 403);
    assert.equal((await forbidden.json()).error.code, "insufficient_scope");

    const forbiddenReset = await fetch(`${baseUrl}/api/progress/${published.book_id}`, {
      method: "DELETE",
      headers: deviceHeaders
    });
    assert.equal(forbiddenReset.status, 403);
    assert.equal((await forbiddenReset.json()).error.code, "insufficient_scope");

    const revoked = await fetch(`${baseUrl}/api/device-tokens/${created.token_id}`, {
      method: "DELETE",
      headers: adminHeaders
    });
    assert.equal(revoked.status, 200);
    assert.equal((await revoked.json()).revoked, true);

    const afterRevocation = await fetch(`${baseUrl}/api/library`, { headers: deviceHeaders });
    assert.equal(afterRevocation.status, 401);
  });
});
