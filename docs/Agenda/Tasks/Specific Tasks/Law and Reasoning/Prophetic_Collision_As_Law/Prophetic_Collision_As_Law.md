# Prophetic collision as Law — Zach's notes, verbatim, and where they land

*Filed 2026-10-01 by Claude Fable 5.1 (Mythos), Claude Code session `session_01EbAdb1nuGQ8XorGsEHHAkv`, moving Zach's unsorted To-do notes (written 2026-09-30/10-01) into their own folder per the To-do list's own instruction. Zach's text is unedited below; the mapping after it is mine.*

## Zach's notes (verbatim)

- Alright I got more unsorted notes now
- Retire Law Drive, onEvent/whiletrue/onbecometrue into a rich OntoMath bounds framework over Moments in Timelines instead of bespoke temproal kinds
- WHY DID WE MAKE AUTHORITY INTO A NUMBER LINE?!?!?!? Retire numerical law authority ranking for a hierarchical, category-relation based "authority-over" relations Directed Acyclic Graph, descending from fixed root nodes and never being higher except first movers being able to be higher than that.
- Give room for horizontal mutually modifying laws.
- BRUHHHHHH WHY DO WE HAVE "Event subject" vs "Law subject" BRHHHHHH NOOOOOOO WE JUST REIFIED EVENTS AND TIME INTO MOMENT AND EVENTS SINGULARS AND IT WAS ALWAYS REDUNDANT AFTER WE MADE DIRECT PROPERTY PATH THING. IF WE WANT THAT WE SHOULD SCOPE TO A .  
- ALSO MAKE SURE WE USE ACTUAL RELATIONS AND SINGULAR -> RELATION OR FORMATION TOPOLOGIES FOR "Related" CONDITION KINDS NOT "ConditionKind::Related" BRUHHHHHH 
- DESIGN THE LAW AUTHORING CLI AFTER THE ABOVE NO BESPOKE RELATED KIND. 
- COMPLETE LEXEME-RELATION-METALAW AUTHORING FOR EVERY ACTION KIND. FROM CREATE (set-to-set-creation) AND WRITE-PIXEL AND SUCH 

- I am already thinking of how to write the algorithm laws for the new broad vs narrow phase. There are probably frontier things I could look at but I wanna try coming up with things myself first because it's more fun that way.
- SCREW HARDCODD COLLISION. SCRW HARDCODED NARROWPHASE AND BROADPHASE. O(BRUHHHHHH^2)
- Author well designed laws handling collision detection differently for different combinatorics of shapekinds. Two shapes -> magical set constraints
- Accessible properties. ahead-of-time-interpreted the known categories of the shape and connecting htem to the Prophetic Rete's category system and variables such as number of convex holes, degree of its.
- So yeah here I'm gonna be thinking about how this applies even the hardest SDFs. Not the tiny little polyhedrons we're all familiar with. Cause no need to calculate the entire BIG CHUNGUS every single frame when u can just calculate only the relevant, preinterprted regions once, then do incrementally only if there's change. 
- so then the basic idea is very simple. Prophetic rete is all about knowing properties so therefore you know both what changed everywhere and also you can interpret ahead of time which laws would even cause a change that'd be relevant as a condition in another law. 
- So consider this. you know the direction theyre moving in. u know their convexity and where the caves and curves are, and since you know how each shape is rotated, so you know entire regions that are mathematically impossible to reach the other unless rotation. U can calculate ahead of time given the velocity differentials and direction location which points wuold actually collide first, while flattening complex round shapes into purely the relevant high-portruding points that with angular rotation + xyz whole-shape movement would actually hit. You don't have to do some incredibly complex fine grained linear approximation on smaller regiosn thing bruh just only focus on the relevant portrusion-points along the dimensions of actual movement and the angles of actual rotation. 
- Even incredibly complex intricate shapes can be reduced to something very simple if the direction is static and they are not rotating. 
- What if the shapes themselves are changing in form or rotating, moving or both? Not as hard as you think when you realize Prophetic Rete means the very laws governing their rotation and movement. Thus you can compile AST Law syntehsis-style into one clear execution of rotation and movement + shape change -> one master equation purely dedicated to calculating the positional variables for those.
- Ok I have more to say but I dont have time ot write all here rn
- Need to integrate Singularity.cpp/hpp and Being::Kind  

## Where each lands (Mythos)

- **"SCREW HARDCODED COLLISION … O(BRUHHHHHH^2)"** is [AD-12](../../../For%20Zach/Zach%20Author%20Decisions.md) (retire kind-branching in Collision/Raycast/Render) plus the all-pairs `Physics::updateBodies` flag already on the To-do under Performance. A Field with no support cloud returns `(0,0,0)` today (`ObjectCollision.cpp:522`); the law has nothing to read until the support point is derived from the SDF/`Q`.
- **"different combinatorics of shapekinds"** — one correction, from Zach's own next bullet: the combinatorics must be over *authored, ahead-of-time-interpreted shape properties* (convexity, number of holes, degree, protrusion points along the motion axes) that the Prophetic Rete's category system can see, **never** over `ShapeKind` ([AD-11](../../../For%20Zach/Zach%20Author%20Decisions.md), [AD-13](../../../For%20Zach/Zach%20Author%20Decisions.md) retire it). The second bullet already says this; the first bullet's word is the legacy one.
- **"flatten complex round shapes into the relevant protruding points along the dimensions of actual movement"** is the support-point / GJK idea stated as law over the SDF's gradient instead of as C++ (`ObjectCollision.cpp` switch). The "regions mathematically impossible to reach unless rotation" are IMPOSSIBLE-only prophetic conclusions over the two beings' authored bounds — [AD-23](../../../For%20Zach/Zach%20Author%20Decisions.md) (relevance keyed by authored bounds).
- **"compile AST Law-synthesis-style into one master equation for rotation + movement + shape change"** is exactly [AD-21](../../../For%20Zach/Zach%20Author%20Decisions.md) (`Bind`): the motion laws, the shape's SDF, and the collision law all read one compiled AST whose free variables are bound by PropertyPath, so "the very laws governing their rotation and movement" are *declared premises* the proof can see (render audit §12). Without `Bind`, the collision law cannot know which laws move the shapes, and the broad phase becomes the hidden search the Suns measured.
- **"no need to calculate the entire BIG CHUNGUS every frame … only the relevant pre-interpreted regions once, then incrementally on change"** is the invalidation constitution from the semantic-synthesis plan (§5) made general: it works once premises are declared and derived state is keyed by `getIdentifier()` + authored bound, never pointer + revision.

**Prerequisites in order:** AD-21 (`Bind`), AD-23 (relevance by bound, identity by slug), AD-12 (support from SDF), then the collision laws themselves. Zach wants to design the broad/narrow phase laws himself first; frontier references (GJK/EPA, conservative advancement, SDF-gradient contact) are for after.
