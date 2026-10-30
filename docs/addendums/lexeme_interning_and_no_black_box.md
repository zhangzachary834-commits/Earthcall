# Lexeme Identity, Future Execution Keys, and No Black Box

*Origin: Jules task 3260200770766759045; reviewed by Sun Squid Intake II, 2026-10-26.*
**Status: design hypothesis; no shipped LexemeID VM operand or measured speedup is asserted.**

[Lexeme Identity and a Future CPU Law Execution VM](../architecture/interrelations/LEXEME_INTERNING_AND_LAW_BYTECODE_VM.md) explicitly says today's Lexeme representation is **not** a bytecode VM integer ABI. The proposed compilation boundary might resolve authored vocabulary to compact execution-local operands; operand representation, cache behavior, and performance benefit remain open to implementation and measurement.

[No Black Box](../architecture/ontology/NO_BLACK_BOX.md) requires authored being state to be addressable under registered paths or explicitly designated Kernel mechanism. A future internal lookup key must not become a second, hidden ontological identity or an ungovernable store of meaning.

## Architectural contract
- Authored Lexeme identity and meaning remain authoritative; an execution key is a derived optimization, not a replacement for the Lexeme.
- Any key mapping needs provenance, collision/ambiguity handling, invalidation, and a legible path back to authored vocabulary.
- An optimization may avoid repeated string work only if executable tests establish equivalent Law behavior and measurements establish a real gain.

This is a **possible bridge** between authored semantics and faster execution, not a description of an already implemented interning table.
