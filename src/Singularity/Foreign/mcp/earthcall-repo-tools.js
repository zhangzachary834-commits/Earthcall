"use strict";

const fs = require("fs");
const path = require("path");
const { execFileSync } = require("child_process");

const DEFAULT_MAX_READ_CHARS = 120000;
const DEFAULT_MAX_SEARCH_RESULTS = 100;
const DEFAULT_MAX_SEARCH_FILES = 5000;
const MAX_SINGLE_FILE_BYTES = 4 * 1024 * 1024;

const DEFAULT_SKIP_DIRS = new Set([
  ".git",
  "node_modules",
  "build",
  ".cache",
  ".idea",
  ".vscode",
  "DerivedData",
]);

const SENSITIVE_BASENAMES = new Set([
  ".env",
  "id_rsa",
  "id_ed25519",
  "credentials.json",
  "service-account.json",
]);

const SENSITIVE_SUFFIXES = [".pem", ".key", ".p12", ".pfx"];

function clampInt(value, fallback, min, max) {
  const n = Number(value);
  if (!Number.isFinite(n)) return fallback;
  return Math.max(min, Math.min(max, Math.trunc(n)));
}

function normalizeRoot(root) {
  const resolved = fs.realpathSync.native(path.resolve(root));
  const stat = fs.statSync(resolved);
  if (!stat.isDirectory()) throw new Error(`Earthcall MCP root is not a directory: ${resolved}`);
  return resolved;
}

function isInsideRoot(root, candidate) {
  const rel = path.relative(root, candidate);
  return rel === "" || (!rel.startsWith(`..${path.sep}`) && rel !== ".." && !path.isAbsolute(rel));
}

function safeResolve(root, requested = ".", { mustExist = true } = {}) {
  if (typeof requested !== "string" || requested.includes("\0")) {
    throw new Error("Path must be a normal string without NUL bytes.");
  }

  const joined = path.resolve(root, requested || ".");
  let resolved;

  if (mustExist) {
    resolved = fs.realpathSync.native(joined);
  } else {
    const parent = fs.realpathSync.native(path.dirname(joined));
    resolved = path.join(parent, path.basename(joined));
  }

  if (!isInsideRoot(root, resolved)) {
    throw new Error(`Path escapes the Earthcall root: ${requested}`);
  }
  return resolved;
}

function toRepoPath(root, absolutePath) {
  const rel = path.relative(root, absolutePath);
  return rel === "" ? "." : rel.split(path.sep).join("/");
}

function isSensitivePath(root, absolutePath) {
  if (process.env.EARTHCALL_MCP_ALLOW_SENSITIVE === "1") return false;

  const rel = toRepoPath(root, absolutePath);
  const parts = rel.split("/");
  if (parts.includes(".git") || parts.includes(".ssh") || parts.includes("secrets")) return true;

  const base = path.basename(absolutePath).toLowerCase();
  if (base === ".env.example" || base === ".env.sample") return false;
  if (SENSITIVE_BASENAMES.has(base)) return true;
  if (base.startsWith(".env.")) return true;
  return SENSITIVE_SUFFIXES.some((suffix) => base.endsWith(suffix));
}

function assertReadablePath(root, absolutePath) {
  if (isSensitivePath(root, absolutePath)) {
    throw new Error(
      `Refusing to expose a secret-like path (${toRepoPath(root, absolutePath)}). ` +
      "Set EARTHCALL_MCP_ALLOW_SENSITIVE=1 only if you intentionally want to disable this guard."
    );
  }
}

function looksBinary(buffer) {
  const sample = buffer.subarray(0, Math.min(buffer.length, 8192));
  for (const byte of sample) {
    if (byte === 0) return true;
  }
  return false;
}

function truncateText(text, maxChars = DEFAULT_MAX_READ_CHARS) {
  if (text.length <= maxChars) return { text, truncated: false, omittedChars: 0 };
  return {
    text: text.slice(0, maxChars),
    truncated: true,
    omittedChars: text.length - maxChars,
  };
}

function listDirectory(rootInput, requestedPath = ".", maxEntries = 200, includeHidden = false) {
  const root = normalizeRoot(rootInput);
  const dir = safeResolve(root, requestedPath);
  assertReadablePath(root, dir);
  const stat = fs.statSync(dir);
  if (!stat.isDirectory()) throw new Error(`Not a directory: ${requestedPath}`);

  const limit = clampInt(maxEntries, 200, 1, 1000);
  const names = fs.readdirSync(dir, { withFileTypes: true })
    .filter((entry) => includeHidden || !entry.name.startsWith("."))
    .sort((a, b) => {
      if (a.isDirectory() !== b.isDirectory()) return a.isDirectory() ? -1 : 1;
      return a.name.localeCompare(b.name);
    });

  const entries = [];
  for (const entry of names.slice(0, limit)) {
    const rawPath = path.join(dir, entry.name);
    let kind = entry.isDirectory() ? "directory" : entry.isFile() ? "file" : entry.isSymbolicLink() ? "symlink" : "other";
    let size = null;
    let target = null;
    let blocked = false;

    try {
      if (entry.isSymbolicLink()) {
        target = fs.realpathSync.native(rawPath);
        blocked = !isInsideRoot(root, target) || isSensitivePath(root, target);
      }
      const s = fs.lstatSync(rawPath);
      if (s.isFile()) size = s.size;
    } catch (_) {
      // Keep listing robust even if a file changes during enumeration.
    }

    entries.push({
      name: entry.name,
      path: toRepoPath(root, rawPath),
      kind,
      ...(size !== null ? { size_bytes: size } : {}),
      ...(target ? { symlink_target: blocked ? "[blocked]" : toRepoPath(root, target), blocked } : {}),
    });
  }

  return {
    root: root,
    path: toRepoPath(root, dir),
    entries,
    returned: entries.length,
    total_visible_entries: names.length,
    truncated: names.length > limit,
  };
}

function readFile(rootInput, requestedPath, startLine = 1, endLine = null, maxChars = DEFAULT_MAX_READ_CHARS) {
  const root = normalizeRoot(rootInput);
  const file = safeResolve(root, requestedPath);
  assertReadablePath(root, file);
  const stat = fs.statSync(file);
  if (!stat.isFile()) throw new Error(`Not a regular file: ${requestedPath}`);
  if (stat.size > MAX_SINGLE_FILE_BYTES && endLine == null) {
    throw new Error(
      `File is ${stat.size} bytes. Specify start_line/end_line for files larger than ${MAX_SINGLE_FILE_BYTES} bytes.`
    );
  }

  const buffer = fs.readFileSync(file);
  if (looksBinary(buffer)) throw new Error(`Refusing to return binary file content: ${requestedPath}`);

  const lines = buffer.toString("utf8").split(/\r?\n/);
  const start = clampInt(startLine, 1, 1, Math.max(1, lines.length));
  const requestedEnd = endLine == null ? Math.min(lines.length, start + 1999) : clampInt(endLine, start, start, lines.length);
  const selected = lines.slice(start - 1, requestedEnd);
  const numbered = selected.map((line, index) => `${start + index}: ${line}`).join("\n");
  const clipped = truncateText(numbered, clampInt(maxChars, DEFAULT_MAX_READ_CHARS, 1000, 250000));

  return {
    path: toRepoPath(root, file),
    start_line: start,
    end_line: requestedEnd,
    total_lines: lines.length,
    content: clipped.text,
    truncated: clipped.truncated || requestedEnd < lines.length,
    ...(clipped.omittedChars ? { omitted_chars: clipped.omittedChars } : {}),
    ...(requestedEnd < lines.length ? { next_start_line: requestedEnd + 1 } : {}),
  };
}

function globToRegExp(glob) {
  const normalized = (glob || "**/*").split(path.sep).join("/");
  let out = "^";
  for (let i = 0; i < normalized.length; i++) {
    const c = normalized[i];
    const next = normalized[i + 1];
    if (c === "*" && next === "*") {
      const after = normalized[i + 2];
      if (after === "/") {
        out += "(?:.*/)?";
        i += 2;
      } else {
        out += ".*";
        i += 1;
      }
    } else if (c === "*") {
      out += "[^/]*";
    } else if (c === "?") {
      out += "[^/]";
    } else {
      out += c.replace(/[\\^$+.()|{}\[\]]/g, "\\$&");
    }
  }
  out += "$";
  return new RegExp(out);
}

function searchText(rootInput, query, requestedPath = ".", includeGlob = "**/*", caseSensitive = false, maxResults = DEFAULT_MAX_SEARCH_RESULTS) {
  const root = normalizeRoot(rootInput);
  if (typeof query !== "string" || query.length === 0) throw new Error("query must be a non-empty string");
  if (query.length > 1000) throw new Error("query is too long");

  const target = safeResolve(root, requestedPath);
  assertReadablePath(root, target);
  const targetStat = fs.statSync(target);
  const matcher = globToRegExp(includeGlob || "**/*");
  const needle = caseSensitive ? query : query.toLowerCase();
  const resultLimit = clampInt(maxResults, DEFAULT_MAX_SEARCH_RESULTS, 1, 200);
  const results = [];
  let filesScanned = 0;
  let filesSkipped = 0;
  let hitFileLimit = false;

  function scanFile(file) {
    if (results.length >= resultLimit || filesScanned >= DEFAULT_MAX_SEARCH_FILES) return;
    if (!isInsideRoot(root, file) || isSensitivePath(root, file)) {
      filesSkipped++;
      return;
    }

    const rel = toRepoPath(root, file);
    if (!matcher.test(rel)) return;

    let stat;
    try { stat = fs.statSync(file); } catch (_) { return; }
    if (!stat.isFile() || stat.size > MAX_SINGLE_FILE_BYTES) {
      filesSkipped++;
      return;
    }

    filesScanned++;
    let buffer;
    try { buffer = fs.readFileSync(file); } catch (_) { return; }
    if (looksBinary(buffer)) {
      filesSkipped++;
      return;
    }

    const lines = buffer.toString("utf8").split(/\r?\n/);
    for (let i = 0; i < lines.length && results.length < resultLimit; i++) {
      const haystack = caseSensitive ? lines[i] : lines[i].toLowerCase();
      const column = haystack.indexOf(needle);
      if (column !== -1) {
        results.push({
          path: rel,
          line: i + 1,
          column: column + 1,
          text: lines[i].length > 600 ? `${lines[i].slice(0, 600)}…` : lines[i],
        });
      }
    }
  }

  function walk(dir) {
    if (results.length >= resultLimit) return;
    if (filesScanned >= DEFAULT_MAX_SEARCH_FILES) {
      hitFileLimit = true;
      return;
    }

    let entries;
    try { entries = fs.readdirSync(dir, { withFileTypes: true }); } catch (_) { return; }
    for (const entry of entries) {
      if (results.length >= resultLimit) break;
      if (entry.isDirectory() && DEFAULT_SKIP_DIRS.has(entry.name)) continue;
      if (entry.name === ".DS_Store") continue;
      const child = path.join(dir, entry.name);
      if (entry.isSymbolicLink()) continue;
      if (entry.isDirectory()) walk(child);
      else if (entry.isFile()) scanFile(child);
      if (filesScanned >= DEFAULT_MAX_SEARCH_FILES) {
        hitFileLimit = true;
        break;
      }
    }
  }

  if (targetStat.isFile()) scanFile(target);
  else walk(target);

  return {
    query,
    path: toRepoPath(root, target),
    include_glob: includeGlob || "**/*",
    case_sensitive: Boolean(caseSensitive),
    matches: results,
    returned: results.length,
    files_scanned: filesScanned,
    files_skipped: filesSkipped,
    truncated: results.length >= resultLimit || hitFileLimit,
    ...(hitFileLimit ? { note: `Stopped after ${DEFAULT_MAX_SEARCH_FILES} files.` } : {}),
  };
}

function runGit(rootInput, args, maxChars = DEFAULT_MAX_READ_CHARS) {
  const root = normalizeRoot(rootInput);
  const output = execFileSync("git", ["-C", root, ...args], {
    encoding: "utf8",
    timeout: 15000,
    maxBuffer: 8 * 1024 * 1024,
    env: { ...process.env, GIT_OPTIONAL_LOCKS: "0", GIT_PAGER: "cat" },
  });
  return truncateText(output, clampInt(maxChars, DEFAULT_MAX_READ_CHARS, 1000, 250000));
}

function gitStatus(rootInput, maxChars = DEFAULT_MAX_READ_CHARS) {
  const root = normalizeRoot(rootInput);
  const result = runGit(root, ["status", "--short", "--branch", "--untracked-files=normal"], maxChars);
  return {
    root,
    output: result.text || "(clean working tree)",
    truncated: result.truncated,
    ...(result.omittedChars ? { omitted_chars: result.omittedChars } : {}),
  };
}

function gitDiff(rootInput, requestedPath = null, staged = false, maxChars = DEFAULT_MAX_READ_CHARS) {
  const root = normalizeRoot(rootInput);
  const args = ["diff", "--no-ext-diff", "--no-color"];
  if (staged) args.push("--cached");

  const pathspecs = [];
  if (requestedPath) {
    const absolute = safeResolve(root, requestedPath);
    assertReadablePath(root, absolute);
    pathspecs.push(toRepoPath(root, absolute));
  } else {
    pathspecs.push(".");
  }

  // Keep the broad diff tool from accidentally returning common local secret files.
  if (process.env.EARTHCALL_MCP_ALLOW_SENSITIVE !== "1") {
    pathspecs.push(
      ":(exclude).env",
      ":(exclude).env.*",
      ":(exclude)**/*.pem",
      ":(exclude)**/*.key",
      ":(exclude)**/*.p12",
      ":(exclude)**/*.pfx",
      ":(exclude)**/secrets/**",
      ":(exclude)**/.ssh/**"
    );
  }
  args.push("--", ...pathspecs);

  const result = runGit(root, args, maxChars);
  return {
    path: requestedPath || ".",
    staged: Boolean(staged),
    output: result.text || "(no diff)",
    truncated: result.truncated,
    ...(result.omittedChars ? { omitted_chars: result.omittedChars } : {}),
  };
}

module.exports = {
  normalizeRoot,
  safeResolve,
  listDirectory,
  readFile,
  searchText,
  gitStatus,
  gitDiff,
  _internal: { isInsideRoot, isSensitivePath, globToRegExp, looksBinary, truncateText },
};
