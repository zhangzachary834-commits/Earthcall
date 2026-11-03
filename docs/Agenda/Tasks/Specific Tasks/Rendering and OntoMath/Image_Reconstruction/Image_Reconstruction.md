# Image pixels into editable Earthcall form

Zach commissioned the reverse modality: “CONVERASTION FROM IMAGE PIXELS TO WEBGPU PIXELS,” then clarified “both but fundamentally the latter” when asked about faithful display versus reconstructed editable form. Reconstruction is the main end; retaining the source is the witness against which it is judged. Zach's existing raster/OntoMath/Formation design supplies the direction. Codex supplied the exact flat-region first rung and native verification, not a claim to have completed semantic or 3D inference.

Codex · GPT-6 · session `01a0fe15-4fe2-7dc0-a2d0-7d823e4ad26c` · 2026-10-02 15:57 PDT.

## Implemented and witnessed

`scripts/reconstruct_flat_image.py` is a First Mover developer tool. It converts opaque image pixels into a lossless partition of existing Shape2D Objects, merging equal horizontal runs vertically. Each region carries editable position, dimensions and face color; stable source-derived identifiers and authored source hash/method/author/index properties persist. Materials are shared by source color, with neutral texture modulation so future copy-on-write painting keeps its intended color. No source texture is needed by the reconstructed geometry.

The new `border.visible` dynamic property lets a Person suppress the existing Shape2D outline. Missing or incorrectly typed values retain the previous outlined behavior. ImageCodec imports explicitly declare false. The engine consumes authored representation data; the partition policy remains outside it.

The tool creates **only new, uninhabited seeds**. It requires the commissioning Person's identifier, records `authors` and `injected_by`, stages and validates the JSON, then publishes without overwriting an existing file, including a racing writer. Existing worlds require a separate targeted patch preserving their pre-image and relationships. There are no grants, positive authority values, inferred categories, invented Person identities or reconstructed behaviors.

On the actual chess screenshot's promotion strip, a **1024×128** source became **329** editable regions. Both faithful ImageCodec display and texture-free reconstructed geometry matched every RGBA byte on the native WebGPU surface. A Law removed a colored region's width, changing **8,547 pixels with zero changes outside its bounds**. Another Law painted the region green, matching the expected pixels without changing its neighbors. Object serialization round-tripped identity, bounds, provenance and Material references. The generated seed also loaded through `Engine::init` / `ZoneManager::loadTestObservation` / `Engine::tick`, producing a viewport screenshot and seven recording frames. This is machine evidence; Zach has not yet accepted this reconstruction as a Person.

The four CPU checks passed, including 100 independently rasterized random opaque images, one-region compression, explicit refusal of transparency/budget overflow, and preservation of existing output. Existing `image_codec_test` and `ontomath_two_direction_witness_test` also completed successfully. The production `earthcall_webgpu` build passed; the full suite was not run. The top-level `reconstructionEvidence` is a developer report and is not claimed to survive generic session serialization; region provenance is carried through existing authored properties.

## Use and verification

The authoring tool and CPU tests require Python with Pillow. The native witness uses the configured production app object files, a compiler and a macOS desktop GPU. Its current physical-pixel fixture assumes even source dimensions and a 2× Retina framebuffer; other display scales refuse explicitly. It draws at source-pixel coordinates; the ordinary engine interprets those authored coordinates as window points, so Retina display scale can enlarge the live result.

```sh
python3 scripts/reconstruct_flat_image.py SOURCE.png NEW_SEED.json --author PERSON_IDENTIFIER
python3 scripts/test_image_reconstruction.py
cmake --build build --target earthcall_webgpu image_codec_test ontomath_two_direction_witness_test -j8
python3 scripts/verify_image_reconstruction.py SOURCE.png NEW_SEED.json OUTPUT_DIRECTORY
```

The witness writes `faithful.png`, `reconstructed.png`, `edited.png`, `painted.png`, and `default-border.png`. The C++ probe lives under `scripts/`, intentionally outside the CMake test glob: it takes explicit source/seed arguments and links the production WebGPU objects. It is an additional native witness, not a registered unattended ctest. Reproduction source, draft seed and captured images from this pass are retained in ignored `scratch/capture-verification/2026-10-02-01a0fe15/reverse/`; the accepted draft is **`reconstruction-v2.json`**, authored by **Zach**, containing the `reconstruction-759525d332e20602-region-*` Objects and their referenced Materials. The earlier `reconstruction.json` remains as the pre-correction artifact; do not use it as the final draft.

## Remaining work and successor guardrails

- Connect import/reconstruction selection to Person-authored interaction and the existing First Mover mutation guard; this pass adds a developer command, not an import button.
- Propose meaningful semantic groups, contours, text, Relations and Formations while keeping source evidence, model hypotheses and Person acceptance distinct; the pixel lettering here has no inferred Text2D meaning and the apparent buttons have no click behavior.
- Add authored approximation/error and primitive-count policy for photographic or antialiased input; the current exact algorithm refuses transparency and images exceeding the 4,096-region default budget rather than silently simplifying them.
- Recover depth, camera, materials and hidden surfaces as explicit uncertain hypotheses; one image does not uniquely determine them, and this pass recovers none of them.
- Support targeted import updates with stable correspondence into an inhabited world before modifying existing saves.
- Verify color-profile interpretation, transparency and alternate display scales separately; the current opaque sample-byte parity is not colorimetric or HDR proof.

Jules and future agents: preserve the neutral Material tint, border declaration and direct pixel witnesses. Do not replace reconstructed Objects with a screenshot texture and call that editable form. Do not promote the older mathematical image-decomposition theory into an implementation claim. Read `docs/architecture/Design/ONTOMATH_RASTER_FORMATION_AND_PROPERTY_GRAPHS.md`, `law/ALGORITHMS_AS_LAW.md`, and `law/FIRST_MOVER_AUTHORING.md` before extending the tool; retain the Person-centered ontology and author-confirmed meaning.
