"use strict";

const test = require("node:test");
const assert = require("node:assert/strict");
const fs = require("fs");
const os = require("os");
const path = require("path");
const { execFileSync } = require("child_process");

function fixturePolicy(root, commandMode = "auto_except_review", fileMode = "auto_sandbox") {
  fs.mkdirSync(path.join(root, "config"), { recursive: true });
  fs.writeFileSync(path.join(root, "config", "earthcall-gemini-sandbox-policy.json"), JSON.stringify({
    commands: { mode: commandMode, reviewPrograms: ["rm"], reviewTokens: ["--hard"] },
    fileEdits: { mode: fileMode, reviewPathPatterns: ["scripts/**"] },
    limits: { timeoutSeconds: 10, maxTimeoutSeconds: 20, cpus: 1, memory: "256m", pids: 64, maxOutputChars: 10000 },
    image: "earthcall-sandbox:latest"
  }, null, 2));
}

function fixtureRepo(commandMode, fileMode) {
  const root = fs.mkdtempSync(path.join(os.tmpdir(), "earthcall-sandbox-fixture-"));
  fs.mkdirSync(path.join(root, "src"));
  fs.mkdirSync(path.join(root, "scripts"));
  fs.writeFileSync(path.join(root, "src", "hello.txt"), "hello\n");
  fs.writeFileSync(path.join(root, "scripts", "build.sh"), "echo hi\n");
  fs.writeFileSync(path.join(root, ".env"), "SECRET=never-copy\n");
  fs.writeFileSync(path.join(root, ".env.example"), "SAFE=example\n");
  fs.writeFileSync(path.join(root, "private.key"), "never-copy\n");
  fixturePolicy(root, commandMode, fileMode);
  return root;
}

const state = fs.mkdtempSync(path.join(os.tmpdir(), "earthcall-mcp-state-"));
process.env.EARTHCALL_MCP_STATE_DIR = state;
const sandbox = require("../../src/Singularity/Foreign/mcp/earthcall-sandbox.js");

test("sandbox session copies working tree but excludes build/git style directories", () => {
  const repo = fixtureRepo();
  fs.mkdirSync(path.join(repo, "build"));
  fs.writeFileSync(path.join(repo, "build", "huge.bin"), "nope");
  const created = sandbox.createSession(repo, "test");
  const loaded = sandbox.loadSession(created.id);
  assert.equal(fs.readFileSync(path.join(loaded.workspace, "src", "hello.txt"), "utf8"), "hello\n");
  assert.equal(fs.existsSync(path.join(loaded.workspace, "build")), false);
  assert.equal(fs.existsSync(path.join(loaded.workspace, ".env")), false);
  assert.equal(fs.existsSync(path.join(loaded.workspace, "private.key")), false);
  assert.equal(fs.existsSync(path.join(loaded.workspace, ".env.example")), true);
});

test("default sandbox file edit is automatic and never changes source repo", () => {
  const repo = fixtureRepo("auto_except_review", "auto_sandbox");
  const created = sandbox.createSession(repo, "edit");
  const result = sandbox.writeSandboxFile(repo, created.id, "src/hello.txt", "changed\n");
  assert.equal(result.status, "written");
  assert.equal(result.host_repo_modified, false);
  assert.equal(fs.readFileSync(path.join(repo, "src", "hello.txt"), "utf8"), "hello\n");
  assert.equal(sandbox.readSandboxFile(created.id, "src/hello.txt").content, "1: changed\n2: ");
});

test("manual file mode emits local approval and exact approval can be consumed", () => {
  const repo = fixtureRepo("auto_except_review", "manual");
  const created = sandbox.createSession(repo, "manual");
  const pending = sandbox.writeSandboxFile(repo, created.id, "src/hello.txt", "approved later\n");
  assert.equal(pending.status, "approval_required");
  const file = sandbox._internal.approvalPath(pending.approval_id);
  const record = JSON.parse(fs.readFileSync(file, "utf8"));
  record.status = "approved";
  fs.writeFileSync(file, JSON.stringify(record, null, 2));
  const done = sandbox.writeSandboxFile(repo, created.id, "src/hello.txt", "approved later\n", pending.approval_id);
  assert.equal(done.status, "written");
  assert.throws(() => sandbox.writeSandboxFile(repo, created.id, "src/hello.txt", "different content\n", pending.approval_id), /does not match|consumed/);
});

test("sandbox diff reports workspace-only edits", () => {
  const repo = fixtureRepo();
  const created = sandbox.createSession(repo, "diff");
  sandbox.writeSandboxFile(repo, created.id, "src/hello.txt", "hello\nworld\n");
  sandbox.writeSandboxFile(repo, created.id, "src/new.txt", "new\n");
  const diff = sandbox.sandboxDiff(created.id, "src");
  assert.ok(diff.changed_files.some((x) => x.path === "src/hello.txt" && x.status === "modified"));
  assert.ok(diff.changed_files.some((x) => x.path === "src/new.txt" && x.status === "added"));
  assert.match(diff.diff, /\+world/);
});

test("command review creates approval before Docker is touched", async () => {
  const repo = fixtureRepo("auto_except_review", "auto_sandbox");
  const created = sandbox.createSession(repo, "command-review");
  const result = await sandbox.runDocker(repo, created.id, ["rm", "-rf", "tmp"], ".");
  assert.equal(result.status, "approval_required");
  assert.ok(result.reasons.some((r) => r === "program:rm"));
});

test("workspace path traversal is rejected", () => {
  const repo = fixtureRepo();
  const created = sandbox.createSession(repo, "jail");
  assert.throws(() => sandbox.readSandboxFile(created.id, "../outside"), /escapes/);
});


test("auto-approved command is wrapped in the hard Docker capability cage", async () => {
  const repo = fixtureRepo("auto", "auto_sandbox");
  const created = sandbox.createSession(repo, "fake-docker");
  const fakeBin = fs.mkdtempSync(path.join(os.tmpdir(), "earthcall-fake-docker-"));
  const fakeDocker = path.join(fakeBin, "docker");
  fs.writeFileSync(fakeDocker, `#!/usr/bin/env bash\nif [ "$1" = "image" ] && [ "$2" = "inspect" ]; then exit 0; fi\nprintf '%s\\n' "$@"\n`, { mode: 0o755 });
  const oldPath = process.env.PATH;
  process.env.PATH = `${fakeBin}:${oldPath}`;
  try {
    const result = await sandbox.runDocker(repo, created.id, ["node", "--version"], ".", 5);
    assert.equal(result.status, "completed");
    assert.equal(result.exit_code, 0);
    assert.match(result.stdout, /--network\nnone/);
    assert.match(result.stdout, /--read-only/);
    assert.match(result.stdout, /--cap-drop\nALL/);
    assert.match(result.stdout, /no-new-privileges=true/);
    assert.match(result.stdout, /--pull=never/);
    assert.match(result.stdout, /node\n--version/);
    assert.doesNotMatch(result.stdout, /docker\.sock/);
    assert.equal(result.network, "none");
    assert.equal(result.host_repo_writable, false);
  } finally {
    process.env.PATH = oldPath;
  }
});
