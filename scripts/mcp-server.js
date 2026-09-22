#!/usr/bin/env node

/**
 * Earthcall Model Context Protocol (MCP) Server Entrypoint (Option A)
 * 
 * Allows AI agents (Claude Desktop, Cursor, Gemini, etc.) to control and
 * author Earthcall in real time over standard MCP stdio JSON-RPC.
 * 
 * Usage:
 *   node scripts/mcp-server.js
 *   npm run mcp
 */

require("../src/Singularity/Foreign/mcp/earthcall-mcp-server.js");
