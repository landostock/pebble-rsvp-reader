import assert from "node:assert/strict";
import test from "node:test";
import { ZipArchive } from "../src/zip.js";
import { createEpubFixture } from "./zip-fixture.js";

const CENTRAL_DIRECTORY_SIGNATURE = Buffer.from([0x50, 0x4b, 0x01, 0x02]);

test("rejects ZIP entries whose declared expansion exceeds the memory limit", () => {
  const bytes = Buffer.from(createEpubFixture());
  const centralOffset = bytes.indexOf(CENTRAL_DIRECTORY_SIGNATURE);
  assert.notEqual(centralOffset, -1);
  bytes.writeUInt32LE(33 * 1024 * 1024, centralOffset + 24);

  assert.throws(() => new ZipArchive(bytes), /uncompressed size limit/u);
});

test("rejects ZIP central-directory pointers outside the uploaded archive", () => {
  const bytes = Buffer.from(createEpubFixture());
  const centralOffset = bytes.indexOf(CENTRAL_DIRECTORY_SIGNATURE);
  assert.notEqual(centralOffset, -1);
  bytes.writeUInt32LE(bytes.length + 100, centralOffset + 42);

  assert.throws(() => new ZipArchive(bytes), /local header offset/u);
});
