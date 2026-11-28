"use strict";

const test = require("node:test");
const assert = require("node:assert/strict");
const fs = require("fs");
const os = require("os");
const path = require("path");
const { execFileSync } = require("child_process");

const tools = require("../../src/Singularity/Foreign/mcp/earthcall-repo-tools.js");

function fixture() {
  const root = fs.mkdtempSync(path.join(os.tmpdir(), "earthcall-mcp-test-"));
  fs.mkdirSync(path.join(root, "src"));
  fs.mkdirSync(path.join(root, "docs"));
  fs.writeFileSync(path.join(root, "src", "hello.cpp"), "alpha\nbeta Earthcall\ngamma\n");
  fs.writeFileSync(path.join(root, "docs", "note.md"), "Firmament sees the world.\n");
  fs.writeFileSync(path.join(root, ".env"), "SECRET=never\n");
  fs.writeFileSync(path.join(root, ".env.example"), "SAFE=example\n");
  fs.writeFileSync(path.join(root, "binary.bin"), Buffer.from([0, 1, 2, 3]));
  return root;
}

test("safeResolve keeps reads inside root", () => {
  const root = fixture();
  assert.equal(tools.safeResolve(root, "src/hello.cpp"), path.join(root, "src", "hello.cpp"));
  assert.throws(() => tools.safeResolve(root, "../outside"), /escapes|ENOENT/);
});

test("listDirectory lists directories before files", () => {
  const root = fixture();
  const result = tools.listDirectory(root, ".", 50, false);
  assert.equal(result.entries[0].kind, "directory");
  assert.ok(result.entries.some((e) => e.path === "src"));
});

test("readFile numbers lines and pages", () => {
  const root = fixture();
  const result = tools.readFile(root, "src/hello.cpp", 2, 3);
  assert.match(result.content, /^2: beta Earthcall/m);
  assert.match(result.content, /^3: gamma/m);
});

test("secret-like and binary files are refused", () => {
  const root = fixture();
  assert.throws(() => tools.readFile(root, ".env"), /Refusing to expose/);
  assert.doesNotThrow(() => tools.readFile(root, ".env.example"));
  assert.throws(() => tools.readFile(root, "binary.bin"), /binary/);
});

test("searchText searches literal text with glob", () => {
  const root = fixture();
  const result = tools.searchText(root, "earthcall", ".", "**/*.cpp", false, 20);
  assert.equal(result.returned, 1);
  assert.equal(result.matches[0].path, "src/hello.cpp");
  assert.equal(result.matches[0].line, 2);
});

test("symlink escape is blocked", { skip: process.platform === "win32" }, () => {
  const root = fixture();
  const outside = fs.mkdtempSync(path.join(os.tmpdir(), "earthcall-outside-"));
  const secret = path.join(outside, "outside.txt");
  fs.writeFileSync(secret, "nope\n");
  fs.symlinkSync(secret, path.join(root, "outside-link"));
  assert.throws(() => tools.readFile(root, "outside-link"), /escapes/);
});

test("gitStatus and gitDiff are read-only and reflect local changes", () => {
  const root = fixture();
  execFileSync("git", ["-C", root, "init", "-q"]);
  execFileSync("git", ["-C", root, "config", "user.email", "test@example.com"]);
  execFileSync("git", ["-C", root, "config", "user.name", "Test"]);
  execFileSync("git", ["-C", root, "add", "src/hello.cpp", "docs/note.md"]);
  execFileSync("git", ["-C", root, "commit", "-qm", "fixture"]);
  fs.appendFileSync(path.join(root, "src", "hello.cpp"), "delta\n");

  const status = tools.gitStatus(root);
  assert.match(status.output, /src\/hello\.cpp/);
  const diff = tools.gitDiff(root, "src/hello.cpp", false);
  assert.match(diff.output, /\+delta/);
});
