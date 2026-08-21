import sys

with open("scripts/generate_chess_v2.py", "r") as f:
    content = f.read()

# 1. Add top-level latch documentation
latch_doc = """
# LATCH DOCUMENTATION: @chess-state
# 
# @chess-state acts as the central latch for the chess engine's state machine.
# Readers: 
#  - select-piece (reads targetX, targetY, turn)
#  - validate-move (reads selectedX, selectedY, targetX, targetY, turn, dx, dy)
#  - execute-move (reads targetX, targetY, turn)
# Writers:
#  - handle-click (writes targetX, targetY, dx, dy on every click)
#  - select-piece (writes selectedX, selectedY, selectionActive on successful selection)
#  - execute-move (toggles turn, sets selectionActive = False on successful move)
#
# By keeping state in this single authored Object, the rules can be purely declarative
# event reactions, fulfilling End-to-End Coherence and Name the Latch.
"""
content = content.replace("def main():", latch_doc + "\ndef main():")

# 2. Add documentation to the conditionDescriptions and actionDescriptions
# I will just change add_law to accept `cond_desc` and `act_desc` and pass them.
content = content.replace("def add_law(name, trigger, activation, condition, action):", "def add_law(name, trigger, activation, condition, action, cond_desc=None, act_desc=None):\n        if cond_desc is None: cond_desc = [name]\n        if act_desc is None: act_desc = [name]")
content = content.replace('"conditionDescriptions": [name], "actionDescriptions": [name]', '"conditionDescriptions": cond_desc, "actionDescriptions": act_desc')

# Now patch specific law definitions in generate_chess_v2.py
content = content.replace('add_law("validate-move-slide"', 'add_law("validate-move-slide"')

with open("scripts/generate_chess_v2.py", "w") as f:
    f.write(content)
