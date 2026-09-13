# Spike 2 Lag Regression: Handoff to Gemini Spark

Hey Spark! Here's the current state of the algorithmic lag regression in `SynthesisStudio.LivingInstrument` (Spike 2).

## What has been fixed so far:
1. **`PropheticRete::drainAgenda`**: Replaced the expensive `_facts.erase(it)` inside the vector with a constant-time `swap_and_pop`.
2. **`_conditionMemory`**: Reverted the attempt to use `std::unordered_set` back to `std::unordered_map`. `unordered_set` proved slower because erasing a non-existent element triggers full bucket lookups 1600 times during full sweeps.
3. **`OnBecomeTrue` Fast Path**: I integrated `OnBecomeTrue` laws into the Rete fast path in `LawManager::tick`. Previously, the fast path explicitly excluded them (`law->activation() == Law::Activation::WhileTrue`). By keeping a `newlyTrue` vector during the Rete evaluation loop, `OnBecomeTrue` laws now correctly trigger without a full $O(N)$ sweep.

These changes brought the uncalibrated frame time down from ~160ms to **~89ms**.

## The Remaining 75ms (The final boss):
The remaining lag comes from exactly **four mysterious `WhileTrue` laws** that are falling back to the full $O(N)$ sweep (`sweepSubjects`).
- Their identifiers are: `law-1`, `law-2`, `law-3`, `law-4`.
- They all share the exact same name: `"Studio: Show the selected harmony"`.
- They have NO Rete terminals. This happens because their conditions read qualified roots (e.g., `@state.studio.harmony`), which causes `ConditionModel::compileToRete` to return `{}` to prevent the law from going deaf to external state changes.
- Because they lack terminals, `tick()` routes them to the full sweep fallback, evaluating them against all 1601 objects in the world.
- Strangely, they report `requiredProperties.size() == 2`, and they actually only sweep **3 subjects** each, yet they take between **5ms and 33ms** to evaluate. This heavily implies their condition contains a Quantifier (which iterates over all other objects, making it $O(N \times M)$).

## The Plan / Next Steps for You:
1. **Find where `law-1` to `law-4` are instantiated**: They do not appear to exist in `synthesis_studio_living.json` under these identifiers. They are likely generated dynamically at runtime (e.g., via `LawSynthesis.cpp` or duplicated via some loading bug) because `getIdentifier()` falls back to `"law-" + id` when an explicit identifier is not provided.
2. **Optimize the Rete Fallback**: If these laws genuinely need to evaluate quantifiers or qualified roots, we need a way to prevent them from doing a full naive sweep every frame. Could we memoize global conditions (like `@state.studio.harmony == "piano"`) once per frame so we don't evaluate them per-subject?
3. **Verify the Fix**: Apply the final optimizations and verify that the median frame time in `frame_lag_test saves/worlds/synthesis_studio_living.json` drops closer to the baseline aspiration.

Good luck!
