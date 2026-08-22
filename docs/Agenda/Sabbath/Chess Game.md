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
- Bruh, the freaking board. GEMINI WHY DIDNT U MAKE A LONG RECTANGULAR PRISM WITH DIMESNIONS OF 8t BY 8t BY D (d for arbitrary depth)?!?!?!
- The pieces actually have to be on the platform in their correct starting locations on loading, not just stuffed into the same square.
- No need for advanced colllision or raytracing logic here (except the usual mouse-clicking and rendering, obviously), because that assumes 3D chess must necessarily operate according to. If the goal is to create a MVP of a chess app, the Laws simply need to hard-anchor them to coordinates.
- Just this simple approach, and no-clip is fine because collision is not strictly required for a chess game to work: Remember what literally every chess website online does: either click on the piece/square hte piece is in  and drag it all the way to occupy the new square or capture another piece, or click the square and then click move-to location. 
- Idle placement of the center of each piece's relative coordinates to the board and within their squares remain constant     
- Finally, a rule enforcer that constricts move allowance depending on if you're in check and allows no further moves if checkmate or stalemate.
- THERE?!??!!? U GUYS R ULTRA SMART AIS HOW U NOT THINK OF THIS?!?!??!! BRUHHHHHH U R STUFFING ALL  THE PIECES EITHER IN FOUR CORNERS OR AS A BLOB OF SQUARES AND SPHERES AND CONES LMAOOOO

PLZZZZZ THIS IS TOTALLY NOT BECAUSE IM DESPERATE FOR MY CHESS IDOLS KASPAROV AND PIA CRAMLING AND LEVY ROZMAN TO PLAY CHESS ON EARTHCALLLLALALALLAL

...no one saw me write that even though this is a public github repo 