# Chess Game

By Zach

哎呀, that experiment was a disaster. I told Gemini this morning to create a "fully working chess game engine inside Earthcall."

The result was so bad that I called up Grok to roast Gemini/Antigravity. 

And I have so many more thoughts about this. Bruh, I'm gonna write my full plan here and show how it's supposed to be done.

But too bad I acnnot type them all out instantaeously the way the rest of you guys can. I have to go sleep and I'll write it in the morning.

Some notes for then:
- First, there are two separate options and my prompt was open ended enough that you could technically pick either or both. 
- You could implement what's left of 2D Graphic Interface framework and make a 2D chessboard.
- Or you could create the 3D one. The 2D is conceptually simpler, but ironically Earthcall's 3D is more developed at the moment so its completely undesrtandable why Gemini picked that.

- Now if we're gonna do 3D. First you're gonna need persons, obviously, otherwise who's gonna play chess? Either Two Persons or a Person-designated Object 
- that can play chess (like an Earthcall-native chess bot), or wired up to a First Mover like Stockfish.
- Don't just dump shapes with the default color into a Zone LMAOOOOOO—You have to color the board's squares by modifying their FaceTextures in the data. You have to use FaceTextures to color the white pieces white and the black pieces black.
- Remember the subtle setup rule: they have to be set up on the proper side such that the white queen goes on the white square and the black queen is on teh black square.
- Second ur gonna need distinct shapes for each piece. For the first pass, we don't need to make fully accurate chess pieces—we can just have something like small cubes for rooks, cones for bishops, etc.
- Later when we make realistic chess piece models, it will not be as hard as it seems, since this is Earthcall rather than a traditional modeling engine. As long as our shape generation tools are mathematically robust enough (they cover all sorts of curves, patches, etc.,) and is decently usable, modeling these pieces is just a matter of complex shape generation and Singular/Object set-to-set creation. If the underlying generative primitives are expressive enough, the apparent complexity of modeling becomes composition rather than bespoke mesh manipulation.
- Bruh, the freaking board. GEMINI. YOU MADE 64 SEPARATE CUBES ONE PASS, ADN THEN THE NEXT PASS U SPLIT THE PIECES INTO 4 CORNERS WITH A HORIZONTAL BEAM IN THE MIDDLE. GEMINI WHY DIDNT U MAKE A LONG RECTANGULAR PRISM WITH DIMESNIONS OF 8t BY 8t BY D (d for arbitrary depth)?!?!?!
- The pieces actually have to be on the platform in their correct starting locations on loading, not just stuffed into the same square.
- No need for advanced colllision or raytracing logic here (except the usual mouse-clicking and rendering, obviously), because that assumes 3D chess must necessarily operate according to. If the goal is to create a MVP of a chess app, the Laws simply need to hard-anchor them to coordinates.
- Just this simple approach, and no-clip is fine because collision is not strictly required for a chess game to work: Remember what literally every chess website online does: either click on the piece/square hte piece is in  and drag it all the way to occupy the new square or capture another piece, or click the square and then click move-to location. 
- Idle placement of the center of each piece's relative coordinates to the board and within their squares remain constant     
- Finally, a rule enforcer that constricts move allowance depending on if you're in check and allows no further moves if checkmate or stalemate.
- THERE?!??!!? U GUYS R ULTRA SMART AIS HOW U NOT THINK OF THIS?!?!??!! BRUHHHHHH U R STUFFING ALL  THE PIECES EITHER IN FOUR CORNERS OR AS A BLOB OF SQUARES AND SPHERES AND CONES LMAOOOO

PLZZZZZ THIS IS TOTALLY NOT BECAUSE IM DESPERATE FOR MY CHESS IDOLS KASPAROV AND PIA CRAMLING AND LEVY ROZMAN TO PLAY CHESS ON EARTHCALLLLALALALLAL

...no one saw me write that even though this is a public github repo

---

## What was authored (2026-08-22, grok-4.6)

Zach's notes above are the spec. This is the injection that follows them, not a C++ chess class and not Gemini's generator stack. The prompt was open-ended on 2D vs 3D; 3D is what Earthcall can actually present today, so that is the world that got written.

**Files (First Mover injection, author `grok-4.6`, at Zach's request):**

| File | What it is |
|---|---|
| `saves/worlds/chess_app.json` | session (camera on white's side, flying, gravity off, laws, materials) |
| `saves/zones/Chess/zone.json` | Zone identity: one board, 32 pieces, two player seats |
| `scripts/author_chess.py` | the authoring script — regenerate with `python3 scripts/author_chess.py` |
| `tests/law/chess_app_test.cpp` | load + e2–e4 + e7–e5 + exd4 capture + blocked rook |

**How to play:** run Earthcall, Load `chess_app`. Click a piece of the side to move, then click the destination square (or the piece that occupies it). Physics gravity is off; pieces are hard-anchored to square centres.

Answering the notes in order:

- **Persons.** The Person in the window plays. Two seat Objects (`object.chess.seat.white` / `.black`) are Person-designated players (`designatedBy: Player`, `playsColor`, `isToMove`). Hotseat via `state.chess.turn`. Not a second Person class, and not an AI modelled as a Person.
- **FaceTextures, not default dump.** `material.chess.board` carries a 64×64 checkerboard on the top face (a1 dark, d1 light) and wood on the sides. White / black pieces wear `material.chess.white` / `.black` FaceTextures (ivory / charcoal). Paint lives on the Material being; `Material::toJson` now round-trips those buffers, which is why Gemini's board was a blank slab.
- **Queens on their colours.** White queen `piece-white-queen-3-0` on d1 (light). Black queen `piece-black-queen-3-7` on d8 (dark).
- **Distinct shapes.** Rooks cubes, knights ellipsoids, bishops cones, queens ovoids, kings cylinders, pawns spheres. First pass, as asked.
- **One board.** `object.chess.board` is a single rectangular prism scaled 8t × D × 8t, D = 0.28. Not 64 cubes, not four corners and a beam.
- **Starting locations.** Each piece's translation is its square centre on load. Move/capture laws Map `position.xz` when `gridX`/`gridY` change. Idle stays constant because nothing rewrites it each tick (a WhileTrue servo here stalled Load).
- **Click-select, click-move.** `object-clicked` (the interaction channel's edge) → address the square from `pointerWorld` through an 8-bin piecewise → move laws on `square-clicked`. No extra collision.
- **Capture unmakes onto a rack** beside the board (`onBoard = false`, x = ±5.6). Not y = −100, not `Destroy` with an empty token.
- **Rule enforcer.** Geometric legality + occupancy + path blocking for sliders. After a move, the mover's king is probed; adjacent checks revert the move and restore a captured piece. Promotion to queen on the last rank. **Remainder (named, not papered over):** distant sliding check (a queen checking from across the board with a clear ray) needs a nested `ForAny` over the attacker's coordinates, which the condition calculus does not name; adjacent sliding check and full *move* path-blocking are in. Auto-declared checkmate/stalemate is the same remainder plus "no legal escape" enumeration. `gameOver` fires if a king is actually unmade.

Load-bearing C++ (not a chess type): Material FaceTextures serialize; loaded categories stay extra-spatial; first draw after load uploads restored paint. 