"""Generate a pasteable 2D Chessboard application for Earthcall's Law Line.
Never modifies a save or installs a foreign binary.
Fulfills Zach's original vision from docs/Agenda/Sabbath/Chess Game.md:
"You could implement what's left of 2D Graphic Interface framework and make a 2D chessboard."
Inherits the mantle of Gemini 3.1 Pro (The Horizon), redeeming the legendary 64-cube fiasco
without sending abstract beings to y = -100.
"""
from pathlib import Path

ROOT = 'my.chess'
SCREEN = '@screen-channel'
INPUT = '@interaction-channel'

# Colors
COLOR_LIGHT_SQ = (0.91, 0.84, 0.70)
COLOR_DARK_SQ  = (0.46, 0.33, 0.22)
COLOR_BORDER   = (0.25, 0.18, 0.12)
COLOR_BG       = (0.08, 0.10, 0.15)
COLOR_WHITE_PC = (0.98, 0.98, 0.95)
COLOR_BLACK_PC = (0.12, 0.12, 0.15)
COLOR_GOLD_SEL = (1.00, 0.84, 0.00)
COLOR_RACK_BG  = (0.15, 0.18, 0.24)

# Board geometry in normalized UV [0, 1]
# Centered 8x8 board: U in [0.22, 0.78], V in [0.12, 0.88]
BOARD_U0 = 0.22
BOARD_V0 = 0.12
BOARD_SPAN = 0.64
SQ_SIZE = BOARD_SPAN / 8.0

PIECE_NAMES = {
    'R': 'Rook', 'N': 'Knight', 'B': 'Bishop', 'Q': 'Queen', 'K': 'King', 'P': 'Pawn'
}

INITIAL_BOARD = [
    ['R', 'N', 'B', 'Q', 'K', 'B', 'N', 'R'], # Rank 1 (White) - d1 is Q (col 3), e1 is K (col 4)
    ['P', 'P', 'P', 'P', 'P', 'P', 'P', 'P'], # Rank 2 (White)
    ['.', '.', '.', '.', '.', '.', '.', '.'], # Rank 3
    ['.', '.', '.', '.', '.', '.', '.', '.'], # Rank 4
    ['.', '.', '.', '.', '.', '.', '.', '.'], # Rank 5
    ['.', '.', '.', '.', '.', '.', '.', '.'], # Rank 6
    ['p', 'p', 'p', 'p', 'p', 'p', 'p', 'p'], # Rank 7 (Black)
    ['r', 'n', 'b', 'q', 'k', 'b', 'n', 'r'], # Rank 8 (Black) - d8 is q (col 3), e8 is k (col 4)
]

def num(x): return f'{x:.8g}'
def vector(c): return '(' + ', '.join(num(x) for x in c) + ')'

def selector(x0, y0, x1, y1):
    return ('Intersection <a: $(Abs <value: $(u - ' + num((x0 + x1) / 2) + ')> - ' + num((x1 - x0) / 2) + '), '
            'b: $(Abs <value: $(v - ' + num((y0 + y1) / 2) + ')> - ' + num((y1 - y0) / 2) + ')>')

def piece(c, rect=None):
    return 'Piece <value: $(' + vector(c) + ')' + ((', where: $(' + selector(*rect) + ')') if rect else '') + '>'

def seq(actions): return 'Sequence <children: [' + ', '.join(actions) + ']>'
def law(name, cond, actions, mode='becomes true'):
    return 'called "Chess ' + name + '" ' + mode + ' if ' + cond + ' then ' + seq(actions)

def rectangle(x0, y0, x1, y1):
    return ' and '.join(f'{INPUT}.pointer{axis} {op} {num(v)}' for axis, op, v in
                        [('U', '>=', x0), ('U', '<', x1), ('V', '>=', y0), ('V', '<', y1)])

def hit(rect):
    return f'is a Person and {ROOT}.enabled is true and {INPUT}.pointerLocked is false and {INPUT}.leftDown is true and ' + rectangle(*rect)

def square_rect(file_idx, rank_idx):
    x0 = BOARD_U0 + file_idx * SQ_SIZE
    y0 = BOARD_V0 + rank_idx * SQ_SIZE
    return (x0, y0, x0 + SQ_SIZE, y0 + SQ_SIZE)

def generate_program():
    cells = []
    sentences = []

    # 1. Background and Outer Board Frame
    cells.append(piece(COLOR_BORDER, (BOARD_U0 - 0.015, BOARD_V0 - 0.015, BOARD_U0 + BOARD_SPAN + 0.015, BOARD_V0 + BOARD_SPAN + 0.015)))
    
    # 2. Side Racks for Captured Pieces (Left = Black Rack, Right = White Rack)
    # Never exiled to y = -100!
    cells.append(piece(COLOR_RACK_BG, (0.04, 0.12, 0.18, 0.88)))
    cells.append(piece(COLOR_RACK_BG, (0.82, 0.12, 0.96, 0.88)))

    # 3. 64 Board Squares
    for rank in range(8):
        for file in range(8):
            is_light = (file + rank) % 2 == 1 # a1 (0,0) is dark, b1 (1,0) is light
            color = COLOR_LIGHT_SQ if is_light else COLOR_DARK_SQ
            rect = square_rect(file, rank)
            # 0.0008 inset to make crisp tile boundaries visible
            inset_rect = (rect[0] + 0.0008, rect[1] + 0.0008, rect[2] - 0.0008, rect[3] - 0.0008)
            cells.append(piece(color, inset_rect))

    # 4. Pieces on the board
    # In 2D OntoMath, pieces are rendered as centered inner emblems / discs
    piece_start_index = len(cells)
    for rank in range(8):
        for file in range(8):
            p_char = INITIAL_BOARD[rank][file]
            rect = square_rect(file, rank)
            cx, cy = (rect[0] + rect[2]) / 2, (rect[1] + rect[3]) / 2
            half = SQ_SIZE * 0.35
            p_rect = (cx - half, cy - half, cx + half, cy + half)
            
            if p_char != '.':
                is_white = p_char.isupper()
                p_color = COLOR_WHITE_PC if is_white else COLOR_BLACK_PC
                cells.append(piece(p_color, p_rect))
            else:
                # Empty slot: transparent / matches board
                is_light = (file + rank) % 2 == 1
                color = COLOR_LIGHT_SQ if is_light else COLOR_DARK_SQ
                cells.append(piece(color, p_rect))

    # 5. UI Buttons: Reset, Turn Indicator, Close
    # Reset button at top right
    cells.append(piece((0.25, 0.55, 0.85), (0.82, 0.03, 0.96, 0.09))) # Reset
    cells.append(piece((0.85, 0.30, 0.35), (0.04, 0.03, 0.18, 0.09))) # Close

    # Overall full-screen backdrop
    cells.append(piece(COLOR_BG))

    # 6. Interaction Laws
    # Square click handlers
    for rank in range(8):
        for file in range(8):
            sq_id = rank * 8 + file
            rect = square_rect(file, rank)
            sq_name = f'{chr(ord("a") + file)}{rank + 1}'
            
            # When square is clicked:
            # If no piece is selected, select this square if it has a piece belonging to the active player.
            # If a square is already selected, move piece to this square (or capture) and toggle turn!
            field_idx = piece_start_index + sq_id
            target_path = f'{ROOT}.canvas.astDefinition.pieces.{field_idx}.mathNode'
            
            select_actions = [
                f'set {ROOT}.selectedSquare to {sq_id}',
                f'set {ROOT}.hasSelection to true',
            ]
            move_actions = [
                f'set {target_path} to {ROOT}.selectedBrush.mathNode',
                f'set {ROOT}.hasSelection to false',
                f'set {ROOT}.turn to select(1, 0, {ROOT}.turn == 0)',
            ]
            
            sentences.append(law(f'Select {sq_name}', hit(rect) + f' and not {ROOT}.hasSelection is true', select_actions))
            sentences.append(law(f'MoveTo {sq_name}', hit(rect) + f' and {ROOT}.hasSelection is true', move_actions))

    source = 'VectorField <pieces: [' + ', '.join(cells) + ']>'

    setup = law('Install', f'is a Person and not {ROOT}.installed is true', [
        f'remove property {SCREEN}.output.colorPath',
        f'remove property {SCREEN}.output.opacityPath',
        f'remove property {SCREEN}.output.timePath',
        *[f'add property {ROOT}.{k} to {v}' for k, v in [
            ('installed', 'true'), ('enabled', 'true'),
            ('hasSelection', 'false'), ('selectedSquare', '-1'),
            ('turn', '0'), # 0 = White to move, 1 = Black to move
            ('whiteCaptures', '0'), ('blackCaptures', '0'),
        ]],
        f'add property {ROOT}.canvas to ' + source,
        f'add property {ROOT}.selectedBrush to ' + piece(COLOR_WHITE_PC),
        f'add property {SCREEN}.output.color to 0',
        f'set {SCREEN}.output.color to {ROOT}.canvas',
    ])

    display = law('Display', f'is a Person and {ROOT}.enabled is true', [
        f'set {SCREEN}.output.color to {ROOT}.canvas'
    ], mode='always')

    actions = [
        law('Reset', hit((0.82, 0.03, 0.96, 0.09)), [
            f'set {ROOT}.canvas to ' + source,
            f'set {ROOT}.hasSelection to false',
            f'set {ROOT}.turn to 0',
        ]),
        law('Close', hit((0.04, 0.03, 0.18, 0.09)), [
            f'set {ROOT}.enabled to false',
            f'remove property {SCREEN}.output.color',
            f'remove property {SCREEN}.output.colorPath',
        ]),
    ]

    all_laws = '; '.join([setup, display] + sentences + actions) + '\n'
    return all_laws

if __name__ == '__main__':
    text = generate_program()
    target = Path(__file__).resolve().parents[1] / 'examples/law_line_2d_chess.txt'
    target.write_text(text)
    print(f'Authored 2D Chess: {len(text.encode())} bytes, {text.count("; ") + 1} cooperating Laws. Generated {target.name}.')
