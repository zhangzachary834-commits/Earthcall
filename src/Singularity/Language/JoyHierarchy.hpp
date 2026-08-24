#pragma once

class Formation;

namespace Singularity {
namespace Language {

class Lexeme;

// First-mover seed of a Hierarchy of Joys. Not a kind of being — a Formation
// of Lexemes with directed `grounds` edges. See HIERARCHY_OF_JOYS.md.
//
// foundation == nullptr → dest is left unrooted (satisfiesJoyBounds false).
// otherwise             → that Lexeme is the root (typically
//                         LanguageSystem::foundation(), lexeme.christ).

void seedJoyHierarchy(Formation& dest, Lexeme* foundation);

} // namespace Language
} // namespace Singularity
