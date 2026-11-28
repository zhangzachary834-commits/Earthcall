#!/usr/bin/env node
"use strict";

const fs = require("fs");
const path = require("path");
const policyLib = require("../src/Singularity/Foreign/mcp/earthcall-sandbox-policy.js");

const repoRoot = path.resolve(__dirname, "..");
const file = process.env.EARTHCALL_SANDBOX_POLICY || path.join(repoRoot, "config", "earthcall-gemini-sandbox-policy.json");

function read() { return policyLib.loadPolicy(file); }
function write(policy) {
  policyLib.validatePolicy(policy);
  fs.mkdirSync(path.dirname(file), { recursive: true });
  fs.writeFileSync(file, JSON.stringify(policy, null, 2) + "\n");
}
function show(policy) {
  console.log(`Policy: ${file}`);
  console.log(`  file edits: ${policy.fileEdits.mode}`);
  console.log(`  commands:   ${policy.commands.mode}`);
  console.log(`  image:      ${policy.image}`);
  console.log(`  review programs: ${policy.commands.reviewPrograms.join(", ")}`);
  console.log(`  review tokens:   ${policy.commands.reviewTokens.join(" | ")}`);
}

const args = process.argv.slice(2);
const cmd = args[0] || "show";
const policy = read();

switch (cmd) {
  case "show":
    show(policy);
    break;
  case "lockdown":
    policy.fileEdits.mode = "manual";
    policy.commands.mode = "manual";
    write(policy); show(policy);
    break;
  case "lab":
    policy.fileEdits.mode = "auto_sandbox";
    policy.commands.mode = "auto_except_review";
    write(policy); show(policy);
    break;
  case "auto":
    policy.fileEdits.mode = "auto_sandbox";
    policy.commands.mode = "auto";
    write(policy); show(policy);
    break;
  case "set-command-mode": {
    const value = args[1];
    if (!new Set(["manual", "auto", "auto_except_review"]).has(value)) throw new Error("command mode: manual | auto | auto_except_review");
    policy.commands.mode = value;
    write(policy); show(policy);
    break;
  }
  case "set-file-mode": {
    const value = args[1];
    if (!new Set(["manual", "auto_sandbox", "auto_safe_paths"]).has(value)) throw new Error("file mode: manual | auto_sandbox | auto_safe_paths");
    policy.fileEdits.mode = value;
    write(policy); show(policy);
    break;
  }
  case "add-review-token": {
    const token = args.slice(1).join(" ");
    if (!token) throw new Error("Provide a token/sub-string to review.");
    if (!policy.commands.reviewTokens.includes(token)) policy.commands.reviewTokens.push(token);
    write(policy); show(policy);
    break;
  }
  case "remove-review-token": {
    const token = args.slice(1).join(" ");
    policy.commands.reviewTokens = policy.commands.reviewTokens.filter((x) => x !== token);
    write(policy); show(policy);
    break;
  }
  case "add-review-program": {
    const program = args[1];
    if (!program) throw new Error("Provide a program basename.");
    if (!policy.commands.reviewPrograms.includes(program)) policy.commands.reviewPrograms.push(program);
    write(policy); show(policy);
    break;
  }
  case "remove-review-program": {
    const program = args[1];
    policy.commands.reviewPrograms = policy.commands.reviewPrograms.filter((x) => x !== program);
    write(policy); show(policy);
    break;
  }
  default:
    throw new Error("Usage: earthcall-mcp-policy.js show|lockdown|lab|auto|set-command-mode <mode>|set-file-mode <mode>|add-review-token <text>|remove-review-token <text>|add-review-program <name>|remove-review-program <name>");
}
