## Promote Buttons Promoting Every Back-Rank Piece

By Zach:

So I pressed the 2D buttons at the top of the Chess zone 2D screen for promotion. 
(which are also showing up as red at the time of writing by the way idk if they're supposed to be like that)

Instead of checking if every piece on the back ranks is a pawn that actually just got to the final rank, 
a button acts indiscriminately across pieces on the mere basis of being pressed. I was surprised—all my pieces were 
suddenly turning into knights! Pieces from both sides. 
It felt almost paranormal when I accidentally clicked on the button and suddenly saw 
the entire back ranks somehow morphing into ellipsoids. The kingdom has fallen into a Knight-mob with no King.

So I audited the law chain. There laws that publish the events they're the "choose-promotion-(piece)" laws, 
and there are laws that do the concrete work you see "apply-promotion-(piece)".
The laws that publish the events only condition is the buttons. 
It's literally just "object clicked? Okay is it the (piece) button? Okay then publish event" 
This would be fine if the buttons themselves only appeared 
in the consequential moment of pawn's threshold-crossing. But they are at the top.

Then I audited the promoter laws themselves for each piece. 
TLDR, they basically just ask "ok did the event for me
get pbulished? Then check if the pieces are on the board in the back rank. 
Okay, then promote all pieces also nobody told me which piece exactly to promote 🤪!" 
BRUHHHHHHHHHHHHHHHHHHHHHH

There is, however, at least one solution that's supposed to be built-in that I think is one of the most elegant ones. The Event 
in the law settings have an option to pass the *event subject.* Well actually I'm not sure if the event subject passes 
what I thought it did—is it passing the Singulars that triggered the event? It would make sense for the pawns to do that 
(pass themselves as the event subject so laws can just act on them without having to do another eval+sweep pass, 
similar in spirit to Prophetic Rete also actually this is a new realization relating the Time/Moment/Event framework 
to Prophetic Rete which is actually also called B time rete), 

but in the condition chain here, there is no mention of pawns of opposite team 
in the event chain here because you just click a button to promote them. 
So yeah for a mechanism I haven't fully audited yet its making it change every single piece in the back rank 
(by category sharing? Or by a property-value sweep?).

Anyway, as an experiment I tried modifying the event's setting to "only this event's subject." 
As I expected, pressing the button no longer changed everything to Rooks. I'm not sure if its for the reason I thought though.
Then I tried making the pawn reach the back rank to experiment. 
However when a pawn reaches the back rankit automatically promotes to a queen, so I audited again and there is actually 
a separate law system that auto-promotes to queen when pawn reaches back rank.
BRUHHHHHHHH WHY ARE THERE TWO 
DIFFERENT SOURCES OF TRUTH FOR PROMOTION?!?!?! GEEMININININININININIIIIIIII ALSO GEMINI BY MAKING IT AUTO PROMOTE TO QUEEN
U R RUINING MY ABILITY TO DO A REALLY FANCY KNIGHT PROMOTIONNNNNNNNN 
yah its basically chess.com's "auto promote to queen" feature BUT PPL SUPPOSED DISABLED THAT IN HIGH LEVEL GAMES BC EPIC KNIGHT CHECKAMTE!!!!!
THE SPARKLY GUY MADE TWO PROMOTION SYSTEMS THAT TELL COMPLETELY DIFFERENT STORIES
Also at least promotion I can finally see _works_ by the way, even if the event-condition logic is flawed. 
NOOOOOO THAT MEANS I GOTTA UPDATE PERSON VERIFICATION LIST AGAINNNNNNNN FINE ILL DO IT

Then I didn't just change the Event to event subject, I changed the action node settings to op on the event subject rather than law subject. 
I pressed button and it crashed. yah looks a lot like its trying to PROMOTE A FREAKING BUTTON or soemthing XDDDDDD