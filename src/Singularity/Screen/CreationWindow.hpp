#pragma once

class Singular;
class Object;
class Zone;

namespace Rendering {

// The Creation Console — Object set-to-set creation (LAW_AND_CREATION_SYSTEM
// §7): assemble a source set, pick and choose which properties carry over and
// HOW they redistribute (per member / folded across the set, through linear
// or exact OntoMath transforms), then capture the concept and/or create the
// new set. Every transfer passes the Singularity TransferPolicy gate.
//
// `selected` is the 3D selection (source-set building + placement);
// `author` signs captured concepts; newborns join the Zone.
void renderCreationWindow(bool* open, Singular& author, Object* selected, Zone& zone);

} // namespace Rendering
