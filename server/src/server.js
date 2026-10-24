import { createServer } from "node:http";
import { readFile } from "node:fs/promises";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { createSession, isSafeMethod, readSession, verifyPassword } from "./auth.js";
import { LibraryService } from "./library-service.js";
import { LocalObjectStore } from "./object-store.js";

const MAX_EPUB_BYTES = 50 * 1024 * 1024;
const MAX_COVER_BYTES = 5 * 1024 * 1024;
const DEFAULT_PORT = 8787;
const LOGIN_WINDOW_MS = 15 * 60 * 1000;
const LOGIN_ATTEMPT_LIMIT = 5;
const MAX_LOGIN_ATTEMPT_KEYS = 4096;
const SECURITY_HEADERS = {
  "content-security-policy": "default-src 'self'; img-src 'self' blob: data:; style-src 'self' 'unsafe-inline'; script-src 'self'; object-src 'none'; base-uri 'none'; frame-ancestors 'none'",
  "permissions-policy": "camera=(), microphone=(), geolocation=()",
  "referrer-policy": "no-referrer",
  "x-content-type-options": "nosniff",
  "x-frame-options": "DENY"
};

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const WEB_ROOT = path.resolve(__dirname, "../../web");
const STATIC_TYPES = new Map([
  [".html", "text/html; charset=utf-8"],
  [".css", "text/css; charset=utf-8"],
  [".js", "text/javascript; charset=utf-8"],
  [".json", "application/json; charset=utf-8"],
  [".webmanifest", "application/manifest+json; charset=utf-8"],
  [".png", "image/png"],
  [".svg", "image/svg+xml"]
]);

function sendJson(response, statusCode, value, headers = {}) {
  response.writeHead(statusCode, {
    ...SECURITY_HEADERS,
    "content-type": "application/json; charset=utf-8",
    ...headers
  });
  response.end(`${JSON.stringify(value, null, 2)}\n`);
}

function sendBytes(response, statusCode, bytes, contentType) {
  response.writeHead(statusCode, {
    ...SECURITY_HEADERS,
    "content-type": contentType,
    "content-length": Buffer.byteLength(bytes)
  });
  response.end(bytes);
}

function sendNotModified(response, etag) {
  response.writeHead(304, {
    ...SECURITY_HEADERS,
    etag,
    "cache-control": "private, no-cache"
  });
  response.end();
}

function libraryEtag(library, settings) {
  return `"library-${library.library_revision}-progress-${library.progress_revision ?? 0}-settings-${settings.revision ?? 0}"`;
}

function errorResponse(response, statusCode, code, message) {
  sendJson(response, statusCode, { error: { code, message } });
}

async function readBody(request, limitBytes) {
  const chunks = [];
  let total = 0;
  for await (const chunk of request) {
    total += chunk.length;
    if (total > limitBytes) {
      throw new Error("request body exceeds size limit");
    }
    chunks.push(chunk);
  }
  return Buffer.concat(chunks);
}

async function readJsonBody(request, limitBytes = 64 * 1024) {
  const body = await readBody(request, limitBytes);
  return body.length === 0 ? {} : JSON.parse(body.toString("utf8"));
}

function routePath(request) {
  return new URL(request.url, "http://localhost").pathname;
}

async function tryServeStatic(pathname, response) {
  const target = pathname === "/" ? "index.html" : pathname.slice(1);
  if (!/^[a-z0-9._/-]+$/iu.test(target) || target.includes("..")) {
    return false;
  }
  const filePath = path.join(WEB_ROOT, target);
  try {
    const bytes = await readFile(filePath);
    sendBytes(response, 200, bytes, STATIC_TYPES.get(path.extname(filePath)) ?? "application/octet-stream");
    return true;
  } catch (error) {
    if (error.code === "ENOENT" || error.code === "EISDIR") {
      return false;
    }
    throw error;
  }
}

function fileNameFromRequest(request) {
  const url = new URL(request.url, "http://localhost");
  return url.searchParams.get("filename") || "upload.epub";
}

function sessionCookieHeader(value, { clear = false, secure = true } = {}) {
  const attributes = [
    `sid=${value}`,
    "HttpOnly",
    "SameSite=Strict",
    "Path=/"
  ];
  if (secure) {
    attributes.splice(2, 0, "Secure");
  }
  if (clear) {
    attributes.push("Max-Age=0");
  }
  return attributes.join("; ");
}

function requireProductionAuthConfig({ adminPasswordHash, sessionSecret, devNoAuth }) {
  if (!devNoAuth && (!adminPasswordHash || !sessionSecret)) {
    throw new Error("set ADMIN_PASSWORD_HASH and SESSION_SECRET, or set PEBBLE_RSVP_DEV_NO_AUTH=1 for local development");
  }
}

function publicSessionState({ session = null, devNoAuth }) {
  return {
    authenticated: devNoAuth || Boolean(session),
    csrf: session?.csrf ?? "",
    dev_no_auth: devNoAuth,
    auth_mode: devNoAuth ? "development" : "password"
  };
}

function bearerToken(request) {
  const match = String(request.headers.authorization ?? "").match(/^Bearer\s+(.+)$/iu);
  return match?.[1] ?? "";
}

function isLoopbackRequest(request) {
  const address = String(request.socket.remoteAddress ?? "");
  return address === "::1" || address.startsWith("127.") || address.startsWith("::ffff:127.");
}

function deviceRouteAllowed(method, pathname) {
  if ((method === "GET" || method === "PUT") && pathname === "/api/settings") {
    return true;
  }
  if (method === "GET" && pathname === "/api/library") {
    return true;
  }
  if (method === "GET" && /^\/api\/books\/[0-9a-f-]+\/(metadata|cover|reader-package)$/iu.test(pathname)) {
    return true;
  }
  return /^\/api\/progress\/[0-9a-f-]+$/iu.test(pathname) && (method === "GET" || method === "PUT");
}

export function createApp({
  service,
  adminPasswordHash = process.env.ADMIN_PASSWORD_HASH,
  sessionSecret = process.env.SESSION_SECRET,
  devNoAuth = process.env.PEBBLE_RSVP_DEV_NO_AUTH === "1"
}) {
  requireProductionAuthConfig({ adminPasswordHash, sessionSecret, devNoAuth });
  const effectiveSessionSecret = sessionSecret ?? "dev-secret";
  const loginAttempts = new Map();

  function loginAttemptState(request, now = Date.now()) {
    const key = String(request.socket.remoteAddress ?? "unknown");
    const existing = loginAttempts.get(key);
    if (!existing || now - existing.startedAt >= LOGIN_WINDOW_MS) {
      for (const [address, attempt] of loginAttempts) {
        if (now - attempt.startedAt >= LOGIN_WINDOW_MS) {
          loginAttempts.delete(address);
        }
      }
      if (!loginAttempts.has(key) && loginAttempts.size >= MAX_LOGIN_ATTEMPT_KEYS) {
        loginAttempts.delete(loginAttempts.keys().next().value);
      }
      const fresh = { count: 0, startedAt: now };
      loginAttempts.set(key, fresh);
      return fresh;
    }
    return existing;
  }

  async function authenticate(request, response) {
    if (devNoAuth) {
      return "admin";
    }

    const session = readSession(request.headers.cookie, effectiveSessionSecret);
    if (session) {
      if (!isSafeMethod(request.method) && request.headers["x-csrf-token"] !== session.csrf) {
        errorResponse(response, 403, "csrf_failed", "CSRF token is missing or invalid");
        return null;
      }
      return "admin";
    }

    const device = await service.authenticateDeviceToken(bearerToken(request));
    if (!device) {
      errorResponse(response, 401, "unauthorized", "Authentication required");
      return null;
    }
    return "device";
  }

  return async function app(request, response) {
    try {
      const pathname = routePath(request);

      if (devNoAuth && pathname.startsWith("/api/") && !isLoopbackRequest(request)) {
        errorResponse(response, 403, "development_scope", "Development authentication is limited to loopback clients");
        return;
      }

      if (request.method === "GET" && !pathname.startsWith("/api/") && await tryServeStatic(pathname, response)) {
        return;
      }

      if (request.method === "GET" && pathname === "/health") {
        sendJson(response, 200, { ok: true });
        return;
      }

      if (request.method === "POST" && pathname === "/api/session") {
        const secureCookie = !devNoAuth;
        if (devNoAuth) {
          sendJson(response, 200, publicSessionState({ devNoAuth }));
          return;
        }
        const attempt = loginAttemptState(request);
        if (attempt.count >= LOGIN_ATTEMPT_LIMIT) {
          response.setHeader("retry-after", Math.max(1, Math.ceil((attempt.startedAt + LOGIN_WINDOW_MS - Date.now()) / 1000)));
          errorResponse(response, 429, "rate_limited", "Too many sign-in attempts; try again later");
          return;
        }
        const { password } = await readJsonBody(request);
        if (typeof password !== "string" || password.length === 0 || password.length > 1024 ||
            !(await verifyPassword(password, adminPasswordHash))) {
          attempt.count += 1;
          errorResponse(response, 401, "invalid_credentials", "Invalid password");
          return;
        }
        loginAttempts.delete(String(request.socket.remoteAddress ?? "unknown"));
        const session = createSession(effectiveSessionSecret);
        sendJson(response, 200, {
          ...publicSessionState({ session, devNoAuth }),
          expires_at: session.expiresAt
        }, {
          "set-cookie": sessionCookieHeader(session.cookieValue, { secure: secureCookie })
        });
        return;
      }

      if (request.method === "GET" && pathname === "/api/session") {
        const session = readSession(request.headers.cookie, effectiveSessionSecret);
        sendJson(response, 200, publicSessionState({ session, devNoAuth }));
        return;
      }

      if (request.method === "DELETE" && pathname === "/api/session") {
        sendJson(response, 200, publicSessionState({ devNoAuth }), {
          "set-cookie": sessionCookieHeader("", { clear: true, secure: !devNoAuth })
        });
        return;
      }

      const principal = await authenticate(request, response);
      if (!principal) {
        return;
      }

      if (principal === "device" && !deviceRouteAllowed(request.method, pathname)) {
        errorResponse(response, 403, "insufficient_scope", "Device token is not permitted for this operation");
        return;
      }

      if (request.method === "GET" && pathname === "/api/device-tokens") {
        sendJson(response, 200, await service.listDeviceTokens());
        return;
      }

      if (request.method === "POST" && pathname === "/api/device-tokens") {
        const { name } = await readJsonBody(request);
        sendJson(response, 201, await service.createDeviceToken(name));
        return;
      }

      const deviceTokenMatch = pathname.match(/^\/api\/device-tokens\/([0-9a-f-]+)$/iu);
      if (deviceTokenMatch && request.method === "DELETE") {
        sendJson(response, 200, await service.revokeDeviceToken(deviceTokenMatch[1]));
        return;
      }

      if (request.method === "GET" && pathname === "/api/diagnostics") {
        sendJson(response, 200, await service.diagnostics());
        return;
      }

      if (request.method === "GET" && pathname === "/api/settings") {
        sendJson(response, 200, await service.readAccountSettings());
        return;
      }

      if (request.method === "PUT" && pathname === "/api/settings") {
        const patch = await readJsonBody(request);
        sendJson(response, 200, principal === "device"
          ? await service.writeReaderSettings(patch)
          : await service.writeAccountSettings(patch));
        return;
      }

      if (request.method === "GET" && pathname === "/api/library") {
        const library = await service.readLibrary();
        const settings = await service.readAccountSettings();
        const etag = libraryEtag(library, settings);
        if (request.headers["if-none-match"] === etag) {
          sendNotModified(response, etag);
          return;
        }
        sendJson(response, 200, { ...library, settings }, {
          etag,
          "cache-control": "private, no-cache"
        });
        return;
      }

      if (request.method === "POST" && pathname === "/api/books") {
        const sourceBytes = await readBody(request, MAX_EPUB_BYTES);
        const result = await service.publishBookFromEpub({
          sourceBytes,
          sourceName: fileNameFromRequest(request)
        });
        sendJson(response, 201, result.metadata);
        return;
      }

      const bookMatch = pathname.match(/^\/api\/books\/([0-9a-f-]+)$/iu);
      if (bookMatch && request.method === "DELETE") {
        sendJson(response, 200, await service.deleteBook(bookMatch[1]));
        return;
      }

      const bookMetadataMatch = pathname.match(/^\/api\/books\/([0-9a-f-]+)\/metadata$/iu);
      if (bookMetadataMatch && request.method === "GET") {
        sendJson(response, 200, await service.getBookMetadata(bookMetadataMatch[1]));
        return;
      }
      if (bookMetadataMatch && request.method === "PUT") {
        sendJson(response, 200, await service.updateBookMetadata(bookMetadataMatch[1], await readJsonBody(request)));
        return;
      }

      const extractCoverMatch = pathname.match(/^\/api\/books\/([0-9a-f-]+)\/cover\/extract$/iu);
      if (extractCoverMatch && request.method === "POST") {
        sendJson(response, 200, await service.extractBookCoverFromSource(extractCoverMatch[1]));
        return;
      }

      const bookCoverMatch = pathname.match(/^\/api\/books\/([0-9a-f-]+)\/cover$/iu);
      if (bookCoverMatch && request.method === "GET") {
        const cover = await service.getBookCover(bookCoverMatch[1]);
        sendBytes(response, 200, cover.bytes, cover.contentType);
        return;
      }
      if (bookCoverMatch && request.method === "POST") {
        const contentType = request.headers["content-type"];
        const metadata = await service.updateBookCover(bookCoverMatch[1], {
          bytes: await readBody(request, MAX_COVER_BYTES),
          contentType
        });
        sendJson(response, 200, metadata);
        return;
      }

      const readerPackageMatch = pathname.match(/^\/api\/books\/([0-9a-f-]+)\/reader-package$/iu);
      if (readerPackageMatch && request.method === "GET") {
        sendBytes(response, 200, await service.getReaderPackageBytes(readerPackageMatch[1]), "application/octet-stream");
        return;
      }

      const progressMatch = pathname.match(/^\/api\/progress\/([0-9a-f-]+)$/iu);
      if (progressMatch && request.method === "GET") {
        sendJson(response, 200, await service.readProgress(progressMatch[1]));
        return;
      }
      if (progressMatch && request.method === "PUT") {
        sendJson(response, 200, await service.writeProgress(progressMatch[1], await readJsonBody(request)));
        return;
      }
      if (progressMatch && request.method === "DELETE") {
        sendJson(response, 200, await service.resetProgress(progressMatch[1]));
        return;
      }

      errorResponse(response, 404, "not_found", "No route matched the request");
    } catch (error) {
      const message = error instanceof Error ? error.message : String(error);
      const statusCode = error?.code === "ENOENT"
        ? 404
        : message.includes("size limit")
        ? 413
        : error instanceof SyntaxError || message.includes("content type") || message.includes("is empty") ||
          message.includes("invalid account setting") || message.includes("reader settings require") ||
          message.includes("invalid book metadata") || message.includes("does not match")
          ? 400
          : message.includes("not found")
            ? 404
            : 500;
      if (statusCode === 500) {
        console.error("Pebble RSVP request failed", error);
      }
      errorResponse(response, statusCode, "request_failed",
                    statusCode === 500 ? "Internal server error" : message);
    }
  };
}

export function createDefaultService() {
  const root = process.env.PEBBLE_RSVP_DATA_DIR ?? path.resolve(__dirname, "../.data");
  return new LibraryService(new LocalObjectStore(root));
}

if (process.argv[1] === fileURLToPath(import.meta.url)) {
  const port = Number(process.env.PORT ?? DEFAULT_PORT);
  const host = process.env.HOST ?? "127.0.0.1";
  const server = createServer(createApp({ service: createDefaultService() }));
  server.listen(port, host, () => {
    console.log(`Pebble RSVP server listening on http://${host}:${port}`);
  });
}
