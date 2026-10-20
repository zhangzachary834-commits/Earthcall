# TerminalLine Naming and Modality Scope Addendum

**Human decree:** Zach, 2026-10-20.

## Decision

The existing implementation name `LawSentence` is a slight naming imprecision.

The concept Zach intended is better named **`TerminalLine`**: the parser/reader for one line of text sensed by the **Terminal modality**. It exists to let `TerminalChannel` interpret the Mac Terminal command line and compile that line through the authored Law Line vocabulary.

This is a naming and architectural-scope clarification. It does **not** by itself rename the existing C++ namespace/files, which may remain `LawSentence` temporarily as a legacy implementation identifier until a deliberate refactor is worthwhile.

## What `LawSentence` / `TerminalLine` is

```text
Terminal modality
  -> TerminalChannel senses a line
  -> TerminalLine (legacy implementation name: LawSentence) parses that line
  -> live Lexemes and their Relations identify authored meaning
  -> Lexeme <--denotes--> Law / Metalaw
  -> the Law carries the executable semantic/action model
```

The parser is a **modality-local ingress surface**. It reads the Terminal's line. It is not itself an in-world language being, not the source of semantic truth, and not the universal representation of a human sentence.

## What it is NOT

Do **not** treat `LawSentence` as:

- Earthcall's universal natural-language parser;
- a canonical sentence ontology;
- an in-world `Sentence` being;
- the required ingress for Web Designer, Voice, Screen, GUI, or future modalities;
- the semantic authority that decides what Lexemes mean;
- a reusable shortcut merely because another modality also accepts text.

The name `LawSentence` must not be used to justify importing Terminal-specific grammar into unrelated modalities.

If another modality needs text ingress, first ask what that modality actually senses and what lower-level mechanism can be shared without importing Terminal assumptions. A shared character-span or Lexeme matching mechanism may be appropriate. Reusing the Terminal's whole line parser by default is not.

## Where meaning lives

The enduring semantic relation remains:

```text
Lexeme <--Relation--> Law / Metalaw
```

A Lexeme may be a word, phrase, sentence, affix, symbol sequence, or arbitrary semantic language unit. Its meaning is not created by `TerminalLine`; the line parser discovers and composes meanings already authored in the world.

This preserves Zach's 2026-09-25 correction:

> We only want a LawSentence to parse the modality channel of the Mac Terminal's command line. We don't want it in-world.

The 2026-10-20 clarification sharpens the naming:

> `LawSentence` was a slight naming imprecision. The intended concept is `TerminalLine`.

## Rule for future agents

When reading existing code or documentation:

- mentally substitute **`TerminalLine`** when `LawSentence` refers to the Terminal parser;
- do not generalize that parser across modalities;
- preserve authored Lexeme/Relation/Law semantics below the modality boundary;
- if a future refactor renames the C++ files/namespace, treat that as a compatibility cleanup, not an ontology change.

### Anti-gift-shop test

If a proposal says:

> “This modality has language or Laws, therefore it should use `LawSentence`,”

the proposal has not established its abstraction boundary.

The correct question is:

> “What does this modality sense, and which authored semantic mechanisms are genuinely modality-neutral?”

This addendum exists specifically to prevent the legacy name from becoming an accidental universal abstraction.
