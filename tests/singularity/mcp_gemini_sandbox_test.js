"use strict";

const test = require("node:test");
const assert = require("node:assert/strict");
const fs = require("fs");
const os = require("os");
const path = require("path");
const { execFileSync, spawnSync } = require("child_process");

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

const importScript = path.resolve(__dirname, "../../scripts/earthcall-sandbox-import.js");

function importFixture(t) {
  const repo = fixtureRepo();
  t.after(() => fs.rmSync(repo, { recursive: true, force: true }));
  function git(...args) {
    return execFileSync("git", ["-c", "core.fsmonitor=false", ...args], { cwd: repo, encoding: "utf8" });
  }
  git("init", "-q");
  fs.writeFileSync(path.join(repo, ".gitignore"), "generated/\n");
  fs.chmodSync(path.join(repo, "scripts/build.sh"), 0o755);
  fs.writeFileSync(path.join(repo, "src/remove me.txt"), "remove me\n");
  git("add", ".");
  fs.appendFileSync(path.join(repo, "src/hello.txt"), "Zach's pre-existing edit\n");
  const { id } = sandbox.createSession(repo, "test handoff");
  const { workspace } = sandbox.loadSession(id);
  t.after(() => sandbox.deleteSession(id));
  function cli(...args) {
    return spawnSync(process.execPath, [importScript, ...args], {
      cwd: repo, encoding: "utf8", env: process.env,
    });
  }
  return { repo, workspace, id, git, cli };
}

test("local import previews and transfers only sandbox changes without staging", (t) => {
  const { repo, workspace, id, git, cli } = importFixture(t);
  const indexBefore = git("ls-files", "--stage");
  const hello = path.join(repo, "src/hello.txt");
  const hostBackup = fs.readFileSync(hello, "utf8");
  const incoming = hostBackup + "Gemini's edit\n";
  sandbox.writeSandboxFile(repo, id, "src/hello.txt", incoming);
  sandbox.writeSandboxFile(repo, id, "scripts/build.sh", "echo updated\n");
  fs.unlinkSync(path.join(workspace, "src/remove me.txt"));
  const asset = Buffer.from([0, 255, 17, 31, 202, 42]);
  fs.writeFileSync(path.join(workspace, "asset copy.bin"), asset);
  fs.mkdirSync(path.join(workspace, "generated"));
  fs.writeFileSync(path.join(workspace, "generated/log.txt"), "generated\n");
  // A checkout-only edit to an untouched sandbox file must never be reverted.
  const policy = path.join(repo, "config/earthcall-gemini-sandbox-policy.json");
  fs.appendFileSync(policy, "\n");
  const localPolicy = fs.readFileSync(policy, "utf8");

  assert.match(cli("list").stdout, new RegExp(id));
  const preview = cli("preview", "test handoff");
  assert.equal(preview.status, 0, preview.stderr);
  assert.match(preview.stdout, /\+Gemini's edit/);
  assert.match(preview.stdout, /deleted file mode/);
  assert.match(preview.stdout, /GIT binary patch/);
  assert.doesNotMatch(preview.stdout, /generated\/log|earthcall-gemini-sandbox-policy/);
  assert.equal(fs.readFileSync(hello, "utf8"), hostBackup);

  const applied = cli("apply", id);
  assert.equal(applied.status, 0, applied.stderr);
  assert.equal(fs.readFileSync(hello, "utf8"), incoming);
  assert.equal(fs.readFileSync(path.join(repo, "scripts/build.sh"), "utf8"), "echo updated\n");
  assert.ok(fs.statSync(path.join(repo, "scripts/build.sh")).mode & 0o100);
  assert.deepEqual(fs.readFileSync(path.join(repo, "asset copy.bin")), asset);
  assert.equal(fs.existsSync(path.join(repo, "src/remove me.txt")), false);
  assert.equal(fs.existsSync(path.join(repo, "generated")), false);
  assert.equal(fs.readFileSync(policy, "utf8"), localPolicy);
  assert.equal(git("ls-files", "--stage"), indexBefore);
  assert.match(cli("apply", id).stdout, /No sandbox changes/);
  assert.equal(fs.readFileSync(path.join(workspace, "src/hello.txt"), "utf8"), incoming);
});

test("local import refuses a batch with overlaps and supports selecting unaffected paths", (t) => {
  const { repo, workspace, id, cli } = importFixture(t);
  fs.writeFileSync(path.join(workspace, "src/hello.txt"), "sandbox edit\n");
  fs.writeFileSync(path.join(repo, "src/hello.txt"), "intervening checkout edit\n");
  fs.writeFileSync(path.join(workspace, "src/ready.txt"), "ready\n");
  const blocked = cli("apply", id);
  assert.equal(blocked.status, 1);
  assert.match(blocked.stderr, /src\/hello.txt.*checkout differs/);
  assert.equal(fs.existsSync(path.join(repo, "src/ready.txt")), false);
  const applied = cli("apply", id, "--", "src/ready.txt");
  assert.equal(applied.status, 0, applied.stderr);
  assert.equal(fs.readFileSync(path.join(repo, "src/ready.txt"), "utf8"), "ready\n");
  assert.equal(fs.readFileSync(path.join(repo, "src/hello.txt"), "utf8"), "intervening checkout edit\n");
  assert.equal(cli("apply", id, "--", "../elsewhere").status, 1);
});

test("local import refuses checkout symlinks and a different checkout", (t) => {
  const { repo, workspace, id, cli } = importFixture(t);
  const outside = fs.mkdtempSync(path.join(os.tmpdir(), "earthcall-import-outside-"));
  t.after(() => fs.rmSync(outside, { recursive: true, force: true }));
  execFileSync("git", ["init", "-q", outside]);
  fs.mkdirSync(path.join(workspace, "linked"));
  fs.writeFileSync(path.join(workspace, "linked", "new.txt"), "escape\n");
  fs.symlinkSync(outside, path.join(repo, "linked"), "dir");
  assert.equal(cli("apply", id).status, 1);
  assert.equal(fs.existsSync(path.join(outside, "new.txt")), false);
  const wrong = spawnSync(process.execPath, [importScript, "apply", id], { cwd: outside, encoding: "utf8" });
  assert.equal(wrong.status, 1);
  assert.match(wrong.stderr, /belongs to/);
});

test("Mac companion applies only the patch returned by its preview", (t) => {
  const { repo, workspace, id, cli } = importFixture(t);
  const incoming = path.join(workspace, "reviewed.txt");
  const destination = path.join(repo, "reviewed.txt");
  fs.writeFileSync(incoming, "original preview\n");
  const listing = JSON.parse(cli("--json", "list").stdout);
  assert.ok(listing.some((session) => session.id === id && session.source_root === fs.realpathSync(repo)));
  const first = JSON.parse(cli("--json", "preview", id).stdout);
  assert.match(first.diff, /\+original preview/);

  fs.writeFileSync(incoming, "updated after preview\n");
  const refused = cli("apply", id, "--expected-diff", first.digest);
  assert.equal(refused.status, 1);
  assert.match(refused.stderr, /differ from the reviewed preview/);
  assert.equal(fs.existsSync(destination), false);

  const second = JSON.parse(cli("--json", "preview", id).stdout);
  assert.notEqual(second.digest, first.digest);
  const applied = cli("apply", id, "--expected-diff", second.digest);
  assert.equal(applied.status, 0, applied.stderr);
  assert.equal(fs.readFileSync(destination, "utf8"), "updated after preview\n");
  assert.deepEqual(JSON.parse(cli("--json", "preview", id).stdout), { diff: "", digest: null });
});

test("repeated sandbox imports append new bytes and preserve divergent checkout edits", (t) => {
  const { repo, workspace, id, cli } = importFixture(t);
  const rel = "src/hello.txt";
  const local = path.join(repo, rel);
  const incoming = path.join(workspace, rel);
  const initial = fs.readFileSync(local, "utf8");
  const first = "First Horizon pass\n";
  const second = "Second Horizon pass\n";

  fs.appendFileSync(incoming, first);
  const initialImport = cli("apply", id);
  assert.equal(initialImport.status, 0, initialImport.stderr);
  assert.equal(fs.readFileSync(local, "utf8"), initial + first);

  fs.appendFileSync(incoming, second);
  const preview = cli("--json", "preview", id);
  assert.equal(preview.status, 0, preview.stderr);
  const { diff, digest } = JSON.parse(preview.stdout);
  assert.deepEqual(diff.split("\n").filter((line) => /^\+[^+]/.test(line)), ["+Second Horizon pass"]);
  const applied = cli("apply", id, "--expected-diff", digest);
  assert.equal(applied.status, 0, applied.stderr);
  assert.equal(fs.readFileSync(local, "utf8"), initial + first + second);
  assert.match(cli("apply", id).stdout, /No sandbox changes/);

  fs.appendFileSync(local, "Checkout-only reply\n");
  fs.appendFileSync(incoming, "A different, longer sandbox reply\n");
  const refused = cli("apply", id);
  assert.equal(refused.status, 1);
  assert.match(refused.stderr, /checkout differs from the sandbox baseline/);
  assert.equal(fs.readFileSync(local, "utf8"), initial + first + second + "Checkout-only reply\n");
});
