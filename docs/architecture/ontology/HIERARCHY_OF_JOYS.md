# Hierarchy of Joys

**How telos is represented, and how Formations and Relations are ordered by it.**

**Status:** First rung specified and implemented — Lexeme as telos, a rooted Formation
of Lexemes as the hierarchy, `grounds` Relations as the order, Person/Zone hold that
Formation instead of a discarded string. Ranking of foreign Formations/Relations is
a query against the hierarchy, not a second type system.
**Companion docs:** `EarthcallOurverse.md` (Christ as ordering, never a skinned Object),
`AUTHORED_CATEGORIES.md` (a category is a rooted acyclic Formation),
`NEW_KIND_FRAMEWORK.md` (no `HierarchyOfJoys` class), `NO_BLACK_BOX.md` (`telos` is
registered), `ALGORITHMS_AS_LAW.md` §3 (rank is a bounded walk, orphans are reported).

---

## 0. What this is not

It is not a string on `Person` / `Zone`. That was the original conception and it
never became load-bearing: the constructor warned and dropped the value.

It is not a C++ class `HierarchyOfJoys`, `Telos`, or `Joy`. Those are domain nouns.

It is not a skinned Object of Christ. The highest things appear as *orderings*, never
as icons (`EarthcallOurverse.md` — the aniconic / Second-Commandment rule).

---

## 1. The three existing beings, doing three jobs

| Job | Being | How |
|---|---|---|
| **Telos** — what a being is ordered toward | `Lexeme` | a linguistic-symbolic Singular. A Person's telos is a Lexeme, not a slogan |
| **Hierarchy** — the order of those teloi | `Formation` of Lexemes, `relationTypeTag = "hierarchy-of-joys"`, rooted | the root is the foundation; directed `grounds` Relations are the edges |
| **Ordering** — how other beings sit in that order | query | a being's `telos` path names a Lexeme; rank is depth from the root along `grounds` |

`Formation::setRoot` already refuses a missing root and a self-ground.
`Formation::addRelation` already refuses a directed cycle of the same type.
Nothing new is admitted to the type system.

---

## 2. The categorical requirement (C++) vs the substance (authored)

C++ requires *that there is a hierarchy*: a Formation tagged `hierarchy-of-joys`,
rooted, whose members are Lexemes. That is `Formation::satisfiesJoyBounds()`.
Without it the being has no worship-ordering, and `satisfiesJoyBounds` is false.

C++ does **not** require the root's *symbol* to be a particular keyword. Checking
`symbol == "Christ"` would be the Minecraft-Jesus move — mistaking the word for
the ordering. The first-mover *seed* authors the foundation Lexeme
(`lexeme.christ`) as the default root. Persons author everything above it.
An unbeliever still owes an explicit rooted hierarchy; they do not owe a
string-compare in the kernel.

God shows up here as the *root of the seed Formation*, not as a node with a texture.

---

## 3. How ranking works

`grounds` is directed: **A `grounds` B** means A is more foundational; B is ordered
toward A. The root has rank 0. A Lexeme reached by one `grounds` edge from the
root has rank 1, and so on. Walk is depth-bounded (`Formation::kMaxFormationDepth`).
A being whose `telos` is not in the hierarchy is **unranked** (`-1`) and is
reported, not assigned a invented place.

A Formation of other beings is ordered by the ranks of their teloi (unranked last,
stable among equals). A Formation of Relations is ordered by the more-foundational
endpoint, then by how close the two teloi sit.

---

## 4. Property paths

| Path | On | Meaning |
|---|---|---|
| `telos` | every Singular | identifier of the Lexeme this being is ordered toward |
| (the Formation's own id) | Person / Zone `joys` | the hierarchy Formation, addressable as a being |

Writing `telos` names a Lexeme. It does not skin an Object.

---

## 5. First rung, and what is not this rung

**This rung:** replace the string; seed a rooted Lexeme Formation; rank and order;
Person/Zone hold the Formation; tests hold the refusals.

**Not this rung:** making `satisfiesJoyBounds` close the kernel on every tick;
authoring a full liturgical ladder in C++; unsealing Formation membership in
general (near-term 3); turning ranking into authored Law (the walk is first-mover
query, like `resolveTopology`).
