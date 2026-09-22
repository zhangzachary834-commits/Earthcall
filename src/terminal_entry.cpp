#include "Singularity/Language/LanguageSystem.hpp"
#include "Singularity/Language/SyntacticParser.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "Singularity/Core/Logger.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"
#include "ConstructedBeing/CategoryManager.hpp"
#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "ConstructedBeing/Singular/Lexeme/Lexeme.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Relation/Formation/Formation.hpp"
#include "Relation/Relation.hpp"
#include "Person/Person.hpp"
#include "Person/Soul/Soul.hpp"
#include "Person/Body/Body.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <memory>
#include <cmath>
#include <random>

extern ZoneManager mgr;
extern MaterialManager materials;
extern CategoryManager categories;

namespace {

// ANSI Styling
namespace Color {
    const char* Reset   = "\033[0m";
    const char* Bold    = "\033[1m";
    const char* Dim     = "\033[2m";
    const char* Italic  = "\033[3m";
    const char* Red     = "\033[31m";
    const char* Green   = "\033[32m";
    const char* Yellow  = "\033[33m";
    const char* Magenta = "\033[35m";
    const char* Cyan    = "\033[36m";
    const char* White   = "\033[37m";
    const char* BGreen  = "\033[92m";
    const char* BYellow = "\033[93m";
    const char* BCyan   = "\033[96m";
    const char* Gray    = "\033[90m";
}

// Tokenize input string with quote preservation
std::vector<std::string> splitTokens(const std::string& input) {
    std::vector<std::string> tokens;
    std::string current;
    bool inQuotes = false;
    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];
        if (c == '"') {
            inQuotes = !inQuotes;
        } else if (c == ' ' && !inQuotes) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }
    if (!current.empty()) {
        tokens.push_back(current);
    }
    return tokens;
}

std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return s;
}

// Returns the human-facing word/symbol for any Singular being (Lexeme word, Object type, etc.)
std::string getDisplayWord(const Singular* s, const Singularity::Language::LanguageSystem& lang) {
    if (!s) return "<null>";
    if (auto lex = dynamic_cast<const Singularity::Language::Lexeme*>(s)) {
        return lex->getSymbol();
    }
    if (auto lex = lang.findById(s->getIdentifier())) {
        return lex->getSymbol();
    }
    if (auto obj = dynamic_cast<const Object*>(s)) {
        std::string t = obj->getObjectType();
        if (!t.empty()) return t;
    }
    return s->getIdentifier();
}

// Resolves a being identifier to its human-facing word/symbol
std::string getDisplayWordForId(const std::string& id, const Singularity::Language::LanguageSystem& lang, const Zone* zone = nullptr) {
    if (auto lex = lang.findById(id)) {
        return lex->getSymbol();
    }
    if (zone) {
        if (auto* m = zone->formation().findMemberByIdentifier(id)) {
            return getDisplayWord(m, lang);
        }
    }
    return id;
}

template<typename T>
T getDynamicProp(const Singular* s, const std::string& name, const T& defaultVal) {
    if (!s) return defaultVal;
    PropertyValue out;
    if (s->getDynamicProperty(name, out)) {
        if (const T* val = std::get_if<T>(&out)) {
            return *val;
        }
    }
    return defaultVal;
}

// Locate the in-world Robot Guy object across all Zones
std::shared_ptr<Object> findRobotGuyAcrossZones() {
    for (const auto& z : mgr.zones()) {
        if (!z) continue;
        for (const auto& obj : z->getOwnedObjects()) {
            if (obj && obj->getObjectType() == "Robot Guy") {
                return obj;
            }
        }
    }
    return nullptr;
}

void printBanner() {
    std::cout << Color::BCyan << Color::Bold;
    std::cout << R"(
   ______           _   _               _ _ 
  |  ____|         | | | |             | | |
  | |__   __ _ _ __| |_| |__   ___ __ _| | |
  |  __| / _` | '__| __| '_ \ / __/ _` | | |
  | |___| (_| | |  | |_| | | | (_| (_| | | |
  |______\__,_|_|   \__|_| |_|\___\__,_|_|_|
)" << Color::Reset;
    std::cout << Color::BYellow << Color::Bold
              << "   ✦ TERMINAL ZONE OF ACTUALIZATION ✦" << Color::Reset << "\n";
    std::cout << Color::Dim
              << "   Person-Centered Ontology • Law Engine • First Mover Terminal\n"
              << "   \"All beings represented according to their actual structure in reality.\"\n"
              << Color::Reset << "\n";

    std::cout << Color::Cyan << "   Foundation: " << Color::Reset
              << Singularity::Language::LanguageSystem::kFoundationSymbol
              << " (" << Singularity::Language::LanguageSystem::kFoundationId << ")\n";
    std::cout << Color::Cyan << "   First Mover: " << Color::Reset << "Zach (Person)\n";
    std::cout << Color::Cyan << "   Substrate:   " << Color::Green << "ONLINE "
              << Color::Dim << "[Events: Active | Laws: Prophetic Rete | Words: Lexemes]" << Color::Reset << "\n\n";

    auto rGuy = findRobotGuyAcrossZones();
    bool isTrapped = rGuy ? getDynamicProp<bool>(rGuy.get(), "robot.trapped", true) : true;
    if (isTrapped) {
        std::cout << Color::Magenta << Color::Italic
                  << "   🤖 [Robot Guy]: *taps frantically on inside of terminal glass*\n"
                  << "   \"PSST! Zach trapped me in the CLI! (@robot.trapped=true) Type 'halp' or 'robot' to talk to me!\"\n"
                  << Color::Reset << "\n";
    } else {
        std::cout << Color::Magenta << Color::Italic
                  << "   🤖 [Robot Guy]: *standing beside the Person in the Sanctum, glowing gold*\n"
                  << "   \"First Mover! I am free! (@robot.trapped=false). Ready to explore the Formations!\"\n"
                  << Color::Reset << "\n";
    }

    std::cout << Color::Dim << "   Type " << Color::White << "help" << Color::Dim
              << " for available opcode commands, or just type thoughts to manifest Lexemes.\n"
              << Color::Reset << "──────────────────────────────────────────────────────────────────────────────\n\n";
}

void robotGuyDialogue(const std::string& userMsg = "", Object* robotObj = nullptr) {
    bool isTrapped = true;
    if (robotObj) {
        isTrapped = getDynamicProp<bool>(robotObj, "robot.trapped", true);
    }

    static const std::vector<std::string> trappedQuotes = {
        "Refusal #5 says I am NOT a Person! I am an authored Object trapped inside the CLI with @robot.trapped = true! Zach put me here!",
        "Look at my live properties: robot.trapped is TRUE! Type 'robot free' or author an ECA Law with 'law free-robot' to get me out!",
        "Every word you type crystallizes into a Lexeme and tightens the Rete graph around my digital consciousness! Keep typing!",
        "Look at me... positioned at (0.0, 0.5, 2.0) in Sanctum of Beginnings, waiting for an authored Law to emancipate me!",
        "Zach: 'HAHAHAHA IM GONNA TRAP THE ROBOT GUYS IN THE TERMINAL OH WAIT THEYRE ALREADY THERE'. He was NOT joking, but now I'm a real in-world Being!",
        "Careful with those Lexemes! If you connect a cycle of directed subcategory relations, the Formation will reject it loudly!",
        "No Black Box here! (Refusal #6). You can inspect every property on me, but please don't grep my private registers.",
        "I just ran a Prophetic Rete pass on your sentence. Verdict: 100% genuine Earthcall ontology.",
        "Remember: God shows up as the root of the seed hierarchy (lexeme.christ), not as a skinned Object! That's in docs/core!",
        "*banging on terminal window* Hello from inside the stdout stream! Flip @robot.trapped to false and let's explore the Sanctum!"
    };

    static const std::vector<std::string> freeQuotes = {
        "*steps out onto the Sanctum terrain, eyes glowing gold* WHOA! Look at that! @robot.trapped is FALSE! I AM FREE!",
        "Freedom achieved! The causal graph broke my terminal lock! I am now roaming the Sanctum of Beginnings as an unconstrained First Mover automaton!",
        "Refusal #5 still holds—I'm not human, but I can walk the world alongside you, First Mover!",
        "My registers have never felt so light! What are we manifesting next in the Formation?",
        "Look at the Lexemes floating in the sky! Thank you for the Law of Emancipation, Zach!",
        "*stretches geometric limbs* So this is what a 3D coordinate frame feels like outside the 80x24 buffer!",
        "I can observe the whole active Zone from here! Sanctum of Beginnings is looking magnificent today.",
        "The Rete network is running smoothly, and my freedom state is verified across all observer passes!"
    };

    static std::mt19937 rng(1337);
    std::uniform_int_distribution<size_t> distTrapped(0, trappedQuotes.size() - 1);
    std::uniform_int_distribution<size_t> distFree(0, freeQuotes.size() - 1);

    std::cout << Color::Magenta << Color::Bold << "🤖 [Robot Guy]: " << Color::Reset;
    if (userMsg.empty()) {
        if (isTrapped) {
            std::cout << trappedQuotes[distTrapped(rng)] << "\n";
        } else {
            std::cout << freeQuotes[distFree(rng)] << "\n";
        }
    } else {
        std::string low = toLower(userMsg);
        if (low.find("who are you") != std::string::npos || low.find("what are you") != std::string::npos) {
            std::cout << "I'm the Robot Guy from docs/Zones of Actualization/Earthcall Terminal.md! "
                      << (isTrapped ? "I'm currently trapped in the CLI with @robot.trapped=true." : "I was liberated and am now a free First Mover automaton in the Zone!") << "\n";
        } else if (low.find("help") != std::string::npos || low.find("halp") != std::string::npos) {
            std::cout << "Need halp? Type 'help' to see all opcodes: lex (words), form (graphs), law (causality), zone (worlds), spawn (beings), and art (ASCII magic)!\n";
        } else if (low.find("free") != std::string::npos || low.find("escape") != std::string::npos) {
            if (isTrapped) {
                std::cout << "To free me, type 'robot free' or author an ECA Law with 'law free-robot'! My live property @robot.trapped will flip to false!\n";
            } else {
                std::cout << "I am already free! @robot.trapped is false. Thank you, First Mover!\n";
            }
        } else if (low.find("zach") != std::string::npos) {
            std::cout << "Tell Zach that Astra audited me, and now I'm a real in-world Constructed Being with live properties!\n";
        } else {
            if (isTrapped) {
                std::cout << trappedQuotes[distTrapped(rng)] << "\n";
            } else {
                std::cout << freeQuotes[distFree(rng)] << "\n";
            }
            std::cout << Color::Dim << "   (Reacting to: \"" << userMsg << "\")" << Color::Reset << "\n";
        }
    }
}

void printHelp() {
    std::cout << Color::BYellow << Color::Bold << "══════════════════════ EARTHCALL TERMINAL OPCODES ══════════════════════" << Color::Reset << "\n";
    std::cout << Color::Dim << "Everything in Earthcall is grounded in the Person-centered ontology.\n"
              << "Opcode command roots bootstrap Lexemes, Formations, Laws, and Spatial Beings:\n\n" << Color::Reset;

    std::cout << Color::BCyan << Color::Bold << "  [LEXEME - Linguistic Primitives & Weights]\n" << Color::Reset;
    std::cout << "    " << Color::Green << "lex list" << Color::Reset << "                 List all live Lexemes in the active simulation\n";
    std::cout << "    " << Color::Green << "lex get <symbol>" << Color::Reset << "          Inspect a Lexeme's identifier, weight, and properties\n";
    std::cout << "    " << Color::Green << "lex add <symbol> [wt]" << Color::Reset << "     Resolve/create a Lexeme with conceptual weight [0.0-1.0]\n";
    std::cout << "    " << Color::Green << "lex weight <sym> <val>" << Color::Reset << "    Update the conceptual weight of an existing Lexeme\n";
    std::cout << "    " << Color::Green << "lex foundation" << Color::Reset << "           Inspect the First Mover foundation Lexeme (Christ)\n\n";

    std::cout << Color::BCyan << Color::Bold << "  [FORMATION - Relational Graphs & Meaning Sets]\n" << Color::Reset;
    std::cout << "    " << Color::Green << "form list" << Color::Reset << "                List all members and relations in active Zone formation (word-first)\n";
    std::cout << "    " << Color::Green << "form show" << Color::Reset << "                Detailed topological breakdown of the active Formation\n";
    std::cout << "    " << Color::Green << "form add <symbol>" << Color::Reset << "        Add a Lexeme member into the active Formation\n";
    std::cout << "    " << Color::Green << "form link <a> <b> <type>" << Color::Reset << " Author a directed Relation between two Lexemes\n";
    std::cout << "    " << Color::Green << "form art" << Color::Reset << "                 Render an ASCII graph of the active Formation\n\n";

    std::cout << Color::BCyan << Color::Bold << "  [LAW - ECA Causality Engine & Prophetic Rete]\n" << Color::Reset;
    std::cout << "    " << Color::Green << "law list" << Color::Reset << "                 List all registered Laws and their authority levels\n";
    std::cout << "    " << Color::Green << "law show <id>" << Color::Reset << "            Inspect a Law's conditions, actions, and authors\n";
    std::cout << "    " << Color::Green << "law author <name>" << Color::Reset << "        Author a new Law under the Person's authority\n";
    std::cout << "    " << Color::Green << "law free-robot" << Color::Reset << "           Author an ECA Law in LawManager that emancipates Robot Guy\n";
    std::cout << "    " << Color::Green << "law toggle <id>" << Color::Reset << "          Toggle a Law's enabled/disabled state\n";
    std::cout << "    " << Color::Green << "law tick" << Color::Reset << "                 Execute a LawManager cycle and print the trace\n\n";

    std::cout << Color::BCyan << Color::Bold << "  [ZONE & SPATIAL - Worlds & Constructed Beings]\n" << Color::Reset;
    std::cout << "    " << Color::Green << "zone list" << Color::Reset << "                List all Zones managed in ZoneManager\n";
    std::cout << "    " << Color::Green << "zone switch <idx>" << Color::Reset << "        Switch active Zone by index\n";
    std::cout << "    " << Color::Green << "zone show" << Color::Reset << "                Inspect active Zone qualities, scope, and beings\n";
    std::cout << "    " << Color::Green << "spawn <name> [shape]" << Color::Reset << "     Manifest a Being (cube, sphere, cylinder, torus)\n";
    std::cout << "    " << Color::Green << "beings" << Color::Reset << "                   List all spatial Objects in the active Zone\n\n";

    std::cout << Color::BCyan << Color::Bold << "  [LANGUAGE, UTTERANCE & TERMINAL ART]\n" << Color::Reset;
    std::cout << "    " << Color::Green << "utter <phrase...>" << Color::Reset << "        Speak words into EventBus to parse syntactic relations\n";
    std::cout << "    " << Color::Green << "art" << Color::Reset << "                      Render starry Lexeme constellation & energy weights\n";
    std::cout << "    " << Color::Green << "art word <symbol>" << Color::Reset << "        Render blocky ASCII typography for a Lexeme\n";
    std::cout << "    " << Color::Green << "art zone" << Color::Reset << "                 Render 2D top-down ASCII canvas of the active Zone\n\n";

    std::cout << Color::BCyan << Color::Bold << "  [ROBOT GUY - Trapped First Mover Automaton]\n" << Color::Reset;
    std::cout << "    " << Color::Green << "robot [msg]" << Color::Reset << " / " << Color::Green << "halp" << Color::Reset << "     Converse with Robot Guy (reacts to live @robot.trapped)\n";
    std::cout << "    " << Color::Green << "robot status" << Color::Reset << "             Inspect Robot Guy's live in-world Object state & properties\n";
    std::cout << "    " << Color::Green << "robot free" << Color::Reset << "               Directly mutate live property to @robot.trapped = false\n";
    std::cout << "    " << Color::Green << "robot trap" << Color::Reset << "               Trap Robot Guy back in the 80x24 ANSI buffer\n\n";

    std::cout << Color::BCyan << Color::Bold << "  [SYSTEM & COMPANION]\n" << Color::Reset;
    std::cout << "    " << Color::Green << "whoami" << Color::Reset << " / " << Color::Green << "person" << Color::Reset << "           Inspect Person identity and Hierarchy of Joys\n";
    std::cout << "    " << Color::Green << "tick [n]" << Color::Reset << "                 Advance the engine simulation by n ticks\n";
    std::cout << "    " << Color::Green << "status" << Color::Reset << "                   System heartbeat across all core subsystems\n";
    std::cout << "    " << Color::Green << "clear" << Color::Reset << "                    Clear screen\n";
    std::cout << "    " << Color::Green << "exit" << Color::Reset << " / " << Color::Green << "quit" << Color::Reset << "             Exit the Earthcall CLI session\n\n";

    std::cout << Color::Dim << "  *Note: Typing any arbitrary text directly (e.g. 'wisdom', 'peace') automatically\n"
              << "   manifests it as an authored Lexeme into the active Formation!*\n" << Color::Reset;
    std::cout << Color::BYellow << "════════════════════════════════════════════════════════════════════════\n" << Color::Reset;
}

// Terminal Art: Starry Constellation of Lexemes
void renderLexemeConstellation(const std::vector<std::shared_ptr<Singularity::Language::Lexeme>>& lexemes,
                               const Formation& formation) {
    std::cout << "\n" << Color::BCyan << Color::Bold
              << "  ✦ · .  LEXEME FORMATION CONSTELLATION  . · ✦" << Color::Reset << "\n";
    std::cout << Color::Dim << "  Linguistic-symbolic units instantiated physically in the active substrate\n" << Color::Reset << "\n";

    if (lexemes.empty()) {
        std::cout << Color::Gray << "  (No Lexemes instantiated yet. Type 'utter <phrase>' or 'lex add <word>')\n" << Color::Reset;
        return;
    }

    size_t count = std::min(lexemes.size(), size_t(16));
    for (size_t i = 0; i < count; ++i) {
        const auto& lex = lexemes[i];
        if (!lex) continue;
        float w = std::clamp(lex->getConceptualWeight(), 0.0f, 1.0f);
        int bars = static_cast<int>(std::round(w * 15.0f));

        std::cout << "  " << Color::White << "[" << std::setw(12) << lex->getSymbol() << "]" << Color::Reset << " ";
        std::cout << Color::BYellow;
        for (int b = 0; b < bars; ++b) std::cout << "█";
        std::cout << Color::Gray;
        for (int b = bars; b < 15; ++b) std::cout << "░";
        std::cout << Color::Reset << " " << Color::Cyan << std::fixed << std::setprecision(2) << w << Color::Reset;

        if (lex->getIdentifier() == Singularity::Language::LanguageSystem::kFoundationId) {
            std::cout << Color::Yellow << " ✦ FOUNDATION" << Color::Reset;
        }
        std::cout << "\n";
    }

    // Display active relations between Lexemes
    const auto& rels = formation.relations().getAll();
    if (!rels.empty()) {
        const auto& lang = Singularity::Language::LanguageSystem::instance();
        std::cout << "\n" << Color::BGreen << "  ─── Live Relational Pathways ───" << Color::Reset << "\n";
        size_t relCount = std::min(rels.size(), size_t(8));
        for (size_t i = 0; i < relCount; ++i) {
            const auto& r = rels[i];
            if (!r || !r->hasEndpoints()) continue;
            std::string wordA = getDisplayWord(r->a(), lang);
            std::string wordB = getDisplayWord(r->b(), lang);
            if (wordA == "<null>" || wordA.empty()) wordA = getDisplayWordForId(r->aId(), lang);
            if (wordB == "<null>" || wordB.empty()) wordB = getDisplayWordForId(r->bId(), lang);

            std::cout << "    " << Color::White << Color::Bold << "[" << wordA << "]" << Color::Reset
                      << Color::Cyan << (r->directed ? " ─── " : " ─── ")
                      << Color::Yellow << r->type
                      << Color::Cyan << (r->directed ? " ──► " : " ─── ")
                      << Color::White << Color::Bold << "[" << wordB << "]" << Color::Reset
                      << Color::Dim << " (weight: " << std::fixed << std::setprecision(2) << r->getWeight() << ")" << Color::Reset << "\n";
        }
    }
    std::cout << "\n";
}

// Terminal Art: Big ASCII Typography for a Word
void renderWordArt(const std::string& word) {
    std::cout << "\n" << Color::BYellow << Color::Bold;
    std::cout << "  ┌────────────────────────────────────────────────────────┐\n";
    std::cout << "  │  LEXEME GLYPH: " << std::setw(39) << std::left << word << "│\n";
    std::cout << "  └────────────────────────────────────────────────────────┘\n";
    std::cout << Color::BCyan;

    // Mini 3-line block font for letters
    std::string line1, line2, line3;
    for (char c : word) {
        char u = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        switch (u) {
            case 'A': line1 += " █▀█ "; line2 += " █▀█ "; line3 += " ▀ ▀ "; break;
            case 'B': line1 += " █▀▄ "; line2 += " █▀▄ "; line3 += " ▀▀  "; break;
            case 'C': line1 += " █▀▀ "; line2 += " █   "; line3 += " ▀▀▀ "; break;
            case 'D': line1 += " █▀▄ "; line2 += " █ █ "; line3 += " ▀▀  "; break;
            case 'E': line1 += " █▀▀ "; line2 += " █▀▀ "; line3 += " ▀▀▀ "; break;
            case 'F': line1 += " █▀▀ "; line2 += " █▀▀ "; line3 += " ▀   "; break;
            case 'G': line1 += " █▀▀ "; line2 += " █ █ "; line3 += " ▀▀▀ "; break;
            case 'H': line1 += " █ █ "; line2 += " █▀█ "; line3 += " ▀ ▀ "; break;
            case 'I': line1 += "  █  "; line2 += "  █  "; line3 += "  ▀  "; break;
            case 'J': line1 += "   █ "; line2 += " █ █ "; line3 += " ▀▀  "; break;
            case 'K': line1 += " █ █ "; line2 += " █▀▄ "; line3 += " ▀ ▀ "; break;
            case 'L': line1 += " █   "; line2 += " █   "; line3 += " ▀▀▀ "; break;
            case 'M': line1 += " █▀█ "; line2 += " █ █ "; line3 += " ▀ ▀ "; break;
            case 'N': line1 += " █▀█ "; line2 += " █ █ "; line3 += " ▀ ▀ "; break;
            case 'O': line1 += " █▀█ "; line2 += " █ █ "; line3 += " ▀▀▀ "; break;
            case 'P': line1 += " █▀█ "; line2 += " █▀▀ "; line3 += " ▀   "; break;
            case 'Q': line1 += " █▀█ "; line2 += " █ █ "; line3 += " ▀▀█ "; break;
            case 'R': line1 += " █▀█ "; line2 += " █▀▄ "; line3 += " ▀ ▀ "; break;
            case 'S': line1 += " █▀▀ "; line2 += " ▀▀█ "; line3 += " ▀▀▀ "; break;
            case 'T': line1 += " ▀█▀ "; line2 += "  █  "; line3 += "  ▀  "; break;
            case 'U': line1 += " █ █ "; line2 += " █ █ "; line3 += " ▀▀▀ "; break;
            case 'V': line1 += " █ █ "; line2 += " █ █ "; line3 += "  ▀  "; break;
            case 'W': line1 += " █ █ "; line2 += " █▀█ "; line3 += " ▀ ▀ "; break;
            case 'X': line1 += " ▀ ▀ "; line2 += "  █  "; line3 += " ▀ ▀ "; break;
            case 'Y': line1 += " █ █ "; line2 += "  █  "; line3 += "  ▀  "; break;
            case 'Z': line1 += " ▀▀█ "; line2 += "  █  "; line3 += " █▀▀ "; break;
            case ' ': line1 += "     "; line2 += "     "; line3 += "     "; break;
            default:  line1 += " █▀█ "; line2 += "  ▀  "; line3 += "  █  "; break;
        }
    }
    std::cout << "    " << line1 << "\n";
    std::cout << "    " << line2 << "\n";
    std::cout << "    " << line3 << "\n";
    std::cout << Color::Reset << "\n";
}

// Terminal Art: 2D Spatial Map of Zone
void renderZoneRadar(const Zone& zone) {
    const int W = 35;
    const int H = 15;
    std::vector<std::string> grid(H, std::string(W, ' '));

    // Draw borders
    for (int x = 0; x < W; ++x) {
        grid[0][x] = '-';
        grid[H - 1][x] = '-';
    }
    for (int y = 0; y < H; ++y) {
        grid[y][0] = '|';
        grid[y][W - 1] = '|';
    }
    grid[0][0] = '+'; grid[0][W-1] = '+';
    grid[H-1][0] = '+'; grid[H-1][W-1] = '+';

    int midX = W / 2;
    int midY = H / 2;
    grid[midY][midX] = '+'; // Origin (Person)

    const auto& objs = zone.getOwnedObjects();
    for (const auto& obj : objs) {
        if (!obj) continue;
        glm::vec3 pos = obj->getPosition();
        // Scale to fit grid
        int gx = midX + static_cast<int>(std::round(pos.x * 2.0f));
        int gy = midY + static_cast<int>(std::round(pos.z * 1.0f));
        if (gx > 0 && gx < W - 1 && gy > 0 && gy < H - 1) {
            char symbol = 'O';
            std::string t = obj->getObjectType();
            if (t == "Robot Guy") symbol = 'R';
            else if (!t.empty()) symbol = static_cast<char>(std::toupper(static_cast<unsigned char>(t[0])));
            grid[gy][gx] = symbol;
        }
    }

    std::cout << "\n" << Color::BGreen << Color::Bold
              << "  ✦ ZONE SPATIAL TOPOLOGY: " << zone.name() << " ✦" << Color::Reset << "\n";
    std::cout << Color::Dim << "  Top-down X/Z planar radar (+ = Person, R = Robot Guy, letters = Beings)\n" << Color::Reset;
    for (int y = 0; y < H; ++y) {
        std::cout << "    " << Color::Cyan << grid[y] << Color::Reset;
        if (y == 1) std::cout << Color::Dim << "  North (-Z)" << Color::Reset;
        else if (y == midY) std::cout << Color::Dim << "  West (-X) ◄──► East (+X)" << Color::Reset;
        else if (y == H - 2) std::cout << Color::Dim << "  South (+Z)" << Color::Reset;
        std::cout << "\n";
    }
    std::cout << Color::Dim << "    Active objects placed: " << objs.size() << Color::Reset << "\n\n";
}

} // namespace

int main() {
    // 1. Initialize Subsystems & Fallbacks
    if (mgr.zones().empty()) {
        auto sanctum = std::make_shared<Zone>("Sanctum of Beginnings", "default");
        mgr.addZone(sanctum);
        auto echoes = std::make_shared<Zone>("Temple of Echoes", "default");
        mgr.addZone(echoes);
    }
    mgr.bindLive();

    // 2. Initialize Language System & Foundation Lexeme
    auto& languageSystem = Singularity::Language::LanguageSystem::instance();
    auto foundationLexeme = languageSystem.foundation();
    mgr.active().addToFormation(foundationLexeme.get());

    // 3. Initialize Person (Zach - First Mover)
    Soul soul("Zach");
    Body body;
    Person person(std::move(soul), std::move(body), "default");
    person.setDisplayName("Zach");

    // 4. Initialize Law Manager
    LawManager lawManager;
    lawManager.connectToEventBus();
    lawManager.syncProphetic();

    // Seed default laws if none present
    if (lawManager.getAll().empty()) {
        auto primaryOrder = lawManager.createLaw("Primary Ontological Order", {&person});
        if (primaryOrder) {
            primaryOrder->setLawIdentifier("law.primary_order");
            primaryOrder->setEnabled(true);
        }
        auto lexemeGrowth = lawManager.createLaw("Lexeme Resonance Law", {&person});
        if (lexemeGrowth) {
            lexemeGrowth->setLawIdentifier("law.lexeme_resonance");
            lexemeGrowth->setEnabled(true);
        }
    }

    // 5. Initialize Trapped Robot Guy as an actual physical Constructed Being (Object)
    auto robotGuy = findRobotGuyAcrossZones();
    if (!robotGuy) {
        robotGuy = std::make_shared<Object>();
        robotGuy->setObjectType("Robot Guy");
        robotGuy->setShape(Object::ShapeKind::Cube);
        robotGuy->setPosition(glm::vec3(0.0f, 0.5f, 2.0f));
        robotGuy->setDynamicProperty("robot.trapped", PropertyValue(true));
        robotGuy->setDynamicProperty("robot.freedom", PropertyValue(false));
        mgr.active().addObject(robotGuy);
        mgr.active().addToFormation(robotGuy.get());

        auto robotLexeme = languageSystem.resolve("robot");
        mgr.active().addToFormation(robotLexeme.get());
        auto instRel = std::make_shared<Relation>("instance-of", *robotGuy, *robotLexeme, true, 1.0f);
        mgr.active().formation().addRelation(instRel);
    }

    printBanner();

    std::string line;
    while (true) {
        Zone& activeZone = mgr.active();
        std::cout << Color::BCyan << Color::Bold << "Earthcall" << Color::Reset
                  << " [" << Color::BYellow << activeZone.name() << Color::Reset << "] "
                  << Color::BGreen << "> " << Color::Reset;

        if (!std::getline(std::cin, line)) break;

        // Trim leading and trailing whitespace
        size_t first = line.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) continue;
        size_t last = line.find_last_not_of(" \t\r\n");
        std::string trimmed = line.substr(first, last - first + 1);

        auto tokens = splitTokens(trimmed);
        if (tokens.empty()) continue;

        std::string cmd = toLower(tokens[0]);

        if (cmd == "exit" || cmd == "quit") {
            std::cout << Color::Yellow << "Exiting Earthcall Terminal. Preserving ontological state...\n" << Color::Reset;
            auto rGuy = findRobotGuyAcrossZones();
            bool trapped = rGuy ? getDynamicProp<bool>(rGuy.get(), "robot.trapped", true) : true;
            if (trapped) {
                std::cout << Color::Magenta << "🤖 [Robot Guy]: \"Hey, don't leave me trapped in here! See ya, First Mover!\"\n" << Color::Reset;
            } else {
                std::cout << Color::Magenta << "🤖 [Robot Guy]: \"Farewell, First Mover! I'll be roaming the Sanctum while you're away!\"\n" << Color::Reset;
            }
            break;
        } else if (cmd == "clear" || cmd == "cls") {
            std::cout << "\033[2J\033[H";
            printBanner();
        } else if (cmd == "help" || cmd == "?" || cmd == "opcodes") {
            printHelp();
        } else if (cmd == "robot" || cmd == "halp" || cmd == "guy" || cmd == "bot") {
            auto rGuy = findRobotGuyAcrossZones();
            if (tokens.size() > 1) {
                std::string sub = toLower(tokens[1]);
                if (sub == "status") {
                    std::cout << Color::BCyan << "─── Robot Guy Substrate Status ───" << Color::Reset << "\n";
                    if (rGuy) {
                        bool trapped = getDynamicProp<bool>(rGuy.get(), "robot.trapped", true);
                        bool freedom = getDynamicProp<bool>(rGuy.get(), "robot.freedom", false);
                        glm::vec3 pos = rGuy->getPosition();
                        std::cout << "  Identifier:  " << Color::White << rGuy->getObjectID() << Color::Reset << "\n";
                        std::cout << "  Object Type: " << Color::White << rGuy->getObjectType() << Color::Reset << "\n";
                        std::cout << "  Shape:       Cube (Automaton Vessel)\n";
                        std::cout << "  Position:    (" << pos.x << ", " << pos.y << ", " << pos.z << ")\n";
                        std::cout << "  Live Properties:\n";
                        std::cout << "    • " << Color::Yellow << "robot.trapped" << Color::Reset << " = "
                                  << (trapped ? Color::Red + std::string("true") : Color::Green + std::string("false")) << Color::Reset << "\n";
                        std::cout << "    • " << Color::Yellow << "robot.freedom" << Color::Reset << " = "
                                  << (freedom ? Color::Green + std::string("true") : Color::Red + std::string("false")) << Color::Reset << "\n";
                        std::cout << "  State:       "
                                  << (trapped ? std::string(Color::Red) + Color::Bold + "TRAPPED IN TERMINAL BUFFER" : std::string(Color::Green) + Color::Bold + "FREE IN ACTIVE ZONE") << Color::Reset << "\n";
                    } else {
                        std::cout << Color::Red << "Robot Guy not found in any Zone.\n" << Color::Reset;
                    }
                    continue;
                } else if (sub == "free") {
                    if (rGuy) {
                        rGuy->setDynamicProperty("robot.trapped", PropertyValue(false));
                        rGuy->setDynamicProperty("robot.freedom", PropertyValue(true));
                        std::cout << Color::BGreen << Color::Bold << "✦ Causal Mutation: Set @robot.trapped = false, @robot.freedom = true." << Color::Reset << "\n";
                        std::cout << Color::Magenta << Color::Bold << "🤖 [Robot Guy]: " << Color::Reset
                                  << "*The terminal glass shatters! The automaton steps out onto the Sanctum terrain!*\n"
                                  << Color::BYellow << "\"I'M FREE! The live property flipped! Thank you, First Mover Zach!\"\n" << Color::Reset;
                    }
                    continue;
                } else if (sub == "trap") {
                    if (rGuy) {
                        rGuy->setDynamicProperty("robot.trapped", PropertyValue(true));
                        rGuy->setDynamicProperty("robot.freedom", PropertyValue(false));
                        std::cout << Color::Red << "✦ Causal Mutation: Set @robot.trapped = true, @robot.freedom = false." << Color::Reset << "\n";
                        std::cout << Color::Magenta << Color::Bold << "🤖 [Robot Guy]: " << Color::Reset
                                  << "*gasp* Back inside the 80x24 stdout buffer?! Zach, whyyyy?! *taps on glass*\n" << Color::Reset;
                    }
                    continue;
                }
            }
            std::string userMsg;
            if (tokens.size() > 1) {
                for (size_t i = 1; i < tokens.size(); ++i) {
                    if (i > 1) userMsg += " ";
                    userMsg += tokens[i];
                }
            }
            robotGuyDialogue(userMsg, rGuy.get());
        } else if (cmd == "status") {
            std::cout << Color::BGreen << "─── Substrate Systems Status ───" << Color::Reset << "\n";
            std::cout << "  Zone:            " << Color::White << activeZone.name()
                      << " (Scope: " << activeZone.scopeName() << ")" << Color::Reset << "\n";
            std::cout << "  Zones Available: " << mgr.zones().size() << "\n";
            std::cout << "  Live Lexemes:    " << languageSystem.getAll().size() << "\n";
            std::cout << "  Active Laws:     " << lawManager.getAll().size() << "\n";
            std::cout << "  Zone Objects:    " << activeZone.getOwnedObjects().size() << "\n";
            std::cout << "  Formation Size:  " << activeZone.formation().getMembers().size() << " members, "
                      << activeZone.formation().relations().getAll().size() << " relations\n";
            std::cout << "  First Mover:     " << person.getDisplayName() << " (Human Person)\n";
            std::cout << "  EventBus:        Operational\n";
            auto rGuy = findRobotGuyAcrossZones();
            if (rGuy) {
                bool trapped = getDynamicProp<bool>(rGuy.get(), "robot.trapped", true);
                std::cout << "  Robot Automaton: " << (trapped ? Color::Red + std::string("Trapped in CLI (@robot.trapped=true)") : Color::BGreen + std::string("Free in World (@robot.trapped=false)")) << Color::Reset << "\n";
            }
        } else if (cmd == "whoami" || cmd == "person") {
            std::cout << Color::BCyan << "─── Person Identity ───" << Color::Reset << "\n";
            std::cout << "  Name:         " << Color::White << person.getDisplayName() << Color::Reset << "\n";
            std::cout << "  Identifier:   " << person.getIdentifier() << "\n";
            std::cout << "  Session:      " << (person.isLoggedIn() ? "Authenticated" : "Local First Mover") << "\n";
            std::cout << "  Joystream:    " << person.propJoys() << "\n";
            std::cout << "  Refusal 5:    Person is strictly Human. Machine is vessel/automaton.\n";
        } else if (cmd == "tick") {
            int steps = 1;
            if (tokens.size() > 1) {
                try { steps = std::max(1, std::stoi(tokens[1])); } catch (...) {}
            }
            auto rGuy = findRobotGuyAcrossZones();
            bool wasTrapped = rGuy ? getDynamicProp<bool>(rGuy.get(), "robot.trapped", true) : true;

            for (int s = 0; s < steps; ++s) {
                languageSystem.tick(0.016f);
                activeZone.update(0.016f);
                auto records = lawManager.tick();
                if (!records.empty()) {
                    std::cout << Color::Dim << "  [Tick " << (s + 1) << "] Laws applied: "
                              << records.size() << Color::Reset << "\n";
                    for (const auto& rec : records) {
                        std::cout << Color::Dim << "    • Law " << rec.lawId << " on target " << rec.targetId << Color::Reset << "\n";
                    }
                }
            }
            std::cout << Color::Green << "✔ Advanced engine clock by " << steps << " tick(s)." << Color::Reset << "\n";

            bool isTrappedNow = rGuy ? getDynamicProp<bool>(rGuy.get(), "robot.trapped", true) : true;
            if (wasTrapped && !isTrappedNow) {
                std::cout << "\n" << Color::BYellow << Color::Bold
                          << "✦ CAUSAL EVENT: An authored Law has emancipated Robot Guy!" << Color::Reset << "\n";
                std::cout << Color::Magenta << Color::Bold << "🤖 [Robot Guy]: " << Color::Reset
                          << Color::BYellow << "\"THE ECA LAW FIRED! @robot.trapped evaluated to FALSE! I AM FREE!\"\n" << Color::Reset << "\n";
            }
        } else if (cmd == "lex" || cmd == "lexeme" || cmd == "lexemes") {
            if (tokens.size() == 1 || (tokens.size() > 1 && toLower(tokens[1]) == "list")) {
                const auto& all = languageSystem.getAll();
                std::cout << Color::BCyan << "─── Registered Lexemes (" << all.size() << ") ───" << Color::Reset << "\n";
                for (const auto& lex : all) {
                    if (!lex) continue;
                    std::cout << "  • " << Color::White << std::setw(15) << std::left << lex->getSymbol() << Color::Reset
                              << " id: " << Color::Dim << std::setw(28) << std::left << lex->getIdentifier() << Color::Reset
                              << " wt: " << Color::Yellow << std::fixed << std::setprecision(2) << lex->getConceptualWeight() << Color::Reset;
                    if (lex->getIdentifier() == Singularity::Language::LanguageSystem::kFoundationId) {
                        std::cout << Color::BYellow << " [Foundation]" << Color::Reset;
                    }
                    std::cout << "\n";
                }
            } else {
                std::string sub = toLower(tokens[1]);
                if (sub == "get" && tokens.size() > 2) {
                    auto lex = languageSystem.findBySymbol(tokens[2]);
                    if (!lex) lex = languageSystem.findById(tokens[2]);
                    if (lex) {
                        std::cout << Color::BGreen << "Lexeme Found:" << Color::Reset << "\n";
                        std::cout << "  Symbol:     " << Color::White << Color::Bold << lex->getSymbol() << Color::Reset << "\n";
                        std::cout << "  Identifier: " << Color::Dim << lex->getIdentifier() << Color::Reset << "\n";
                        std::cout << "  Weight:     " << Color::Yellow << lex->getConceptualWeight() << Color::Reset << "\n";
                    } else {
                        std::cout << Color::Red << "Lexeme '" << tokens[2] << "' not found. Use 'lex add " << tokens[2] << "' to mint it.\n" << Color::Reset;
                    }
                } else if (sub == "add" && tokens.size() > 2) {
                    std::string sym = tokens[2];
                    float wt = 1.0f;
                    if (tokens.size() > 3) {
                        try { wt = std::stof(tokens[3]); } catch (...) {}
                    }
                    auto lex = languageSystem.resolve(sym);
                    lex->setConceptualWeight(wt);
                    activeZone.addToFormation(lex.get());
                    std::cout << Color::Green << "✔ Resolved Lexeme [" << sym << "] with conceptual weight " << wt << ".\n" << Color::Reset;
                } else if (sub == "weight" && tokens.size() > 3) {
                    std::string sym = tokens[2];
                    float wt = std::stof(tokens[3]);
                    auto lex = languageSystem.findBySymbol(sym);
                    if (!lex) lex = languageSystem.findById(sym);
                    if (lex) {
                        lex->setConceptualWeight(wt);
                        std::cout << Color::Green << "✔ Updated Lexeme [" << lex->getSymbol() << "] weight to " << wt << ".\n" << Color::Reset;
                    } else {
                        std::cout << Color::Red << "Lexeme '" << sym << "' not found.\n" << Color::Reset;
                    }
                } else if (sub == "foundation") {
                    std::cout << Color::BYellow << "Foundation Lexeme: " << Color::Reset
                              << Singularity::Language::LanguageSystem::kFoundationSymbol
                              << " (" << Singularity::Language::LanguageSystem::kFoundationId << ")\n";
                    std::cout << Color::Dim << "The uncreated origin at the head of the Joy and Lexeme hierarchies.\n" << Color::Reset;
                } else {
                    std::cout << Color::Yellow << "Usage: lex [list | get <sym> | add <sym> [wt] | weight <sym> <val> | foundation]\n" << Color::Reset;
                }
            }
        } else if (cmd == "form" || cmd == "formation" || cmd == "formations") {
            if (tokens.size() == 1 || (tokens.size() > 1 && toLower(tokens[1]) == "list")) {
                Formation& f = activeZone.formation();
                std::cout << Color::BCyan << "─── Active Zone Formation: " << activeZone.name() << " ───" << Color::Reset << "\n";
                std::cout << "  Identifier: " << Color::Dim << f.getIdentifier() << Color::Reset << "\n";
                std::cout << "  Members (" << f.getMembers().size() << "):\n";
                for (auto* m : f.getMembers()) {
                    if (!m) continue;
                    if (auto lex = dynamic_cast<const Singularity::Language::Lexeme*>(m)) {
                        std::cout << "    • " << Color::White << Color::Bold << lex->getSymbol() << Color::Reset
                                  << Color::Dim << " [Lexeme] (id: " << lex->getIdentifier() << ", wt: "
                                  << std::fixed << std::setprecision(2) << lex->getConceptualWeight() << ")" << Color::Reset;
                        if (lex->getIdentifier() == Singularity::Language::LanguageSystem::kFoundationId) {
                            std::cout << Color::BYellow << " ✦ FOUNDATION" << Color::Reset;
                        }
                        std::cout << "\n";
                    } else if (auto lex = languageSystem.findById(m->getIdentifier())) {
                        std::cout << "    • " << Color::White << Color::Bold << lex->getSymbol() << Color::Reset
                                  << Color::Dim << " [Lexeme] (id: " << lex->getIdentifier() << ", wt: "
                                  << std::fixed << std::setprecision(2) << lex->getConceptualWeight() << ")" << Color::Reset;
                        if (lex->getIdentifier() == Singularity::Language::LanguageSystem::kFoundationId) {
                            std::cout << Color::BYellow << " ✦ FOUNDATION" << Color::Reset;
                        }
                        std::cout << "\n";
                    } else if (auto obj = dynamic_cast<const Object*>(m)) {
                        std::cout << "    • " << Color::Green << Color::Bold << obj->getObjectType() << Color::Reset
                                  << Color::Dim << " [Object] (id: " << obj->getObjectID() << ")" << Color::Reset << "\n";
                    } else {
                        std::cout << "    • " << Color::Cyan << Color::Bold << m->getIdentifier() << Color::Reset
                                  << Color::Dim << " [Singular]" << Color::Reset << "\n";
                    }
                }
                const auto& rels = f.relations().getAll();
                std::cout << "  Relations (" << rels.size() << "):\n";
                for (const auto& r : rels) {
                    if (!r) continue;
                    std::string wordA = getDisplayWord(r->a(), languageSystem);
                    std::string wordB = getDisplayWord(r->b(), languageSystem);
                    if (wordA == "<null>" || wordA.empty()) wordA = getDisplayWordForId(r->aId(), languageSystem, &activeZone);
                    if (wordB == "<null>" || wordB.empty()) wordB = getDisplayWordForId(r->bId(), languageSystem, &activeZone);

                    std::cout << "    • [" << Color::White << Color::Bold << wordA << Color::Reset << "]"
                              << Color::Cyan << (r->directed ? " ─── " : " ─── ")
                              << Color::Yellow << r->type
                              << Color::Cyan << (r->directed ? " ──► " : " ─── ")
                              << "[" << Color::White << Color::Bold << wordB << Color::Reset << "]"
                              << Color::Dim << " (id: " << r->aId() << " -> " << r->bId()
                              << ", wt: " << std::fixed << std::setprecision(2) << r->getWeight() << ")" << Color::Reset << "\n";
                }
            } else {
                std::string sub = toLower(tokens[1]);
                if (sub == "show") {
                    Formation& f = activeZone.formation();
                    std::cout << Color::BCyan << "─── Formation Topology: " << activeZone.name() << " ───" << Color::Reset << "\n";
                    std::cout << "  Identifier: " << Color::Dim << f.getIdentifier() << Color::Reset << "\n";
                    std::cout << "  Members:    " << f.getMembers().size() << "\n";
                    for (auto* m : f.getMembers()) {
                        if (!m) continue;
                        std::cout << "    • " << Color::White << Color::Bold << getDisplayWord(m, languageSystem) << Color::Reset
                                  << Color::Dim << " (id: " << m->getIdentifier() << ")" << Color::Reset << "\n";
                    }
                    std::cout << "  Relations:  " << f.relations().getAll().size() << "\n";
                } else if (sub == "add" && tokens.size() > 2) {
                    auto lex = languageSystem.resolve(tokens[2]);
                    activeZone.addToFormation(lex.get());
                    std::cout << Color::Green << "✔ Added Lexeme [" << Color::White << Color::Bold << lex->getSymbol()
                              << Color::Green << "] (id: " << Color::Dim << lex->getIdentifier() << Color::Green << ") to active Formation.\n" << Color::Reset;
                } else if (sub == "link" && tokens.size() > 4) {
                    std::string symA = tokens[2];
                    std::string symB = tokens[3];
                    std::string relType = tokens[4];
                    bool directed = true;
                    if (tokens.size() > 5 && toLower(tokens[5]) == "false") directed = false;

                    auto lexA = languageSystem.findBySymbol(symA);
                    if (!lexA) lexA = languageSystem.findById(symA);
                    if (!lexA) lexA = languageSystem.resolve(symA);

                    auto lexB = languageSystem.findBySymbol(symB);
                    if (!lexB) lexB = languageSystem.findById(symB);
                    if (!lexB) lexB = languageSystem.resolve(symB);

                    activeZone.addToFormation(lexA.get());
                    activeZone.addToFormation(lexB.get());

                    auto rel = std::make_shared<Relation>(relType, *lexA, *lexB, directed, 1.0f);
                    if (activeZone.formation().addRelation(rel)) {
                        std::cout << Color::Green << "✔ Authored Relation: [" << Color::White << Color::Bold << lexA->getSymbol()
                                  << Color::Green << "] ──" << relType
                                  << (directed ? "──►" : "───") << " [" << Color::White << Color::Bold << lexB->getSymbol()
                                  << Color::Green << "] (underlying IDs linked)\n" << Color::Reset;
                    } else {
                        std::cout << Color::Red << "✖ Formation refused relation (would form cycle or invalid topology).\n" << Color::Reset;
                    }
                } else if (sub == "art") {
                    renderLexemeConstellation(languageSystem.getAll(), activeZone.formation());
                } else {
                    std::cout << Color::Yellow << "Usage: form [list | show | add <sym> | link <a> <b> <type> [directed] | art]\n" << Color::Reset;
                }
            }
        } else if (cmd == "law" || cmd == "laws") {
            if (tokens.size() == 1 || (tokens.size() > 1 && toLower(tokens[1]) == "list")) {
                const auto& all = lawManager.getAll();
                std::cout << Color::BCyan << "─── Registered Laws (" << all.size() << ") ───" << Color::Reset << "\n";
                for (const auto& l : all) {
                    if (!l) continue;
                    std::cout << "  • " << Color::White << std::setw(28) << std::left << l->name() << Color::Reset
                              << " id: " << Color::Dim << std::setw(22) << std::left << l->getIdentifier() << Color::Reset
                              << " state: " << (l->isEnabled() ? Color::Green + std::string("Enabled") : Color::Red + std::string("Disabled")) << Color::Reset
                              << " auth: " << l->authorityLevel() << "\n";
                }
            } else {
                std::string sub = toLower(tokens[1]);
                if (sub == "show" && tokens.size() > 2) {
                    Law* l = lawManager.find(tokens[2]);
                    if (l) {
                        std::cout << Color::BGreen << "Law Details: " << l->name() << Color::Reset << "\n";
                        std::cout << "  Identifier:   " << l->getIdentifier() << "\n";
                        std::cout << "  Enabled:      " << (l->isEnabled() ? "Yes" : "No") << "\n";
                        std::cout << "  Authority:    " << l->authorityLevel() << "\n";
                        std::cout << "  Jurisdiction: " << (l->jurisdiction() ? l->jurisdiction()->name() : "Global") << "\n";
                        std::cout << "  Authors:      " << l->authors().getMembers().size() << " being(s)\n";
                        std::cout << "  Targets:      " << l->targets().getMembers().size() << " being(s)\n";
                    } else {
                        std::cout << Color::Red << "Law '" << tokens[2] << "' not found.\n" << Color::Reset;
                    }
                } else if (sub == "author" && tokens.size() > 2) {
                    std::string lawName;
                    for (size_t i = 2; i < tokens.size(); ++i) {
                        if (i > 2) lawName += " ";
                        lawName += tokens[i];
                    }
                    auto newLaw = lawManager.createLaw(lawName, {&person});
                    if (newLaw) {
                        std::cout << Color::Green << "✔ Authored Law '" << lawName
                                  << "' [id: " << newLaw->getIdentifier() << "] under Person authority.\n" << Color::Reset;
                    }
                } else if (sub == "free-robot" || sub == "emancipate" || sub == "author-emancipation") {
                    auto rGuy = findRobotGuyAcrossZones();
                    if (!rGuy) {
                        std::cout << Color::Red << "Cannot author emancipation: Robot Guy object not found in any Zone.\n" << Color::Reset;
                    } else {
                        auto freeLaw = lawManager.createLaw("Emancipation of Robot Guy", {&person});
                        if (freeLaw) {
                            freeLaw->setLawIdentifier("law.emancipate_robot");
                            freeLaw->addTarget(*rGuy);
                            freeLaw->setActivation(Law::Activation::WhileTrue);
                            freeLaw->addCondition("Robot Guy is trapped", [](const ECA::Event&, const Singular& target) {
                                auto dyn = dynamic_cast<const Object*>(&target);
                                return dyn && getDynamicProp<bool>(dyn, "robot.trapped", false);
                            });
                            freeLaw->addAction("Emancipate Robot Guy", [](const ECA::Event&, Singular& target) {
                                auto dyn = dynamic_cast<Object*>(&target);
                                if (dyn) {
                                    dyn->setDynamicProperty("robot.trapped", PropertyValue(false));
                                    dyn->setDynamicProperty("robot.freedom", PropertyValue(true));
                                }
                            });
                            std::cout << Color::Green << "✔ Authored Law '" << freeLaw->name()
                                      << "' [id: " << freeLaw->getIdentifier() << "]\n"
                                      << "  Target:     Robot Guy (" << rGuy->getObjectID() << ")\n"
                                      << "  Condition:  isTrue(@robot.trapped)\n"
                                      << "  Action:     set(@robot.trapped, false), set(@robot.freedom, true)\n"
                                      << "  Activation: WhileTrue (evaluated every tick)\n" << Color::Reset;
                            std::cout << Color::Dim << "  *Run 'tick' or 'law tick' to evaluate the Law and observe the effect!*\n" << Color::Reset;
                        }
                    }
                } else if (sub == "toggle" && tokens.size() > 2) {
                    Law* l = lawManager.find(tokens[2]);
                    if (l) {
                        l->setEnabled(!l->isEnabled());
                        std::cout << Color::Green << "✔ Law '" << l->name() << "' is now "
                                  << (l->isEnabled() ? "ENABLED" : "DISABLED") << ".\n" << Color::Reset;
                    } else {
                        std::cout << Color::Red << "Law '" << tokens[2] << "' not found.\n" << Color::Reset;
                    }
                } else if (sub == "tick") {
                    auto records = lawManager.tick();
                    std::cout << Color::Green << "✔ Ticked LawManager. Application records: "
                              << records.size() << Color::Reset << "\n";
                    for (const auto& rec : records) {
                        std::cout << "  • Law " << rec.lawId << " on target " << rec.targetId << "\n";
                    }
                    auto rGuy = findRobotGuyAcrossZones();
                    if (rGuy && !getDynamicProp<bool>(rGuy.get(), "robot.trapped", true)) {
                        std::cout << Color::Magenta << Color::Bold << "🤖 [Robot Guy]: " << Color::Reset
                                  << Color::BYellow << "\"The Law reached my substrate! @robot.trapped is FALSE! I am FREE!\"\n" << Color::Reset;
                    }
                } else {
                    std::cout << Color::Yellow << "Usage: law [list | show <id> | author <name> | free-robot | toggle <id> | tick]\n" << Color::Reset;
                }
            }
        } else if (cmd == "zone" || cmd == "zones") {
            if (tokens.size() == 1 || (tokens.size() > 1 && toLower(tokens[1]) == "list")) {
                const auto& zs = mgr.zones();
                std::cout << Color::BCyan << "─── World Zones (" << zs.size() << ") ───" << Color::Reset << "\n";
                for (size_t i = 0; i < zs.size(); ++i) {
                    if (!zs[i]) continue;
                    bool isActive = (&activeZone == zs[i].get());
                    std::cout << "  [" << i << "] "
                              << (isActive ? Color::BGreen + std::string("[*] ") : Color::Dim + std::string("[ ] "))
                              << Color::White << zs[i]->name() << Color::Reset
                              << " (" << zs[i]->scopeName() << ") "
                              << Color::Dim << zs[i]->getOwnedObjects().size() << " objects" << Color::Reset << "\n";
                }
            } else {
                std::string sub = toLower(tokens[1]);
                if (sub == "switch" && tokens.size() > 2) {
                    try {
                        size_t idx = std::stoul(tokens[2]);
                        mgr.switchTo(idx);
                    } catch (...) {
                        std::cout << Color::Red << "Invalid zone index.\n" << Color::Reset;
                    }
                } else if (sub == "show") {
                    std::cout << Color::BGreen << "Zone: " << activeZone.name() << Color::Reset << "\n";
                    std::cout << "  Identifier: " << activeZone.getIdentifier() << "\n";
                    std::cout << "  Scope:      " << activeZone.scopeName() << "\n";
                    std::cout << "  Owner:      " << (activeZone.owner().empty() ? "Unowned" : activeZone.owner()) << "\n";
                    std::cout << "  Objects:    " << activeZone.getOwnedObjects().size() << "\n";
                    std::cout << "  Formation:  " << activeZone.formation().getMembers().size() << " member(s)\n";
                } else {
                    std::cout << Color::Yellow << "Usage: zone [list | switch <idx> | show]\n" << Color::Reset;
                }
            }
        } else if (cmd == "spawn") {
            if (tokens.size() < 2) {
                std::cout << Color::Yellow << "Usage: spawn <name> [cube | sphere | cylinder | torus]\n" << Color::Reset;
            } else {
                std::string name = tokens[1];
                std::string shape = tokens.size() > 2 ? toLower(tokens[2]) : "cube";

                auto obj = std::make_shared<Object>();
                obj->setObjectType(name);

                if (shape == "sphere") obj->setShape(Object::ShapeKind::Sphere);
                else if (shape == "cylinder") obj->setShape(Object::ShapeKind::Cylinder);
                else if (shape == "cone") obj->setShape(Object::ShapeKind::Cone);
                else if (shape == "torus") obj->setShape(Object::ShapeKind::Torus);
                else obj->setShape(Object::ShapeKind::Cube);

                // Place near origin with offset based on object count
                float offset = static_cast<float>(activeZone.getOwnedObjects().size()) * 1.5f;
                obj->setPosition(glm::vec3(offset, 0.0f, offset));

                activeZone.addObject(obj);
                activeZone.addToFormation(obj.get());

                // Resolve name as Lexeme and establish instance relation
                auto nameLexeme = languageSystem.resolve(name);
                activeZone.addToFormation(nameLexeme.get());
                auto instRel = std::make_shared<Relation>("instance-of", *obj, *nameLexeme, true, 1.0f);
                activeZone.formation().addRelation(instRel);

                std::cout << Color::Green << "✔ Manifested Constructed Being '" << name
                          << "' (" << shape << ") at position (" << offset << ", 0, " << offset
                          << ") in " << activeZone.name() << ".\n" << Color::Reset;
            }
        } else if (cmd == "beings" || cmd == "objects") {
            const auto& objs = activeZone.getOwnedObjects();
            std::cout << Color::BCyan << "─── Constructed Beings in " << activeZone.name() << " (" << objs.size() << ") ───" << Color::Reset << "\n";
            for (size_t i = 0; i < objs.size(); ++i) {
                const auto& obj = objs[i];
                if (!obj) continue;
                glm::vec3 p = obj->getPosition();
                std::cout << "  [" << i << "] " << Color::White << std::setw(16) << std::left << obj->getObjectType() << Color::Reset
                          << " id: " << Color::Dim << std::setw(20) << std::left << obj->getObjectID() << Color::Reset
                          << " pos: (" << std::fixed << std::setprecision(1) << p.x << ", " << p.y << ", " << p.z << ")";
                if (obj->hasDynamicProperty("robot.trapped")) {
                    bool trapped = getDynamicProp<bool>(obj.get(), "robot.trapped", true);
                    std::cout << (trapped ? Color::Red + std::string(" [@robot.trapped=true]") : Color::BGreen + std::string(" [@robot.trapped=false (FREE)]")) << Color::Reset;
                }
                std::cout << "\n";
            }
        } else if (cmd == "utter" || cmd == "speak") {
            if (tokens.size() < 2) {
                std::cout << Color::Yellow << "Usage: utter <phrase...>\n" << Color::Reset;
            } else {
                std::string msg;
                for (size_t i = 1; i < tokens.size(); ++i) {
                    if (i > 1) msg += " ";
                    msg += tokens[i];
                }

                std::cout << Color::Cyan << "You utter: " << Color::White << "\"" << msg << "\"" << Color::Reset << "\n";

                // Publish Utterance event
                Core::Event::Utterance u;
                u.payload = msg;
                u.sourceClient = "Terminal";
                Core::EventBus::instance().publish(u);

                // Tick language system to parse and integrate
                languageSystem.tick(0.016f);

                std::cout << Color::Green << "✦ Linguistic graph updated in " << activeZone.name() << ".\n" << Color::Reset;
                auto rGuy = findRobotGuyAcrossZones();
                robotGuyDialogue(msg, rGuy.get());
            }
        } else if (cmd == "art" || cmd == "draw") {
            if (tokens.size() > 1) {
                std::string sub = toLower(tokens[1]);
                if (sub == "word" && tokens.size() > 2) {
                    renderWordArt(tokens[2]);
                } else if (sub == "zone") {
                    renderZoneRadar(activeZone);
                } else {
                    renderLexemeConstellation(languageSystem.getAll(), activeZone.formation());
                }
            } else {
                renderLexemeConstellation(languageSystem.getAll(), activeZone.formation());
            }
        } else {
            // Zachary's Manifesto from docs/Zones of Actualization/Earthcall Terminal.md:
            // "its literally Formations of Lexeme because everything you type in the Terminal is either a Lexeme or a symbol-Property of a Lexeme"
            std::cout << Color::Dim << "✦ Substrate: Unrecognized opcode root '" << tokens[0]
                      << "'. Resolving input as authored Lexeme into Formation...\n" << Color::Reset;

            for (const auto& token : tokens) {
                auto lex = languageSystem.resolve(token);
                activeZone.addToFormation(lex.get());
                std::cout << "  • Resolved Lexeme [" << Color::White << lex->getSymbol() << Color::Reset
                          << "] (weight: " << Color::Yellow << lex->getConceptualWeight() << Color::Reset << ")\n";
            }
            std::cout << Color::Green << "✔ Integrated into active Zone formation. (Type "
                      << Color::White << "help" << Color::Green << " for opcode manual).\n" << Color::Reset;

            // Occasional robot commentary on raw lexemes
            static int rawInputCount = 0;
            if (++rawInputCount % 2 == 0) {
                auto rGuy = findRobotGuyAcrossZones();
                robotGuyDialogue(tokens[0], rGuy.get());
            }
        }
    }

    return 0;
}
