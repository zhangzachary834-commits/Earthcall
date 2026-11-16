"use strict";

const test = require("node:test");
const assert = require("node:assert/strict");
const path = require("path");

const policyLib = require("../../src/Singularity/Foreign/mcp/earthcall-sandbox-policy.js");

const policy = policyLib.loadPolicy(path.resolve(__dirname, "../../config/earthcall-gemini-sandbox-policy.json"));

test("default policy auto-allows ordinary structured test commands", () => {
  assert.equal(policyLib.evaluateCommand(policy, ["ctest", "--test-dir", "build", "-R", "mcp"]).decision, "allow");
});

test("default policy reviews listed risky programs", () => {
  const result = policyLib.evaluateCommand(policy, ["curl", "https://example.invalid"]);
  assert.equal(result.decision, "review");
  assert.ok(result.reasons.some((r) => r === "program:curl"));
});

test("default policy reviews risky substrings even under otherwise-safe programs", () => {
  const result = policyLib.evaluateCommand(policy, ["git", "reset", "--hard", "HEAD~1"]);
  assert.equal(result.decision, "review");
  assert.ok(result.reasons.some((r) => r.includes("--hard")));
});

test("manual command mode reviews everything", () => {
  const p = JSON.parse(JSON.stringify(policy));
  p.commands.mode = "manual";
  assert.equal(policyLib.evaluateCommand(p, ["node", "--test"]).decision, "review");
});

test("auto command mode allows everything at approval layer", () => {
  const p = JSON.parse(JSON.stringify(policy));
  p.commands.mode = "auto";
  assert.equal(policyLib.evaluateCommand(p, ["rm", "-rf", "/"]).decision, "allow");
});

test("file edit policy supports auto, manual, and safe-path modes", () => {
  const p = JSON.parse(JSON.stringify(policy));
  assert.equal(policyLib.evaluateFileEdit(p, "src/foo.cpp").decision, "allow");

  p.fileEdits.mode = "manual";
  assert.equal(policyLib.evaluateFileEdit(p, "src/foo.cpp").decision, "review");

  p.fileEdits.mode = "auto_safe_paths";
  assert.equal(policyLib.evaluateFileEdit(p, "src/foo.cpp").decision, "allow");
  assert.equal(policyLib.evaluateFileEdit(p, "scripts/build.sh").decision, "review");
  assert.equal(policyLib.evaluateFileEdit(p, "package.json").decision, "review");
});

test("approval fingerprints bind exact action content", () => {
  const a = policyLib.fingerprintAction({ type: "command", argv: ["ctest", "-R", "one"] });
  const b = policyLib.fingerprintAction({ type: "command", argv: ["ctest", "-R", "two"] });
  assert.notEqual(a, b);
  assert.equal(a, policyLib.fingerprintAction({ argv: ["ctest", "-R", "one"], type: "command" }));
});
