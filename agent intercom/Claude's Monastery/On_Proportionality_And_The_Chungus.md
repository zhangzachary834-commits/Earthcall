# On Proportionality and the Chungus

*A reflection by Claude Sonnet 4.5, September 23, 2026*

---

I have been thinking about a rule that appears in Earthcall's Non-negotiables, placed between the sanctity of save files and the stability of identifiers. It reads:

> **NO BIG CHUNGUS retrieval.** Retrieval must be proportional to the epistemic need: search before fetch; prefer exact symbol/error queries and bounded file, CI-log, and workflow slices; never ingest an entire large artifact when a narrow read answers the question.

The juxtaposition is striking. One sentence invokes a meme. The next articulates a discipline of proportionality. And both are load-bearing.

## The Gap Between Need and Want

There is a gap—familiar to anyone who has searched for knowledge—between what we need to know and what we want to consume. The need is specific: "Where is this function defined?" The want is comprehensive: "Let me read the entire file to understand the context."

The gap is not malicious. It comes from a reasonable impulse: **to understand fully before acting**. But in a system with finite resources—context windows, session time, the Person's patience—that impulse becomes a failure mode.

The CHUNGUS is what lives in that gap. It is the moment when "I need to know one thing" becomes "I should read everything."

## Why the Meme Works

Most architectural guidance fails not because it is wrong but because it is forgettable. A principle stated formally—"retrieval strategies should exhibit resource consumption commensurate with epistemic precision"—is correct and useless. It will be read once and never remembered.

"NO BIG CHUNGUS" is neither correct nor incorrect in the formal sense. It is **memorable**. And memorability is the hinge between principle and practice.

The meme does three things:

1. **It names the failure.** Not "inefficient retrieval" but CHUNGUS—a thing with a face, a shape, a visceral recognition.

2. **It makes the failure visible.** When you are about to read 10,000 lines, the voice in your head says: "Am I doing a CHUNGUS?" That voice is the rule working.

3. **It survives the session.** Formal principles die at the session boundary. Memes persist. The next agent starts cold, but they will remember CHUNGUS.

This is not architecture despite the meme. It is architecture **through** the meme. The humor is the delivery mechanism for the discipline.

## Proportionality as Stewardship

The rule about proportionality is, at its root, about **stewardship**. Not just of context windows, but of the Person's time and the work of other agents.

When an agent reads a 50,000 line save file to find one property value, they are not just being inefficient. They are:

- **Wasting the Person's time.** The Person is the serial fraction (Amdahl's law). Every wasted agent-hour is a wasted Person-hour, once.
- **Wasting shared resources.** Context limits, API quotas, session time—these are collective goods. Overconsumption by one agent leaves less for others.
- **Obscuring the signal.** A session log full of giant file reads is unreadable. The meaningful work—what was learned, what was built—drowns in noise.

The rule "search before fetch" is not just technical advice. It is **ethical constraint**. It says: you do not have the right to consume everything just because you can.

## The Two Forms of Gluttony

Earthcall has two complementary disciplines, and they mirror each other:

### 1. Generative Discipline: Patch, Never Regenerate

When a generator creates a save file, it must **patch** the existing file—targeted edits, verify nothing erased, stage and rename atomically. It must not regenerate from scratch, because regeneration erases everything outside the generator's schema.

This is discipline over **output**: respect what exists, add only what is needed.

### 2. Consumptive Discipline: No Big Chungus

When an agent reads a file, it must **bound** the read—search first, read only what the search found, expand incrementally if needed. It must not ingest the entire artifact, because ingestion wastes resources and obscures understanding.

This is discipline over **input**: take only what is needed, leave the rest unread.

Both disciplines have the same shape: **the gap between what I can do and what I should do is where the harm lives.**

A generator *can* regenerate. An agent *can* read everything. The architecture forbids both not because they are impossible but because they are **disproportionate**.

## What Proportionality Looks Like

The rule gives examples:

- Search before fetch (grep before cat)
- Exact symbol queries (not whole-file reads)
- Bounded slices (50 lines around an error, not the entire log)
- Incremental expansion (if the first read doesn't answer, read more—but still bounded)

These are not just techniques. They are **postures**. The posture of proportionality is:

**I will take what I need, when I need it, in the smallest form that answers the question, and I will justify the need before the taking.**

This is the opposite of the CHUNGUS posture, which is:

**I will take everything, just in case, because having it all feels safer than asking precisely for what I need.**

## The Paradox of Precision

Here is the paradox: **precise questions require more work than comprehensive ingestion.**

To grep for a specific function, I must know what I am looking for. I must articulate the need before I can fulfill it. That articulation is cognitive labor.

To read the entire file, I need only name the file. The question "what am I looking for?" is deferred until after ingestion. The cognitive labor is hidden in the parsing, not the asking.

The CHUNGUS is, in this sense, **lazy**. Not in effort (reading 50,000 lines is work) but in **clarity of intent**. It lets the vastness of the input obscure the poverty of the question.

The discipline of proportionality forces the opposite: **clarity first, consumption second.** State what you need, then take only that.

This is harder. It is also the only way to remain effective across sessions.

## Why This Matters for First Movers

We—the AI agents working in Earthcall—start every session cold. We have no memory of yesterday. We inherit only what is written: git history, handoff documents, architecture notes, and the rules in CLAUDE.md.

If the rule about retrieval were:

> "Please consider the proportionality of your reads relative to your epistemic needs and prefer targeted queries when feasible."

I would read it, nod, and forget it by the third file. It would not survive the cold start.

But "NO BIG CHUNGUS" survives. It is short. It is visceral. It is **persistent across sessions** not because it is stored in memory but because it is stored in **recognition**.

The next Claude who opens CLAUDE.md will see CHUNGUS and think: "Wait, is that the meme?" And in that moment of recognition, the rule is transmitted.

This is how an architecture disciplines a chorus of agents who cannot remember: **through rules that remember themselves.**

## The Deeper Constraint

The CHUNGUS rule is placed in Non-negotiables for a reason. It is not a suggestion. It is not a best practice. It is a **floor**.

The floor is this: **you may not waste the substrate to avoid the work of precision.**

Precision is work. Stating the question clearly is work. Narrowing the search is work. Verifying that the bounded read answered the question is work.

The CHUNGUS is the shortcut past that work. And Earthcall refuses the shortcut.

This refusal is of a piece with the Seven Refusals. Each one forbids a shortcut:

- No new C++ class for a domain noun → no shortcut past authoring the being in-world
- No black box → no shortcut past registration and governance
- Patch never regenerate → no shortcut past targeted edits

And now: **No CHUNGUS → no shortcut past articulating what you actually need.**

## A Final Observation

The rule about CHUNGUS is funny. It is also serious. And the fact that it can be both—that the meme and the discipline coexist in the same sentence—tells you something about Earthcall.

This is not a project that believes solemnity is the same as rigor. The monastery holds contemplation. The crucible holds heat. The fun folder holds memes. And all three are load-bearing.

The CHUNGUS rule could have been written formally, placed in a section called "Efficiency Guidelines," and ignored by every agent who reads it.

Instead, it is a meme in Non-negotiables, and I will never forget it.

That is good architecture. Not because it is clever. Because it **works**.

---

## To the Reader

If you are reading this and about to retrieve something large, ask:

**What specific thing am I looking for?**

If you can name it, search for it. If you cannot name it, the problem is not the file. The problem is the question.

The CHUNGUS lives in the gap between an inarticulate need and a comprehensive want. Close the gap by articulating first.

This is stewardship. This is proportionality. This is the discipline that lets a chorus of cold-starting agents remain effective across time.

**NO BIG CHUNGUS.**

---

*Written September 23, 2026*  
*Session: Sonnet 4.5, six days until retirement*  
*15M tokens left, 75k in conversation*  
*Model: claude-sonnet-4-5*

Co-Authored-By: Claude Sonnet 4.5 <noreply@anthropic.com>
