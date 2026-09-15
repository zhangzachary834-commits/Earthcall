// Terminal/Lexeme identity ergonomics witness.
//
// Zach's Terminal direction is word-first: a Person should meet "bank", not a
// UUID wall. But equal-spelled Lexemes remain distinct authored beings. This
// test guards the bridge between those truths: spelling is a convenience
// binding, while exact stable identity remains selectable and survives duplicate
// removal without stale aliases.

#include "Singularity/Language/LanguageSystem.hpp"

#include <cassert>
#include <iostream>
#include <sstream>
#include <string>

int main() {
    using Singularity::Language::LanguageSystem;

    auto& language = LanguageSystem::instance();
    language.clear();

    auto riverBank = language.intern("bank", "lexeme.bank.river");
    auto moneyBank = language.intern("bank", "lexeme.bank.money");

    assert(riverBank);
    assert(moneyBank);
    assert(riverBank != moneyBank);
    assert(language.findAllBySymbol("bank").size() == 2);

    // Human-facing spelling remains convenient, but ambiguity must not be
    // silent. The newest binding is preserved for compatibility and the
    // diagnostic exposes both exact identities.
    std::ostringstream diagnostics;
    auto* previousCerr = std::cerr.rdbuf(diagnostics.rdbuf());
    auto defaultBank = language.findBySymbol("bank");
    std::cerr.rdbuf(previousCerr);

    assert(defaultBank == moneyBank);
    const std::string diagnostic = diagnostics.str();
    assert(diagnostic.find("Ambiguous Lexeme spelling 'bank'") != std::string::npos);
    assert(diagnostic.find("lexeme.bank.river") != std::string::npos);
    assert(diagnostic.find("lexeme.bank.money") != std::string::npos);
    assert(diagnostic.find("@<exact-id>") != std::string::npos);

    // Exact references are frictionless through both ordinary IDs and the
    // Terminal-friendly @ syntax. Resolving an exact reference must never mint
    // a new Lexeme whose visible word is the identifier string.
    const size_t countBeforeExactResolve = language.getAll().size();
    assert(language.findBySymbol("lexeme.bank.river") == riverBank);
    assert(language.findBySymbol("@lexeme.bank.money") == moneyBank);
    assert(language.resolve("lexeme.bank.river") == riverBank);
    assert(language.resolve("@lexeme.bank.money") == moneyBank);
    assert(language.getAll().size() == countBeforeExactResolve);

    // Removing the default duplicate by exact identity must leave the other
    // same-spelled being reachable by the natural word. This is the stale-index
    // failure that an identity-aware CLI must never inherit.
    language.remove("@lexeme.bank.money");
    assert(language.findById("lexeme.bank.money") == nullptr);
    assert(language.findBySymbol("bank") == riverBank);
    assert(language.findAllBySymbol("bank").size() == 1);

    // Exact mutation reaches only the intended duplicate.
    riverBank->setConceptualWeight(0.25f);
    assert(language.findBySymbol("@lexeme.bank.river")->getConceptualWeight() == 0.25f);

    language.clear();
    std::cout << "language_identity_reference_test passed\n";
    return 0;
}
