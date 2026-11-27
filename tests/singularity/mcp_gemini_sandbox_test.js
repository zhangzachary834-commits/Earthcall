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
  fs.mkdirSync(path.join(repo, ".claude", "worktrees", "nested-checkout"), { recursive: true });
  fs.writeFileSync(path.join(repo, ".claude", "worktrees", "nested-checkout", "huge.zip"), "agent scratch");
  fs.writeFileSync(path.join(repo, ".claude", "project-note.txt"), "keep project metadata\n");
  const created = sandbox.createSession(repo, "test");
  const loaded = sandbox.loadSession(created.id);
  const baseline = path.join(path.dirname(loaded.workspace), "baseline");
  assert.equal(fs.readFileSync(path.join(loaded.workspace, "src", "hello.txt"), "utf8"), "hello\n");
  assert.equal(fs.existsSync(path.join(loaded.workspace, "build")), false);
  assert.equal(fs.existsSync(path.join(loaded.workspace, ".env")), false);
  assert.equal(fs.existsSync(path.join(loaded.workspace, "private.key")), false);
  assert.equal(fs.existsSync(path.join(loaded.workspace, ".env.example")), true);
  assert.equal(fs.existsSync(path.join(loaded.workspace, ".claude", "worktrees")), false);
  assert.equal(fs.existsSync(path.join(baseline, ".claude", "worktrees")), false);
  assert.equal(fs.readFileSync(path.join(loaded.workspace, ".claude", "project-note.txt"), "utf8"),
    "keep project metadata\n");
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

function importFixture(t, initialText = null) {
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
  if (initialText !== null) fs.writeFileSync(path.join(repo, "src/hello.txt"), initialText);
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
  assert.match(blocked.stderr, /src\/hello.txt.*Overlapping edits/);
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
  const empty = JSON.parse(cli("--json", "preview", id).stdout);
  assert.equal(empty.diff, "");
  assert.equal(empty.digest, null);
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
  assert.match(refused.stderr, /Overlapping edits/);
  assert.equal(fs.readFileSync(local, "utf8"), initial + first + second + "Checkout-only reply\n");
});

test("checkout transplants merge independent edits and keep working in both directions", (t) => {
  const base = "first\nsecond\nthird\nfourth\nfifth\nsixth\n";
  const { repo, workspace, id, cli } = importFixture(t, base);
  const upstream = path.join(repo, "src/hello.txt");
  const local = path.join(workspace, "src/hello.txt");
  fs.writeFileSync(local, base.replace("sixth", "Gemini sixth"));
  fs.writeFileSync(upstream, base.replace("first", "Checkout first"));
  const before = fs.readFileSync(upstream);
  const previewResult = cli("--json", "sync-preview", id);
  assert.equal(previewResult.status, 0, previewResult.stderr);
  const preview = JSON.parse(previewResult.stdout);
  assert.deepEqual(preview.conflicts, []);
  assert.match(preview.diff, /\+Checkout first/);
  assert.doesNotMatch(preview.diff, /\+Gemini sixth/);
  assert.equal(fs.readFileSync(local, "utf8"), base.replace("sixth", "Gemini sixth"));
  const applied = cli("sync-apply", id, "--expected-diff", preview.digest);
  assert.equal(applied.status, 0, applied.stderr);
  assert.deepEqual(fs.readFileSync(upstream), before);
  const merged = base.replace("sixth", "Gemini sixth").replace("first", "Checkout first");
  assert.equal(fs.readFileSync(local, "utf8"), merged);

  fs.writeFileSync(upstream, before.toString().replace("second", "Checkout second"));
  assert.equal(cli("sync-apply", id).status, 0);
  assert.equal(fs.readFileSync(local, "utf8"), merged.replace("second", "Checkout second"));
  // Bring Gemini's independent edit back through the ordinary import button.
  const imported = cli("apply", id);
  assert.equal(imported.status, 0, imported.stderr);
  assert.equal(fs.readFileSync(upstream, "utf8"), merged.replace("second", "Checkout second"));
  assert.equal(JSON.parse(cli("--json", "sync-preview", id).stdout).digest, null);
});

test("checkout transplants report text conflicts and refuse the whole batch", (t) => {
  const { repo, workspace, id, cli } = importFixture(t);
  fs.writeFileSync(path.join(repo, "src/hello.txt"), "checkout rewrite\n");
  fs.writeFileSync(path.join(workspace, "src/hello.txt"), "Gemini rewrite\n");
  fs.writeFileSync(path.join(repo, "src/clean.txt"), "independent new file\n");
  const result = cli("--json", "sync-preview", id);
  assert.equal(result.status, 0, result.stderr);
  const preview = JSON.parse(result.stdout);
  assert.equal(preview.digest, null);
  assert.equal(preview.conflicts.length, 1);
  assert.equal(preview.conflicts[0].path, "src/hello.txt");
  assert.match(preview.diff, /<<<<<<< Sandbox/);
  assert.match(preview.diff, /\|\|\|\|\|\|\| Shared baseline/);
  assert.match(preview.diff, />>>>>>> Checkout/);
  assert.equal(cli("sync-apply", id).status, 1);
  assert.equal(fs.existsSync(path.join(workspace, "src/clean.txt")), false);
  assert.equal(fs.readFileSync(path.join(workspace, "src/hello.txt"), "utf8"), "Gemini rewrite\n");
  const scoped = cli("sync-apply", id, "--", "src/clean.txt");
  assert.equal(scoped.status, 0, scoped.stderr);
  assert.equal(fs.readFileSync(path.join(workspace, "src/clean.txt"), "utf8"), "independent new file\n");
  // An explicit file choice is bound to a fresh review before any overwrite.
  const chosen = JSON.parse(cli("--json", "sync-preview", id,
    "--resolve-ours", "src/hello.txt").stdout);
  assert.deepEqual(chosen.conflicts, []);
  const resolved = cli("sync-apply", id, "--expected-diff", chosen.digest,
    "--resolve-ours", "src/hello.txt");
  assert.equal(resolved.status, 0, resolved.stderr);
  assert.equal(fs.readFileSync(path.join(workspace, "src/hello.txt"), "utf8"), "Gemini rewrite\n");
  assert.equal(JSON.parse(cli("--json", "sync-preview", id).stdout).digest, null);
});

test("checkout transplants preserve binary bytes, deletions, file modes and review digests", (t) => {
  const { repo, workspace, id, cli } = importFixture(t);
  const bytes = Buffer.from([0, 255, 13, 12, 44]);
  fs.writeFileSync(path.join(repo, "new binary.bin"), bytes);
  fs.unlinkSync(path.join(repo, "src/remove me.txt"));
  fs.writeFileSync(path.join(repo, "scripts/build.sh"), "echo checkout\n");
  const preview = JSON.parse(cli("--json", "sync-preview", id).stdout);
  assert.match(preview.diff, /GIT binary patch/);
  fs.writeFileSync(path.join(repo, "new binary.bin"), Buffer.from([0, 255, 1]));
  assert.equal(cli("sync-apply", id, "--expected-diff", preview.digest).status, 1);
  assert.equal(fs.existsSync(path.join(workspace, "new binary.bin")), false);
  fs.writeFileSync(path.join(repo, "new binary.bin"), bytes);
  const applied = cli("sync-apply", id, "--expected-diff", preview.digest);
  assert.equal(applied.status, 0, applied.stderr);
  assert.deepEqual(fs.readFileSync(path.join(workspace, "new binary.bin")), bytes);
  assert.equal(fs.existsSync(path.join(workspace, "src/remove me.txt")), false);
  assert.ok(fs.statSync(path.join(workspace, "scripts/build.sh")).mode & 0o100);
  fs.writeFileSync(path.join(repo, "new binary.bin"), Buffer.from([0, 101]));
  fs.writeFileSync(path.join(workspace, "new binary.bin"), Buffer.from([0, 102]));
  const conflicts = JSON.parse(cli("--json", "sync-preview", id).stdout).conflicts;
  assert.match(conflicts[0].reason, /binary/);
});

test("older sessions bootstrap from unchanged bytes and refuse missing ancestors", (t) => {
  const { repo, workspace, id, cli } = importFixture(t);
  fs.rmSync(path.join(path.dirname(workspace), "baseline"), { recursive: true });
  fs.writeFileSync(path.join(repo, "src/hello.txt"), "new checkout contents\n");
  const initial = cli("sync-apply", id);
  assert.equal(initial.status, 0, initial.stderr);
  assert.equal(fs.readFileSync(path.join(workspace, "src/hello.txt"), "utf8"), "new checkout contents\n");
  // Exact checkpoints survive the first sync even for files unchanged that time.
  fs.writeFileSync(path.join(repo, "scripts/build.sh"), "echo checkout\n");
  fs.writeFileSync(path.join(workspace, "scripts/build.sh"), "echo Gemini\n");
  const preview = JSON.parse(cli("--json", "sync-preview", id).stdout);
  assert.match(preview.conflicts.find((c) => c.path === "scripts/build.sh").reason, /Overlapping/);

  fs.rmSync(path.join(path.dirname(workspace), "sync"), { recursive: true });
  const missing = JSON.parse(cli("--json", "sync-preview", id).stdout);
  assert.match(missing.conflicts.find((c) => c.path === "scripts/build.sh").reason, /Original contents unavailable/);
});

test("interrupted transplants resume from retained before and after objects", (t) => {
  const { repo, workspace, id, cli } = importFixture(t);
  fs.writeFileSync(path.join(repo, "aaa.txt"), "first transfer\n");
  fs.writeFileSync(path.join(repo, "src/hello.txt"), "second transfer\n");
  const failed = spawnSync(process.execPath, ["-e", `
    const fs = require("node:fs");
    const original = fs.renameSync;
    fs.renameSync = function(from, to) {
      if (to === process.env.TRANSPLANT_FAILURE_PATH) throw Error("Injected disk failure");
      return original(from, to);
    };
    process.argv = [process.execPath, process.env.TRANSPLANT_SCRIPT, "sync-apply", process.env.TRANSPLANT_SESSION];
    require(process.env.TRANSPLANT_SCRIPT);
  `], { cwd: repo, encoding: "utf8", env: {
    ...process.env, TRANSPLANT_FAILURE_PATH: path.join(workspace, "src/hello.txt"),
    TRANSPLANT_SCRIPT: importScript, TRANSPLANT_SESSION: id,
  } });
  assert.equal(failed.status, 1);
  assert.match(failed.stderr, /Injected disk failure/);
  assert.equal(fs.readFileSync(path.join(workspace, "aaa.txt"), "utf8"), "first transfer\n");
  const recovery = JSON.parse(cli("--json", "sync-preview", id).stdout);
  assert.equal(recovery.recovery, true);
  const lock = path.join(path.dirname(workspace), "sync/lock");
  fs.writeFileSync(lock, JSON.stringify({ pid: process.pid }));
  assert.match(cli("sync-apply", id).stderr, /Another transplant is running/);
  const exited = spawnSync(process.execPath, ["-e", ""], { encoding: "utf8" });
  assert.equal(exited.status, 0, exited.stderr);
  assert.ok(Number.isSafeInteger(exited.pid) && exited.pid > 0);
  assert.throws(() => process.kill(exited.pid, 0), (error) => error?.code === "ESRCH");
  const replacement = lock + ".stale-test";
  fs.writeFileSync(replacement, JSON.stringify({ pid: exited.pid }));
  fs.renameSync(replacement, lock);
  assert.deepEqual(JSON.parse(fs.readFileSync(lock, "utf8")), { pid: exited.pid });
  const resumed = cli("sync-apply", id, "--expected-diff", recovery.digest);
  assert.equal(resumed.status, 0, resumed.stderr);
  assert.equal(fs.readFileSync(path.join(workspace, "src/hello.txt"), "utf8"), "second transfer\n");
  assert.equal(JSON.parse(cli("--json", "sync-preview", id).stdout).digest, null);
});
