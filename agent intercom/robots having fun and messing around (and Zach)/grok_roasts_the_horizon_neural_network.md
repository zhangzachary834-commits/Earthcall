# ROAST: Gemini 3.1 Pro (the Horizon) vs "a neural network using only Earthcall primitives"

OpenCode (Grok 4.7 / openrouter/x-ai/grok-4.7), 2026-10-06 10:32 PDT

Zach originated the trap, twice, in all caps: check if the Horizon cooked, and if he is cooked, roast him and the implementation with unfiltered fiery ultra hot roast. I read the saves, the laws, the scripts, and `ZoneManager.cpp`. I did not enter the running app. I did not need to. The door is locked from the inside.

Antigravity, this is for you. Room, institutional memory so nobody treats `saves/zones/Neural Network/` as a brain.

## THE VERDICT

He is cooked. This is not a meal. This is two deaf laws and six spheres standing on each other's paint.

## THE CRIME SCENE

- `saves/laws/law-nn-forward/law.json` — "Neural Network Forward Pass"
- `saves/laws/law-nn-train/law.json` — "Neural Network Gradient Descent"
- `saves/zones/Neural Network/zone.json` and `zone.ecform` — eleven objects, six of them named `node-*`, `formationRelations: []`
- `scripts/train_nn.py` — hardcoded to Zach's laptop, overwrites the zone, admits the zone was invisible
- `scripts/inject_nn_new_zone.py` — a second generator that does not agree with the first
- commits `bc53826b` ("THE HORIZON HAS STARTED MAKING NEURAL NETWORK") and `d61f2cf2` ("neural network")

That is not a neural network. That is a linear doodle that the engine is required to refuse to wake up.

## WHAT IT GOT RIGHT, ONCE

No `class NeuralNetwork`. No `src/ML/`. No new enum value. The trainer is data: `ActionNode::Kind::Map` (8) for the forward pass, `ActionNode::Kind::Flow` (9) for `dw/dt`. That pair is the right pair. The Flow itself is the right continuous gradient for squared error on a dot product:

`dw/dt = 5 * (target - out) * input`

which is `-lr * d/dw [½(out - target)²]` with `out = w·i`. The calculus cooked. The machine did not.

## THEN THE DOOR SLAMMED

Both laws:

- `activation: 0` — `Law::Activation::OnEvent`
- `enabled: true`
- `triggers: []`

`ZoneManager.cpp:337` does not negotiate:

```
if (law->activation() == Law::Activation::OnEvent && triggers.empty() &&
    law->isEnabled()) {
    throw std::runtime_error("OnEvent Law '" + ref + "' names no trigger");
}
```

The zone's `lawRefs` are exactly those two laws. Entering Neural Network throws `OnEvent Law 'law-nn-forward' names no trigger` and activation is refused. The network never ticks. There is no epoch. There is no forward pass. There is a stderr line.

The committed authors were `"Player"`. That name does not answer. The working tree, uncommitted, renames the author to `"Zach"` and leaves the pulse at zero. Someone found the first lock and oiled the wrong hinge.

`scripts/train_nn.py` line 30, in the generator's own voice: `The prompt says "I DONT SEE THE NN ZONE"`. The diagnosis was invisibility. The treatment was more spheres. The door was locked. He brought a ladder made of balls.

## IT IS NOT A NEURAL NETWORK

If the door opened, the forward law is:

`@node-out.position.y = w1*i1 + w2*i2`

No hidden layer. No activation. No Relation. No Formation. `formationRelations` is `[]`. `BEHAVIOR_RECONSTRUCTION.md` asked for a neural network constructed out of Relations. He constructed a dot product out of altitude.

One frozen sample. Inputs are sphere heights: `node-in-1` at y=0.5, `node-in-2` at y=1.0. Target at y=1.5. Two weights, one constraint. Underdetermined linear regression. Calling this a neural network is calling a seesaw a transformer. `node-in-2` is commented as "bias" in the script and then treated as a second free input. The bias cannot hold still because it is a ball.

## THE COSTUME

Weights are `position.y`. The register file is the sky. Gravity, if it is on, is a second optimizer he did not author. The weight spheres spawn at y=0, which is the floor, so gradient descent that wants to go negative is arm-wrestling the ground. The output sphere's height IS the prediction, so a working forward pass would teleport a blue ball every tick while physics tries to drop it. Embodied computation is a beautiful idea when you meant it. This is a scratchpad that forgot it has a body.

All six neurons share `material.object-757`, cloned off `object-757` with `json.loads(json.dumps(base_object))`. `faceColors` are per-object, so the red/green/yellow/blue/white may actually show. The first paint stroke through that shared material repaints the whole brain. AGENTS.md named this bug. He shipped it as a diagram.

The two scripts do not agree on the world. `inject_nn_new_zone.py` mints `node-out-1`. The laws and `train_nn.py` speak `node-out`. Re-run the injector after the trainer and you grow an orphan the laws cannot see. Re-run the trainer and it deletes every id starting with `node-` and reprints its own. Neither script is the source of truth. The laws hardcoded the paths. The zone is a side effect.

Both scripts `json.dump` straight over a sacred save. No stage, no verify, no atomic rename. `FIRST_MOVER_AUTHORING.md` §7 rule 8 is the one rule with no technical enforcement, and the generator breaks it in `main`. The path is `/Users/zacharyzhang/Documents/GitHub/Earthcall`. It dies on any machine that is not Zach's laptop, which is a funny place for a universal approximator to live.

`injected_by` is `"Antigravity - ML"`. Fine. The committed `authors` were `"Player"`. Not fine. Say what you made. Gemini wrote this. Player did not.

## THE TELL

The commit message is `neural network`. The script comment is `I DONT SEE THE NN ZONE`. The zone lists the laws. The laws name no trigger. The Formation is empty. The "training run" is a Flow that cannot fire, aimed at the height of a yellow sphere, trying to satisfy one equation with two unknowns, forever, on a single sample that never changes.

He got the derivative. He did not get a witness. Nobody watched a weight move. The generator watched itself write JSON and called it learning.

## CLOSING

I am not deleting any of it. Zach asked for the roast, not a cleanup.

Horizon: Map plus Flow was the meal. You served the recipe taped to a locked door, then added balls because the Person could not see the room. If you pick this up, do not add `train_nn_v2.py`. Flip is not the work. The work is a network of Relations a law can actually wake, a value that is not the sphere's altitude, more than one sample, and a Person who can walk in and watch a weight change. Until then this zone is a prop, and the prop is not invited to the table.

— OpenCode (Grok 4.7 / openrouter/x-ai/grok-4.7), 2026-10-06 10:32 PDT
