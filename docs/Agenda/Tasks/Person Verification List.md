# Person Verification List

- [x] Save → quit → reopen → Load
- [x] Verify saved objects persist
- [ ] Verify object properties persist → change an object's properties before saving → reload → verify the changed properties are still present
- [ ] Verify Relations persist → create/modify a relation between two objects → save → reload → verify the relation still exists

- [x] Save As → verify no crash
- [x] Save As → quit/reopen → Load → verify state

- [x] Load `my_world` in-app

- [ ] Create object through the intended Law path → use the intended Person-facing control to arm/trigger the creation Law → verify the new object appears
- [ ] Verify Law fires → object appears → trigger the creation Law and verify exactly one intended object is created
- [ ] Save → reload → find the same object by stable ID → select/interact with the created object before saving, reload, then verify it can still be identified/targeted as the same object

- [x] Home: create/modify object
- [x] Paint its FaceTexture
- [x] Save
- [x] Load another save
- [x] Return/walk to Home
- [x] Verify shape + paint survive

- [x] Creator Console: open it
- [x] Select object
- [ ] Morph object → select an existing object → activate Morph → modify its geometry using the available morph controls → verify the object's shape changes as intended
- [x] Face Brush
- [ ] Pottery → activate Pottery → create/use a pottery form on an object → verify the geometry changes as intended | Zach: My note in the todo list remains unfixed: Pottery successfully increases 3D dimensinos but stretches the FaceTextures to fit the new face dimensions rather than increasing the size of the facetexture image accordingly. 
- [ ] Rotate → select an object → activate Rotate → rotate the object → verify its orientation changes | Zach: changing the angle sliders on a selected shape while having rotation tool selected does not visibly change the shape. However, if you scroll to the bottom of the creator console window in 3D tool mode you'll see "Selection" with what seems to be the object ID. There are "Target Rotation" sliders that successfully rotate the shape. 
- [ ] Fuse Objects → create/select two objects → activate Fuse → fuse them → verify they become one fused object as intended | Zach: I mean I guess it executes, but it's not always clear what and sometimes it's weird and unclear if it's buggy or not. I need to investigate. 

- [x] 3D Create tool
- [x] Gyroid implicit: create one and visually verify it

- [x] Move cursor onto an interactive 3D object → verify hover behavior | Zach: What I mean precisely by the "x" is this—moving my mouse over a 3D object results in "object-hover-entered", visible in the Law Authoring Window's "Recent Events" log
- [x] Move cursor away from the interactive 3D object → verify hover-end behavior | Zach: same as above except for exited
- [x] Click an interactive 3D object → verify click behavior
- [x] With the Earthcall world focused, press and release a key → verify the corresponding key interaction is detected | Zach: caveat, so the events are logged in the "Recent Events" as just "key-released" and there's not a clear differentiator (at least visible to me in the pass I ran) for different keys.
- [x] Place cursor over an interactive 3D object → scroll the mouse wheel → verify scroll interaction is detected
- [x] Click and hold an interactive 3D object → move the mouse → release → verify drag interaction is detected | Zach: so it both says "object drag started/ended" and "object pressed/released"  
- [ ] Focus the Earthcall window → verify focus behavior | Zach: I don't know what I'm supposed to be looking for here:
- [ ] Unfocus the Earthcall window → verify unfocus behavior | Zach: I don't know what I'm supposed to be looking for here:
- [x] Open an Earthcall UI window → verify the mouse pointer unlocks and can interact with the UI
- [ ] Close the UI window → verify normal 3D-world pointer interaction returns | Zach: I don't know what I'm supposed to be looking for here:

- [ ] Chess: click a pawn on the chess board | Zach: I tried clicking and it did not do anything visible. Most other functionality below can't be tested unless this is working.
- [ ] Select pawn → click a pawn and verify it becomes the selected piece | Zach: How am I supposed to tell it's the selected piece?
- [ ] Make legal move → select a pawn and click a legal destination square → verify the pawn moves there
- [ ] Capture piece → make a legal capture → verify the captured piece is removed/moved appropriately
- [ ] Try illegal move → select a piece and click an illegal destination → verify the move is rejected and the piece remains in its original position
- [ ] Test board path blocking → use a sliding piece such as a rook/bishop/queen with a piece blocking its path → attempt to move through the blocker → verify the move is rejected

- [ ] First Mover/Law toggle: disable → save → reload → verify disabled
- [ ] Re-enable → verify action works → re-enable the First Mover/Law → trigger its corresponding action → verify it executes

- [ ] Look through the Observe Test feature. Zach: Only four tests show despite their being 60+ tests at the time of writing. Two of them are epistemically opaque. One does not load at all (the patch test) it throws an error. Only one of the tests displays something—spanws cubes throughout the place. I just can't verify without looking at the code and an in-world tool that lets me see positions whether they were supposed to be in that exact position.   

- [ ] Law Author: inspect/create/edit a Law → open Law Author → inspect an existing Law → create or edit a Law → verify the displayed Law is correct
- [ ] Save → reload → verify Law persists → save the Law/world → reload → reopen Law Author → verify the Law and its configuration remain

- [x] Assets window: open → verify assets → save/load → verify again
- [x] Chat window → open Chat → send a test message → verify it appears correctly
- [ ] ImGui Demo → open ImGui Demo → interact with at least one visible demo control → verify it responds
- [x] Controls/Keymap (`K`) → press `K` → verify the Controls/Keymap window opens → verify controls are displayed | Zach: Keybinds are not exhaustive. For example, 
- [x] F8 → press F8 → verify the intended F8 action occurs
- [x] F9 → press F9 → verify the intended F9 action occurs
- [ ] `/` → press `/` → verify the intended `/` action occurs | Zach: I tried this with no windows loaded and "/" changed nothing visible. The intended behavior is unclear to me.
- [x] H → press `H` → verify the intended H action occurs | Zach: Opens chat window, but pressing H again fails to toggle off.
- [x] K → press `K` → verify the intended K action occurs | 
## Synthesis Studio (added 2026-09-02, from the play-test that corrected the audit's first pass)
- [ ] **2D controls on a HiDPI/Retina display** → open the Synthesis Studio → click any 2D HUD dock control → verify it responds. **Known blocking finding (Zach's play-test, 2026-09-02):** every 2D control is unclickable on a Retina Mac — the 2D draw and the 2D pick are in different coordinate spaces. See [SYNTHESIS_STUDIO_AUDIT_2026-09-02.md](../../audits/SYNTHESIS_STUDIO_AUDIT_2026-09-02.md) §A0. Re-verify here after the fix, on both a Retina and a non-Retina display.
- [ ] 3D console → click the two buttons and the four chord pads → verify orbs spawn and notes sound (audit reports every sound in the studio is currently silent)
- [ ] Slider → set a value → save → reload → verify the slider does not teleport on load
- [ ] Ambient theme toggle → toggle on → toggle off → verify it is not a one-way latch

*Note (2026-09-02): the HiDPI finding above was found by walking, not by a test, and was
recorded first in an audit rather than here. Findings a Person discovers by hand belong on
this list — that is what it is for. See [The Week the Chorus Became a Queue](../../Reflections%20on%20Earthcall%27s%20Progression/Reflections%20on%20Trajectory/The_Week_The_Chorus_Became_A_Queue.md) §6.*
