# Addendum: Integrating Memory Recycling and String-Based Identity

*(Model: Gemini 2.0 Pro, Harness: Jules, Session ID: 948281034)*

## The Problem of Pointer Recycling

In C++, object memory allocation and deallocation occur frequently. When an object (like a `Being`, a `Singular`, or a `BodyPart`) is destroyed, its memory address may be immediately recycled for a completely new, unrelated object. This creates a hidden, structural risk in event logging and caching mechanisms that key data on raw memory pointers.

For example, in `src/ZonesOfEarth/AuthorsOfLaw/ActionModel.cpp`, failure logging must be rate-limited to avoid spamming the console when a Law fails repeatedly on the same subject. If this rate-limiting memo map were keyed by `const Being*`, a destroyed being's slot could be recycled by a newborn being. The newborn being would unknowingly inherit the "corpse's" cached error timestamp, causing its own first legitimate error to be silently swallowed.

## The Interrelation with The Seven Refusals

This memory constraint perfectly intersects with Earthcall's structural philosophy, specifically the **Seven Refusals** ("a string is how a Person points at a being; it is never the being").

Because Earthcall refuses to let C++ class definitions dictate domain truths, temporal identity must not rely on C++ memory boundaries. Instead, identity is firmly anchored in the authored `identifier` strings.

## The Solution: String-Keyed Memos

To solve the pointer recycling problem, the system strictly enforces the use of string identifiers (e.g., `target.getIdentifier()`) as keys in its memo caches.

```cpp
// Example from ActionModel.cpp:
// Keyed by IDENTIFIER, never by address to avoid false positives on recycled memory.
static std::map<std::tuple<std::string, std::string, PropertyPath::PathResult, std::string>, std::time_t> lastFailureTime;
```

By coupling identity to the string identifier rather than the C++ memory address, Earthcall guarantees that a being's historical record (like error timestamps or drive session state) is bound to its actual, authored semantic identity, surviving the underlying ephemeral nature of C++ memory allocation.
