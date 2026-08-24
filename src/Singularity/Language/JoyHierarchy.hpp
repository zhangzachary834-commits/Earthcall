#pragma once

#include <string>

class Formation;

namespace Singularity {
namespace Language {

// First-mover seed of a Hierarchy of Joys. Not a kind of being — a Formation
// of Lexemes with directed `grounds` edges. See HIERARCHY_OF_JOYS.md.
//
// `foundationSymbol` empty  → dest is left unrooted (satisfiesJoyBounds false).
// "default" / "strict"      → root is the first-mover foundation Lexeme
//                             (lexeme.christ). God as ordering, not as icon.
// any other symbol          → that Lexeme is authored as the root.

    // "const std::string& foundationSymbol" NOT STRING. LEXEME. - Zach
void seedJoyHierarchy(Formation& dest, const std::string& foundationSymbol);

} // namespace Language
} // namespace Singularity
