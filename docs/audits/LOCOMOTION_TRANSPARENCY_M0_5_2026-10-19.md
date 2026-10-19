# Locomotion transparency audit (movement rung M0.5)

**Commissioned by:** Zachary Zhang ("YAAASSSSSS", approving M0 → M0.5 → value-branch segmentation).
**Asked for by:** the Original Prism Sun's revised movement ladder, §8 of its [locomotion decree](../../agent%20intercom/communication-threads/sdf-and-rendering/PRISM_PROPHECY_MOVEMENT_DIRTY_RAY_SENSITIVITY_ARCHITECTURE_2026-10-19.md) (its reply "locomotion is a black-box seam; path AND envelope"): "Enumerate every pose writer; classify sensing vs variable movement meaning vs irreducible actuation; identify direct setters bypassing notification."
**Method:** source reading on `claude/ray-domain-transport-rung0` at `f0bebdefa`. Every write to `Camera::pos/front/up/speed`, `Person::position/velocity/cameraPos/cameraForward` in `src/` was found by pattern search and read. **Nothing here was run.** No code was changed.

## 1. Where the observer's pose lives

- **`Core::Camera`** (`src/Singularity/Screen/Camera.hpp:12`) holds `pos`, `front`, `up` and `speed`. It is what the renderer reads (`EngineRender.cpp:77–94`). It is not a Singular and registers no properties.
- **`Person`** registers `position`, `velocity`, `cameraPos` and `cameraForward` as property paths (`Person.cpp:32–43`), so Laws can *read* the pose.
- **The bridge is a per-frame copy:** `Engine.cpp:381–382` copies `Camera::pos/front` into `Person::cameraPos/cameraForward` every frame. `LocomotionChannel.cpp:339` sets `person.position()` from `camera.pos`.
- **Consequences:**
  - `camera.up` and `camera.speed` are unreachable by any Law (Refusal 6: unregistered, not named as beneath the Kernel).
  - `player.cameraPos` reads correctly, but a write to it is overwritten by the next frame's copy. Whether the Kernel guard already refuses that write was not checked.
  - The Camera holds the authoritative pose, and the Person holds a mirror of it. For a Person-centred ontology that is backwards; it is recorded here, not decided.

## 2. Every pose writer, classified

Classes, per Prism §2: **S** irreducible sensing · **K** Kernel safety invariant · **P** variable movement policy or mathematics (the migration target) · **A** irreducible actuation · **T** teleport-like placement.

| Where | What it does | Class |
|---|---|---|
| `LocomotionChannel.cpp:229–246`, `:287` | reads W/A/S/D, V (sprint), Left Alt (slow) and Space via `glfwGetKey` | S |
| `MouseHandler.cpp:81–87`; `EngineUpdate.cpp:173` | reads mouse delta into yaw and pitch, then overwrites `camera.front` | S (the read) · P (sensitivity, pitch limits, look mapping) · A (the write) |
| `EngineUpdate.cpp:180` | `canMove`: cursor locked, menu closed, no text field active | S |
| `LocomotionChannel.cpp:229–231` | speed `camera.speed` (0.1, `Camera.hpp:18`), ×2.5 sprint, ×0.3 slow | P |
| `LocomotionChannel.cpp:246` | horizontal step `normalize(move) · speed` per frame, **no `dt`** | P (and the frame-rate dependence is a policy nobody chose) |
| `LocomotionChannel.cpp:250–265` | collision probe: radius 0.3, five offsets at two heights, horizontal push from `Physics::enforceCollisions` | P (radius and probe shape) · K? (non-penetration, if Zach makes it one) |
| `LocomotionChannel.cpp:268–286` | support height from collision boxes, stand tolerance 0.05 | P |
| `LocomotionChannel.cpp:287–314` | flying vertical step; gravity 9.81 and jump speed 5.0 (`:296–297`), with `dt`; floor clamp | P · A |
| `LocomotionChannel.cpp:216–224` | latch: if `person.position` ≠ `camera.pos − eye height`, take the Person's position (detects a teleport) | A (and the only teleport *edge* in the code) |
| `LocomotionChannel.cpp:339` | `person.position() = camera.pos − eyeH` | A |
| `LocomotionChannel.cpp:62–110, 162` | walk and idle sway clips on BodyParts, tempo from speed | already authored Automation (data), driven by P |
| `LocomotionChannel.cpp:330–337` | publishes `locomotion-started` / `locomotion-stopped` on transitions, and `LocomotionChanged` | edges, correctly (non-negotiable: events are edges) |
| `Engine.cpp:381–382` | Camera → Person mirror each frame | A (bookkeeping) |
| `ZoneManager.cpp:997–1003` `settlePersonToCamera` | sets Person position, mirror and zero velocity from the camera | T |
| `ZoneManager.cpp:1007–1013` `applyLook` | sets camera position and front (observations, tests) | T |
| `ZoneManager.cpp:2241–2254`, `:2769–2773` | load camera pos/front/up from a save | T |
| `PersonSerialization.cpp:56–59` | load Person position and velocity | T |
| `EngineInit.cpp:706` | spawn anchor | T |
| `WebSocketServer.cpp:1613–1617` | foreign `teleport_player` is refused: "a Person's body is guarded in C++; no foreign First Mover may move it" | K |
| `Law.cpp:32–79`, `:473–572` `writesPersonsMotionWithoutActuationConsent` | refuses every positive Law write to a Person's body or location; the comment at `:54` says no actuation-consent evidence exists yet | K |

## 3. Notification and revision

- **Path writes notify.** `PropertyRef::set` calls `Singular::notifyPropertyChanged` when the value changes (`PropertyRef.hpp:50–62`), and path writes answer `Unchanged` upstream.
- **Locomotion does not use them.** Every write in the table goes through the reference accessors (`person.position() = …`, `person.velocity().y = …`) or raw `Camera` members. No notification fires for any per-frame pose change. This is the boundary PROPHETIC_RETE §6 already names ("a direct C++ setter … writes the transform without going through the property vocabulary").
- **No pose revision exists.** Prism's §4 asks for one authoritative source before any counter is added. The candidate already in the code is the property notification itself. If locomotion actuated through `PropertyRef::set` (or a path write), `notifyPropertyChanged(person, "position")` would be the pose's edge, and a revision could hang from the ledger's existing property dependency. This is a candidate, not a decision.

## 4. What the migration target is, concretely

The **P** rows are the whole of Person movement meaning today: per-frame speed and its two multipliers; the dt-free horizontal step; probe shape and push; support tolerance; flying step; gravity and jump constants; and mouse sensitivity and look limits. About a dozen numbers and four small equations. All of it is OntoMath-expressible (piecewise-linear and quadratic in t), so the conditional path Prism describes (q_P(t) while premises hold) exists in closed form for walking, jumping and falling between collision events.

## 5. Decisions only Zach can make (⚑)

1. **⚑ Self-actuation authority.** The Kernel guard refuses every Law write to a Person's motion. If the P rows become Law, the Person's own key press must carry the authority to move, or the guard refuses Zach's walking. Two shapes I can see: a standing, revocable self-consent sub-Relation; or the Input channel's First-Mover act carrying the authority for that one firing. Either keeps the guard closed to every *other* author.
2. **⚑ Frame-rate dependence.** Walking speed today scales with frame rate (`:246`) while falling does not. Intended, or should the step be `speed · dt` (and if so, what speed in units per second)?
3. **⚑ Where the pose belongs.** Camera authoritative with a Person mirror (today), or Person (or the Person's Perspective) authoritative and the Camera derived?
4. **⚑ Collision non-penetration**: Kernel invariant or variable policy? Today it is a toggle (`Physics::isCollisionEnabled`).

## 6. What follows

- **M1** (Prism): one Law-visible premise, forward while W is held, published as shared movement math that actuation and the renderer both consume. It cannot start before decision 1.
- **Cheap and decision-free:** routing the existing per-frame pose writes through `PropertyRef::set` would give notification and an edge, with no change to meaning. Not done here: it touches the Kernel boundary file set, and Prism asked for the audit before implementation.
- Ray transport continues meanwhile: M0 needed none of this (plan §6s), and value-branch segmentation is next.

---

*Claude Code · Claude Opus 5.5 · session `f3af84cf-bfe3-429d-b417-2d533119f169` · 2026-10-19T11:57 PDT. Human origin: Zachary Zhang (the movement question; approval of M0.5). The classification scheme and the request are the Original Prism Sun's; the source trace and the notification candidate are this session's.*
