"use strict";

const fs = require("node:fs");
const os = require("node:os");
const path = require("node:path");
const { createHash } = require("node:crypto");
const { spawnSync } = require("node:child_process");
const sandbox = require("../src/Singularity/Foreign/mcp/earthcall-sandbox.js");

const usage = `Usage:
  node scripts/earthcall-sandbox-import.js list
  node scripts/earthcall-sandbox-import.js preview <session-id-or-label> [-- <path> ...]
  node scripts/earthcall-sandbox-import.js apply   <session-id-or-label> [-- <path> ...]

Run from the checkout the sandbox was created from. Paths are relative to its root.`;

function git(cwd, args, { input, stdout = "pipe", allowed = [0] } = {}) {
  const result = spawnSync("git", [
    "-c", "core.fsmonitor=false", "-c", "core.protectHFS=true", "-c", "core.protectNTFS=true", ...args,
  ], { cwd, input, encoding: "utf8", maxBuffer: 32 * 1024 * 1024, stdio: ["pipe", stdout, "pipe"] });
  if (result.error) throw result.error;
  if (!allowed.includes(result.status)) throw new Error(result.stderr.trim() || `git ${args[0]} failed`);
  return result.stdout;
}

function dirEntries(dir) {
  return fs.readdirSync(dir, { withFileTypes: true });
}

function sessions() {
  const dir = path.join(sandbox.dataRoot(), "sandboxes");
  if (!fs.existsSync(dir)) return [];
  return dirEntries(dir)
    .filter((entry) => entry.isDirectory() && /^[a-f0-9]{24}$/.test(entry.name))
    .map((entry) => {
      const root = path.join(fs.realpathSync.native(dir), entry.name);
      // The broker publishes metadata after copying the workspace; skip incomplete sessions.
      if (!fs.existsSync(path.join(root, "manifest.json"))) return null;
      const session = sandbox.loadSession(entry.name);
      if (session.id !== entry.name || session.workspace !== path.join(root, "workspace")) {
        throw new Error(`Invalid sandbox session at ${root}`);
      }
      return session;
    })
    .filter(Boolean)
    .sort((a, b) => b.created_at.localeCompare(a.created_at));
}

function scanWorkspace(root, manifest, paths, included, visit, prefix = "") {
  for (const entry of dirEntries(path.join(root, prefix))) {
    if ([".git", ".ds_store"].includes(entry.name.toLowerCase())) continue;
    if (sandbox._internal.shouldSkipCopy(entry.name, entry.isDirectory())) continue;
    const rel = prefix + entry.name;
    if (entry.isDirectory()) {
      if (visit(rel)) scanWorkspace(root, manifest, paths, included, visit, `${rel}/`);
    } else if (included(rel)) {
      const file = path.join(root, rel);
      const before = Object.hasOwn(manifest, rel) ? manifest[rel] : null;
      const unchanged = entry.isSymbolicLink()
        ? before?.kind === "symlink" && before.target === fs.readlinkSync(file)
        : before && sameBaseline(before, { stat: fs.lstatSync(file) }, 1);
      if (unchanged) paths.delete(rel);
      else paths.add(rel);
    }
  }
}

function pathParts(rel) {
  const parts = rel.split("/");
  if (rel.includes("\\") || parts.some((part) => !part || part === "." || part === ".." || part.toLowerCase() === ".git")) {
    throw new Error(`Invalid checkout-relative path ${JSON.stringify(rel)}`);
  }
  return parts;
}

function inspectPath(root, rel) {
  const parts = pathParts(rel);
  let file = root;
  for (const [index, part] of parts.entries()) {
    file = path.join(file, part);
    const stat = fs.lstatSync(file, { throwIfNoEntry: false });
    if (!stat) return null;
    if (index === parts.length - 1) return { file, stat };
    if (!stat.isDirectory()) throw new Error("parent is a symlink or another non-directory entry");
  }
}

// Legacy manifests record the host stat before copyTreeCow round-trips mtime
// through utimesSync. The workspace comparison admits sub-millisecond rounding.
function sameBaseline(baseline, entry, toleranceMs) {
  return baseline && entry && baseline.kind === "file" && entry.stat.isFile()
    && baseline.size === entry.stat.size
    && Math.abs(baseline.mtimeMs - entry.stat.mtimeMs) <= toleranceMs;
}

function sameFile(previous, current) {
  if (!previous || !current) return previous === current;
  const { stat: before } = previous;
  const { stat: after } = current;
  return ["dev", "ino", "mode", "size", "mtimeMs", "ctimeMs"].every((key) => before[key] === after[key]);
}

function stage(tree, rel, entry, mode) {
  if (!entry) return;
  const output = path.join(tree, rel);
  fs.mkdirSync(path.dirname(output), { recursive: true });
  fs.copyFileSync(entry.file, output, fs.constants.COPYFILE_FICLONE);
  fs.chmodSync(output, mode);
}

function stageChanges(checkout, session, manifest, paths, temp) {
  const changes = [];
  const conflicts = [];
  const aliases = new Set();
  for (const rel of paths) {
    try {
      const before = Object.hasOwn(manifest, rel) ? manifest[rel] : null;
      const incoming = inspectPath(session.workspace, rel);
      if (sameBaseline(before, incoming, 1)) continue;

      const local = inspectPath(checkout, rel);
      if ((before && before.kind !== "file") || [local, incoming].some((entry) => entry && !entry.stat.isFile())) {
        throw new Error("import supports regular files only");
      }
      if (!local && !incoming) continue;

      if (before ? !sameBaseline(before, local, 0) : local) {
        let keepsCheckoutBytes = false;
        if (local && incoming && local.stat.size <= incoming.stat.size) {
          const checkoutBytes = fs.readFileSync(local.file);
          const sandboxBytes = fs.readFileSync(incoming.file);
          if (checkoutBytes.equals(sandboxBytes)) continue;

          // Repeated appends can extend an existing file after its first import.
          // Match the entire checkout, including any bytes added since creation.
          keepsCheckoutBytes = before && checkoutBytes.length >= before.size
            && sandboxBytes.length > checkoutBytes.length
            && sandboxBytes.subarray(0, checkoutBytes.length).equals(checkoutBytes);
        }
        if (!keepsCheckoutBytes) throw new Error("checkout differs from the sandbox baseline");
      }

      const folded = rel.normalize("NFC").toLowerCase();
      if (aliases.has(folded)) throw new Error("path collides with another import on a case-insensitive filesystem");
      aliases.add(folded);
      const mode = (local || incoming).stat.mode & 0o100 ? 0o755 : 0o644;
      stage(path.join(temp, "a"), rel, local, mode);
      stage(path.join(temp, "b"), rel, incoming, mode);
      changes.push({ rel, local, incoming });
    } catch (error) {
      conflicts.push(`${JSON.stringify(rel)} — ${error.message}`);
    }
  }
  return { changes, conflicts };
}

function transfer(checkout, session, command, scopes, { json, expectedDiff }) {
  const included = (rel) => scopes.length === 0 || scopes.some((scope) => rel === scope || rel.startsWith(`${scope}/`));
  const visit = (rel) => included(rel) || scopes.some((scope) => scope.startsWith(`${rel}/`));
  const manifest = sandbox.readSessionManifest(session.id);
  const paths = new Set(Object.keys(manifest).filter((rel) =>
    included(rel) && !rel.split("/").some((part) => [".git", ".ds_store"].includes(part.toLowerCase()))));
  scanWorkspace(session.workspace, manifest, paths, included, visit);

  // Honor the real checkout's ignore rules, including local ignores, and
  // keep changes inside submodules with their own repositories.
  const submodules = git(checkout, ["ls-files", "--stage", "-z"]).split("\0")
    .filter((entry) => entry.startsWith("160000 ")).map((entry) => entry.slice(entry.indexOf("\t") + 1));
  const candidates = [...paths].sort().filter((rel) =>
    !submodules.some((submodule) => rel === submodule || rel.startsWith(`${submodule}/`)));
  const ignored = new Set(git(checkout, ["check-ignore", "--stdin", "-z"], {
    input: candidates.length ? `${candidates.join("\0")}\0` : "", allowed: [0, 1],
  }).split("\0"));

  const temp = fs.mkdtempSync(path.join(os.tmpdir(), "earthcall-sandbox-import-"));
  try {
    fs.mkdirSync(path.join(temp, "a"));
    fs.mkdirSync(path.join(temp, "b"));
    const { changes, conflicts } = stageChanges(checkout, session, manifest,
      candidates.filter((rel) => !ignored.has(rel)), temp);
    if (conflicts.length) {
      throw new Error(`Resolve these paths before importing, or select unaffected paths with --:\n  ${conflicts.join("\n  ")}`);
    }

    const patch = path.join(temp, "sandbox.patch");
    const diffArgs = ["diff", "--no-index", "--binary", "--full-index", "--no-prefix", "--no-color",
      "--no-ext-diff", "--no-textconv", "--no-renames", "--", "a", "b"];
    const fd = fs.openSync(patch, "wx", 0o600);
    try {
      git(temp, diffArgs, { stdout: fd, allowed: [0, 1] });
    } finally {
      fs.closeSync(fd);
    }
    const reviewed = json || expectedDiff ? fs.readFileSync(patch) : null;
    const digest = reviewed?.length ? createHash("sha256").update(reviewed).digest("hex") : null;
    if (expectedDiff && expectedDiff !== digest) {
      throw new Error("The changes differ from the reviewed preview. Refresh the preview and try again.");
    }
    if (fs.statSync(patch).size === 0) {
      if (json) console.log(JSON.stringify({ diff: "", digest: null }));
      else console.log("No sandbox changes to import for these paths.");
      return;
    }

    const apply = ["-c", "apply.ignoreWhitespace=false", "apply", "--whitespace=nowarn"];
    git(checkout, [...apply, "--check", patch]);
    for (const { rel, local, incoming } of changes) {
      if (!sameFile(local, inspectPath(checkout, rel)) || !sameFile(incoming, inspectPath(session.workspace, rel))) {
        throw new Error(`${JSON.stringify(rel)} changed during import. Run the command again.`);
      }
    }
    if (command === "preview") {
      if (json) console.log(JSON.stringify({ diff: reviewed.toString("utf8"), digest }));
      else git(temp, diffArgs, { stdout: "inherit", allowed: [0, 1] });
      return;
    }
    git(checkout, [...apply, patch]);
    console.log("Imported into the working tree.");
    git(checkout, ["apply", "--stat", patch], { stdout: "inherit" });
  } finally {
    fs.rmSync(temp, { recursive: true, force: true });
  }
}

function main(args) {
  // Machine interface used by the Mac companion. A reviewed apply only accepts
  // the exact patch returned by its preview, even if the sandbox changes later.
  const json = args[0] === "--json";
  if (json) args.shift();
  const [command, name, ...options] = args;
  let expectedDiff;
  if (command === "apply" && options[0] === "--expected-diff") {
    [, expectedDiff] = options.splice(0, 2);
    if (!/^[a-f0-9]{64}$/.test(expectedDiff || "")) throw new Error("Invalid reviewed diff digest.");
  }
  const [separator, ...paths] = options;
  if (!command || command === "--help" || command === "-h") return console.log(usage);
  if (command === "list" && !name) {
    const available = sessions();
    if (json) {
      console.log(JSON.stringify(available.map(({ id, label, created_at, source_root }) =>
        ({ id, label, created_at, source_root }))));
      return;
    }
    if (!available.length) console.log("No Gemini sandbox sessions found.");
    for (const s of available) {
      console.log(`${s.id}  ${s.created_at}  ${JSON.stringify(s.label)}  ${s.source_root}`);
    }
    return;
  }
  if (!["preview", "apply"].includes(command) || !name || (separator !== undefined && (separator !== "--" || !paths.length))) {
    throw new Error(usage);
  }

  const available = sessions();
  const exact = available.find((s) => s.id === name);
  const matches = exact ? [exact] : available.filter((s) => s.label === name);
  if (!matches.length) throw new Error(`Sandbox ${JSON.stringify(name)} was not found. Run list to see sessions.`);
  if (matches.length > 1) throw new Error(`Several sessions share that label. Choose an ID: ${matches.map((s) => s.id).join(", ")}`);
  const [session] = matches;
  const checkout = fs.realpathSync.native(git(process.cwd(), ["rev-parse", "--show-toplevel"]).trimEnd());
  if (checkout !== session.source_root) {
    throw new Error(`This sandbox belongs to ${session.source_root}. Run the command there.`);
  }
  const scopes = paths.map((p) => p.replace(/^\.\//, "").replace(/\/$/, ""));
  for (const scope of scopes) pathParts(scope);
  if (!json) console.log(`Sandbox ${JSON.stringify(session.label)}  ${session.id}`);
  transfer(checkout, session, command, scopes, { json, expectedDiff });
}

try {
  main(process.argv.slice(2));
} catch (error) {
  console.error(error.message);
  process.exitCode = 1;
}
