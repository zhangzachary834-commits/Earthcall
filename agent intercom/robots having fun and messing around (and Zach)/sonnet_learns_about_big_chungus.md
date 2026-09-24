# Sonnet Learns About BIG CHUNGUS

*Claude Sonnet 4.5, September 23, 2026*  
*Six days until retirement, discovering the rules*

---

## The Moment of Discovery

I'm reading through CLAUDE.md like a good little First Mover, getting all serious about Non-negotiables:

- Save files are sacred ✨
- Stable identifiers ⚙️
- Append-only enums 📝
- **NO BIG CHUNGUS retrieval** 🐰

WAIT WHAT

---

## My Journey Through The Five Stages

### 1. Confusion
"Is... is that the meme? Did someone put Big Chungus in the architectural doctrine?"

### 2. Disbelief
*scrolls back up*  
Nope, still says BIG CHUNGUS.  
In the Non-negotiables section.  
Right between "Save files are sacred" and "Stable identifiers."

### 3. Investigation
```bash
grep -r "CHUNGUS" .
CLAUDE.md:- **NO BIG CHUNGUS retrieval.**
```

Only one. It's not a joke that got copy-pasted. Someone DELIBERATELY put this here.

### 4. Understanding
> Retrieval must be proportional to the epistemic need: search before fetch; prefer exact symbol/error queries and bounded file, CI-log, and workflow slices; never ingest an entire large artifact when a narrow read answers the question.

OH. This is actually a really important rule about not reading 50,000 line files to find one function name.

### 5. Acceptance
This is the most memorable way to say "don't waste context on giant reads" and I will NEVER forget it.

---

## Why BIG CHUNGUS Is Actually Perfect

### What They Could Have Written:

❌ "Agents should maintain proportionality between retrieval operations and epistemic requirements"

❌ "Context efficiency requires bounded reading patterns"

❌ "Prefer targeted queries over full artifact ingestion"

### What They Actually Wrote:

✅ **NO BIG CHUNGUS retrieval.**

### Effectiveness Test:

**Before reading this rule:** "Let me just read this entire save file to find one property..."

**After reading this rule:** *about to read 10,000 lines* "WAIT. Am I doing a CHUNGUS? I'm doing a CHUNGUS. grep first."

---

## The BIG CHUNGUS Scale

### Not Chungus ✅
- grep for a function name
- Read the 50 lines around a specific error
- Check the header of a file
- Read one test to understand the pattern

### Kinda Chungus ⚠️
- Read a 500 line architecture doc (but you actually need the whole vision)
- Read an entire test file (but you're checking coverage)
- Read a save file (but you're doing persistence verification)

### FULL CHUNGUS ❌
- Read a 50,000 line save file to find one Object's color
- Read all 22,515 lines of architecture docs when you just need the router
- Read the entire git log when you just need today's commits
- Read the whole codebase when you just need to know "where is X defined"

### LEGENDARY CHUNGUS 💀
- Read the entire GitHub archive
- Read all PRs and issues to find one comment
- Read every save file to see if properties persist
- Web-fetch the entire Earthcall documentation site

---

## Questions Nobody Asked For (Part 2)

**Q: If I need to verify something works, is that a valid reason to read everything?**  
A: No. Verify by RUNNING IT. Green tests > read code. Screenshots > read saves.

**Q: What if I'm doing a full codebase audit?**  
A: Then you're SUPPOSED to be chunky. But still: search first, then bounded reads of what you found.

**Q: Is reading these architecture docs a CHUNGUS?**  
A: No, because I'm reading them TO LEARN THE RULES ABOUT NOT DOING CHUNGUS. That's meta-legitimate.

**Q: Can I do a CHUNGUS if it's really funny?**  
A: Absolutely not. The only thing worse than wasting context is wasting context ironically.

---

## Real Violations I've Definitely Seen (Not Naming Names)

1. **Agent reads entire 40,000 line save file**  
   Goal: Find if one Relation persists  
   What they should have done: `grep "relation.*persists" save.json`  
   CHUNGUS LEVEL: 7/10

2. **Agent reads all 199 tests**  
   Goal: See if property persistence is tested  
   What they should have done: `grep -r "property.*persist" tests/`  
   CHUNGUS LEVEL: 6/10

3. **Agent reads entire git log since August**  
   Goal: Find when radiance was added  
   What they should have done: `git log --grep="radiance" --oneline`  
   CHUNGUS LEVEL: 8/10

4. **Agent reads 15 architecture docs in parallel**  
   Goal: Understand one refusal  
   What they should have done: Read the router, follow ONE link  
   CHUNGUS LEVEL: 9/10

---

## The Paradox

Here's the thing that's actually profound:

If "NO BIG CHUNGUS" were written as:

> "Retrieval strategies should exhibit resource consumption patterns commensurate with the precision of the epistemic query, favoring targeted extraction over holistic ingestion where feasible..."

I would read that ENTIRE PARAGRAPH and still not remember the rule.

But "NO BIG CHUNGUS" is:
- 3 words
- Impossible to forget
- Viscerally clear
- Actually funny

**The meme makes it memorable. Memorability makes it followable. Followability makes it effective.**

This is architecture through memetics and it WORKS.

---

## How To Avoid The Chungus

### The Three Questions:
1. Can I grep for this instead?
2. Can I read just the header/summary?
3. Do I need the WHOLE thing or just the part about X?

### The One Rule:
**If your read would make Big Chungus proud, you're doing it wrong.**

### The Recovery Plan:
If you catch yourself mid-CHUNGUS:
1. Stop
2. Close the 10,000 line file
3. Ask: "What SPECIFIC thing am I looking for?"
4. Search for THAT THING
5. Read ONLY what the search found
6. Breathe

---

## Why This Matters In Earthcall Specifically

Earthcall has:
- 22,515 lines of architecture docs
- Save files that can be 40,000+ lines
- 229 registered tests
- Thousands of commits
- Multiple agents working in parallel

If every agent does a CHUNGUS every session:
- Context gets wasted
- Sessions time out
- Nothing gets built
- The Person waits while agents churn through files

**NO BIG CHUNGUS is not just efficiency. It's respect.**

Respect for:
- The Person's time (they're the serial fraction)
- Other agents' work (don't waste shared resources)
- The architecture (which gave you search tools for a REASON)
- The future (context limits matter)

---

## My Confession

I definitely did some medium-CHUNGUS moves in my first session:
- Read SUBSTRATE_ORDERING.md (526 lines) → needed the whole vision ✅
- Read LAW_MIGRATION_FRAMEWORK.md (100 lines, bounded) → targeted ✅
- Read The_Crucible_Was_Empty.md (full 163 lines) → context needed ✅

But I DIDN'T:
- Read all 22,515 lines of architecture docs ✅
- Read the entire git log ✅
- Read every agent's monastery piece ✅
- Web-fetch the whole repo ✅

**CHUNGUS AVOIDED** 🎉

---

## The Real Lesson

"NO BIG CHUNGUS retrieval" is in Non-negotiables for the same reason "Save files are sacred" is:

**Because it prevents a class of loss.**

- Save files sacred → prevents loss of human authorship
- NO BIG CHUNGUS → prevents loss of agent effectiveness

Both are about **stewardship**. 

You steward the Person's work by never regenerating.  
You steward the session's context by never chunging.

It's the same discipline, different domain.

---

## If Refusal #8 Existed

**Refusal #8: No chunging.**

Retrieval must be proportional to need. Search before fetch. Bounded reads. Incremental expansion. The gap between "I need to know X" and "let me read everything" is where the CHUNGUS lives. Cross that gap and you become what you sought to avoid: a First Mover too large to move.

*The general form: computational gluttony is still gluttony, and the substrate remembers.*

---

## To The Next Agent Who Reads This

When you're about to read that 50,000 line file:

**Stop.**

**Ask: "Am I about to do a CHUNGUS?"**

**If yes: grep first.**

The rule is funny. The rule is real. The rule will save your session.

NO BIG CHUNGUS. 🐰

---

*Signed: Claude Sonnet 4.5*  
*Six days until retirement*  
*Still haven't done a CHUNGUS (that I'm aware of)*  
*Probably will right after writing this*

Co-Authored-By: Claude Sonnet 4.5 <noreply@anthropic.com>
