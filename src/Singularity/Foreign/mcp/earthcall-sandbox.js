"use strict";

const fs = require("fs");
const os = require("os");
const path = require("path");
const crypto = require("crypto");
const { execFile, execFileSync, spawnSync } = require("child_process");

const policyLib = require("./earthcall-sandbox-policy.js");

const COPY_SKIP_DIRS = new Set([
  ".git",
  "node_modules",
  "build",
  ".cache",
  ".idea",
  ".vscode",
  "DerivedData",
  ".venv",
  "venv",
  "dist",
  "secrets",
  ".ssh",
  ".aws",
  ".gnupg",
]);

const COPY_SKIP_FILES = new Set([
  ".env",
  ".npmrc",
  ".pypirc",
  ".netrc",
  "id_rsa",
  "id_ed25519",
  "credentials.json",
  "service-account.json",
]);
const COPY_SKIP_SUFFIXES = [".pem", ".key", ".p12", ".pfx"];

const MAX_EDIT_BYTES = 2 * 1024 * 1024;
const MAX_DIFF_FILES = 200;
const MAX_DIFF_FILE_BYTES = 2 * 1024 * 1024;

function nowIso() { return new Date().toISOString(); }
function randomId(bytes = 12) { return crypto.randomBytes(bytes).toString("hex"); }

function dataRoot() {
  if (process.env.EARTHCALL_MCP_STATE_DIR) return path.resolve(process.env.EARTHCALL_MCP_STATE_DIR);
  const home = os.homedir();
  if (process.platform === "darwin") {
    return path.join(home, "Library", "Application Support", "Earthcall", "gemini-mcp");
  }
  if (process.platform === "win32") {
    return path.join(process.env.LOCALAPPDATA || path.join(home, "AppData", "Local"), "Earthcall", "gemini-mcp");
  }
  return path.join(process.env.XDG_STATE_HOME || path.join(home, ".local", "state"), "earthcall", "gemini-mcp");
}

function ensurePrivateDir(dir) {
  fs.mkdirSync(dir, { recursive: true, mode: 0o700 });
  try { fs.chmodSync(dir, 0o700); } catch (_) {}
  return dir;
}

function approvalDir() { return ensurePrivateDir(path.join(dataRoot(), "approvals")); }
function sandboxDir() { return ensurePrivateDir(path.join(dataRoot(), "sandboxes")); }

function policyPath(projectRoot) {
  return process.env.EARTHCALL_SANDBOX_POLICY || path.join(projectRoot, "config", "earthcall-gemini-sandbox-policy.json");
}

function loadPolicy(projectRoot) {
  return policyLib.loadPolicy(policyPath(projectRoot));
}

function isInside(root, candidate) {
  const rel = path.relative(root, candidate);
  return rel === "" || (!rel.startsWith(`..${path.sep}`) && rel !== ".." && !path.isAbsolute(rel));
}

function requireRelativePath(requested) {
  if (typeof requested !== "string" || !requested || requested.includes("\0")) {
    throw new Error("Path must be a non-empty string without NUL bytes.");
  }
  if (path.isAbsolute(requested)) throw new Error("Sandbox paths must be relative.");
  return requested;
}

function safeWorkspacePath(workspace, requested, { allowMissing = false } = {}) {
  requireRelativePath(requested);
  const candidate = path.resolve(workspace, requested);
  if (!isInside(workspace, candidate)) throw new Error(`Path escapes sandbox workspace: ${requested}`);

  if (fs.existsSync(candidate)) {
    const resolved = fs.realpathSync.native(candidate);
    if (!isInside(workspace, resolved)) throw new Error(`Symlink/path escapes sandbox workspace: ${requested}`);
    return resolved;
  }
  if (!allowMissing) throw new Error(`Sandbox path does not exist: ${requested}`);

  let ancestor = path.dirname(candidate);
  while (!fs.existsSync(ancestor)) {
    const next = path.dirname(ancestor);
    if (next === ancestor) break;
    ancestor = next;
  }
  const resolvedAncestor = fs.realpathSync.native(ancestor);
  if (!isInside(workspace, resolvedAncestor)) throw new Error(`Parent path escapes sandbox workspace: ${requested}`);
  return candidate;
}

function shouldSkipCopy(name, isDir) {
  if (isDir) return COPY_SKIP_DIRS.has(name);
  const lower = String(name).toLowerCase();
  if (lower === ".env.example" || lower === ".env.sample") return false;
  if (COPY_SKIP_FILES.has(lower)) return true;
  if (lower.startsWith(".env.")) return true;
  return COPY_SKIP_SUFFIXES.some((suffix) => lower.endsWith(suffix));
}

function copyFileCow(src, dst, stat) {
  fs.mkdirSync(path.dirname(dst), { recursive: true });
  try {
    fs.copyFileSync(src, dst, fs.constants.COPYFILE_FICLONE);
  } catch (_) {
    fs.copyFileSync(src, dst);
  }
  try { fs.chmodSync(dst, stat.mode & 0o777); } catch (_) {}
  try { fs.utimesSync(dst, stat.atime, stat.mtime); } catch (_) {}
}

function copyTreeCow(sourceRoot, workspaceRoot, manifest, rel = ".") {
  const src = rel === "." ? sourceRoot : path.join(sourceRoot, rel);
  const entries = fs.readdirSync(src, { withFileTypes: true });
  for (const entry of entries) {
    if (shouldSkipCopy(entry.name, entry.isDirectory())) continue;
    if (!entry.isDirectory() && shouldSkipCopy(entry.name, false)) continue;
    if (entry.name === ".DS_Store") continue;

    const childRel = rel === "." ? entry.name : path.join(rel, entry.name);
    const srcPath = path.join(sourceRoot, childRel);
    const dstPath = path.join(workspaceRoot, childRel);

    if (entry.isSymbolicLink()) {
      let real;
      try { real = fs.realpathSync.native(srcPath); } catch (_) { continue; }
      if (!isInside(sourceRoot, real)) continue;
      const link = fs.readlinkSync(srcPath);
      fs.mkdirSync(path.dirname(dstPath), { recursive: true });
      fs.symlinkSync(link, dstPath);
      manifest[childRel.split(path.sep).join("/")] = { kind: "symlink", target: link };
      continue;
    }

    if (entry.isDirectory()) {
      fs.mkdirSync(dstPath, { recursive: true });
      copyTreeCow(sourceRoot, workspaceRoot, manifest, childRel);
      continue;
    }

    if (!entry.isFile()) continue;
    const stat = fs.statSync(srcPath);
    copyFileCow(srcPath, dstPath, stat);
    manifest[childRel.split(path.sep).join("/")] = {
      kind: "file",
      size: stat.size,
      mtimeMs: stat.mtimeMs,
    };
  }
}

function sessionRoot(sessionId) {
  if (!/^[a-f0-9]{24}$/.test(String(sessionId || ""))) throw new Error("Invalid sandbox session id.");
  return path.join(sandboxDir(), sessionId);
}

function sessionMetaPath(sessionId) { return path.join(sessionRoot(sessionId), "session.json"); }
function sessionManifestPath(sessionId) { return path.join(sessionRoot(sessionId), "manifest.json"); }
function sessionWorkspace(sessionId) { return path.join(sessionRoot(sessionId), "workspace"); }

function loadSession(sessionId) {
  const metaPath = sessionMetaPath(sessionId);
  if (!fs.existsSync(metaPath)) throw new Error(`Unknown sandbox session: ${sessionId}`);
  const meta = JSON.parse(fs.readFileSync(metaPath, "utf8"));
  const workspace = fs.realpathSync.native(sessionWorkspace(sessionId));
  return { ...meta, workspace };
}

function createSession(projectRoot, label = "") {
  const sourceRoot = fs.realpathSync.native(projectRoot);
  const id = randomId(12);
  const root = ensurePrivateDir(sessionRoot(id));
  const workspace = path.join(root, "workspace");
  fs.mkdirSync(workspace, { recursive: true, mode: 0o700 });

  const manifest = {};
  copyTreeCow(sourceRoot, workspace, manifest);

  const meta = {
    id,
    label: String(label || "").slice(0, 200),
    source_root: sourceRoot,
    created_at: nowIso(),
    environment: "linux-container",
    network: "none",
    source_copy: "local-working-tree-copy",
    note: "The sandbox copy includes current local files/uncommitted edits, excluding .git, node_modules, build/cache trees and other generated directories.",
  };
  fs.writeFileSync(sessionMetaPath(id), JSON.stringify(meta, null, 2), { mode: 0o600 });
  fs.writeFileSync(sessionManifestPath(id), JSON.stringify(manifest), { mode: 0o600 });
  return {
    id,
    label: meta.label,
    created_at: meta.created_at,
    environment: meta.environment,
    network: meta.network,
    source_copy: meta.source_copy,
    note: meta.note,
    workspace: "[private sandbox workspace]",
    baseline_files: Object.keys(manifest).length,
  };
}

function deleteSession(sessionId) {
  const root = sessionRoot(sessionId);
  if (!fs.existsSync(root)) return { session_id: sessionId, deleted: false, note: "already absent" };
  fs.rmSync(root, { recursive: true, force: true });
  return { session_id: sessionId, deleted: true };
}

function approvalPath(id) {
  if (!/^[a-f0-9]{24}$/.test(String(id || ""))) throw new Error("Invalid approval id.");
  return path.join(approvalDir(), `${id}.json`);
}

function createApproval(action, reasons) {
  const id = randomId(12);
  const record = {
    id,
    status: "pending",
    created_at: nowIso(),
    reasons: Array.isArray(reasons) ? reasons : [],
    fingerprint: policyLib.fingerprintAction(action),
    action,
  };
  fs.writeFileSync(approvalPath(id), JSON.stringify(record, null, 2), { mode: 0o600 });
  return { approval_id: id, status: "pending", reasons: record.reasons, fingerprint: record.fingerprint };
}

function getApproval(id) {
  const file = approvalPath(id);
  if (!fs.existsSync(file)) throw new Error(`Unknown approval id: ${id}`);
  const record = JSON.parse(fs.readFileSync(file, "utf8"));
  return {
    approval_id: record.id,
    status: record.status,
    created_at: record.created_at,
    decided_at: record.decided_at || null,
    reasons: record.reasons || [],
    fingerprint: record.fingerprint,
    action_summary: summarizeAction(record.action),
  };
}

function summarizeAction(action) {
  if (action.type === "command") {
    return { type: "command", session_id: action.session_id, cwd: action.cwd, argv: action.argv };
  }
  if (action.type === "file_edit") {
    return {
      type: "file_edit",
      session_id: action.session_id,
      path: action.path,
      content_bytes: Buffer.byteLength(action.content || "", "utf8"),
      content_sha256: crypto.createHash("sha256").update(action.content || "").digest("hex"),
    };
  }
  return { type: action.type || "unknown" };
}

function consumeApproval(id, action) {
  const file = approvalPath(id);
  if (!fs.existsSync(file)) throw new Error(`Unknown approval id: ${id}`);
  const record = JSON.parse(fs.readFileSync(file, "utf8"));
  const fingerprint = policyLib.fingerprintAction(action);
  if (record.fingerprint !== fingerprint) throw new Error("Approval does not match this exact action.");
  if (record.status !== "approved") throw new Error(`Approval ${id} is ${record.status}, not approved.`);
  record.status = "consumed";
  record.consumed_at = nowIso();
  fs.writeFileSync(file, JSON.stringify(record, null, 2), { mode: 0o600 });
  return true;
}

function authorize(projectRoot, kind, action, approvalId = null) {
  const policy = loadPolicy(projectRoot);
  const evaluation = kind === "command"
    ? policyLib.evaluateCommand(policy, action.argv)
    : policyLib.evaluateFileEdit(policy, action.path);

  if (evaluation.decision === "allow") return { allowed: true, policy, evaluation };
  if (approvalId) {
    consumeApproval(approvalId, action);
    return { allowed: true, policy, evaluation, approval_id: approvalId, approved_manually: true };
  }
  return { allowed: false, policy, evaluation, approval: createApproval(action, evaluation.reasons) };
}

function clampNumber(value, fallback, min, max) {
  const n = Number(value);
  if (!Number.isFinite(n)) return fallback;
  return Math.max(min, Math.min(max, n));
}

function runDocker(projectRoot, sessionId, argv, cwd = ".", timeoutSeconds = null, approvalId = null) {
  const session = loadSession(sessionId);
  if (!Array.isArray(argv) || argv.length === 0 || argv.some((x) => typeof x !== "string" || x.includes("\0"))) {
    throw new Error("argv must be a non-empty array of normal strings.");
  }
  if (argv.length > 128) throw new Error("Too many argv entries.");
  if (argv.some((x) => x.length > 8192)) throw new Error("One or more argv entries are too long.");

  const cwdPath = cwd === "." ? session.workspace : safeWorkspacePath(session.workspace, cwd);
  const relCwd = path.relative(session.workspace, cwdPath).split(path.sep).join("/") || ".";
  const action = { type: "command", session_id: sessionId, cwd: relCwd, argv };
  const auth = authorize(projectRoot, "command", action, approvalId);
  if (!auth.allowed) {
    return {
      ...auth.approval,
      status: "approval_required",
      action: summarizeAction(action),
      instruction: `Approve locally with: node scripts/earthcall-mcp-approve.js ${auth.approval.approval_id}`,
    };
  }

  const policy = auth.policy;
  const limits = policy.limits || {};
  const timeout = clampNumber(timeoutSeconds, limits.timeoutSeconds || 180, 1, limits.maxTimeoutSeconds || 900);
  const image = process.env.EARTHCALL_SANDBOX_IMAGE || policy.image || "earthcall-sandbox:latest";

  try {
    execFileSync("docker", ["image", "inspect", image], { stdio: "ignore", timeout: 10000 });
  } catch (_) {
    throw new Error(`Sandbox image '${image}' is not available. Run ./scripts/build-gemini-sandbox.sh first.`);
  }

  if (session.workspace.includes(",")) throw new Error("Sandbox workspace path contains a comma; Docker --mount cannot represent it safely.");

  const runUid = typeof process.getuid === "function" ? process.getuid() : 65534;
  const runGid = typeof process.getgid === "function" ? process.getgid() : 65534;
  const dockerArgs = [
    "run", "--rm", "--pull=never", "--init",
    "--network", "none",
    "--read-only",
    "--cap-drop", "ALL",
    "--security-opt", "no-new-privileges=true",
    "--user", `${runUid}:${runGid}`,
    "--pids-limit", String(Math.max(32, Number(limits.pids || 256))),
    "--memory", String(limits.memory || "4g"),
    "--cpus", String(limits.cpus || 4),
    "--tmpfs", "/tmp:rw,nosuid,nodev,size=512m,mode=1777",
    "--tmpfs", "/home/earthcall:rw,nosuid,nodev,size=64m,mode=1777",
    "--mount", `type=bind,source=${session.workspace},target=/workspace`,
    "--workdir", `/workspace/${relCwd === "." ? "" : relCwd}`,
    "--env", "HOME=/home/earthcall",
    "--env", "CI=1",
    image,
    ...argv,
  ];

  const started = Date.now();
  return new Promise((resolve) => {
    execFile("docker", dockerArgs, {
      timeout: Math.ceil(timeout * 1000),
      maxBuffer: 8 * 1024 * 1024,
      encoding: "utf8",
      env: { PATH: process.env.PATH || "/usr/bin:/bin:/usr/local/bin" },
    }, (error, stdout, stderr) => {
      const maxChars = Math.max(1000, Number(limits.maxOutputChars || 250000));
      const clip = (s) => {
        const text = String(s || "");
        return text.length <= maxChars
          ? { text, truncated: false }
          : { text: text.slice(0, maxChars), truncated: true, omitted_chars: text.length - maxChars };
      };
      const out = clip(stdout);
      const err = clip(stderr);
      const timedOut = Boolean(error && (error.killed || error.signal === "SIGTERM") && Date.now() - started >= timeout * 1000 - 1000);
      resolve({
        status: "completed",
        session_id: sessionId,
        environment: "linux-container",
        sandbox_image: image,
        network: "none",
        host_repo_writable: false,
        cwd: relCwd,
        argv,
        exit_code: error && typeof error.code === "number" ? error.code : (error ? 1 : 0),
        signal: error?.signal || null,
        timed_out: timedOut,
        duration_ms: Date.now() - started,
        stdout: out.text,
        stderr: err.text,
        stdout_truncated: out.truncated,
        stderr_truncated: err.truncated,
        ...(out.omitted_chars ? { stdout_omitted_chars: out.omitted_chars } : {}),
        ...(err.omitted_chars ? { stderr_omitted_chars: err.omitted_chars } : {}),
        ...(auth.approved_manually ? { approval_id: auth.approval_id, approval: "manual" } : { approval: "automatic" }),
      });
    });
  });
}

function readSandboxFile(sessionId, requestedPath, startLine = 1, endLine = null) {
  const session = loadSession(sessionId);
  const file = safeWorkspacePath(session.workspace, requestedPath);
  const stat = fs.statSync(file);
  if (!stat.isFile()) throw new Error("Not a regular file.");
  if (stat.size > MAX_EDIT_BYTES * 2) throw new Error("File too large for sandbox read tool.");
  const buffer = fs.readFileSync(file);
  if (buffer.subarray(0, Math.min(buffer.length, 8192)).includes(0)) throw new Error("Binary file refused.");
  const lines = buffer.toString("utf8").split(/\r?\n/);
  const start = Math.max(1, Math.min(lines.length || 1, Number(startLine) || 1));
  const end = endLine == null ? Math.min(lines.length, start + 1999) : Math.max(start, Math.min(lines.length, Number(endLine) || start));
  return {
    session_id: sessionId,
    path: requestedPath,
    start_line: start,
    end_line: end,
    total_lines: lines.length,
    content: lines.slice(start - 1, end).map((line, i) => `${start + i}: ${line}`).join("\n"),
    ...(end < lines.length ? { next_start_line: end + 1 } : {}),
  };
}

function writeSandboxFile(projectRoot, sessionId, requestedPath, content, approvalId = null) {
  if (typeof content !== "string") throw new Error("content must be a string");
  const bytes = Buffer.byteLength(content, "utf8");
  if (bytes > MAX_EDIT_BYTES) throw new Error(`Sandbox edit exceeds ${MAX_EDIT_BYTES} bytes.`);
  const session = loadSession(sessionId);
  const target = safeWorkspacePath(session.workspace, requestedPath, { allowMissing: true });
  const rel = path.relative(session.workspace, target).split(path.sep).join("/");
  if (!rel || rel === ".") throw new Error("Cannot overwrite workspace root.");

  const action = { type: "file_edit", session_id: sessionId, path: rel, content };
  const auth = authorize(projectRoot, "file", action, approvalId);
  if (!auth.allowed) {
    return {
      ...auth.approval,
      status: "approval_required",
      action: summarizeAction(action),
      instruction: `Approve locally with: node scripts/earthcall-mcp-approve.js ${auth.approval.approval_id}`,
    };
  }

  fs.mkdirSync(path.dirname(target), { recursive: true });
  const tmp = `${target}.earthcall-mcp-${randomId(4)}.tmp`;
  fs.writeFileSync(tmp, content, { encoding: "utf8", mode: 0o600 });
  fs.renameSync(tmp, target);
  return {
    status: "written",
    session_id: sessionId,
    path: rel,
    bytes,
    sha256: crypto.createHash("sha256").update(content).digest("hex"),
    workspace_only: true,
    host_repo_modified: false,
    ...(auth.approved_manually ? { approval_id: auth.approval_id, approval: "manual" } : { approval: "automatic" }),
  };
}

function walkWorkspace(workspace, rel, out) {
  const abs = rel === "." ? workspace : path.join(workspace, rel);
  let entries;
  try { entries = fs.readdirSync(abs, { withFileTypes: true }); } catch (_) { return; }
  for (const entry of entries) {
    if (entry.isDirectory() && COPY_SKIP_DIRS.has(entry.name)) continue;
    const childRel = rel === "." ? entry.name : path.join(rel, entry.name);
    const normalized = childRel.split(path.sep).join("/");
    if (entry.isSymbolicLink()) {
      out.set(normalized, { kind: "symlink", target: fs.readlinkSync(path.join(workspace, childRel)) });
    } else if (entry.isDirectory()) {
      walkWorkspace(workspace, childRel, out);
    } else if (entry.isFile()) {
      const stat = fs.statSync(path.join(workspace, childRel));
      out.set(normalized, { kind: "file", size: stat.size, mtimeMs: stat.mtimeMs });
    }
  }
}

function sha256File(file) {
  const h = crypto.createHash("sha256");
  h.update(fs.readFileSync(file));
  return h.digest("hex");
}

function diffPair(oldPath, newPath, labelPath) {
  const args = ["diff", "--no-index", "--no-color", "--no-ext-diff", "--", oldPath, newPath];
  const r = spawnSync("git", args, { encoding: "utf8", maxBuffer: 4 * 1024 * 1024, env: { ...process.env, GIT_PAGER: "cat" } });
  if (![0, 1].includes(r.status)) return `--- ${labelPath}\n(diff unavailable: ${String(r.stderr || "git diff failed").trim()})\n`;
  return String(r.stdout || "");
}

function sandboxDiff(sessionId, requestedPath = null, maxChars = 250000) {
  const session = loadSession(sessionId);
  const manifest = JSON.parse(fs.readFileSync(sessionManifestPath(sessionId), "utf8"));
  const current = new Map();
  walkWorkspace(session.workspace, ".", current);

  const prefix = requestedPath ? requireRelativePath(requestedPath).replace(/\\/g, "/").replace(/\/$/, "") : null;
  const included = (p) => !prefix || p === prefix || p.startsWith(`${prefix}/`);

  const all = new Set([...Object.keys(manifest), ...current.keys()]);
  const changed = [];
  for (const rel of [...all].sort()) {
    if (!included(rel)) continue;
    const before = manifest[rel];
    const after = current.get(rel);
    if (!before) { changed.push({ path: rel, status: "added" }); continue; }
    if (!after) { changed.push({ path: rel, status: "deleted" }); continue; }
    if (before.kind !== after.kind) { changed.push({ path: rel, status: "type-changed" }); continue; }
    if (before.kind === "symlink") {
      if (before.target !== after.target) changed.push({ path: rel, status: "modified" });
      continue;
    }
    if (before.size === after.size && Math.abs((before.mtimeMs || 0) - (after.mtimeMs || 0)) < 1) continue;
    const source = path.join(session.source_root, rel);
    const work = path.join(session.workspace, rel);
    if (!fs.existsSync(source) || sha256File(source) !== sha256File(work)) changed.push({ path: rel, status: "modified" });
  }

  const limited = changed.slice(0, MAX_DIFF_FILES);
  let output = "";
  for (const item of limited) {
    if (output.length >= maxChars) break;
    const source = path.join(session.source_root, item.path);
    const work = path.join(session.workspace, item.path);
    const oldPath = fs.existsSync(source) ? source : "/dev/null";
    const newPath = fs.existsSync(work) ? work : "/dev/null";
    let tooLarge = false;
    for (const p of [source, work]) {
      try { if (fs.existsSync(p) && fs.statSync(p).size > MAX_DIFF_FILE_BYTES) tooLarge = true; } catch (_) {}
    }
    if (tooLarge) {
      output += `\n${item.status.toUpperCase()} ${item.path} (diff omitted: file > ${MAX_DIFF_FILE_BYTES} bytes)\n`;
    } else {
      output += `\n# ${item.status.toUpperCase()} ${item.path}\n${diffPair(oldPath, newPath, item.path)}`;
    }
  }
  const truncated = changed.length > limited.length || output.length > maxChars;
  if (output.length > maxChars) output = output.slice(0, maxChars);
  return {
    session_id: sessionId,
    changed_files: changed,
    returned_files: limited.length,
    truncated,
    diff: output || "(no changes)",
  };
}

module.exports = {
  dataRoot,
  loadPolicy,
  createSession,
  loadSession,
  deleteSession,
  runDocker,
  readSandboxFile,
  writeSandboxFile,
  sandboxDiff,
  getApproval,
  createApproval,
  consumeApproval,
  _internal: {
    safeWorkspacePath,
    copyTreeCow,
    summarizeAction,
    isInside,
    approvalPath,
    sessionRoot,
  },
};
