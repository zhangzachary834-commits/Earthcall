import re

with open("docs/plans/MTG_IN_EARTHCALL_IMPLEMENTATION_PLAN_2026-10-01.md", "r") as f:
    content = f.read()

content = content.replace("Status:** IN PROGRESS", "Status:** COMPLETED")

new_steps = """## 6. Execution Steps
1. **[DONE] Seed the World:** Create baseline `MTG/zone.json` save with test cards, fixing a C++ Engine Serialization Bug where `Lexeme` and `Relation` properties were dropped.
2. **[DONE] Wire the State:** Implemented `@state.mtg` and the turn-phase progression events. `button.mtg.pass` physically steps the game through `untap`, `upkeep`, `main1`, `combat`, `main2`, `end` using 7 Phase Progression Laws.
3. **[DONE] Wire the Board:** Implemented physical layout Laws using `WhileTrue` and `ActionNode::Kind::Map` / `ActionNode::Kind::Set`. Hand cards snap to `[handOrder * 1.5, 0.5, 2.0]` and Stack cards snap to `[stackOrder * 1.5, 0.5, -2.0]`. Used `Lexeme`s instead of Formations to track zones (`@self.mtg.zone = "hand.1"`).
4. **[DONE] Implement the Stack:** Wrote the Law `mtg-cast-card` which intercepts a `pointer-click` on a hand card during `main1`. It maps `@state.mtg.stackCounter` to the card's `mtg.stackOrder`, increments the counter, and sets the card's zone to `"stack"`.
5. **[DONE] Implement Card Resolution:** Created a `button.mtg.resolve` object. When clicked, it publishes a `resolve-stack` event. The `mtg-execute-resolve` Law catches this event, verifies the card is at the top of the stack (`@self.mtg.stackOrder == @state.mtg.topStackOrder`), moves it to the `"battlefield"`, sets its layout order, and decrements the stack counters.
6. **[DONE] Implement State-Based Actions:** Added the `mtg-sba-lethal-damage` Law. It runs on `WhileTrue` and compares `@self.mtg.damage >= @self.mtg.toughness`. If true, it moves the card from `"battlefield"` to `"graveyard"`, incrementing the `graveyardCounter` and dynamically adjusting its layout position.
"""

content = re.sub(r"## 6\. Execution Steps.*", new_steps, content, flags=re.DOTALL)

with open("docs/plans/MTG_IN_EARTHCALL_IMPLEMENTATION_PLAN_2026-10-01.md", "w") as f:
    f.write(content)

with open("/Users/zacharyzhang/.gemini/antigravity/brain/e1346ea7-7d86-4945-8831-01e3e33dd09a/implementation_plan.md", "w") as f:
    f.write(content)
