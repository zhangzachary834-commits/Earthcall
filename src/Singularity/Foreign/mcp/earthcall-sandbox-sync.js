"use strict";

// Local developer tooling. The remote broker never exposes this writer and
// Docker only mounts workspace. Baselines, checkpoints and recovery objects
// stay outside that mount, beside the session metadata.
const fs = require("node:fs");
const os = require("node:os");
const path = require("node:path");
const crypto = require("node:crypto");
const { spawnSync } = require("node:child_process");
const sandbox = require("./earthcall-sandbox.js");

const hash = (bytes) => crypto.createHash("sha256").update(bytes).digest("hex");
const equal = (a, b) => a === b || (a && b && a.hash === b.hash && a.mode === b.mode);

function git(root, args, input) {
  const result = spawnSync("git", ["-c", "core.fsmonitor=false", ...args], {
    cwd: root, input, maxBuffer: 256 * 1024 * 1024,
  });
  if (result.error) throw result.error;
  if (result.status !== 0) throw new Error(String(result.stderr).trim() || "Git command failed.");
  return result.stdout;
}

function privateDirectory(dir) {
  fs.mkdirSync(dir, { recursive: true, mode: 0o700 });
}

function atomicWrite(file, bytes, mode = 0o600) {
  privateDirectory(path.dirname(file));
  const temporary = file + "." + crypto.randomBytes(8).toString("hex") + ".tmp";
  try {
    fs.writeFileSync(temporary, bytes, { mode, flag: "wx" });
    fs.chmodSync(temporary, mode);
    fs.renameSync(temporary, file);
  } finally {
    fs.rmSync(temporary, { force: true });
  }
}

function checkedPath(root, rel) {
  const parts = rel.split("/");
  if (path.isAbsolute(rel) || rel.includes("\\") || rel.includes("\0")
    || parts.some((p) => !p || p === "." || p === ".." || p.toLowerCase() === ".git")) {
    throw new Error("Invalid relative file path.");
  }
  let file = root;
  for (const [index, part] of parts.entries()) {
    file = path.join(file, part);
    const stat = fs.lstatSync(file, { throwIfNoEntry: false });
    if (!stat) continue;
    if (stat.isSymbolicLink()) throw new Error("Symlinks require manual resolution.");
    if (index < parts.length - 1 && !stat.isDirectory()) throw new Error("Parent is not a directory.");
  }
  return file;
}

function snapshot(root, rel) {
  const file = checkedPath(root, rel);
  const stat = fs.lstatSync(file, { throwIfNoEntry: false });
  if (!stat) return null;
  if (!stat.isFile()) throw new Error("Only regular files can be transplanted.");
  const bytes = fs.readFileSync(file);
  const after = fs.statSync(file);
  if (stat.size !== after.size || stat.mtimeMs !== after.mtimeMs || stat.ctimeMs !== after.ctimeMs) {
    throw new Error("File changed while being read. Refresh and retry.");
  }
  return { file, bytes, hash: hash(bytes), mode: stat.mode & 0o100 ? 0o755 : 0o644, mtimeMs: stat.mtimeMs };
}

function reference(value) {
  return value ? { hash: value.hash, mode: value.mode } : null;
}

function storeObject(store, value) {
  if (!value) return;
  const file = path.join(store, "objects", value.hash);
  if (fs.existsSync(file)) return;
  if (value.file) {
    privateDirectory(path.dirname(file));
    const temporary = file + "." + crypto.randomBytes(8).toString("hex") + ".tmp";
    try {
      fs.copyFileSync(value.file, temporary, fs.constants.COPYFILE_FICLONE);
      if (hash(fs.readFileSync(temporary)) !== value.hash) throw new Error("File changed during baseline capture.");
      fs.chmodSync(temporary, 0o600);
      fs.renameSync(temporary, file);
    } finally {
      fs.rmSync(temporary, { force: true });
    }
  } else {
    atomicWrite(file, value.bytes);
  }
}

function loadObject(store, ref) {
  if (!ref) return null;
  if (!/^[a-f0-9]{64}$/.test(ref.hash) || ![0o644, 0o755].includes(ref.mode)) {
    throw new Error("Invalid merge checkpoint.");
  }
  const bytes = fs.readFileSync(path.join(store, "objects", ref.hash));
  if (hash(bytes) !== ref.hash) throw new Error("Merge checkpoint content is damaged.");
  return { ...ref, bytes };
}

function permitted(rel) {
  const parts = rel.split("/");
  return parts.every((p, index) => p.toLowerCase() !== ".ds_store"
    && !sandbox._internal.shouldSkipCopy(p, index < parts.length - 1)
    && !sandbox._internal.shouldSkipCopy(p, false));
}

function candidates(checkout, session, manifest, checkpoints, scopes, toCheckout) {
  const entries = git(checkout, ["ls-files", "--stage", "-z"]).toString().split("\0");
  const submodules = entries.filter((s) => s.startsWith("160000 ")).map((s) => s.slice(s.indexOf("\t") + 1));
  const tracked = entries.filter((s) => s && !s.startsWith("160000 ")).map((s) => s.slice(s.indexOf("\t") + 1));
  const untracked = git(checkout, ["ls-files", "--others", "--exclude-standard", "-z"]).toString().split("\0");
  const incomingPaths = [];
  function walk(prefix = "") {
    for (const entry of fs.readdirSync(path.join(session.workspace, prefix), { withFileTypes: true })) {
      if (!permitted(prefix + entry.name)) continue;
      if (entry.isDirectory()) {
        if (!sandbox._internal.shouldSkipCopy(entry.name, true)) walk(prefix + entry.name + "/");
      } else {
        incomingPaths.push(prefix + entry.name);
      }
    }
  }
  if (toCheckout) walk();
  const paths = [...new Set([...tracked, ...untracked, ...incomingPaths, ...Object.keys(manifest), ...Object.keys(checkpoints)])]
    .filter((rel) => rel && permitted(rel) && !submodules.some((p) => rel === p || rel.startsWith(p + "/"))
      && (scopes.length === 0 || scopes.some((p) => rel === p || rel.startsWith(p + "/"))));
  // check-ignore keeps tracked paths even if a broad ignore pattern matches.
  const result = spawnSync("git", ["-c", "core.fsmonitor=false", "check-ignore", "--stdin", "-z"], {
    cwd: checkout, input: paths.length ? paths.join("\0") + "\0" : "", maxBuffer: 32 * 1024 * 1024,
  });
  if (result.error || ![0, 1].includes(result.status)) throw result.error || new Error(String(result.stderr));
  const ignored = new Set(result.stdout.toString().split("\0"));
  return paths.filter((p) => !ignored.has(p)).sort();
}

function baseline(session, store, checkpoints, manifest, rel, upstream, local) {
  if (Object.hasOwn(checkpoints, rel)) return loadObject(store, checkpoints[rel]);
  const original = manifest[rel];
  if (!original) return null;
  if (original.kind !== "file") throw new Error("Original file was a symlink.");
  const retained = snapshot(path.join(path.dirname(session.workspace), "baseline"), rel);
  if (retained) return retained;

  // Compatibility for sessions created before exact baseline retention. Reuse
  // an unchanged side only. A changed size/time on both sides supplies no base.
  const unchanged = (value) => value && value.bytes.length === original.size
    && Math.abs(value.mtimeMs - original.mtimeMs) < 1;
  if (unchanged(local)) return local;
  if (unchanged(upstream)) return upstream;
  if (equal(local, upstream)) return local;
  // An old append-only thread can retain a shared prefix even after both sides
  // advanced. This agreed prefix is a usable common ancestor for its tails.
  if (local && upstream && local.bytes.length >= original.size && upstream.bytes.length >= original.size
    && local.bytes.subarray(0, original.size).equals(upstream.bytes.subarray(0, original.size))) {
    const bytes = local.bytes.subarray(0, original.size);
    return { bytes, hash: hash(bytes), mode: original.mode || local.mode };
  }
  throw new Error("Original contents unavailable for this older session. Reconcile this file on both sides first.");
}

function mergeFile(base, local, incoming, temp, toCheckout) {
  if (equal(local, incoming) || equal(incoming, base)) return local;
  if (equal(local, base)) return incoming;
  if (!base || !local || !incoming) {
    throw new Error(!base ? "Both sides added different files." : "Deletion conflicts with edits on the other side.");
  }
  const mode = local.mode === incoming.mode || incoming.mode === base.mode ? local.mode : incoming.mode;
  if (local.hash === incoming.hash || incoming.hash === base.hash) return { ...local, mode };
  if (local.hash === base.hash) return { ...incoming, mode };
  if ([base, local, incoming].some((v) => v.bytes.includes(0))) throw new Error("Both sides changed this binary file.");

  const names = ["sandbox", "base", "checkout"];
  [local, base, incoming].forEach((value, i) => fs.writeFileSync(path.join(temp, names[i]), value.bytes));
  const result = spawnSync("git", ["merge-file", "-p", "--diff3",
    "-L", toCheckout ? "Checkout" : "Sandbox", "-L", "Shared baseline",
    "-L", toCheckout ? "Sandbox" : "Checkout", ...names], {
    cwd: temp, maxBuffer: 256 * 1024 * 1024,
  });
  if (result.error) throw result.error;
  if (result.status !== 0) {
    const error = new Error("Overlapping edits.");
    if (result.status > 0 && result.status < 128) error.merge = result.stdout.toString();
    else error.message = String(result.stderr).trim() || "Git could not merge the file.";
    throw error;
  }
  return { bytes: result.stdout, hash: hash(result.stdout), mode };
}

function stageFile(root, rel, value) {
  if (!value) return;
  const file = checkedPath(root, rel);
  privateDirectory(path.dirname(file));
  fs.writeFileSync(file, value.bytes, { mode: value.mode });
  fs.chmodSync(file, value.mode);
}

function patchFor(temp, changes) {
  for (const change of changes) {
    // Use a separate pair of directories for the complete patch, so Git handles
    // paths, spaces, binary data, modes, additions and deletions itself.
    stageFile(path.join(temp, "a"), change.rel, change.before);
    stageFile(path.join(temp, "b"), change.rel, change.after);
  }
  privateDirectory(path.join(temp, "a"));
  privateDirectory(path.join(temp, "b"));
  const result = spawnSync("git", ["diff", "--no-index", "--binary", "--full-index", "--no-prefix",
    "--no-color", "--no-ext-diff", "--no-textconv", "--no-renames", "--", "a", "b"], {
    cwd: temp, maxBuffer: 256 * 1024 * 1024,
  });
  if (result.error || ![0, 1].includes(result.status)) throw result.error || new Error(String(result.stderr));
  return result.stdout.toString();
}

function readJSON(file, fallback) {
  return fs.existsSync(file) ? JSON.parse(fs.readFileSync(file, "utf8")) : fallback;
}

function acquireLock(store) {
  const lock = path.join(store, "lock");
  const reservation = path.join(store, "lock-" + crypto.randomBytes(8).toString("hex"));
  fs.writeFileSync(reservation, JSON.stringify({ pid: process.pid }), { mode: 0o600, flag: "wx" });
  try {
    try {
      fs.linkSync(reservation, lock);
    } catch (error) {
      if (error.code !== "EEXIST") throw error;
      const owner = readJSON(lock, null);
      if (!Number.isSafeInteger(owner?.pid) || owner.pid <= 0) throw new Error("Invalid transplant lock.");
      try {
        process.kill(owner.pid, 0);
        throw new Error("Another transplant is running. Retry after it finishes.");
      } catch (running) {
        if (running.code !== "ESRCH") throw running;
      }
      // Linking a fully written record makes ownership atomic. Re-read before
      // removing a dead owner's lock so a second recovery can't steal a new one.
      if (fs.statSync(lock).ino !== fs.statSync(reservation).ino
        && readJSON(lock, null)?.pid === owner.pid) fs.unlinkSync(lock);
      try { fs.linkSync(reservation, lock); }
      catch (retry) {
        if (retry.code === "EEXIST") throw new Error("Another transplant is recovering. Retry after it finishes.");
        throw retry;
      }
    }
  } finally {
    fs.unlinkSync(reservation);
  }
}

function commitTransaction(targetRoot, store, transaction, checkpoints) {
  // Recheck the whole batch before the first write, including a recovery retry.
  // A crash retains immutable before/after objects and the journal. Retrying
  // advances files already written and resumes the remaining files.
  for (const change of transaction.changes) {
    const current = snapshot(targetRoot, change.rel);
    if (!equal(current, change.before) && !equal(current, change.after)) {
      throw new Error(`${change.rel} changed during the transplant. Restore or reconcile it before retrying.`);
    }
  }
  for (const change of transaction.changes) {
    const current = snapshot(targetRoot, change.rel);
    if (equal(current, change.after)) continue;
    if (!equal(current, change.before)) throw new Error(`${change.rel} changed during the transplant.`);
    const target = checkedPath(targetRoot, change.rel);
    if (change.after) {
      const value = loadObject(store, change.after);
      atomicWrite(target, value.bytes, value.mode);
    } else {
      fs.unlinkSync(target);
    }
  }
  const next = { ...checkpoints, ...transaction.checkpoints };
  atomicWrite(path.join(store, "checkpoints.json"), JSON.stringify(next));
  fs.unlinkSync(path.join(store, "transaction.json"));
}

function transplant(checkout, session, scopes, {
  apply = false, expectedDiff, toCheckout = false, resolutions = {},
} = {}) {
  const store = path.join(path.dirname(session.workspace), "sync");
  const journal = path.join(store, "transaction.json");
  const statePath = path.join(store, "checkpoints.json");
  const targetRoot = toCheckout ? checkout : session.workspace;
  const sourceRoot = toCheckout ? session.workspace : checkout;
  let locked = false;
  const temp = fs.mkdtempSync(path.join(os.tmpdir(), "earthcall-transplant-"));
  try {
    if (apply) {
      privateDirectory(store);
      acquireLock(store);
      locked = true;
    }
    const checkpoints = readJSON(statePath, {});
    const pending = readJSON(journal, null);
    if (pending) {
      if (pending.toCheckout !== toCheckout) throw new Error("Finish the interrupted transplant in its original direction first.");
      const digest = hash(JSON.stringify(pending));
      if (apply) {
        if (expectedDiff && expectedDiff !== digest) throw new Error("Refresh to review the interrupted transplant before resuming.");
        commitTransaction(targetRoot, store, pending, checkpoints);
      }
      return { diff: pending.diff, digest, conflicts: [], recovery: true };
    }
    const manifest = sandbox.readSessionManifest(session.id);
    const legacySession = !fs.existsSync(path.join(path.dirname(session.workspace), "baseline"));
    const changes = [];
    const updates = {};
    const conflicts = [];
    const observations = [];
    for (const rel of candidates(checkout, session, manifest, checkpoints, scopes, toCheckout)) {
      let incoming;
      let local;
      let canResolve = false;
      try {
        // Identical symlinks are inert. Changed links receive an explicit conflict.
        if (manifest[rel]?.kind === "symlink") {
          const original = manifest[rel].target;
          const unchanged = [checkout, session.workspace].every((root) => {
            const file = path.join(root, rel);
            return fs.lstatSync(file, { throwIfNoEntry: false })?.isSymbolicLink() && fs.readlinkSync(file) === original;
          });
          if (unchanged) continue;
        }
        incoming = snapshot(sourceRoot, rel);
        local = snapshot(targetRoot, rel);
        canResolve = true;
        const base = baseline(session, store, checkpoints, manifest, rel, incoming, local);
        observations.push([rel, reference(incoming), reference(local), reference(base)]);
        // Bootstrap exact checkpoints for legacy sessions as well. The checkpoint
        // remains the source version, so independent destination edits stay local.
        if (legacySession && !Object.hasOwn(checkpoints, rel)) {
          updates[rel] = reference(base);
          if (apply) storeObject(store, base);
        }
        if (equal(base, incoming)) continue;
        const merged = mergeFile(base, local, incoming, temp, toCheckout);
        updates[rel] = reference(incoming);
        if (!equal(local, merged)) changes.push({ rel, before: local, after: merged });
        if (apply) storeObject(store, incoming);
      } catch (error) {
        if (canResolve && ["ours", "theirs"].includes(resolutions[rel])) {
          const chosen = resolutions[rel] === "ours" ? local : incoming;
          updates[rel] = reference(incoming);
          if (!equal(local, chosen)) changes.push({ rel, before: local, after: chosen });
          observations.push([rel, reference(incoming), reference(local), "resolved-" + resolutions[rel]]);
          if (apply) storeObject(store, incoming);
        } else {
          conflicts.push({ path: rel, reason: error.message, merge: error.merge || "", canResolve });
        }
      }
    }
    const aliases = new Set();
    for (const change of changes) {
      const folded = change.rel.normalize("NFC").toLowerCase();
      if (aliases.has(folded)) {
        conflicts.push({ path: change.rel, reason: "Case-insensitive filename collision.", merge: "", canResolve: false });
      }
      aliases.add(folded);
    }
    const diff = patchFor(temp, changes);
    const digest = hash(JSON.stringify({ observations, checkpoints, diff, resolutions }));
    const conflictDiff = conflicts.map((c) => `\n# CONFLICT ${JSON.stringify(c.path)}\n${c.reason}\n${c.merge}`).join("");
    if (conflicts.length) {
      if (apply) throw new Error("Resolve these paths before transplanting, or select unaffected paths with --:\n"
        + conflicts.map((c) => `  ${JSON.stringify(c.path)} — ${c.reason}`).join("\n"));
      return { diff: diff + conflictDiff, digest: null, conflicts };
    }
    if (apply) {
      if (expectedDiff && expectedDiff !== digest) throw new Error("The changes differ from the reviewed preview. Refresh before transplanting.");
      // Re-read both sides after all merges, before publishing a recovery journal.
      for (const [rel, incoming, local] of observations) {
        if (!equal(snapshot(sourceRoot, rel), incoming) || !equal(snapshot(targetRoot, rel), local)) {
          throw new Error(`${rel} changed during the preview. Refresh and retry.`);
        }
      }
      const records = changes.map((c) => {
        storeObject(store, c.before);
        storeObject(store, c.after);
        return { rel: c.rel, before: reference(c.before), after: reference(c.after) };
      });
      const transaction = { changes: records, checkpoints: updates, diff, toCheckout };
      atomicWrite(journal, JSON.stringify(transaction));
      commitTransaction(targetRoot, store, transaction, checkpoints);
    }
    return { diff, digest: Object.keys(updates).length ? digest : null, conflicts: [], checkpointOnly: !diff };
  } finally {
    if (locked) fs.unlinkSync(path.join(store, "lock"));
    fs.rmSync(temp, { recursive: true, force: true });
  }
}

module.exports = { transplant };
