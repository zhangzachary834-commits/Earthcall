#!/usr/bin/env node
"use strict";

/**
 * Earthcall Gemini Remote MCP
 *
 * Phase I: read-only access to the actual local Earthcall checkout.
 * Phase II: disposable, network-isolated Docker laboratory with local-only
 * approval gates for commands/file edits. The lab never writes the real repo.
 */

const http = require("http");
const crypto = require("crypto");
const path = require("path");
const { Server } = require("@modelcontextprotocol/sdk/server/index.js");
const { StreamableHTTPServerTransport } = require("@modelcontextprotocol/sdk/server/streamableHttp.js");
const {
  CallToolRequestSchema,
  ListToolsRequestSchema,
  ErrorCode,
  McpError,
} = require("@modelcontextprotocol/sdk/types.js");

const repoTools = require("../src/Singularity/Foreign/mcp/earthcall-repo-tools.js");
const sandbox = require("../src/Singularity/Foreign/mcp/earthcall-sandbox.js");

const PROJECT_ROOT = repoTools.normalizeRoot(
  process.env.EARTHCALL_MCP_ROOT || path.resolve(__dirname, "..")
);
const HOST = process.env.EARTHCALL_MCP_HOST || "127.0.0.1";
const PORT = Number(process.env.EARTHCALL_MCP_PORT || "8765");
const TOKEN = process.env.EARTHCALL_MCP_TOKEN || "";

if (!TOKEN || TOKEN.length < 24) {
  console.error("[Earthcall Gemini MCP] Refusing to start without EARTHCALL_MCP_TOKEN (minimum 24 characters).");
  console.error("Generate one with: openssl rand -hex 32");
  process.exit(2);
}

const READ_ONLY = {
  readOnlyHint: true,
  destructiveHint: false,
  idempotentHint: true,
  openWorldHint: false,
};

const SANDBOX_MUTATION = {
  readOnlyHint: false,
  destructiveHint: false,
  idempotentHint: false,
  openWorldHint: false,
};

const TOOLS = [
  {
    name: "earthcall_list_directory",
    description: "List one directory inside the real local Earthcall checkout. Paths are root-relative and cannot escape Earthcall.",
    annotations: READ_ONLY,
    inputSchema: {
      type: "object",
      properties: {
        path: { type: "string", default: "." },
        max_entries: { type: "integer", minimum: 1, maximum: 1000, default: 200 },
        include_hidden: { type: "boolean", default: false },
      },
    },
  },
  {
    name: "earthcall_read_file",
    description: "Read a UTF-8 text file from the real Earthcall checkout with line numbers. Secret-like/binary/out-of-root paths are refused.",
    annotations: READ_ONLY,
    inputSchema: {
      type: "object",
      required: ["path"],
      properties: {
        path: { type: "string" },
        start_line: { type: "integer", minimum: 1, default: 1 },
        end_line: { type: "integer", minimum: 1 },
        max_chars: { type: "integer", minimum: 1000, maximum: 250000, default: 120000 },
      },
    },
  },
  {
    name: "earthcall_search_text",
    description: "Literal-text search across the real Earthcall checkout, skipping generated/binary/secret-like paths.",
    annotations: READ_ONLY,
    inputSchema: {
      type: "object",
      required: ["query"],
      properties: {
        query: { type: "string", minLength: 1, maxLength: 1000 },
        path: { type: "string", default: "." },
        include_glob: { type: "string", default: "**/*" },
        case_sensitive: { type: "boolean", default: false },
        max_results: { type: "integer", minimum: 1, maximum: 200, default: 100 },
      },
    },
  },
  {
    name: "earthcall_git_status",
    description: "Read local git working-tree status for the real Earthcall checkout, including uncommitted/untracked paths.",
    annotations: READ_ONLY,
    inputSchema: { type: "object", properties: { max_chars: { type: "integer", minimum: 1000, maximum: 250000, default: 120000 } } },
  },
  {
    name: "earthcall_git_diff",
    description: "Read local Earthcall git diff, optionally scoped to one path and/or staged changes.",
    annotations: READ_ONLY,
    inputSchema: {
      type: "object",
      properties: {
        path: { type: "string" },
        staged: { type: "boolean", default: false },
        max_chars: { type: "integer", minimum: 1000, maximum: 250000, default: 120000 },
      },
    },
  },
  {
    name: "earthcall_get_sandbox_policy",
    description: "Read the current WARTCHALL sandbox approval policy. Policy changes must be made locally; this tool cannot weaken its own permissions.",
    annotations: READ_ONLY,
    inputSchema: { type: "object", properties: {} },
  },
  {
    name: "earthcall_sandbox_create",
    description: "Create a disposable writable sandbox copy of the current local Earthcall working tree. It includes current uncommitted files but excludes .git, node_modules, build/cache trees. The real Earthcall checkout is not writable from this sandbox.",
    annotations: SANDBOX_MUTATION,
    inputSchema: {
      type: "object",
      properties: { label: { type: "string", maxLength: 200 } },
    },
  },
  {
    name: "earthcall_sandbox_read_file",
    description: "Read a text file from a disposable sandbox session. Use this after sandbox edits to inspect the laboratory copy.",
    annotations: READ_ONLY,
    inputSchema: {
      type: "object",
      required: ["session_id", "path"],
      properties: {
        session_id: { type: "string" },
        path: { type: "string" },
        start_line: { type: "integer", minimum: 1, default: 1 },
        end_line: { type: "integer", minimum: 1 },
      },
    },
  },
  {
    name: "earthcall_sandbox_write_file",
    description: "Write/replace one UTF-8 file inside the disposable sandbox only. Approval is governed by fileEdits.mode. Default auto_sandbox accepts sandbox edits automatically; manual mode creates a local approval request. This never writes the real Earthcall checkout.",
    annotations: SANDBOX_MUTATION,
    inputSchema: {
      type: "object",
      required: ["session_id", "path", "content"],
      properties: {
        session_id: { type: "string" },
        path: { type: "string" },
        content: { type: "string" },
        approval_id: { type: "string", description: "One-time local approval id when a prior attempt returned approval_required." },
      },
    },
  },
  {
    name: "earthcall_sandbox_run",
    description: "Execute a STRUCTURED argv inside a disposable Docker laboratory. No shell string is accepted. Runtime is network-none, read-only container root, all Linux capabilities dropped, no-new-privileges, bounded CPU/RAM/PIDs/time, no Docker socket, no host HOME/secrets, and only the disposable sandbox workspace is writable. Command approval follows commands.mode; auto_except_review pauses commands whose configured program/tokens match.",
    annotations: SANDBOX_MUTATION,
    inputSchema: {
      type: "object",
      required: ["session_id", "argv"],
      properties: {
        session_id: { type: "string" },
        argv: { type: "array", minItems: 1, maxItems: 128, items: { type: "string" } },
        cwd: { type: "string", default: "." },
        timeout_seconds: { type: "number", minimum: 1, maximum: 900 },
        approval_id: { type: "string", description: "One-time local approval id when a prior attempt returned approval_required." },
      },
    },
  },
  {
    name: "earthcall_sandbox_diff",
    description: "Compare a disposable sandbox session against the real Earthcall working tree snapshot it was created from. Returns workspace-only changes; does not apply them.",
    annotations: READ_ONLY,
    inputSchema: {
      type: "object",
      required: ["session_id"],
      properties: {
        session_id: { type: "string" },
        path: { type: "string" },
        max_chars: { type: "integer", minimum: 1000, maximum: 250000, default: 250000 },
      },
    },
  },
  {
    name: "earthcall_get_approval_status",
    description: "Read one local manual-approval request. Only the local Mac CLI can approve/deny; the remote model cannot self-approve.",
    annotations: READ_ONLY,
    inputSchema: {
      type: "object",
      required: ["approval_id"],
      properties: { approval_id: { type: "string" } },
    },
  },
  {
    name: "earthcall_sandbox_discard",
    description: "Delete a disposable sandbox session and all of its workspace-only edits. The real Earthcall checkout is untouched.",
    annotations: { readOnlyHint: false, destructiveHint: true, idempotentHint: true, openWorldHint: false },
    inputSchema: {
      type: "object",
      required: ["session_id"],
      properties: { session_id: { type: "string" } },
    },
  },
];

function asText(obj) {
  return { content: [{ type: "text", text: JSON.stringify(obj, null, 2) }] };
}

function buildMcpServer() {
  const server = new Server(
    { name: "earthcall-gemini-local-bridge", version: "0.2.0" },
    {
      capabilities: { tools: {} },
      instructions:
        "This connected app gives read-only access to the Person's actual local Earthcall checkout plus a disposable execution laboratory. " +
        "Inspect the real checkout with list/search/read/git tools. For experiments, create a sandbox, edit only that sandbox, and execute only structured argv through earthcall_sandbox_run. " +
        "Sandbox execution has no network, no host secrets/HOME, no Docker socket, bounded resources, and cannot write the real Earthcall checkout. " +
        "When a tool returns approval_required, report the approval id to the Person; only the local Mac approval CLI can authorize it. Never claim a pending action ran.",
    }
  );

  server.setRequestHandler(ListToolsRequestSchema, async () => ({ tools: TOOLS }));

  server.setRequestHandler(CallToolRequestSchema, async (request) => {
    const { name, arguments: args = {} } = request.params;
    try {
      switch (name) {
        case "earthcall_list_directory":
          return asText(repoTools.listDirectory(PROJECT_ROOT, args.path || ".", args.max_entries, Boolean(args.include_hidden)));
        case "earthcall_read_file":
          return asText(repoTools.readFile(PROJECT_ROOT, args.path, args.start_line, args.end_line, args.max_chars));
        case "earthcall_search_text":
          return asText(repoTools.searchText(PROJECT_ROOT, args.query, args.path || ".", args.include_glob || "**/*", Boolean(args.case_sensitive), args.max_results));
        case "earthcall_git_status":
          return asText(repoTools.gitStatus(PROJECT_ROOT, args.max_chars));
        case "earthcall_git_diff":
          return asText(repoTools.gitDiff(PROJECT_ROOT, args.path || null, Boolean(args.staged), args.max_chars));
        case "earthcall_get_sandbox_policy":
          return asText({ policy_file: process.env.EARTHCALL_SANDBOX_POLICY || "config/earthcall-gemini-sandbox-policy.json", policy: sandbox.loadPolicy(PROJECT_ROOT) });
        case "earthcall_sandbox_create":
          return asText(sandbox.createSession(PROJECT_ROOT, args.label || ""));
        case "earthcall_sandbox_read_file":
          return asText(sandbox.readSandboxFile(args.session_id, args.path, args.start_line, args.end_line));
        case "earthcall_sandbox_write_file":
          return asText(sandbox.writeSandboxFile(PROJECT_ROOT, args.session_id, args.path, args.content, args.approval_id || null));
        case "earthcall_sandbox_run":
          return asText(await sandbox.runDocker(PROJECT_ROOT, args.session_id, args.argv, args.cwd || ".", args.timeout_seconds, args.approval_id || null));
        case "earthcall_sandbox_diff":
          return asText(sandbox.sandboxDiff(args.session_id, args.path || null, args.max_chars || 250000));
        case "earthcall_get_approval_status":
          return asText(sandbox.getApproval(args.approval_id));
        case "earthcall_sandbox_discard":
          return asText(sandbox.deleteSession(args.session_id));
        default:
          throw new McpError(ErrorCode.MethodNotFound, `Unknown tool: ${name}`);
      }
    } catch (error) {
      return {
        content: [{ type: "text", text: JSON.stringify({ error: error.message || String(error) }, null, 2) }],
        isError: true,
      };
    }
  });

  return server;
}

function timingSafeTokenEqual(candidate) {
  const a = Buffer.from(String(candidate || ""));
  const b = Buffer.from(TOKEN);
  return a.length === b.length && crypto.timingSafeEqual(a, b);
}

function suppliedToken(req, url) {
  const auth = req.headers.authorization || "";
  if (auth.startsWith("Bearer ")) return auth.slice("Bearer ".length);
  return url.searchParams.get("token") || "";
}

function reject(res, status, message) {
  const body = JSON.stringify({ error: message });
  res.writeHead(status, {
    "content-type": "application/json; charset=utf-8",
    "content-length": Buffer.byteLength(body),
    "cache-control": "no-store",
  });
  res.end(body);
}

const httpServer = http.createServer(async (req, res) => {
  const url = new URL(req.url || "/", `http://${req.headers.host || "localhost"}`);

  if (req.method === "GET" && url.pathname === "/healthz") {
    const policy = sandbox.loadPolicy(PROJECT_ROOT);
    const body = JSON.stringify({
      ok: true,
      service: "earthcall-gemini-local-bridge",
      version: "0.2.0",
      root: path.basename(PROJECT_ROOT),
      sandbox: {
        command_mode: policy.commands.mode,
        file_edit_mode: policy.fileEdits.mode,
        image: process.env.EARTHCALL_SANDBOX_IMAGE || policy.image,
      },
    });
    res.writeHead(200, { "content-type": "application/json; charset=utf-8", "cache-control": "no-store" });
    res.end(body);
    return;
  }

  if (url.pathname !== "/mcp") {
    reject(res, 404, "Not found");
    return;
  }
  if (!timingSafeTokenEqual(suppliedToken(req, url))) {
    reject(res, 401, "Unauthorized");
    return;
  }
  if (req.method !== "POST") {
    res.setHeader("allow", "POST");
    reject(res, 405, "Only POST is supported by this stateless MCP endpoint");
    return;
  }

  res.setHeader("cache-control", "no-store");
  res.setHeader("x-content-type-options", "nosniff");

  const mcpServer = buildMcpServer();
  const transport = new StreamableHTTPServerTransport({
    sessionIdGenerator: undefined,
    enableJsonResponse: true,
  });

  try {
    await mcpServer.connect(transport);
    await transport.handleRequest(req, res);
  } catch (error) {
    console.error("[Earthcall Gemini MCP] Request failed:", error);
    if (!res.headersSent) reject(res, 500, "MCP request failed");
    else if (!res.writableEnded) res.end();
  } finally {
    try { await transport.close(); } catch (_) {}
    try { await mcpServer.close(); } catch (_) {}
  }
});

httpServer.on("clientError", (err, socket) => {
  console.error("[Earthcall Gemini MCP] clientError:", err.message);
  socket.end("HTTP/1.1 400 Bad Request\r\n\r\n");
});

httpServer.listen(PORT, HOST, () => {
  const policy = sandbox.loadPolicy(PROJECT_ROOT);
  console.error(`[Earthcall Gemini MCP] bridge v0.2 listening at http://${HOST}:${PORT}/mcp`);
  console.error(`[Earthcall Gemini MCP] Root jail: ${PROJECT_ROOT}`);
  console.error(`[Earthcall Gemini MCP] Sandbox: commands=${policy.commands.mode}, fileEdits=${policy.fileEdits.mode}, network=none`);
  console.error("[Earthcall Gemini MCP] Tunnel localhost over HTTPS, then append ?token=<your token> in Gemini Developer Mode.");
});
