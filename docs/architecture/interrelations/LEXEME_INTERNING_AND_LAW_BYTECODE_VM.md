# Lexeme Identity and a Future CPU Law Execution VM

**How Earthcall's first-class vocabulary could inform operands for the proposed Law Bytecode VM without pretending that today's Lexeme representation is already that VM's integer ABI.**

**Status:** Conceptual design hypothesis; not implemented.
**Connected Documents:**
* `LEXEMES_AS_THE_ATOMS_OF_NO_BLACK_BOX.md`
* `../law/LAW_EXECUTION_FRONTIER.md`

---

## The Interrelation

`LAW_EXECUTION_FRONTIER.md` proposes a future CPU bytecode VM after nearer-term property-lookup work. Earthcall also has first-class `Lexeme` beings and a `LanguageSystem`. Those ideas are related, but the current repository does **not** establish a `LexemeID` bytecode operand, a Law VM instruction ABI, or a direct-index property array keyed by Lexeme identity.

That distinction matters. `Lexeme` currently carries a string symbol and Singular identity. A future compiler may resolve authored vocabulary to a compact execution key, but the compact key is an execution representation; it must not be confused with the ontological identity of the Lexeme itself.

### A possible compilation boundary

A future Law compiler could:

1. Resolve an authored property name through the language/vocabulary layer.
2. Translate that resolved semantic identity into a compact VM-local operand.
3. Emit an instruction such as `OP_READ_PROP <operand>`.
4. Preserve enough mapping/provenance that the bytecode remains inspectable back to the authored Lexeme/property path.

This would let execution avoid repeated textual lookup in a hot loop while preserving No-Black-Box legibility at the compiler boundary.

The important architectural claim is therefore narrower than “Lexemes are native opcodes”: **first-class semantic identity gives a future VM a principled source from which to derive compact operands.** The exact operand representation, lookup structure, cache behavior, and performance benefit require implementation and measurement; they are not facts established by the current code.
