import { createHmac, randomBytes, scrypt as scryptCallback, timingSafeEqual } from "node:crypto";
import { promisify } from "node:util";

const scrypt = promisify(scryptCallback);
const SESSION_TTL_MS = 7 * 24 * 60 * 60 * 1000;

function base64UrlEncode(bytes) {
  return Buffer.from(bytes).toString("base64url");
}

function base64UrlDecode(value) {
  return Buffer.from(value, "base64url");
}

function sign(value, secret) {
  return createHmac("sha256", secret).update(value).digest("base64url");
}

export async function hashPassword(password, { salt = randomBytes(16).toString("base64url") } = {}) {
  const key = await scrypt(password, salt, 64);
  return `scrypt$16384$8$1$${salt}$${Buffer.from(key).toString("base64url")}`;
}

export async function verifyPassword(password, encodedHash) {
  const [algorithm, n, r, p, salt, expected] = String(encodedHash).split("$");
  if (algorithm !== "scrypt" || !salt || !expected) {
    throw new Error("unsupported password hash format");
  }
  const key = await scrypt(password, salt, 64, {
    N: Number(n),
    r: Number(r),
    p: Number(p)
  });
  const expectedBytes = base64UrlDecode(expected);
  return expectedBytes.length === key.length && timingSafeEqual(expectedBytes, key);
}

export function createSession(secret, now = new Date()) {
  const payload = {
    exp: now.getTime() + SESSION_TTL_MS,
    csrf: randomBytes(18).toString("base64url")
  };
  const encoded = base64UrlEncode(JSON.stringify(payload));
  return {
    cookieValue: `${encoded}.${sign(encoded, secret)}`,
    csrf: payload.csrf,
    expiresAt: new Date(payload.exp).toISOString()
  };
}

export function readSession(cookieHeader, secret, now = new Date()) {
  const cookies = Object.fromEntries(
    String(cookieHeader ?? "")
      .split(";")
      .map((part) => part.trim())
      .filter(Boolean)
      .map((part) => {
        const equals = part.indexOf("=");
        return equals === -1 ? [part, ""] : [part.slice(0, equals), part.slice(equals + 1)];
      })
  );
  const session = cookies.sid;
  if (!session) {
    return null;
  }
  const [encoded, signature] = session.split(".");
  if (!encoded || !signature) {
    return null;
  }
  const expectedSignature = Buffer.from(sign(encoded, secret), "utf8");
  const suppliedSignature = Buffer.from(signature, "utf8");
  if (expectedSignature.length !== suppliedSignature.length ||
      !timingSafeEqual(expectedSignature, suppliedSignature)) {
    return null;
  }
  try {
    const payload = JSON.parse(base64UrlDecode(encoded).toString("utf8"));
    if (!Number.isFinite(payload.exp) || payload.exp <= now.getTime() || typeof payload.csrf !== "string") {
      return null;
    }
    return payload;
  } catch {
    return null;
  }
}

export function isSafeMethod(method) {
  return method === "GET" || method === "HEAD" || method === "OPTIONS";
}
