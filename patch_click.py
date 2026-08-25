import re

with open("scripts/author_chess.py", "r") as f:
    text = f.read()

# Change law-chess-select to trigger on square-clicked
text = re.sub(
r'''    add_law\(
        "law-chess-select",
        "select-own-piece",
        0,
        \["object-clicked"\],''',
r'''    add_law(
        "law-chess-select",
        "select-own-piece",
        0,
        ["square-clicked"],''',
text)

with open("scripts/author_chess.py", "w") as f:
    f.write(text)

