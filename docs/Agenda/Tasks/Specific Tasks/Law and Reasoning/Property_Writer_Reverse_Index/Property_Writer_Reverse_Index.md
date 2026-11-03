# Property Writer Reverse Index

**Status:** implementation complete; Person verification remains

## Human direction

Zach asked on 2026-09-07 for a direct way to find every Action node that writes one property, organized by Law and/or authored Relations: “who knew I'd need grep for Laws.” He then added two necessary pivots: organization by condition branches and by parent Action nodes.

The underlying need is reverse navigation through authored causality. A Person looking at `position.y`, `color.r`, or an authored property should be able to ask what can change it and arrive at the exact Law text responsible, without opening Laws one by one.

## Implemented response

`Property Writers` is a separate window reachable from both the Law Author toolbar and Law Library. It provides:

- the same Singular-first Property Lens used by Action/Condition editing, plus direct text entry;
- Exact path, Path family, and Contains-text matching;
- a reverse index over every nested Action tree, including Sequence, Parallel, Create, and Synthesize descendants;
- write recognition for Set, Add, Scale, Lerp, Drive, Map, Flow, AddProperty, RemoveProperty, WritePixel, and ElevatePixels;
- deliberate exclusion of read-only operands such as PlayAudio frequency/amplitude paths;
- result counts by Action node and owning Law;
- grouping by Law, actual authored Relations, IF branches, or immediate parent Action nodes;
- truthful IF labeling: each branch states that the complete condition still gates the write, and preserves ALL/ANY/NOT/quantifier context;
- direct navigation from a result to the exact Action card in Law Author.

Relation grouping reads only actual `Relation` beings incident to matching Laws. It does not infer a Relation merely because Laws write the same property. Runtime-coordinate pixel mutation is reported honestly as the wildcard `surface.pixel.*`; elevated pixel properties are indexed by their authored property name and companion `surface.selection.<name>` path.

The reverse index is a read-only view over serialized `ActionModel` and the existing world Relation graph. It adds no action kinds, categories, domain classes, or persistence format.

## Verification

- `cmake --build build --target earthcall_webgpu -j8` — pass.
- `cmake --build build --target law_graph_test channel_paths_test no_black_box_test -j8` — pass.
- `ctest --test-dir build --output-on-failure -R '^(law_graph_test|channel_paths_test|no_black_box_test)$'` — 3/3 pass with desktop access.
- `law_graph_test` now proves nested model paths, ordinary property writes, authored-property grants, elevated-property writes, wildcard pixel writes, and the exclusion of PlayAudio read paths.

Recorded by Codex, session `01a07a4c-2b09-7f62-b975-3a23084ddeaf`, 2026-09-07 17:02 PDT.
