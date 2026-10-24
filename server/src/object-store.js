import { mkdir, readFile, rename, rm, stat, writeFile } from "node:fs/promises";
import path from "node:path";

function normalizeKey(key) {
  const normalized = String(key).replace(/\\/gu, "/").replace(/^\/+/u, "");
  if (!normalized || normalized.split("/").includes("..")) {
    throw new Error(`invalid object key: ${key}`);
  }
  return normalized;
}

export class LocalObjectStore {
  constructor(rootDirectory) {
    this.rootDirectory = rootDirectory;
  }

  pathFor(key) {
    return path.join(this.rootDirectory, normalizeKey(key));
  }

  async exists(key) {
    try {
      await stat(this.pathFor(key));
      return true;
    } catch (error) {
      if (error.code === "ENOENT") {
        return false;
      }
      throw error;
    }
  }

  async getBytes(key) {
    return readFile(this.pathFor(key));
  }

  async putBytes(key, bytes) {
    const target = this.pathFor(key);
    await mkdir(path.dirname(target), { recursive: true });
    const temporary = `${target}.tmp-${process.pid}-${Date.now()}-${Math.random().toString(16).slice(2)}`;
    await writeFile(temporary, Buffer.from(bytes));
    await rename(temporary, target);
  }

  async getJson(key, fallback = undefined) {
    try {
      return JSON.parse((await this.getBytes(key)).toString("utf8"));
    } catch (error) {
      if (error.code === "ENOENT" && fallback !== undefined) {
        return fallback;
      }
      throw error;
    }
  }

  async putJson(key, value) {
    await this.putBytes(key, Buffer.from(`${JSON.stringify(value, null, 2)}\n`, "utf8"));
  }

  async delete(key) {
    await rm(this.pathFor(key), { recursive: true, force: true });
  }

  async approximateBytes() {
    async function walk(directory) {
      let total = 0;
      let entries;
      try {
        entries = await import("node:fs/promises").then((fs) => fs.readdir(directory, { withFileTypes: true }));
      } catch (error) {
        if (error.code === "ENOENT") {
          return 0;
        }
        throw error;
      }
      for (const entry of entries) {
        const fullPath = path.join(directory, entry.name);
        if (entry.isDirectory()) {
          total += await walk(fullPath);
        } else if (entry.isFile()) {
          total += (await stat(fullPath)).size;
        }
      }
      return total;
    }

    return walk(this.rootDirectory);
  }
}
