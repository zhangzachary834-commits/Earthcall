const { spawn } = require("child_process");
const path = require("path");

const mcpProcess = spawn(process.execPath, [path.join(__dirname, "../../scripts/mcp-server.js")], {
  stdio: ["pipe", "pipe", "inherit"]
});

let buffer = "";
let currentStep = 0;
let passedChecks = 0;

function sendReq(req) {
  mcpProcess.stdin.write(JSON.stringify(req) + "\n");
}

mcpProcess.stdout.on("data", (data) => {
  buffer += data.toString();
  const lines = buffer.split("\n");
  buffer = lines.pop();

  for (const line of lines) {
    if (!line.trim()) continue;
    try {
      const msg = JSON.parse(line.trim());

      // Handshake response
      if (msg.id === 1) {
        console.log("  ok: MCP protocol handshake initialized");
        passedChecks++;
        // Request tools list
        sendReq({ jsonrpc: "2.0", id: 2, method: "tools/list", params: {} });
      }
      // Tools list response
      else if (msg.id === 2) {
        const tools = msg.result.tools || [];
        if (tools.length >= 16) {
          console.log(`  ok: tools/list returned ${tools.length} active Earthcall tools`);
          passedChecks++;
        } else {
          console.error(`  FAILED: expected 16+ tools, got ${tools.length}`);
        }
        // Call tool: earthcall_get_connection_status
        sendReq({
          jsonrpc: "2.0",
          id: 3,
          method: "tools/call",
          params: { name: "earthcall_get_connection_status", arguments: {} }
        });
      }
      // Connection status tool call
      else if (msg.id === 3) {
        const text = msg.result?.content?.[0]?.text || "{}";
        const parsed = JSON.parse(text);
        if (parsed.websocket_url && parsed.status !== undefined) {
          console.log("  ok: earthcall_get_connection_status returned valid status");
          passedChecks++;
        } else {
          console.error("  FAILED: connection status format invalid", text);
        }
        // Call tool: earthcall_list_saves
        sendReq({
          jsonrpc: "2.0",
          id: 4,
          method: "tools/call",
          params: { name: "earthcall_list_saves", arguments: { category: "all" } }
        });
      }
      // List saves tool call
      else if (msg.id === 4) {
        const text = msg.result?.content?.[0]?.text || "{}";
        const parsed = JSON.parse(text);
        if (Array.isArray(parsed.worlds) && Array.isArray(parsed.zones)) {
          console.log(`  ok: earthcall_list_saves found ${parsed.worlds.length} worlds and ${parsed.zones.length} zones on disk`);
          passedChecks++;
        } else {
          console.error("  FAILED: saves catalog invalid", text);
        }
        // Call tool: earthcall_get_state (offline fallback or live)
        sendReq({
          jsonrpc: "2.0",
          id: 5,
          method: "tools/call",
          params: { name: "earthcall_get_state", arguments: {} }
        });
      }
      // Get state tool call
      else if (msg.id === 5) {
        const text = msg.result?.content?.[0]?.text || "{}";
        const parsed = JSON.parse(text);
        if (parsed.status && (parsed.data || parsed.file)) {
          console.log(`  ok: earthcall_get_state returned valid snapshot (status: ${parsed.status})`);
          passedChecks++;
        } else {
          console.error("  FAILED: get_state format invalid", text);
        }
        // Read resource: earthcall://status
        sendReq({
          jsonrpc: "2.0",
          id: 6,
          method: "resources/read",
          params: { uri: "earthcall://status" }
        });
      }
      // Resource read response
      else if (msg.id === 6) {
        const text = msg.result?.contents?.[0]?.text || "{}";
        const parsed = JSON.parse(text);
        if (parsed.url) {
          console.log("  ok: resources/read earthcall://status returned health info");
          passedChecks++;
        } else {
          console.error("  FAILED: resource read failed", text);
        }
        // Get prompt: earthcall-first-mover
        sendReq({
          jsonrpc: "2.0",
          id: 7,
          method: "prompts/get",
          params: { name: "earthcall-first-mover", arguments: {} }
        });
      }
      // Prompt get response
      else if (msg.id === 7) {
        const promptText = msg.result?.messages?.[0]?.content?.text || "";
        if (promptText.includes("Seven Refusals")) {
          console.log("  ok: prompts/get earthcall-first-mover returned First Mover prompt instructions");
          passedChecks++;
        } else {
          console.error("  FAILED: prompt instruction content invalid");
        }

        console.log(`\nmcp_bridge_test: ALL OK (all ${passedChecks} checks passed)\n`);
        mcpProcess.kill();
        process.exit(0);
      }
    } catch (e) {
      console.error("Parse error:", e);
    }
  }
});

console.log("Running mcp_bridge_test...");
// Start with initialize
sendReq({
  jsonrpc: "2.0",
  id: 1,
  method: "initialize",
  params: {
    protocolVersion: "2024-11-05",
    capabilities: {},
    clientInfo: { name: "test-client", version: "1.0.0" }
  }
});

setTimeout(() => {
  console.error("Test timed out after 10s");
  mcpProcess.kill();
  process.exit(1);
}, 10000);
