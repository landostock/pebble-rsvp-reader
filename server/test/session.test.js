import assert from "node:assert/strict";
import { createServer } from "node:http";
import { mkdtemp, rm } from "node:fs/promises";
import os from "node:os";
import path from "node:path";
import test from "node:test";
import { hashPassword } from "../src/auth.js";
import { createApp } from "../src/server.js";
import { LibraryService } from "../src/library-service.js";
import { LocalObjectStore } from "../src/object-store.js";

async function withHttpApp(options, callback) {
  const directory = await mkdtemp(path.join(os.tmpdir(), "pebble-rsvp-session-test-"));
  const service = new LibraryService(new LocalObjectStore(directory));
  const server = createServer(createApp({ service, ...options }));
  await new Promise((resolve) => server.listen(0, "127.0.0.1", resolve));
  const { port } = server.address();
  try {
    await callback(`http://127.0.0.1:${port}`);
  } finally {
    await new Promise((resolve) => server.close(resolve));
    await rm(directory, { recursive: true, force: true });
  }
}

test("reports unauthenticated session state without blocking the login UI", async () => {
  const passwordHash = await hashPassword("secret", { salt: "test-salt" });
  await withHttpApp({ adminPasswordHash: passwordHash, sessionSecret: "session-secret" }, async (baseUrl) => {
    const response = await fetch(`${baseUrl}/api/session`);
    const session = await response.json();

    assert.equal(response.status, 200);
    assert.equal(session.authenticated, false);
    assert.equal(session.csrf, "");
    assert.equal(session.auth_mode, "password");
    assert.match(response.headers.get("content-security-policy"), /frame-ancestors 'none'/u);
  });
});

test("treats malformed signed-session cookies as signed out", async () => {
  const passwordHash = await hashPassword("secret", { salt: "test-salt" });
  await withHttpApp({ adminPasswordHash: passwordHash, sessionSecret: "session-secret" }, async (baseUrl) => {
    const response = await fetch(`${baseUrl}/api/session`, {
      headers: { cookie: "sid=not-json.not-a-valid-signature" }
    });
    const session = await response.json();

    assert.equal(response.status, 200);
    assert.equal(session.authenticated, false);
  });
});

test("rate limits repeated administrator password failures", async () => {
  const passwordHash = await hashPassword("secret", { salt: "test-salt" });
  await withHttpApp({ adminPasswordHash: passwordHash, sessionSecret: "session-secret" }, async (baseUrl) => {
    for (let attempt = 0; attempt < 5; attempt += 1) {
      const response = await fetch(`${baseUrl}/api/session`, {
        method: "POST",
        headers: { "content-type": "application/json" },
        body: JSON.stringify({ password: "wrong" })
      });
      assert.equal(response.status, 401);
    }
    const blocked = await fetch(`${baseUrl}/api/session`, {
      method: "POST",
      headers: { "content-type": "application/json" },
      body: JSON.stringify({ password: "wrong" })
    });
    assert.equal(blocked.status, 429);
    assert.ok(Number(blocked.headers.get("retry-after")) > 0);
  });
});

test("reports development auth bypass explicitly", async () => {
  await withHttpApp({ devNoAuth: true }, async (baseUrl) => {
    const response = await fetch(`${baseUrl}/api/session`);
    const session = await response.json();

    assert.equal(response.status, 200);
    assert.equal(session.authenticated, true);
    assert.equal(session.dev_no_auth, true);
    assert.equal(session.auth_mode, "development");
  });
});

test("development auth bypass does not mint a login cookie", async () => {
  await withHttpApp({ devNoAuth: true }, async (baseUrl) => {
    const response = await fetch(`${baseUrl}/api/session`, {
      method: "POST",
      headers: { "content-type": "application/json" },
      body: JSON.stringify({ password: "" })
    });
    const session = await response.json();

    assert.equal(response.status, 200);
    assert.equal(session.authenticated, true);
    assert.equal(session.auth_mode, "development");
    assert.equal(response.headers.get("set-cookie"), null);
  });
});

test("production auth protects library endpoints until a valid password is provided", async () => {
  const passwordHash = await hashPassword("secret", { salt: "test-salt" });
  await withHttpApp({ adminPasswordHash: passwordHash, sessionSecret: "session-secret" }, async (baseUrl) => {
    const blocked = await fetch(`${baseUrl}/api/library`);
    assert.equal(blocked.status, 401);

    const emptyPassword = await fetch(`${baseUrl}/api/session`, {
      method: "POST",
      headers: { "content-type": "application/json" },
      body: JSON.stringify({ password: "" })
    });
    assert.equal(emptyPassword.status, 401);

    const login = await fetch(`${baseUrl}/api/session`, {
      method: "POST",
      headers: { "content-type": "application/json" },
      body: JSON.stringify({ password: "secret" })
    });
    const cookie = login.headers.get("set-cookie");
    assert.equal(login.status, 200);
    assert.match(cookie, /sid=/u);

    const allowed = await fetch(`${baseUrl}/api/library`, {
      headers: { cookie }
    });
    assert.equal(allowed.status, 200);
  });
});

test("creates and clears signed sessions", async () => {
  const passwordHash = await hashPassword("secret", { salt: "test-salt" });
  await withHttpApp({ adminPasswordHash: passwordHash, sessionSecret: "session-secret" }, async (baseUrl) => {
    const login = await fetch(`${baseUrl}/api/session`, {
      method: "POST",
      headers: { "content-type": "application/json" },
      body: JSON.stringify({ password: "secret" })
    });
    const cookie = login.headers.get("set-cookie");
    const session = await login.json();

    assert.equal(login.status, 200);
    assert.equal(session.authenticated, true);
    assert.equal(session.auth_mode, "password");
    assert.match(session.csrf, /^[a-zA-Z0-9_-]+$/u);
    assert.match(cookie, /sid=/u);

    const current = await fetch(`${baseUrl}/api/session`, {
      headers: { cookie }
    }).then((response) => response.json());
    assert.equal(current.authenticated, true);
    assert.equal(current.csrf, session.csrf);

    const logout = await fetch(`${baseUrl}/api/session`, {
      method: "DELETE",
      headers: { cookie }
    });
    assert.equal(logout.status, 200);
    assert.match(logout.headers.get("set-cookie"), /Max-Age=0/u);
  });
});
