#!/usr/bin/env node
"use strict";

const fs = require("fs");
const path = require("path");
const readline = require("readline");
const os = require("os");

function stateRoot() {
  if (process.env.EARTHCALL_MCP_STATE_DIR) return path.resolve(process.env.EARTHCALL_MCP_STATE_DIR);
  const home = os.homedir();
  if (process.platform === "darwin") return path.join(home, "Library", "Application Support", "Earthcall", "gemini-mcp");
  if (process.platform === "win32") return path.join(process.env.LOCALAPPDATA || path.join(home, "AppData", "Local"), "Earthcall", "gemini-mcp");
  return path.join(process.env.XDG_STATE_HOME || path.join(home, ".local", "state"), "earthcall", "gemini-mcp");
}

function approvalPath(id) {
  if (!/^[a-f0-9]{24}$/.test(String(id || ""))) throw new Error("Invalid approval id.");
  return path.join(stateRoot(), "approvals", `${id}.json`);
}

function summarize(record) {
  const a = record.action || {};
  console.log(`Approval: ${record.id}`);
  console.log(`Status:   ${record.status}`);
  console.log(`Created:  ${record.created_at}`);
  console.log(`Reasons:  ${(record.reasons || []).join(", ") || "(none)"}`);
  console.log(`Hash:     ${record.fingerprint}`);
  console.log();
  if (a.type === "command") {
    console.log("COMMAND");
    console.log(`  session: ${a.session_id}`);
    console.log(`  cwd:     ${a.cwd}`);
    console.log(`  argv:    ${JSON.stringify(a.argv)}`);
  } else if (a.type === "file_edit") {
    const content = String(a.content || "");
    console.log("FILE EDIT");
    console.log(`  session: ${a.session_id}`);
    console.log(`  path:    ${a.path}`);
    console.log(`  bytes:   ${Buffer.byteLength(content, "utf8")}`);
    console.log("  preview:");
    console.log(content.slice(0, 2000).split("\n").map((l) => `    ${l}`).join("\n"));
    if (content.length > 2000) console.log("    … [preview truncated]");
  } else {
    console.log(JSON.stringify(a, null, 2));
  }
}

function save(file, record) {
  record.decided_at = new Date().toISOString();
  fs.writeFileSync(file, JSON.stringify(record, null, 2), { mode: 0o600 });
}

async function main() {
  const id = process.argv[2];
  if (!id) {
    console.error("Usage: node scripts/earthcall-mcp-approve.js <approval-id> [--yes|--deny]");
    process.exit(2);
  }
  const file = approvalPath(id);
  if (!fs.existsSync(file)) throw new Error(`Unknown approval id: ${id}`);
  const record = JSON.parse(fs.readFileSync(file, "utf8"));
  summarize(record);
  if (record.status !== "pending") {
    console.log(`No action taken: request is already ${record.status}.`);
    return;
  }

  if (process.argv.includes("--deny")) {
    record.status = "denied";
    save(file, record);
    console.log("DENIED.");
    return;
  }
  if (process.argv.includes("--yes")) {
    record.status = "approved";
    save(file, record);
    console.log("APPROVED. The remote agent must retry the exact action with this approval id.");
    return;
  }

  const rl = readline.createInterface({ input: process.stdin, output: process.stdout });
  const answer = await new Promise((resolve) => rl.question("Approve this exact action? [y/N] ", resolve));
  rl.close();
  if (/^y(es)?$/i.test(String(answer).trim())) {
    record.status = "approved";
    save(file, record);
    console.log("APPROVED. The remote agent must retry the exact action with this approval id.");
  } else {
    record.status = "denied";
    save(file, record);
    console.log("DENIED.");
  }
}

main().catch((err) => {
  console.error(err.message || String(err));
  process.exit(1);
});
