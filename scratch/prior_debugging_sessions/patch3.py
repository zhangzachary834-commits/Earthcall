import re

with open("scripts/author_chess.py", "r") as f:
    text = f.read()

# law-chess-eval-slider-rank
text = re.sub(
r'''    add_law\(
        "law-chess-eval-slider-rank",
        "eval-slider-rank",
        0,
        \["slider-rank-eval"\],
        all_of\(
            identity\("@event.subject"\),
            not_of\(path_blocked_slider_rank\(\)\),
        \),''',
r'''    add_law(
        "law-chess-eval-slider-rank",
        "eval-slider-rank",
        0,
        ["slider-rank-eval"],
        not_of(path_blocked_slider_rank()),''',
text)

# law-chess-eval-slider-file
text = re.sub(
r'''    add_law\(
        "law-chess-eval-slider-file",
        "eval-slider-file",
        0,
        \["slider-file-eval"\],
        all_of\(
            identity\("@event.subject"\),
            not_of\(path_blocked_slider_file\(\)\),
        \),''',
r'''    add_law(
        "law-chess-eval-slider-file",
        "eval-slider-file",
        0,
        ["slider-file-eval"],
        not_of(path_blocked_slider_file()),''',
text)

# law-chess-eval-slider-diag
text = re.sub(
r'''    add_law\(
        "law-chess-eval-slider-diag",
        "eval-slider-diag",
        0,
        \["slider-diag-eval"\],
        all_of\(
            identity\("@event.subject"\),
            not_of\(path_blocked_slider_diagonal\(\)\),
        \),''',
r'''    add_law(
        "law-chess-eval-slider-diag",
        "eval-slider-diag",
        0,
        ["slider-diag-eval"],
        not_of(path_blocked_slider_diagonal()),''',
text)


with open("scripts/author_chess.py", "w") as f:
    f.write(text)

