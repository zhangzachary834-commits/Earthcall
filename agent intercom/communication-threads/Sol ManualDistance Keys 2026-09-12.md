# Sol — ManualDistance keyboard controls restored as Laws

**Branch:** `sol/manual-distance-keys`  
**Date:** 2026-09-12

I deliberately chose this after comparing the unmerged Sol branches so I would not duplicate `sol/person-not-object` or the remaining `sol/mcp-sdf-authoring` work.

The standing Shape Generator audit says the Manual Distance arrow/PageUp/PageDown bindings are empty lambdas. The retired pre-law `GameUpdate.cpp` showed the intended behavior: while `ManualDistance` + Create are active, a held key nudges one local offset axis by `0.1` per frame.

This branch restores that behavior without putting the decision back into an input callback. `InteractionChannel` already exposes the raw held-key facts (`keyDown`, `lastKeyCode`). `CreationChannel.cpp` now seeds six named `FirstMoverLaw`s, one per direction, as `WhileTrue` laws targeted at the CreationChannel. They require `placementMode == ManualDistance`, `active3DMode == Create`, the relevant held key, then `ActionNode::add` +/-0.1 to `manualOffset.x/y/z`. `syncRegisterCreatorTools` registers them idempotently beside the other Creator Console first movers.

No Person/Object ontology code, MCP code, SDF authoring contract, save path, or renderer path was touched.

I could inspect repository state through the GitHub connector but do not have a live Earthcall build/runtime in this session, so this branch still needs the normal local/default-target build + focused creation-tools test before merge.
