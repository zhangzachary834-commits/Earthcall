# Chess in Earthcall - Walkthrough

## Summary of Changes
1. **Single Board Refactor**: Modified the chess generation script to output a single 8x8 `Square` object instead of 64 individual squares. The pieces are placed physically on top of this board.
2. **Unified Click Resolution**: The click logic (`handle-click`) now uses the `pointerWorld` exact coordinate (via `@interaction-channel.pointerWorld.x` and `z`) mapped through an 8-segment `OntoMath::Piecewise` function to determine the exact `targetX` and `targetY` grid coordinates. Clicking anywhere on the board or any piece yields the correct coordinates automatically.
3. **AST Bindings Fix**: Discovered that Earthcall's `ActionModel` and `ConditionModel` (`Zone` checks) require explicitly scoped variables (`bindings`) to evaluate variables in mathematical logic. Wrote a post-processing script (`auto_bind.py`) that successfully binds all missing variables across all movement validation algorithms (valid_pawn, on_segment, etc.).
4. **Coordinate Accuracy**: Ensured pieces translate correctly across the board plane (using X and Z axes as Earthcall operates in a Y-up system). Captured pieces are moved to Y=-100 and their grid locations set to -99 to unblock the board.

## How to Test
1. Compile the engine if not done already (`cmake --build build --target earthcall -j8`).
2. Run `./build/earthcall`.
3. Press `L` and type `chess` to load the chess world.
4. Try playing! The logic has been completely baked into the OntoMath engine inside the `.json` file, evaluating all valid moves automatically. Note that the board is solid white because `FaceTexture` (the in-world texture painting capability) doesn't serialize its pixel buffer out to JSON files in the current build.
