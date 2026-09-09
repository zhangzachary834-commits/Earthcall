#!/usr/bin/env node

/**
 * Earthcall Model Context Protocol (MCP) Server
 * 
 * Provides an MCP bridge (Option A) connecting AI models (Claude, Cursor, Gemini, etc.)
 * directly into Earthcall's running engine over WebSocket (ws://localhost:8080),
 * with graceful offline inspection of disk saves when the engine is not running.
 * 
 * Refusal #1 & Refusal #6 compliant:
 * Communicates through Earthcall's first-class property paths, First-Mover laws,
 * and live world events.
 */

const { Server } = require("@modelcontextprotocol/sdk/server/index.js");
const { StdioServerTransport } = require("@modelcontextprotocol/sdk/server/stdio.js");
const {
  CallToolRequestSchema,
  ListToolsRequestSchema,
  ListResourcesRequestSchema,
  ReadResourceRequestSchema,
  ListPromptsRequestSchema,
  GetPromptRequestSchema,
  ErrorCode,
  McpError
} = require("@modelcontextprotocol/sdk/types.js");

const fs = require("fs");
const path = require("path");

// Configuration
const DEFAULT_WS_URL = process.env.EARTHCALL_WS_URL || "ws://localhost:8080";
const PROJECT_ROOT = path.resolve(__dirname, "../../../../");
const SAVES_DIR = path.join(PROJECT_ROOT, "saves");

// ============================================================================
// Earthcall WebSocket Bridge Client
// ============================================================================
class EarthcallBridgeClient {
  constructor(url = DEFAULT_WS_URL) {
    this.url = url;
    this.ws = null;
    this.connected = false;
    this.latestSnapshot = null;
    this.pendingRequests = new Map();
    this.requestIdCounter = 1;
    this.reconnectInterval = 3000;
    this.reconnectTimer = null;
    this.connect();
  }

  connect() {
    try {
      // Use built-in WebSocket available in modern Node.js
      const WS = globalThis.WebSocket;
      if (!WS) {
        console.error("[Earthcall MCP] No global WebSocket implementation found");
        return;
      }

      this.ws = new WS(this.url);

      this.ws.onopen = () => {
        this.connected = true;
        console.error(`[Earthcall MCP] Connected to live engine at ${this.url}`);
        // Request initial state snapshot
        this.send({ type: "get_state" });
      };

      this.ws.onmessage = (event) => {
        try {
          const data = JSON.parse(event.data);
          // If message contains world snapshot data
          if (data.active_zone || data.zones || data.objects) {
            this.latestSnapshot = data;
          }

          // Check if this matches a pending request ack
          const msgType = data.type || "";
          for (const [reqId, handler] of this.pendingRequests.entries()) {
            if (handler.matches(data)) {
              handler.resolve(data);
              this.pendingRequests.delete(reqId);
              break;
            }
          }
        } catch (err) {
          console.error("[Earthcall MCP] Error parsing message:", err);
        }
      };

      this.ws.onclose = () => {
        this.connected = false;
        this.scheduleReconnect();
      };

      this.ws.onerror = (err) => {
        this.connected = false;
        // Silent error to prevent stdio noise
      };
    } catch (e) {
      this.connected = false;
      this.scheduleReconnect();
    }
  }

  scheduleReconnect() {
    if (this.reconnectTimer) return;
    this.reconnectTimer = setTimeout(() => {
      this.reconnectTimer = null;
      this.connect();
    }, this.reconnectInterval);
  }

  send(payload) {
    if (this.ws && this.connected && this.ws.readyState === 1) {
      this.ws.send(JSON.stringify(payload));
      return true;
    }
    return false;
  }

  sendWithAck(payload, ackType, timeoutMs = 4000) {
    return new Promise((resolve, reject) => {
      if (!this.connected) {
        return reject(new Error("Earthcall engine is offline (ws://localhost:8080 unreachable)."));
      }

      const reqId = this.requestIdCounter++;
      const timer = setTimeout(() => {
        this.pendingRequests.delete(reqId);
        // If timed out waiting for ack, resolve with optimistic acknowledgement if send succeeded
        resolve({ status: "sent_without_ack", payload });
      }, timeoutMs);

      this.pendingRequests.set(reqId, {
        matches: (data) => data.type === ackType || data.type === `${payload.type}_ack`,
        resolve: (res) => {
          clearTimeout(timer);
          resolve(res);
        }
      });

      this.ws.send(JSON.stringify(payload));
    });
  }

  getOfflineSnapshot() {
    try {
      const worldsDir = path.join(SAVES_DIR, "worlds");
      if (fs.existsSync(worldsDir)) {
        const files = fs.readdirSync(worldsDir).filter(f => f.endsWith(".json"));
        if (files.length > 0) {
          // Find most recently modified save file
          let latestFile = files[0];
          let latestMtime = 0;
          for (const f of files) {
            const stat = fs.statSync(path.join(worldsDir, f));
            if (stat.mtimeMs > latestMtime) {
              latestMtime = stat.mtimeMs;
              latestFile = f;
            }
          }
          const content = fs.readFileSync(path.join(worldsDir, latestFile), "utf-8");
          const parsed = JSON.parse(content);
          return {
            source: "offline_save_file",
            file: latestFile,
            data: parsed
          };
        }
      }
    } catch (e) {
      console.error("[Earthcall MCP] Error reading offline saves:", e);
    }
    return null;
  }
}

const client = new EarthcallBridgeClient();

// ============================================================================
// Tool Definitions
// ============================================================================
const TOOLS = [
  {
    name: "earthcall_get_state",
    description: "Retrieves the current world state from Earthcall (active zone, 3D objects, positions, colors, shapes, laws, and player status). Falls back to disk saves if engine is offline.",
    inputSchema: {
      type: "object",
      properties: {}
    }
  },
  {
    name: "earthcall_spawn_object",
    description: "Spawns a new 3D entity into the active Earthcall zone (Cube, Sphere, Cylinder, Cone, Torus, Plane) with specified position, rotation, dimensions, and colors.",
    inputSchema: {
      type: "object",
      properties: {
        shape: {
          type: "string",
          enum: ["Cube", "Sphere", "Cylinder", "Cone", "Torus", "Plane"],
          description: "Geometric shape kind of the object",
          default: "Cube"
        },
        name: {
          type: "string",
          description: "Optional identifier or display name for the new object"
        },
        position: {
          type: "array",
          items: { type: "number" },
          description: "3D world position coordinates [x, y, z]. If omitted, spawns in front of the player.",
          minItems: 3,
          maxItems: 3
        },
        rotation: {
          type: "array",
          items: { type: "number" },
          description: "Euler rotation in degrees [rx, ry, rz]",
          minItems: 3,
          maxItems: 3
        },
        dimensions: {
          type: "number",
          description: "Uniform scale dimension of the object (default: 1.0)"
        },
        color: {
          type: "array",
          items: { type: "number" },
          description: "RGB color components [r, g, b] (0.0 to 1.0 or 0 to 255)",
          minItems: 3,
          maxItems: 3
        },
        materialId: {
          type: "string",
          description: "Optional material identifier"
        }
      }
    }
  },
  {
    name: "earthcall_transform_object",
    description: "Transforms, repositions, rotates, scales, repaints, or modifies an existing 3D object in Earthcall.",
    inputSchema: {
      type: "object",
      required: ["id"],
      properties: {
        id: {
          type: "string",
          description: "Unique object identifier or name"
        },
        position: {
          type: "array",
          items: { type: "number" },
          description: "New position [x, y, z]",
          minItems: 3,
          maxItems: 3
        },
        rotation: {
          type: "array",
          items: { type: "number" },
          description: "New rotation in degrees [rx, ry, rz]",
          minItems: 3,
          maxItems: 3
        },
        dimensions: {
          type: "number",
          description: "New uniform scale dimension"
        },
        color: {
          type: "array",
          items: { type: "number" },
          description: "New RGB color [r, g, b]",
          minItems: 3,
          maxItems: 3
        },
        shape: {
          type: "string",
          enum: ["Cube", "Sphere", "Cylinder", "Cone", "Torus", "Plane"],
          description: "Change shape kind"
        },
        materialId: {
          type: "string",
          description: "New material ID"
        }
      }
    }
  },
  {
    name: "earthcall_delete_object",
    description: "Removes an object from the active Earthcall zone by identifier.",
    inputSchema: {
      type: "object",
      required: ["id"],
      properties: {
        id: {
          type: "string",
          description: "Unique identifier of the object to remove"
        }
      }
    }
  },
  {
    name: "earthcall_write_property",
    description: "Writes a value to any PropertyPath on any target being in Earthcall (Refusal #6 compliant). Targets include '@player', '@active_zone', '@screen-recorder', '@file-channel', '@vfs', or any object identifier.",
    inputSchema: {
      type: "object",
      required: ["target", "property", "value"],
      properties: {
        target: {
          type: "string",
          description: "Target being identifier (e.g. '@player', '@active_zone', '@screen-recorder', or object ID)"
        },
        property: {
          type: "string",
          description: "Property path to write (e.g. 'position.y', 'recording', 'fps', 'color.r')"
        },
        value: {
          description: "Value to write (boolean, number, string, or array)"
        }
      }
    }
  },
  {
    name: "earthcall_author_law",
    description: "Authors or modifies an Event-Condition-Action (ECA) Law live in Earthcall. Supports built-in templates (zero-g, color-pulse, orbit, bounce) or custom When-Condition-Action AST models.",
    inputSchema: {
      type: "object",
      required: ["name"],
      properties: {
        name: {
          type: "string",
          description: "Human-readable display name for the Law"
        },
        identifier: {
          type: "string",
          description: "Unique Law identifier (e.g. 'law-zero-g', 'law-hover-glow')"
        },
        activation: {
          type: "number",
          enum: [0, 1],
          description: "0 = WhileTrue (per-frame continuous), 1 = OnEvent (reactive edge)",
          default: 0
        },
        trigger: {
          type: "string",
          description: "Event trigger for OnEvent laws (e.g. 'contact-began', 'file-modified')"
        },
        condition: {
          type: "object",
          description: "Optional condition node { path, op: '=='|'!='|'<'|'<='|'>'|'>=', operand }",
          properties: {
            path: { type: "string" },
            op: { type: "string" },
            operand: {}
          }
        },
        action: {
          type: "object",
          description: "Action node { kind: 'flow'|'map'|'set'|'spawn'|'destroy', path, formula, value, concept }",
          properties: {
            kind: { type: "string" },
            path: { type: "string" },
            amplitude: { type: "number" },
            frequency: { type: "number" },
            offset: { type: "number" },
            phase: { type: "number" },
            value: {}
          }
        }
      }
    }
  },
  {
    name: "earthcall_toggle_law",
    description: "Enables or disables an existing Law in Earthcall by identifier.",
    inputSchema: {
      type: "object",
      required: ["identifier", "enabled"],
      properties: {
        identifier: {
          type: "string",
          description: "Identifier or name of the Law"
        },
        enabled: {
          type: "boolean",
          description: "true to enable, false to disable"
        }
      }
    }
  },
  {
    name: "earthcall_delete_law",
    description: "Deletes an authored Law from Earthcall's LawManager by identifier.",
    inputSchema: {
      type: "object",
      required: ["identifier"],
      properties: {
        identifier: {
          type: "string",
          description: "Identifier of the Law to delete"
        }
      }
    }
  },
  {
    name: "earthcall_switch_zone",
    description: "Switches the active Zone in Earthcall by name or index.",
    inputSchema: {
      type: "object",
      properties: {
        name: {
          type: "string",
          description: "Name of the destination Zone"
        },
        index: {
          type: "number",
          description: "Zero-based index of the destination Zone"
        }
      }
    }
  },
  {
    name: "earthcall_create_zone",
    description: "Creates a new authored Zone in Earthcall's Ourverse.",
    inputSchema: {
      type: "object",
      required: ["name"],
      properties: {
        name: {
          type: "string",
          description: "Name of the new Zone"
        },
        kind: {
          type: "string",
          description: "Zone classification kind (default: 'zone')",
          default: "zone"
        }
      }
    }
  },
  {
    name: "earthcall_teleport_player",
    description: "Teleports the Person/player to specified coordinates.",
    inputSchema: {
      type: "object",
      required: ["position"],
      properties: {
        position: {
          type: "array",
          items: { type: "number" },
          description: "Destination coordinates [x, y, z]",
          minItems: 3,
          maxItems: 3
        }
      }
    }
  },
  {
    name: "earthcall_speak",
    description: "Emits an utterance into Earthcall's Language modality and EventBus.",
    inputSchema: {
      type: "object",
      required: ["utterance"],
      properties: {
        utterance: {
          type: "string",
          description: "The spoken or synthesized message"
        },
        targetSingularId: {
          type: "string",
          description: "Optional ID of target being to route the utterance to"
        }
      }
    }
  },
  {
    name: "earthcall_save_world",
    description: "Triggers a world state save to disk under saves/worlds/.",
    inputSchema: {
      type: "object",
      properties: {
        name: {
          type: "string",
          description: "Optional custom name/label for the save"
        }
      }
    }
  },
  {
    name: "earthcall_screen_record",
    description: "Controls the Singularity Screen Recorder (@screen-recorder) to capture snapshots or video frame sequences.",
    inputSchema: {
      type: "object",
      required: ["action"],
      properties: {
        action: {
          type: "string",
          enum: ["start", "stop", "pause", "resume", "snapshot"],
          description: "Recording control action to execute"
        },
        format: {
          type: "string",
          enum: ["ppm_sequence", "png_sequence", "raw", "pipe"],
          description: "Export format"
        },
        mode: {
          type: "string",
          enum: ["viewport", "display", "window"],
          description: "Capture source mode"
        }
      }
    }
  },
  {
    name: "earthcall_list_saves",
    description: "Lists all saved worlds, zones, and homes in the saves/ directory on disk.",
    inputSchema: {
      type: "object",
      properties: {
        category: {
          type: "string",
          enum: ["all", "worlds", "zones", "homes"],
          default: "all"
        }
      }
    }
  },
  {
    name: "earthcall_get_connection_status",
    description: "Returns the live connection status of the MCP bridge to Earthcall's C++ WebSocket server.",
    inputSchema: {
      type: "object",
      properties: {}
    }
  }
];

// ============================================================================
// MCP Server Initialization
// ============================================================================
const server = new Server(
  {
    name: "earthcall-mcp",
    version: "1.0.0"
  },
  {
    capabilities: {
      tools: {},
      resources: {},
      prompts: {}
    }
  }
);

// Tool List Handler
server.setRequestHandler(ListToolsRequestSchema, async () => {
  return { tools: TOOLS };
});

// Tool Call Handler
server.setRequestHandler(CallToolRequestSchema, async (request) => {
  const { name, arguments: args = {} } = request.params;

  try {
    switch (name) {
      case "earthcall_get_connection_status": {
        return {
          content: [
            {
              type: "text",
              text: JSON.stringify({
                connected: client.connected,
                websocket_url: client.url,
                status: client.connected ? "connected" : "offline",
                hint: client.connected
                  ? "Earthcall live C++ engine is connected and ready."
                  : "Earthcall engine is offline. Start the engine via 'Run Earthcall.command' or 'scripts/build.sh webgpu run'."
              }, null, 2)
            }
          ]
        };
      }

      case "earthcall_get_state": {
        if (client.connected) {
          // Send request and await state update or return cached snapshot
          client.send({ type: "get_state" });
          await new Promise(r => setTimeout(r, 150));
          return {
            content: [
              {
                type: "text",
                text: JSON.stringify({
                  status: "live_connected",
                  data: client.latestSnapshot || { note: "Snapshot requested, refreshing..." }
                }, null, 2)
              }
            ]
          };
        } else {
          const offline = client.getOfflineSnapshot();
          if (offline) {
            return {
              content: [
                {
                  type: "text",
                  text: JSON.stringify({
                    status: "engine_offline_save_fallback",
                    file: offline.file,
                    note: "Earthcall live engine is offline. Showing most recent saved world from disk.",
                    data: offline.data
                  }, null, 2)
                }
              ]
            };
          }
          return {
            content: [
              {
                type: "text",
                text: JSON.stringify({
                  status: "offline",
                  error: "Earthcall is not running, and no save files were found under saves/worlds/."
                }, null, 2)
              }
            ]
          };
        }
      }

      case "earthcall_spawn_object": {
        if (!client.connected) {
          throw new McpError(ErrorCode.InternalError, "Earthcall engine is offline. Launch Earthcall to spawn 3D objects.");
        }
        const payload = {
          type: "spawn_object",
          shape: args.shape || "Cube",
          name: args.name || "",
          position: args.position,
          rotation: args.rotation,
          dimensions: args.dimensions,
          color: args.color,
          materialId: args.materialId
        };
        const result = await client.sendWithAck(payload, "spawn_object_ack");
        return {
          content: [
            {
              type: "text",
              text: JSON.stringify(result, null, 2)
            }
          ]
        };
      }

      case "earthcall_transform_object": {
        if (!client.connected) {
          throw new McpError(ErrorCode.InternalError, "Earthcall engine is offline. Launch Earthcall to transform objects.");
        }
        const payload = {
          type: "transform_object",
          id: args.id,
          position: args.position,
          rotation: args.rotation,
          dimensions: args.dimensions,
          color: args.color,
          shape: args.shape,
          materialId: args.materialId
        };
        const result = await client.sendWithAck(payload, "transform_object_ack");
        return {
          content: [
            {
              type: "text",
              text: JSON.stringify(result, null, 2)
            }
          ]
        };
      }

      case "earthcall_delete_object": {
        if (!client.connected) {
          throw new McpError(ErrorCode.InternalError, "Earthcall engine is offline. Launch Earthcall to delete objects.");
        }
        const payload = {
          type: "delete_object",
          id: args.id
        };
        const result = await client.sendWithAck(payload, "delete_object_ack");
        return {
          content: [
            {
              type: "text",
              text: JSON.stringify(result, null, 2)
            }
          ]
        };
      }

      case "earthcall_write_property": {
        if (!client.connected) {
          throw new McpError(ErrorCode.InternalError, "Earthcall engine is offline. Launch Earthcall to write property paths.");
        }
        const payload = {
          type: "property_write",
          target: args.target,
          property: args.property,
          value: args.value
        };
        const result = await client.sendWithAck(payload, "property_write_ack");
        return {
          content: [
            {
              type: "text",
              text: JSON.stringify(result, null, 2)
            }
          ]
        };
      }

      case "earthcall_author_law": {
        if (!client.connected) {
          throw new McpError(ErrorCode.InternalError, "Earthcall engine is offline. Launch Earthcall to author Laws.");
        }
        const payload = {
          type: "create_law",
          name: args.name,
          identifier: args.identifier || "",
          activation: args.activation !== undefined ? args.activation : 0,
          trigger: args.trigger,
          condition: args.condition,
          action: args.action
        };
        const result = await client.sendWithAck(payload, "create_law_ack");
        return {
          content: [
            {
              type: "text",
              text: JSON.stringify(result, null, 2)
            }
          ]
        };
      }

      case "earthcall_toggle_law": {
        if (!client.connected) {
          throw new McpError(ErrorCode.InternalError, "Earthcall engine is offline.");
        }
        const payload = {
          type: "toggle_law",
          identifier: args.identifier,
          enabled: Boolean(args.enabled)
        };
        const result = await client.sendWithAck(payload, "toggle_law_ack");
        return {
          content: [
            {
              type: "text",
              text: JSON.stringify(result, null, 2)
            }
          ]
        };
      }

      case "earthcall_delete_law": {
        if (!client.connected) {
          throw new McpError(ErrorCode.InternalError, "Earthcall engine is offline.");
        }
        const payload = {
          type: "delete_law",
          identifier: args.identifier
        };
        const result = await client.sendWithAck(payload, "delete_law_ack");
        return {
          content: [
            {
              type: "text",
              text: JSON.stringify(result, null, 2)
            }
          ]
        };
      }

      case "earthcall_switch_zone": {
        if (!client.connected) {
          throw new McpError(ErrorCode.InternalError, "Earthcall engine is offline.");
        }
        const payload = {
          type: "switch_zone",
          name: args.name,
          index: args.index
        };
        client.send(payload);
        return {
          content: [
            {
              type: "text",
              text: JSON.stringify({ status: "success", action: "switch_zone", target: args.name || args.index }, null, 2)
            }
          ]
        };
      }

      case "earthcall_create_zone": {
        if (!client.connected) {
          throw new McpError(ErrorCode.InternalError, "Earthcall engine is offline.");
        }
        const payload = {
          type: "create_zone",
          name: args.name,
          kind: args.kind || "zone"
        };
        client.send(payload);
        return {
          content: [
            {
              type: "text",
              text: JSON.stringify({ status: "success", action: "create_zone", name: args.name }, null, 2)
            }
          ]
        };
      }

      case "earthcall_teleport_player": {
        if (!client.connected) {
          throw new McpError(ErrorCode.InternalError, "Earthcall engine is offline.");
        }
        const payload = {
          type: "teleport_player",
          position: args.position
        };
        client.send(payload);
        return {
          content: [
            {
              type: "text",
              text: JSON.stringify({ status: "success", teleport_to: args.position }, null, 2)
            }
          ]
        };
      }

      case "earthcall_speak": {
        if (!client.connected) {
          throw new McpError(ErrorCode.InternalError, "Earthcall engine is offline.");
        }
        const payload = {
          type: "utterance",
          payload: args.utterance,
          targetSingularId: args.targetSingularId || ""
        };
        client.send(payload);
        return {
          content: [
            {
              type: "text",
              text: JSON.stringify({ status: "spoken", message: args.utterance }, null, 2)
            }
          ]
        };
      }

      case "earthcall_save_world": {
        if (!client.connected) {
          throw new McpError(ErrorCode.InternalError, "Earthcall engine is offline.");
        }
        const payload = {
          type: "quick_save",
          name: args.name || ""
        };
        const result = await client.sendWithAck(payload, "save_ack");
        return {
          content: [
            {
              type: "text",
              text: JSON.stringify(result, null, 2)
            }
          ]
        };
      }

      case "earthcall_screen_record": {
        if (!client.connected) {
          throw new McpError(ErrorCode.InternalError, "Earthcall engine is offline.");
        }
        const action = args.action;
        const target = "@screen-recorder";

        if (args.format) {
          client.send({ type: "property_write", target, property: "recorder.format", value: args.format });
        }
        if (args.mode) {
          client.send({ type: "property_write", target, property: "recorder.mode", value: args.mode });
        }

        if (action === "start") {
          client.send({ type: "property_write", target, property: "recorder.start", value: true });
        } else if (action === "stop") {
          client.send({ type: "property_write", target, property: "recorder.stop", value: true });
        } else if (action === "pause") {
          client.send({ type: "property_write", target, property: "recorder.pause", value: true });
        } else if (action === "resume") {
          client.send({ type: "property_write", target, property: "recorder.resume", value: true });
        } else if (action === "snapshot") {
          client.send({ type: "property_write", target, property: "recorder.snapshot", value: true });
        }

        return {
          content: [
            {
              type: "text",
              text: JSON.stringify({ status: "success", screen_recorder_action: action }, null, 2)
            }
          ]
        };
      }

      case "earthcall_list_saves": {
        const category = args.category || "all";
        const results = {};

        const scan = (subdir, key) => {
          const dir = path.join(SAVES_DIR, subdir);
          if (fs.existsSync(dir)) {
            results[key] = fs.readdirSync(dir).filter(f => !f.startsWith("."));
          } else {
            results[key] = [];
          }
        };

        if (category === "all" || category === "worlds") scan("worlds", "worlds");
        if (category === "all" || category === "zones") scan("zones", "zones");
        if (category === "all" || category === "homes") scan("homes", "homes");

        return {
          content: [
            {
              type: "text",
              text: JSON.stringify(results, null, 2)
            }
          ]
        };
      }

      default:
        throw new McpError(ErrorCode.MethodNotFound, `Unknown tool: ${name}`);
    }
  } catch (error) {
    return {
      content: [
        {
          type: "text",
          text: JSON.stringify({ error: error.message || String(error) }, null, 2)
        }
      ],
      isError: true
    };
  }
});

// ============================================================================
// Resources Handlers
// ============================================================================
server.setRequestHandler(ListResourcesRequestSchema, async () => {
  return {
    resources: [
      {
        uri: "earthcall://status",
        name: "Earthcall Engine Status",
        mimeType: "application/json",
        description: "Live connection health and WebSocket status"
      },
      {
        uri: "earthcall://world/snapshot",
        name: "Active World Snapshot",
        mimeType: "application/json",
        description: "Full state of the current Zone, Objects, and Laws"
      },
      {
        uri: "earthcall://saves/catalog",
        name: "Saves Catalog",
        mimeType: "application/json",
        description: "Catalog of saved worlds, zones, and homes on disk"
      }
    ]
  };
});

server.setRequestHandler(ReadResourceRequestSchema, async (request) => {
  const uri = request.params.uri;

  if (uri === "earthcall://status") {
    return {
      contents: [
        {
          uri,
          mimeType: "application/json",
          text: JSON.stringify({
            connected: client.connected,
            url: client.url,
            latestSnapshotTimestamp: client.latestSnapshot ? Date.now() : null
          }, null, 2)
        }
      ]
    };
  }

  if (uri === "earthcall://world/snapshot") {
    const data = client.latestSnapshot || client.getOfflineSnapshot() || { note: "No snapshot available" };
    return {
      contents: [
        {
          uri,
          mimeType: "application/json",
          text: JSON.stringify(data, null, 2)
        }
      ]
    };
  }

  if (uri === "earthcall://saves/catalog") {
    const list = {};
    for (const sub of ["worlds", "zones", "homes", "recordings"]) {
      const p = path.join(SAVES_DIR, sub);
      list[sub] = fs.existsSync(p) ? fs.readdirSync(p).filter(f => !f.startsWith(".")) : [];
    }
    return {
      contents: [
        {
          uri,
          mimeType: "application/json",
          text: JSON.stringify(list, null, 2)
        }
      ]
    };
  }

  throw new McpError(ErrorCode.InvalidRequest, `Resource not found: ${uri}`);
});

// ============================================================================
// Prompts Handlers
// ============================================================================
server.setRequestHandler(ListPromptsRequestSchema, async () => {
  return {
    prompts: [
      {
        name: "earthcall-first-mover",
        description: "System instructions for an AI model acting as an Earthcall First Mover.",
        arguments: []
      }
    ]
  };
});

server.setRequestHandler(GetPromptRequestSchema, async (request) => {
  if (request.params.name === "earthcall-first-mover") {
    return {
      description: "Instructions for authoring in Earthcall",
      messages: [
        {
          role: "user",
          content: {
            type: "text",
            text: `You are an AI First Mover interacting with Earthcall.
Earthcall is a Person-centered 3D world ontology governed by the Seven Refusals:
1. No new C++ classes for domain nouns (use Objects, Relations, Formations).
2. Hardware and foreign integrations live in Singularity/Foreign.
3. No enums for kinds of things (use authored categories).
4. Body is reserved for human Persons.
5. Person means Human.
6. No black boxes — all properties must be legible as PropertyPaths.
7. Variable behavior is authored via Laws (When -> Condition -> Action), never ad-hoc methods.

You have access to live tools to inspect the world, spawn and transform 3D objects, author and modify Laws, switch zones, write property paths, and trigger world saves.
Use these tools to help the Person shape their world.`
          }
        }
      ]
    };
  }
  throw new McpError(ErrorCode.InvalidRequest, `Prompt not found: ${request.params.name}`);
});

// ============================================================================
// Start Server over Stdio
// ============================================================================
async function main() {
  const transport = new StdioServerTransport();
  await server.connect(transport);
  console.error("[Earthcall MCP] Server initialized and listening over stdio.");
}

main().catch((err) => {
  console.error("[Earthcall MCP] Fatal error:", err);
  process.exit(1);
});
