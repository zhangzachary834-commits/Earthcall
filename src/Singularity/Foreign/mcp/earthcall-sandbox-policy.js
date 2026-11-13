"use strict";

const fs = require("fs");
const path = require("path");
const crypto = require("crypto");

const DEFAULT_POLICY = Object.freeze({
  version: 1,
  fileEdits: {
    mode: "auto_sandbox",
    reviewPathPatterns: [
      "package.json",
      "CMakeLists.txt",
      ".github/**",
      "scripts/**",
      "**/*.sh",
      "**/*.command",
      "config/**",
    ],
  },
  commands: {
    mode: "auto_except_review",
    reviewPrograms: [
      "rm", "sudo", "curl", "wget", "ssh", "scp", "sftp", "rsync",
      "chmod", "chown", "kill", "pkill", "launchctl", "security", "open",
      "osascript", "docker", "podman", "nerdctl", "nc", "ncat", "socat",
      "bash", "sh", "zsh", "fish",
    ],
    reviewTokens: [
      "--force", "--hard", "--delete", "--recursive", "--privileged",
      "--network", "--mount", "/etc", "/usr", "/Applications", "~/.ssh",
      "~/Library/Keychains", "git push", "git reset", "git clean",
    ],
  },
  limits: {
    timeoutSeconds: 180,
    maxTimeoutSeconds: 900,
    cpus: 4,
    memory: "4g",
    pids: 256,
    maxOutputChars: 250000,
  },
  image: "earthcall-sandbox:latest",
});

const VALID_COMMAND_MODES = new Set(["manual", "auto", "auto_except_review"]);
const VALID_FILE_MODES = new Set(["manual", "auto_sandbox", "auto_safe_paths"]);

function deepClone(value) {
  return JSON.parse(JSON.stringify(value));
}

function mergeObject(base, incoming) {
  if (!incoming || typeof incoming !== "object" || Array.isArray(incoming)) return deepClone(base);
  const out = deepClone(base);
  for (const [key, value] of Object.entries(incoming)) {
    if (value && typeof value === "object" && !Array.isArray(value) &&
        out[key] && typeof out[key] === "object" && !Array.isArray(out[key])) {
      out[key] = mergeObject(out[key], value);
    } else {
      out[key] = deepClone(value);
    }
  }
  return out;
}

function loadPolicy(filePath) {
  let policy = deepClone(DEFAULT_POLICY);
  if (filePath && fs.existsSync(filePath)) {
    const parsed = JSON.parse(fs.readFileSync(filePath, "utf8"));
    policy = mergeObject(policy, parsed);
  }
  validatePolicy(policy);
  return policy;
}

function validatePolicy(policy) {
  if (!policy || typeof policy !== "object") throw new Error("Sandbox policy must be an object.");
  if (!VALID_COMMAND_MODES.has(policy.commands?.mode)) {
    throw new Error(`Invalid commands.mode: ${policy.commands?.mode}`);
  }
  if (!VALID_FILE_MODES.has(policy.fileEdits?.mode)) {
    throw new Error(`Invalid fileEdits.mode: ${policy.fileEdits?.mode}`);
  }
  for (const key of ["reviewPrograms", "reviewTokens"]) {
    if (!Array.isArray(policy.commands?.[key])) throw new Error(`commands.${key} must be an array.`);
  }
  if (!Array.isArray(policy.fileEdits?.reviewPathPatterns)) {
    throw new Error("fileEdits.reviewPathPatterns must be an array.");
  }
  return true;
}

function normalizeProgram(program) {
  return path.basename(String(program || "")).toLowerCase();
}

function commandText(argv) {
  return argv.map((x) => String(x)).join(" ");
}

function evaluateCommand(policy, argv) {
  if (!Array.isArray(argv) || argv.length === 0 || argv.some((x) => typeof x !== "string")) {
    throw new Error("argv must be a non-empty array of strings.");
  }

  const mode = policy.commands.mode;
  if (mode === "manual") {
    return { decision: "review", reasons: ["command policy is manual"] };
  }
  if (mode === "auto") {
    return { decision: "allow", reasons: [] };
  }

  const reasons = [];
  const program = normalizeProgram(argv[0]);
  const reviewPrograms = new Set(policy.commands.reviewPrograms.map((x) => String(x).toLowerCase()));
  if (reviewPrograms.has(program)) reasons.push(`program:${program}`);

  const joined = commandText(argv).toLowerCase();
  for (const token of policy.commands.reviewTokens) {
    const normalized = String(token).toLowerCase();
    if (normalized && joined.includes(normalized)) reasons.push(`token:${token}`);
  }

  return reasons.length ? { decision: "review", reasons } : { decision: "allow", reasons: [] };
}

function globToRegExp(glob) {
  const normalized = String(glob || "").replace(/\\/g, "/");
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

function evaluateFileEdit(policy, relativePath) {
  const mode = policy.fileEdits.mode;
  if (mode === "manual") {
    return { decision: "review", reasons: ["file edit policy is manual"] };
  }
  if (mode === "auto_sandbox") {
    return { decision: "allow", reasons: [] };
  }

  const normalized = String(relativePath || "").replace(/\\/g, "/");
  const matches = policy.fileEdits.reviewPathPatterns.filter((pattern) => globToRegExp(pattern).test(normalized));
  return matches.length
    ? { decision: "review", reasons: matches.map((p) => `path:${p}`) }
    : { decision: "allow", reasons: [] };
}

function stableAction(action) {
  if (!action || typeof action !== "object") throw new Error("approval action must be an object");
  const ordered = {};
  for (const key of Object.keys(action).sort()) {
    const value = action[key];
    ordered[key] = value && typeof value === "object" && !Array.isArray(value)
      ? stableAction(value)
      : value;
  }
  return ordered;
}

function fingerprintAction(action) {
  const canonical = JSON.stringify(stableAction(action));
  return crypto.createHash("sha256").update(canonical).digest("hex");
}

module.exports = {
  DEFAULT_POLICY,
  loadPolicy,
  validatePolicy,
  evaluateCommand,
  evaluateFileEdit,
  fingerprintAction,
  _internal: { mergeObject, globToRegExp, commandText, normalizeProgram, stableAction },
};
