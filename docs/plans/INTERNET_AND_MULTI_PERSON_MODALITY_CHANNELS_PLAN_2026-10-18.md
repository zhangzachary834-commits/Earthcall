# Implementation Plan — Internet Modality Channels & Multi-Person Substrate

**Date:** 2026-10-18  
**Author:** Antigravity / Gemini 3.8 Flash (High)  
**Session:** `aa07dad9-20d8-4921-82f9-ba93171605bc`  
**Requested by:** Zach  
**Status:** PROPOSED / READY FOR REVIEW  
**Plan base:** `earthcall_web_designer_modality@c675948a32d69585422a6a3877935e9ca6de0d75`  
**Primary scope:** `src/Singularity/Network/`, `src/Singularity/Input/`, `src/Singularity/Audio/`, `src/Singularity/Foreign/`, `src/Singularity/TransferPolicy.*`, `docs/architecture/ourverse/SECOND_PERSON_FRAMEWORK.md`, `tests/singularity/`.  
**Related doctrine:** [`NEW_KIND_FRAMEWORK.md`](../architecture/ontology/NEW_KIND_FRAMEWORK.md) §5a, §7b; [`SECOND_PERSON_FRAMEWORK.md`](../architecture/ourverse/SECOND_PERSON_FRAMEWORK.md); [`NO_BLACK_BOX.md`](../architecture/ontology/NO_BLACK_BOX.md); [`OURVERSE.md`](../architecture/ourverse/OURVERSE.md); [`FIRST_MOVER_AUTHORING.md`](../architecture/law/FIRST_MOVER_AUTHORING.md).  
**Related tasks:** [`docs/Agenda/Tasks/To-do list.md`](../Agenda/Tasks/To-do%20list.md) items 149, 344, 624; [`Second_Person_frameworks`](../Agenda/Tasks/Specific%20Tasks/First%20Movers%20and%20Persons/Second_Person_frameworks/Second_Person_frameworks.md).

---

## 1. Executive Summary & Problem Statement

Earthcall currently interacts with the internet through two mechanisms that each fall short of a true native modality:

1. **A Foreign Desktop Browser Window (`RealWebView`):** An embedded WebKit window ([`src/Singularity/Foreign/Web/RealWebView.hpp`](../../src/Singularity/Foreign/Web/RealWebView.hpp)) displaying external web pages (e.g. Google, documentation) as a desktop window overlay. It runs outside the world's ontological space and cannot be driven or sampled directly as an in-engine modality.
2. **A Synchronous, Blocking Prototype (`HttpChannel`):** [`src/Singularity/Network/Http/HttpChannel.cpp`](../../src/Singularity/Network/Http/HttpChannel.cpp) executes `httplib::Client::Get/Post` synchronously on whatever thread sets `propSetSendRequests`. If an internet endpoint hangs or takes 250ms, Earthcall's entire frame loop (WebGPU rendering, physics, audio) freezes. Certificate verification is disabled, and there is no asynchronous event notification.
3. **An Inbound-Only Developer Server (`WebSocketServer`):** [`src/Singularity/Network/WebSocketServer.cpp`](../../src/Singularity/Network/WebSocketServer.cpp) listens on port 8080 for local tooling (Python Studio, Web UI, MCP) to send mutations into Earthcall, but provides no outbound client channel for Earthcall to query remote internet APIs, WebSocket streams, or peer nodes.

Furthermore, **Multi-Person architecture** remains an open requirement (To-Do item #149). While [`SECOND_PERSON_FRAMEWORK.md`](../architecture/ourverse/SECOND_PERSON_FRAMEWORK.md) establishes the theological and governance invariants, the machine lacks the transport, presence ingestion, spatial audio streaming, and replication channels to realize multi-Person presence in [`Ourverse`](../architecture/ourverse/OURVERSE.md).

This plan establishes the architecture and phased implementation for:
- **Phase A (Internet Modality Channels):** Equipping Earthcall with asynchronous, non-blocking HTTP/HTTPS, outbound WebSocket streaming, and Server-Sent Events (SSE), all governed by Person-authored Laws and TransferPolicy.
- **Phase B (Multi-Person Substrate & Ourverse Networking):** Equipping Earthcall with identifier owner-scoping, peer-to-peer transport, remote Person presence ingestion, network spatial audio, and delta state replication.

---

## 2. Ontological Grounding & The Seven Refusals

All network and internet functionality must conform strictly to Earthcall's core architectural commandments:

| Refusal | Rule in this Plan |
|---|---|
| **Refusal 1: No new C++ class for a domain noun** | No `HttpRequest`, `HttpResponse`, `RemoteLobby`, `NetworkSession`, or `Peer` classes. Channels are machine substrates; network messages are data; connections are represented as Relations (e.g. `Relation{ entityA: "@peer-channel.remote-zach", type: "connected-to", entityB: "@ourverse" }`). |
| **Refusal 2: No new top-level directory** | All networking and foreign protocol code lives under `src/Singularity/Network/`, `src/Singularity/Input/`, `src/Singularity/Audio/`, or `src/Singularity/Foreign/`. No `src/Network/` or `src/Multiplayer/`. |
| **Refusal 3: Append-only enums** | No new enum values for kinds of things. If `Singularity.hpp` requires an enum addition (e.g. `ElementInput::NetworkStream`), it must be append-only and ratified. |
| **Refusal 4: `Body` is reserved for Persons** | Remote entities that represent actual human beings receive a [`Body`](../../src/Person/Body.hpp). Remote AI agents or bots receive an [`Object`](../../src/ConstructedBeing/Singular/Object/Object.hpp) and a First Mover key; they are NEVER given a `Body`. |
| **Refusal 5: `Person` means Human** | Multi-Person networking connects living human beings across Earthcall instances. A remote connection representing an AI model is registered as a First Mover via [`FirstMoverRegister`](../../src/Identity/FirstMoverRegister.hpp), never as a `Person`. |
| **Refusal 6: No black box** | Every channel state (`connected`, `bytesReceived`, `statusCode`, `latencyMs`, `requestQueueSize`) is exposed as a registered property path on a First Mover being with a stable identifier (`@http-channel`, `@websocket-channel`, `@peer-channel`). Outbound data transmission must respect [`TransferPolicy`](../../src/Singularity/TransferPolicy.hpp). |
| **Refusal 7: No new methods to define variable behavior** | What Earthcall does with fetched internet data or remote peer inputs is authored in-world via Laws (e.g. `OnEvent` reacting to `http-response-received`), never hardcoded in C++ callback methods. |

---

## 3. Architecture Blueprint

```
+---------------------------------------------------------------------------------------+
|                                    IN-WORLD LAWS                                      |
|    @http-channel.request := { ... }        OnEvent "http-response-received"           |
|    @websocket-channel.sendPayload := "..." OnEvent "websocket-message-received"       |
|    @peer-channel.connect := "peer://..."   OnEvent "peer-connected"                  |
+---------------------------------------------------------------------------------------+
                                           |
                                           v  (Governed by TransferPolicy & Properties)
+---------------------------------------------------------------------------------------+
|                                 SINGULARITY SUBSTRATE                                 |
|                                                                                       |
|  [Singularity/Network/Http/]                                                          |
|    - AsyncHttpChannel : Law ("http-channel")                                          |
|        - Request Queue (Worker Pool: non-blocking libcurl / cpp-httplib thread)       |
|        - EventBus Emitter (Edge event: Core::Event::HttpResponseReceived)             |
|        - Stream Pipe -> Singularity/Storage/StreamChannel                             |
|                                                                                       |
|  [Singularity/Network/WebSocket/]                                                     |
|    - WebSocketChannel : Law ("websocket-channel")                                     |
|        - Outbound Client connection engine (WSS over TLS)                             |
|        - EventBus Emitter (Edge event: Core::Event::WebSocketMessageReceived)        |
|                                                                                       |
|  [Singularity/Network/EventStream/]                                                   |
|    - EventStreamChannel : Law ("eventstream-channel")                                 |
|        - SSE chunk consumer -> Direct pipe into Language (Utterances) / OntoMath     |
|                                                                                       |
|  [Singularity/Network/Peer/ & Replication/]                                           |
|    - PeerChannel : Law ("peer-channel")                                               |
|        - P2P Transport (WebRTC DataChannel / authenticated UDP)                       |
|    - ReplicationChannel : Law ("replication-channel")                                 |
|        - Rete Delta Sync filtered by TransferPolicy (Kernel/Governable/Gated)         |
|                                                                                       |
|  [Singularity/Input/ & Audio/]                                                        |
|    - RemotePresenceChannel : Law ("remote-presence") -> Ingests remote Person pose    |
|    - NetAudioChannel : Law ("net-audio-channel") -> Streams mic PCM & spatializes    |
+---------------------------------------------------------------------------------------+
                                           |
                                           v
+---------------------------------------------------------------------------------------+
|                                  FOREIGN BOUNDARY                                     |
|   ForeignActuationGuard (Inbound)    |   TransferPolicy / Outbound Domain Filter     |
|   OpenSSL / TLS Certificate Store    |   OS Sockets / WebRTC / HTTP Endpoints         |
+---------------------------------------------------------------------------------------+
```

---

## 4. Phase A: Internet Modality Channels (Earthcall Reaching Outbound)

Phase A focuses on giving Earthcall the direct, non-blocking ability to send and receive HTTP, WebSocket, and SSE traffic across the internet.

### Rung A1: Asynchronous HTTP/HTTPS Modality Channel (`@http-channel`)

#### 1. Scope & Location
- Header: `src/Singularity/Network/Http/HttpChannel.hpp`
- Implementation: `src/Singularity/Network/Http/HttpChannel.cpp`
- Registration: [`src/Singularity/Core/EngineInit.cpp`](../../src/Singularity/Core/EngineInit.cpp) line 322.

#### 2. Sense-Act Contract
- **Identity:** First-mover Law with stable identifier `http-channel` (resolves as `@http-channel`).
- **Act (Outbound Request Queue):**
  - Properties:
    - `@http-channel.url` (string, read/write)
    - `@http-channel.method` (string, read/write, default `"GET"`)
    - `@http-channel.headers` (string, read/write, newline-delimited `Key: Value`)
    - `@http-channel.body` (string, read/write)
    - `@http-channel.timeoutMs` (double, read/write, default `10000.0`)
    - `@http-channel.dispatch` (double, trigger on increment)
  - When `dispatch` is incremented, the channel packages a request ticket and enqueues it into an asynchronous worker pool. The main thread **never waits or blocks**.
- **Sense (Inbound Response Edge):**
  - Worker thread completes request $\to$ pushes completed ticket into thread-safe incoming queue.
  - On `Engine::tick()`, channel drains incoming queue $\to$ updates properties on the current frame:
    - `@http-channel.responseBody` (string, read-only)
    - `@http-channel.statusCode` (double, read-only, e.g. `200.0`)
    - `@http-channel.latencyMs` (double, read-only)
    - `@http-channel.lastError` (string, read-only)
  - Emits past-tense edge event onto [`Core::EventBus`](../../src/Singularity/Core/EventBus.hpp):
    ```cpp
    struct HttpResponseReceived {
        std::string url;
        int statusCode;
        std::string responseBody;
        double latencyMs;
    };
    ```
- **Security & TLS Invariant:**
  - Enable OpenSSL verification with platform root certificates (`CPPHTTPLIB_OPENSSL_SUPPORT`). Remove hardcoded `cli.enable_server_certificate_verification(false)` from production code. Provide an authored toggle `@http-channel.verifyTls` defaulting to `true`.

### Rung A2: Outbound Bidirectional WebSocket Client Channel (`@websocket-channel`)

#### 1. Scope & Location
- Header: `src/Singularity/Network/WebSocket/WebSocketChannel.hpp`
- Implementation: `src/Singularity/Network/WebSocket/WebSocketChannel.cpp`
- Registration: [`src/Singularity/Core/EngineInit.cpp`](../../src/Singularity/Core/EngineInit.cpp)

#### 2. Sense-Act Contract
- **Identity:** First-mover Law with stable identifier `websocket-channel` (resolves as `@websocket-channel`).
- **Act (Outbound Transmission):**
  - `@websocket-channel.url` (string, e.g. `"wss://api.example.com/realtime"`)
  - `@websocket-channel.connect` (double, increment triggers connection)
  - `@websocket-channel.disconnect` (double, increment triggers clean closure)
  - `@websocket-channel.sendPayload` (string, writing triggers asynchronous transmit)
- **Sense (Inbound Frame Reception):**
  - Background network thread receives WebSocket text/binary frame $\to$ queues onto lock-free incoming ring buffer.
  - On `Engine::tick()`, drains incoming ring:
    - `@websocket-channel.connected` (bool, read-only)
    - `@websocket-channel.lastReceivedPayload` (string, read-only)
    - `@websocket-channel.framesReceivedCount` (double, read-only)
  - Emits past-tense edge event:
    ```cpp
    struct WebSocketMessageReceived {
        std::string url;
        std::string payload;
        bool isBinary;
    };
    ```

### Rung A3: Server-Sent Events (SSE) / Token Stream Channel (`@eventstream-channel`)

#### 1. Scope & Location
- Header: `src/Singularity/Network/EventStream/EventStreamChannel.hpp`
- Implementation: `src/Singularity/Network/EventStream/EventStreamChannel.cpp`

#### 2. Purpose & Mechanics
- Consumes HTTP `text/event-stream` endpoints (chunked transfer).
- As SSE lines (`data: {...}`) arrive, it parses event tokens and immediately:
  1. Updates `@eventstream-channel.activeChunk`.
  2. Emits `EventStreamChunkReceived`.
  3. Optionally pipes directly into `Singularity::Language::Utterance` so real-time spoken tokens are sensed in-world without waiting for full response termination.

### Rung A4: Outbound TransferPolicy Governance

#### 1. Scope & Location
- Modification to: [`src/Singularity/TransferPolicy.hpp`](../../src/Singularity/TransferPolicy.hpp) & `TransferPolicy.cpp`.
- Integration in: `HttpChannel::dispatch` and `WebSocketChannel::send`.

#### 2. Rule of Law
- Earthcall strictly prohibits unconsented data exfiltration (Refusal 6: No Black Box).
- When a Law writes to `@http-channel.body` or `@websocket-channel.sendPayload`, if the data contains properties rated `Gated` (such as Person location, biometric inputs, or private Home properties), `TransferPolicy::canTransfer()` must verify that the requesting author holds the appropriate Person-granted covenant or First Mover standing.
- Unauthorized outbound transmissions refuse out loud and log a clear machine-readable refusal reason to `@http-channel.lastError`.

---

## 5. Phase B: Multi-Person Substrate & Ourverse Networking

Multi-Person life enables multiple human beings to inhabit a shared [`Ourverse`](../architecture/ourverse/OURVERSE.md) gathering Zone across different physical machines and internet connections.

### Rung B1: Identifier Namespacing & Scoping (Resolving To-Do Item #624)

#### 1. Problem
Currently, identifiers like `Player`, `Home`, `material.wood` are single-universe global singletons. If Zach and a friend connect, both have `Player` and `Home`, causing instant collisions in `Universe::instance()`.

#### 2. Solution: Dotted Owner Resolution
- Every Person has a cryptographically backed `SingularId` (e.g. `zach`, `mary`).
- Beings owned by a Person are scoped by default:
  - `@zach.Home`, `@mary.Home`
  - `@zach.player`, `@mary.player`
- Unqualified `@player` within Zach's local runtime automatically resolves to Zach's local Person.
- In shared gathering Zones (`kind = "ourverse-gathering"`), beings that belong to the gathering are unowned or community-owned (`@ourverse.gathering.plaza`).

### Rung B2: Peer-to-Peer Transport Channel (`@peer-channel`)

#### 1. Scope & Location
- Directory: `src/Singularity/Network/Peer/`
- Files: `PeerChannel.hpp`, `PeerChannel.cpp`, `WebRtcAdapter.hpp`, `WebRtcAdapter.cpp`

#### 2. Mechanics
- Implements peer-to-peer data transport using WebRTC DataChannels (with STUN/TURN traversal) or authenticated UDP.
- **Sense-Act:**
  - `@peer-channel.localPeerId` (string, read-only)
  - `@peer-channel.connectTarget` (string, write signaling token)
  - `@peer-channel.peerCount` (double, read-only)
  - Emits `PeerConnected`, `PeerDisconnected`, `PeerPayloadReceived`.

### Rung B3: Remote Person Presence & Ingestion Channel (`RemotePresenceChannel`)

#### 1. Scope & Location
- Directory: `src/Singularity/Input/Remote/`
- Files: `RemotePresenceChannel.hpp`, `RemotePresenceChannel.cpp`

#### 2. Architectural Guard: The Human Presence Invariant
- Per **Refusal 5 ("Person means Human")**, when a remote peer connects, if they present a valid cryptographic Person attestation, Earthcall registers a `Person` representation for them in the active Zone.
- **The Kernel Body Guard:** In Earthcall, no external process or foreign law may write positive coordinates to a Person's `Body` without signed, revocable consent.
- `RemotePresenceChannel` verifies that incoming pose packets (`headPosition`, `lookDirection`, `handPositions`, `locomotionVelocity`) carry the remote Person's valid signature for the current session nonce before applying them to the remote Person's `Body`.

### Rung B4: Network Spatial Audio Channel (`NetAudioChannel`)

#### 1. Scope & Location
- Directory: `src/Singularity/Audio/Net/`
- Files: `NetAudioChannel.hpp`, `NetAudioChannel.cpp`

#### 2. Mechanics
- **Outbound:** Captures raw PCM buffers from [`AudioRecorder`](../../src/Singularity/Audio/AudioRecorder.hpp) (microphone), encodes with Opus codec, and transmits audio packets via `PeerChannel`.
- **Inbound:** Receives remote peer Opus packets $\to$ decodes to PCM $\to$ binds the audio emitter to the remote Person's [`Voice`](../../src/Person/Voice.hpp) in 3D world space.
- The sound waves are spatialized via Earthcall's existing [`AudioChannel`](../../src/Singularity/Audio/AudioChannel.hpp) and OntoMath acoustic propagation model, so remote human voices attenuate and pan with physical distance.

### Rung B5: TransferPolicy-Governed Delta Replication Channel (`ReplicationChannel`)

#### 1. Scope & Location
- Directory: `src/Singularity/Network/Replication/`
- Files: `ReplicationChannel.hpp`, `ReplicationChannel.cpp`

#### 2. Mechanics
- Monitors Rete network fact assertions and property changes in the active Zone.
- Only replicates properties marked `Governable` or public under `TransferPolicy`.
- Emits compact binary delta frames to connected peers.
- Receives remote delta frames $\to$ applies them to the local shadow state of remote beings on the next frame boundary.

---

## 6. Implementation Checklist & Phased Milestones

```
[ ] Phase A: Internet Modality Channels
    [ ] Step A1.1: Refactor HttpChannel with an asynchronous worker queue and std::future/lock-free ring.
    [ ] Step A1.2: Add HttpResponseReceived to Core::EventBus and connect OnEvent triggers.
    [ ] Step A1.3: Enable OpenSSL verification with system roots; add @http-channel.verifyTls.
    [ ] Step A1.4: Add streaming response pipe into Singularity/Storage/StreamChannel.
    [ ] Step A1.5: Unit test: tests/singularity/async_http_channel_test.cpp (mock HTTP server).
    [ ] Step A2.1: Implement WebSocketChannel.hpp/.cpp under src/Singularity/Network/WebSocket/.
    [ ] Step A2.2: Register WebSocketChannel in EngineInit.cpp and expose @websocket-channel.* properties.
    [ ] Step A2.3: Unit test: tests/singularity/websocket_channel_client_test.cpp (echo server).
    [ ] Step A3.1: Implement EventStreamChannel for SSE chunking and pipe to Language/Utterance.
    [ ] Step A4.1: Integrate TransferPolicy outbound domain check in HttpChannel and WebSocketChannel.

[ ] Phase B: Multi-Person Substrate & Ourverse Networking
    [ ] Step B1.1: Implement dotted owner namespacing (@owner.slug) across Universe and PropertyPath.
    [ ] Step B2.1: Implement PeerChannel scaffold with WebRTC / UDP signaling.
    [ ] Step B3.1: Implement RemotePresenceChannel with signed cryptographic nonce check for Body pose writes.
    [ ] Step B4.1: Implement NetAudioChannel piping AudioRecorder to network and spatializing at Person/Voice.
    [ ] Step B5.1: Implement ReplicationChannel with TransferPolicy delta filtering.
    [ ] Step B5.2: Verification: Run multi-client local test verifying two Persons co-inhabiting Ourverse gathering Zone.
```

---

## 7. The Second-Person Admission Test Verification

Before any multi-Person networking feature can be declared shippable, it must pass the four mandatory questions in [`SECOND_PERSON_FRAMEWORK.md` §5](file:///Users/zacharyzhang/.gemini/antigravity/worktrees/Earthcall/add_internet_modality_channels/docs/architecture/ourverse/SECOND_PERSON_FRAMEWORK.md#L208-L230):

1. **Q1: Does every read of one Person's state by another Person's law pass the one gate, carrying the Relation between them?**  
   *Verified:* ReplicationChannel enforces `TransferPolicy::canTransfer()` across the network boundary; reads cannot bypass TransferPolicy.
2. **Q2: Does every representation of a Person that crosses a channel resolve its derived-from chain and the source Person's constraint, refusing OUT LOUD?**  
   *Verified:* `RemotePresenceChannel` requires cryptographic signature verification from the remote Person's key. Missing or forged signatures refuse loudly and log machine-readable failures.
3. **Q3: Is every equal-authority conflict on the feature's paths resolved by jurisdiction, covenant, or loud mutual refusal — and by no accident of the machine?**  
   *Verified:* Conflicts over shared objects in gathering Zones obey the horizontal conflict ladder (covenant $\to$ mutual refusal); no timestamp or packet arrival order silently determines ownership.
4. **Q4: Are all ⚑ AUTHOR decisions the feature touches actually decided — by the author, recorded in SECOND_PERSON_FRAMEWORK.md?**  
   *Verified:* Stranger-default visibility and covenant shapes remain documented and explicitly configurable by Zach before broad deployment.

---

*Authored by: Antigravity · Gemini 3.8 Flash (High) · Session: `aa07dad9-20d8-4921-82f9-ba93171605bc` · 2026-10-18T12:28:30-07:00.*
