#!/usr/bin/env node
"use strict";

/**
 * Earthcall Gemini Remote Filesystem MCP
 *
 * A deliberately READ-ONLY remote MCP gateway that lets Gemini Tasks inspect
 * the local Earthcall checkout without exposing Earthcall's mutation-capable
 * First Mover tools to the public internet.
 *
 * Transport: MCP Streamable HTTP (2025-era protocol, matching Gemini Connected Apps)
 * Endpoint:  POST /mcp?token=<EARTHCALL_MCP_TOKEN>
 * Health:    GET  /healthz
 *
 * Recommended topology:
 *   Gemini -> HTTPS (Cloudflare Tunnel) -> 127.0.0.1:8765 -> this server -> Earthcall repo
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

const TOOLS = [
  {
    name: "earthcall_list_directory",
    description: "List one directory inside the local Earthcall checkout. Use this to discover files and subdirectories before reading them. Paths are root-relative and cannot escape the Earthcall checkout.",
    annotations: READ_ONLY,
    inputSchema: {
      type: "object",
      properties: {
        path: { type: "string", description: "Earthcall-root-relative directory path. Use '.' for repository root.", default: "." },
        max_entries: { type: "integer", minimum: 1, maximum: 1000, default: 200 },
        include_hidden: { type: "boolean", default: false },
      },
    },
  },
  {
    name: "earthcall_read_file",
    description: "Read a UTF-8 text file inside the local Earthcall checkout with line numbers. Secret-like files, binary files, and paths outside Earthcall are refused. Use start_line/end_line to page through large files.",
    annotations: READ_ONLY,
    inputSchema: {
      type: "object",
      required: ["path"],
      properties: {
        path: { type: "string", description: "Earthcall-root-relative file path." },
        start_line: { type: "integer", minimum: 1, default: 1 },
        end_line: { type: "integer", minimum: 1, description: "Optional inclusive last line. At most ~2000 lines are returned per call." },
        max_chars: { type: "integer", minimum: 1000, maximum: 250000, default: 120000 },
      },
    },
  },
  {
    name: "earthcall_search_text",
    description: "Literal-text search across the local Earthcall checkout. Returns file path, line, column, and matching line. Search skips .git, node_modules, build outputs, binary files, and secret-like paths.",
    annotations: READ_ONLY,
    inputSchema: {
      type: "object",
      required: ["query"],
      properties: {
        query: { type: "string", minLength: 1, maxLength: 1000, description: "Literal text to find (not a regex)." },
        path: { type: "string", default: ".", description: "Optional Earthcall-root-relative subtree to search." },
        include_glob: { type: "string", default: "**/*", description: "Optional glob such as '**/*.md' or 'src/**/*.cpp'." },
        case_sensitive: { type: "boolean", default: false },
        max_results: { type: "integer", minimum: 1, maximum: 200, default: 100 },
      },
    },
  },
  {
    name: "earthcall_git_status",
    description: "Read local git working-tree status for Earthcall, including uncommitted and untracked paths. This is read-only and uses GIT_OPTIONAL_LOCKS=0.",
    annotations: READ_ONLY,
    inputSchema: {
      type: "object",
      properties: {
        max_chars: { type: "integer", minimum: 1000, maximum: 250000, default: 120000 },
      },
    },
  },
  {
    name: "earthcall_git_diff",
    description: "Read the local Earthcall git diff, optionally limited to one path and/or staged changes. This lets Gemini inspect changes that do not exist on GitHub yet.",
    annotations: READ_ONLY,
    inputSchema: {
      type: "object",
      properties: {
        path: { type: "string", description: "Optional Earthcall-root-relative file or directory path." },
        staged: { type: "boolean", default: false },
        max_chars: { type: "integer", minimum: 1000, maximum: 250000, default: 120000 },
      },
    },
  },
];

function asText(obj) {
  return { content: [{ type: "text", text: JSON.stringify(obj, null, 2) }] };
}

function buildMcpServer() {
  const server = new Server(
    {
      name: "earthcall-gemini-local-bridge",
      version: "0.1.0",
    },
    {
      capabilities: { tools: {} },
      instructions:
        "This connected app gives read-only access to the Person's local Earthcall source checkout. " +
        "Use list/search/read/git tools to inspect the actual workspace instead of guessing from conversation context. " +
        "All paths are constrained to Earthcall. The app cannot write, delete, execute arbitrary commands, or escape the repository root.",
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
          return asText(repoTools.searchText(
            PROJECT_ROOT,
            args.query,
            args.path || ".",
            args.include_glob || "**/*",
            Boolean(args.case_sensitive),
            args.max_results
          ));
        case "earthcall_git_status":
          return asText(repoTools.gitStatus(PROJECT_ROOT, args.max_chars));
        case "earthcall_git_diff":
          return asText(repoTools.gitDiff(PROJECT_ROOT, args.path || null, Boolean(args.staged), args.max_chars));
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
    const body = JSON.stringify({ ok: true, service: "earthcall-gemini-local-bridge", root: path.basename(PROJECT_ROOT) });
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

  // Never let a proxy cache source-code-bearing responses.
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
  console.error(`[Earthcall Gemini MCP] READ-ONLY bridge listening at http://${HOST}:${PORT}/mcp`);
  console.error(`[Earthcall Gemini MCP] Root jail: ${PROJECT_ROOT}`);
  console.error("[Earthcall Gemini MCP] Tunnel this localhost endpoint over HTTPS, then append ?token=<your token> in Gemini Developer Mode.");
});
