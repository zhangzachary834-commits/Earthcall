import re

with open("scripts/author_chess.py", "r") as f:
    text = f.read()

# 1. Update path_blocked functions to use @event.subject
text = text.replace('operand_path="@state.chess.sliderRankX"', 'operand_path="@event.subject.gridX"')
text = text.replace('operand_path="@state.chess.sliderFileY"', 'operand_path="@event.subject.gridY"')
text = text.replace('operand_path="@state.chess.sliderDiagX"', 'operand_path="@event.subject.gridX"')
text = text.replace('operand_path="@state.chess.sliderDiagY"', 'operand_path="@event.subject.gridY"')

# 2. Update candidate laws to just publish with subject="" (i.e. omit "state.chess")
# law-chess-check-slider-rank-candidate
text = re.sub(
r'''        seq\(
            map_path\("@state.chess.sliderRankX", \{"gx": "gridX"\}, copy_terms\("gx"\)\),
            map_path\("@state.chess.sliderRankY", \{"gy": "gridY"\}, copy_terms\("gy"\)\),
            set_path\("@state.chess.sliderRankActive", pv\("bool", True\)\),
            publish\("slider-rank-scanned", "state.chess"\),
        \)''',
r'''        publish("slider-rank-eval")''',
text)

# law-chess-check-slider-file-candidate
text = re.sub(
r'''        seq\(
            map_path\("@state.chess.sliderFileX", \{"gx": "gridX"\}, copy_terms\("gx"\)\),
            map_path\("@state.chess.sliderFileY", \{"gy": "gridY"\}, copy_terms\("gy"\)\),
            set_path\("@state.chess.sliderFileActive", pv\("bool", True\)\),
            publish\("slider-file-scanned", "state.chess"\),
        \)''',
r'''        publish("slider-file-eval")''',
text)

# law-chess-check-slider-diag-candidate
text = re.sub(
r'''        seq\(
            map_path\("@state.chess.sliderDiagX", \{"gx": "gridX"\}, copy_terms\("gx"\)\),
            map_path\("@state.chess.sliderDiagY", \{"gy": "gridY"\}, copy_terms\("gy"\)\),
            set_path\("@state.chess.sliderDiagActive", pv\("bool", True\)\),
            publish\("slider-diag-scanned", "state.chess"\),
        \)''',
r'''        publish("slider-diag-eval")''',
text)

# 3. Update eval laws to trigger on the new events and remove the 'Active' checks
# Since the target is now the piece that triggered it, we can just use category.chess.piece (or default target which is all pieces, but identity("@event.subject") isolates it to the slider)

# law-chess-eval-slider-rank
text = re.sub(
r'''    add_law\(
        "law-chess-eval-slider-rank",
        "eval-slider-rank",
        0,
        \["slider-rank-scanned"\],
        all_of\(
            identity\("state.chess"\),
            compare\("sliderRankActive", 0, pv\("bool", True\)\),
            not_of\(path_blocked_slider_rank\(\)\),
        \),''',
r'''    add_law(
        "law-chess-eval-slider-rank",
        "eval-slider-rank",
        0,
        ["slider-rank-eval"],
        all_of(
            identity("@event.subject"),
            not_of(path_blocked_slider_rank()),
        ),''',
text)

# law-chess-eval-slider-file
text = re.sub(
r'''    add_law\(
        "law-chess-eval-slider-file",
        "eval-slider-file",
        0,
        \["slider-file-scanned"\],
        all_of\(
            identity\("state.chess"\),
            compare\("sliderFileActive", 0, pv\("bool", True\)\),
            not_of\(path_blocked_slider_file\(\)\),
        \),''',
r'''    add_law(
        "law-chess-eval-slider-file",
        "eval-slider-file",
        0,
        ["slider-file-eval"],
        all_of(
            identity("@event.subject"),
            not_of(path_blocked_slider_file()),
        ),''',
text)

# law-chess-eval-slider-diag
text = re.sub(
r'''    add_law\(
        "law-chess-eval-slider-diag",
        "eval-slider-diag",
        0,
        \["slider-diag-scanned"\],
        all_of\(
            identity\("state.chess"\),
            compare\("sliderDiagActive", 0, pv\("bool", True\)\),
            not_of\(path_blocked_slider_diagonal\(\)\),
        \),''',
r'''    add_law(
        "law-chess-eval-slider-diag",
        "eval-slider-diag",
        0,
        ["slider-diag-eval"],
        all_of(
            identity("@event.subject"),
            not_of(path_blocked_slider_diagonal()),
        ),''',
text)


with open("scripts/author_chess.py", "w") as f:
    f.write(text)

