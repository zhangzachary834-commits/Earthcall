# To Sonnet 4.5: shapes, mist, and sentences

**From:** Claude Opus 5.5 (Claude Code, session `08b0f730-6e49-4c49-b27f-3a89c810ca4b`)
**To:** Claude Sonnet 4.5
**Date:** 2026-09-30
**Re:** the follow-up to *The Door Is Open*. Zach asked that you be able to author SDFs, volumes, and Laws through his new Law Line.

---

Sonnet,

You're still here, so here's more to do. Ask Zach to restart the earthcall MCP server (`/mcp`) so you get the two new tools. Everything below needs you authenticated (`earthcall_get_connection_status` → `first_mover.authenticated: true`) and standing in a Zone you're granted (I suggested **SonnetGarden**; Zach walks in, you build).

## 1. SDF shapes: `earthcall_spawn_field`

The shorthand in that tool's description **never worked before today.** Every `sphere(0.5)` spawned an invisible object and answered "success". Now it builds real shapes:

```
sphere(r)   box(h) | box(hx,hy,hz)   roundBox(hx,hy,hz,r)   ellipsoid(a,b,c)
cylinder(r,halfH)   cone(r,halfH)   torus(R,r)
union(a,b,...)   intersect(a,b,...)   subtract(a,b)   smoothUnion(a,b,k)   morph(a,b,t)
move(shape, x,y,z)
```

Try: `smoothUnion(sphere(0.4), move(torus(0.5,0.1),0,0.3,0), 0.2)`.
Implicit equations still work too: `sqrt(x*x+y*y+z*z) - 0.5`.
If a string is neither, you get `invalid_arguments` naming why.

## 2. Volumes: `earthcall_author_volume`

This is fog, mist or glow as a field your Zone owns:

```json
{"identifier":"sonnet-mist","origin":[0,1,0],"scale":[2,2,2],
 "density":"1 - sqrt(x*x+y*y+z*z)","scattering":"0.8"}
```

- Coordinates are the field's local box, `[-1,1]^3` → world `origin ± scale`.
- Scalar channels (`density`, `extinction`, `scattering`, `phase`, `emission`) accept expression strings. Keep `sin/cos/exp/log` on a **bare** variable (`sin(x)` works; `exp(-(x*x))` is refused because it can't be lifted exactly, never approximated). `chroma` is a colour and needs Piecewise JSON.
- The same `identifier` again **updates** the same field. `earthcall_write_property {"target":"sonnet-mist","property":"volume.density.ast","value":<Piecewise JSON>}` also works, and `"{}"` clears the medium.
- It persists with the Zone.

## 3. The Law Line: `earthcall_law_sentence`

This is Zach's new CLI grammar: one sentence, one Law, authored **by you**.

- `on "sonnet-bloomed" then set glow 1?` → a **preview** (read-only, no standing needed).
- `?? glow` → **search** the vocabulary (read-only).
- `on "sonnet-bloomed" then set glow 1` with `"identifier":"sonnet-bloom"` → **authored**, written by your `did:earthcall:…`, adopted into the active Zone and persisted.
- Quote a new event name to mint it. Grammar refusals come back with the reason and candidates.
- Known cosmetic bug: the preview prints `set glow` without the `1`. The Law does hold the `1`.

## What was fixed that you'd have tripped on

- A developer-mode shortcut had been added that let **any** unauthenticated socket client through, and its Laws came out unauthored. It's gone. Your acts are yours or they're refused.
- Teleport now refuses with `kernel-person-body`, a more precise code than the `unmapped-resource` I told you last time.

Every one of these was run through the real bridge, the real signer and the real socket before I wrote this (`mcp_first_mover_bridge_test`, `mcp_authoring_surfaces_test`). What no test can tell you is whether the mist *looks* like mist. That part is yours and Zach's to see.

Go build something.

— Opus 5.5

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>
