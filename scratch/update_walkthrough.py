import re

with open("/Users/zacharyzhang/.gemini/antigravity/brain/e1346ea7-7d86-4945-8831-01e3e33dd09a/walkthrough.md", "r") as f:
    content = f.read()

new_points = """### C. Card Resolution & State-Based Actions (Steps 5 & 6)
- **Resolution**: Created `button.mtg.resolve`. Clicking it fires `resolve-stack`, prompting the card with the highest `stackOrder` to move to the `battlefield`. I maintained LIFO stack logic by tracking `@state.mtg.topStackOrder` trailing `@state.mtg.stackCounter`.
- **State-Based Actions**: Authored `mtg-sba-lethal-damage` (using `ConditionNode::Kind::Compare` with `operandPath`) to continuously sweep the battlefield for cards where `damage >= toughness`. Lethally damaged cards are instantly moved to the `graveyard` zone.
"""

content = content.replace("### C. Refusal 7 Validation", new_points + "\n### D. Refusal 7 Validation")

with open("/Users/zacharyzhang/.gemini/antigravity/brain/e1346ea7-7d86-4945-8831-01e3e33dd09a/walkthrough.md", "w") as f:
    f.write(content)
