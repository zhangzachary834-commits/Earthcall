# Magic: The Gathering inside Earthcall — Implementation Plan

**Date:** 2026-10-01
**Author:** Antigravity (Gemini 3.1 Pro)
**Status:** DRAFT

## 1. The Core Philosophy (The Seven Refusals)
MTG is completely governed by rules, state-based actions, and relational zones. Earthcall’s Rete/Law engine is uniquely suited to model this without a single new C++ class, heavily relying on **Refusal 7** (No new methods to define variable behavior).
- **No `Card` class:** Cards are `Object` instances of `category.mtg.card`.
- **No `Deck` class:** Libraries, Graveyards, Hands, and the Battlefield are `Formation`s. Cards belong to them via first-class `Relation`s.
- **No hardcoded Turn Loop:** Phases are advanced via past-tense `noun-verbed` event edges governed by Laws, not C++ `update()` methods.
- **No hardcoded Relation Weights:** We use authored properties (e.g. `properties.mtg.stackOrder`) on Relations instead of hardcoded C++ variables to track things like Stack order or Library deck order.

## 2. Ontological Layout (The Nouns)

### The Zones as Formations
In MTG, "Zones" (Library, Hand, Battlefield, Graveyard, Stack, Exile) map perfectly to Earthcall `Formation`s. 
- `mtg.formation.library.<player_id>`
- `mtg.formation.hand.<player_id>`
- `mtg.formation.battlefield`
- `mtg.formation.stack`
- `mtg.formation.graveyard.<player_id>`

Cards move between zones by dissolving their existing `member-of` Relation and forming a new one. The Rete matches on these relations to apply continuous effects.

### The Cards
Cards are `Object`s holding authored properties.
- `mtg.power` (int)
- `mtg.toughness` (int)
- `mtg.damage` (int)
- `mtg.type` (string/int enum)
- `mtg.manaCost` (complex property)

## 3. The Stack & Priority (The Verbs)

The Stack is a LIFO queue mapped to a Formation.
1. **Casting:** Player A targets a card in their Hand and triggers `action-attempt-cast`.
2. **Validation Law:** A Law intercepts `action-attempt-cast`. 
   - *Action:* Deduct mana, dissolve `member-of` Hand, establish `member-of` Stack. 
   - *Crucial Detail:* The new Relation is given an **authored property** `mtg.stackOrder` (incremented via `@mtg-state.stackCounter`).
3. **Priority Passing:** A singleton object `@mtg-state` holds `priorityPlayer`. Laws pass priority back and forth via `priority-passed` events.
4. **Resolution Law:** When priority is passed in succession without new spells, a Law triggers on the Stack Formation, finds the card whose Relation has the highest authored `mtg.stackOrder` property, applies its effects, and moves it to the Graveyard.

## 4. State-Based Actions (SBAs)
Earthcall's Rete handles SBAs natively via reactive Laws.
- **Lethal Damage Law:** 
  - *Condition:* `Any(Cmp(relation: member-of, mtg.formation.battlefield), Cmp("mtg.damage", ">=", "@self.mtg.toughness"))`
  - *Action:* Publish `creature-died` event, move to Graveyard.

## 5. Physical Representation (Avoiding the Origin Explosion)
To avoid the infamous 64-cube frag grenade:
- When a card enters a Formation, an `update-layout` Law calculates non-overlapping X/Z coordinates dynamically based on the current count of cards in the Formation. 

## 6. Execution Steps
1. **Seed the World:** Create `scratch/seed_mtg_ontology.py` to generate the baseline `mtg.json` save with the Formations and test cards.
2. **Wire the State:** Implement `@mtg-state` and the turn-phase progression events.
3. **Wire the Board:** Implement physical layout Laws.
4. **Implement the Stack:** Write the Laws for priority passing and basic casting.
