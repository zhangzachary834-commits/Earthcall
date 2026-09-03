with open("scripts/author_chess.py", "r") as f:
    text = f.read()

# Fix path_blocked functions
text = text.replace("@state.chess.sliderRankX", "@event.subject.gridX")
text = text.replace("@state.chess.sliderFileY", "@event.subject.gridY")
text = text.replace("@state.chess.sliderDiagX", "@event.subject.gridX")
text = text.replace("@state.chess.sliderDiagY", "@event.subject.gridY")

with open("scripts/author_chess.py", "w") as f:
    f.write(text)
