#include "Singularity/Terminal/TerminalChannel.hpp"

#include "ConstructedBeing/Singular/Lexeme/Lexeme.hpp"
#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"
#include "ConstructedBeing/Singular/Property/Property.hpp"
#include "ConstructedBeing/Singular/Property/PropertyRef.hpp"
#include "Relation/Relation.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "Singularity/Core/StringId.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <set>
#include <type_traits>
#include <typeinfo>
#include <uuid/uuid.h>

#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
#include <sys/select.h>
#include <unistd.h>
#define EARTHCALL_TERMINAL_POSIX 1
#endif

// macOS ships libedit with the readline API, including the callback
// ("alternate") interface: the frame loop feeds it one character at a time
// when stdin is readable, so the line editor never blocks and never needs a
// thread — Tab completion runs on the main thread beside the live world.
#if defined(EARTHCALL_HAS_LIBEDIT) && defined(EARTHCALL_TERMINAL_POSIX)
#include <readline/history.h>
#include <readline/readline.h>
#include <csignal>
#include <termios.h>
#define EARTHCALL_TERMINAL_LIBEDIT 1
#endif

namespace Singularity {
namespace Terminal {

namespace {

constexpr const char* kLineEntered = "terminal-line-entered";

// libedit's callbacks are C function pointers: they reach the attached
// channel through these (below the Kernel; the C library is process-global).
TerminalChannel* g_attached = nullptr;
LawManager* g_laws = nullptr;
// True from the moment a line is entered until the next keystroke: the input
// is empty then, and libedit's own buffer still holds the old line.
bool g_freshLine = false;
std::vector<std::string> g_matches;

// The event vocabulary, as it actually occurs: every event type the world has
// published this session. No central list of event names.
std::set<std::string>& heardEvents() {
    static std::set<std::string> heard;
    return heard;
}

void listenForEvents() {
    static const bool subscribed = [] {
        Core::EventBus::instance().subscribe<ECA::Event>([](const ECA::Event& e) {
            if (!e.type.empty()) heardEvents().insert(e.type);
        });
        return true;
    }();
    (void)subscribed;
}

// Individuated like a Lexeme (`lexeme_<uuid>`): the display name a sentence
// gives may be shared by many Laws; the identifier never is.
std::string mintLawId() {
    uuid_t uuid;
    uuid_generate(uuid);
    char text[37];
    uuid_unparse_lower(uuid, text);
    return "law_" + std::string(text);
}

Singular* findBeing(const std::string& id) {
    if (id.empty()) return nullptr;
    for (Singular* being : Universe::instance().beings()) {
        if (being && being->getIdentifier() == id) return being;
    }
    return nullptr;
}

void collectPublished(const ActionNode& node, std::set<std::string>& out) {
    if (node.kind == ActionNode::Kind::Publish && !node.eventType.empty()) out.insert(node.eventType);
    for (const auto& child : node.children) collectPublished(child, out);
}

std::string join(const std::vector<std::string>& parts, const std::string& sep) {
    std::string out;
    for (std::size_t i = 0; i < parts.size(); ++i) {
        if (i) out += sep;
        out += parts[i];
    }
    return out;
}

// A derived or sensed level: legible to every Law, writable by none
// (NO_BLACK_BOX: "readable by law, writable unless genuinely derived").
template <typename T>
class ReadOnlyLevel : public Property {
public:
    ReadOnlyLevel(std::string name, const T* member)
        : _name(std::move(name)), _id(Earthcall::StringInterner::intern(_name)), _member(member) {}
    std::string name() const override { return _name; }
    Earthcall::StringId nameId() const override { return _id; }
    std::string typeName() const override { return typeid(T).name(); }
    PropertyValue value() const override { return PropertyValue(*_member); }
    bool setValue(const PropertyValue&) override { return false; }
    Singular* asSingular() const override { return nullptr; }

private:
    std::string _name;
    Earthcall::StringId _id;
    const T* _member;
};

#ifdef EARTHCALL_TERMINAL_LIBEDIT
void restoreTerminal();

extern "C" void onLine(char* line) {
    if (!g_attached) {
        std::free(line);
        return;
    }
    if (!line) {   // Ctrl-D: the Person closed the line; the world keeps running
        std::fputs("\n(terminal line closed)\n", stdout);
        restoreTerminal();
        return;
    }
    if (*line) add_history(line);
    g_attached->inject(line);
    g_freshLine = true;
    std::free(line);
}

extern "C" char* generateMatch(const char*, int state) {
    static std::size_t next = 0;
    if (state == 0) next = 0;
    if (next >= g_matches.size()) return nullptr;
    return strdup(g_matches[next++].c_str());
}

extern "C" char** onComplete(const char* text, int, int end) {
    rl_attempted_completion_over = 1;   // never fall back to file names
    g_matches.clear();
    if (!g_attached || !g_laws) return nullptr;
    const std::string before(rl_line_buffer, rl_line_buffer + end);
    g_matches = LawSentence::complete(before, g_attached->vocabulary(*g_laws));
    if (g_matches.empty()) return nullptr;
    return rl_completion_matches(text, generateMatch);
}

// libedit switches the terminal to character mode only inside el_gets, which
// the callback interface reaches only once select() reports input — and a
// terminal in line mode reports none until Enter. So Tab would never arrive.
// Hold the terminal in character mode ourselves (keeping ISIG: Ctrl-C still
// interrupts), re-assert it after each line (libedit restores line mode when
// a line ends), and put the Person's own settings back on the way out.
termios g_savedTermios{};
bool g_haveSavedTermios = false;

void holdCharacterMode() {
    termios t{};
    if (tcgetattr(STDIN_FILENO, &t) != 0) return;
    if (!g_haveSavedTermios) {
        g_savedTermios = t;
        g_haveSavedTermios = true;
    }
    if ((t.c_lflag & (ICANON | ECHO)) == 0) return;
    t.c_lflag &= ~static_cast<tcflag_t>(ICANON | ECHO);
    t.c_cc[VMIN] = 1;
    t.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

void restoreTerminal() {
    if (g_attached) rl_callback_handler_remove();
    g_attached = nullptr;
    if (g_haveSavedTermios) tcsetattr(STDIN_FILENO, TCSANOW, &g_savedTermios);
}

// Ctrl-C, a kill, or a crash ends the process without atexit. Give the Person
// their terminal back first (tcsetattr is async-signal-safe), then let the
// signal do exactly what it always did.
extern "C" void restoreOnSignal(int sig) {
    if (g_haveSavedTermios) tcsetattr(STDIN_FILENO, TCSANOW, &g_savedTermios);
    std::signal(sig, SIG_DFL);
    std::raise(sig);
}

void restoreOnFatalSignals() {
    for (int sig : {SIGINT, SIGTERM, SIGHUP, SIGQUIT, SIGABRT, SIGSEGV, SIGBUS}) {
        std::signal(sig, restoreOnSignal);
    }
}
#endif

} // namespace

TerminalChannel::TerminalChannel() : Law("terminal-channel") {
    setName("Terminal Channel");
    listenForEvents();
}

TerminalChannel::~TerminalChannel() { detach(); }

void TerminalChannel::syncRegister(LawManager& laws) {
    if (laws.find("terminal-channel")) return;
    laws.add(std::make_shared<TerminalChannel>());
}

TerminalChannel* TerminalChannel::find(LawManager& laws) {
    return dynamic_cast<TerminalChannel*>(laws.find("terminal-channel"));
}

void TerminalChannel::attach(LawManager& laws) {
    _attachTried = true;
#ifdef EARTHCALL_TERMINAL_LIBEDIT
    // Only a real terminal: under ctest, an IDE, or Finder there is no
    // keyboard on stdin, and the channel stays quiet.
    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO) || g_attached) {
        // Say once why, so a quiet line is never a mystery.
        std::fprintf(stderr, "[terminal-channel] the Law Line is not listening: %s\n",
                     g_attached ? "another Terminal channel already holds this terminal"
                                : "stdin/stdout is not an interactive terminal");
        return;
    }
    g_attached = this;
    g_laws = &laws;
    rl_readline_name = const_cast<char*>("earthcall");
    rl_completer_word_break_characters = const_cast<char*>(" \t\n");
    rl_attempted_completion_function = onComplete;
    std::atexit(restoreTerminal);
    std::fputs("\nThe Law Line is listening. Type a sentence, Tab to complete, "
               "'?' at the end to preview, '?? word' to search.\n", stdout);
    // Keep the Person's own settings BEFORE libedit touches the terminal:
    // they are what every exit path puts back.
    if (tcgetattr(STDIN_FILENO, &g_savedTermios) == 0) g_haveSavedTermios = true;
    rl_callback_handler_install(_prompt.c_str(), onLine);
    holdCharacterMode();
    restoreOnFatalSignals();
    _attached = true;
#else
    (void)laws;
    std::fprintf(stderr, "[terminal-channel] the Law Line is not listening: this build has no "
                         "line editor (libedit was not found at configure time)\n");
#endif
}

void TerminalChannel::detach() {
#ifdef EARTHCALL_TERMINAL_LIBEDIT
    if (_attached && g_attached == this) restoreTerminal();
#endif
    _attached = false;
}

void TerminalChannel::sense(LawManager& laws) {
    _laws = &laws;
    if (!isEnabled()) return;
    if (!_attachTried) attach(laws);

#ifdef EARTHCALL_TERMINAL_LIBEDIT
    if (_attached) {
        g_laws = &laws;
        // Drain whatever the keyboard has sent, without ever waiting for it.
        for (int budget = 0; budget < 4096 && g_attached == this; ++budget) {
            fd_set readable;
            FD_ZERO(&readable);
            FD_SET(STDIN_FILENO, &readable);
            timeval now{0, 0};
            if (select(STDIN_FILENO + 1, &readable, nullptr, nullptr, &now) <= 0) break;
            g_freshLine = false;
            rl_callback_read_char();
            holdCharacterMode();
        }
        if (g_attached != this) _attached = false;   // Ctrl-D
    }
#endif

    if (_pending.empty()) return;
    std::string line = _pending.front();
    _pending.pop_front();
    if (line.find_first_not_of(" \t") == std::string::npos) return;

    _lastLine = line;
    _linesEntered += 1.0;
    Core::EventBus::instance().publish(
        ECA::Event{kLineEntered, this, nullptr, std::time(nullptr), std::string{}});
    if (!laws.rete().hearsType(kLineEntered)) {
        say("(no Law in this Zone hears terminal-line-entered, so nothing will act on that line. "
            "The LawLine Zone carries the seed Laws that do.)");
    }
}

void TerminalChannel::act(LawManager& laws) {
    _laws = &laws;
    if (_speakRequests == _spoken) return;
    _spoken = _speakRequests;
    speak(laws, _lastLine);
}

LawSentence::Vocabulary TerminalChannel::vocabulary(LawManager& laws) {
    LawSentence::Vocabulary v;
    v.words = LawSentence::canonicalWords();

    // Lexeme <--denotes--> Law: the Law holds the opcode.
    for (Relation* r : Universe::instance().relations()) {
        if (!r || r->type != _lexemeRelation) continue;
        auto* lexeme = dynamic_cast<Singularity::Language::Lexeme*>(r->a());
        auto* law = dynamic_cast<Law*>(r->b());
        if (!lexeme || !law || law == this) continue;
        LawSentence::Preset preset;
        const std::string opcode = LawSentence::classify(*law, laws.triggersOf(law->getIdentifier()), preset);
        if (opcode.empty()) continue;
        v.words.push_back({lexeme->getSymbol(), opcode, lexeme->getIdentifier(), law->getIdentifier()});
        if (opcode == "preset") v.presets.push_back(preset);
    }

    std::set<std::string> events = heardEvents();
    for (const auto& law : laws.getAll()) {
        if (!law) continue;
        for (const auto& t : laws.triggersOf(law->getIdentifier())) events.insert(t);
        if (law->hasActionModel()) collectPublished(*law->actionModel(), events);
    }
    v.events.assign(events.begin(), events.end());

    std::set<std::string> beings;
    for (Singular* being : Universe::instance().beings()) {
        if (being && !being->getIdentifier().empty()) beings.insert(being->getIdentifier());
    }
    v.beings.assign(beings.begin(), beings.end());
    v.scopeBeing = _scopeBeing;

    v.propertiesOf = [](const std::string& id) {
        std::vector<std::string> names;
        Singular* being = findBeing(id);
        if (!being) return names;
        for (Property* p : being->listProperties()) {
            if (p) names.push_back(p->name());
        }
        for (const auto& entry : being->dynamicProperties()) {
            names.push_back(Earthcall::StringInterner::resolve(entry.first));
        }
        std::sort(names.begin(), names.end());
        names.erase(std::unique(names.begin(), names.end()), names.end());
        return names;
    };
    v.resolve = [this, &laws](const LawSentence::Ambiguity& a) { return resolveByMetalaw(laws, a); };
    return v;
}

LawSentence::Resolution TerminalChannel::resolveByMetalaw(LawManager& laws,
                                                          const LawSentence::Ambiguity& a) {
    _ambiguitySymbol = a.symbol;
    _ambiguitySlot = a.slot;
    std::vector<std::string> names;
    for (const auto& c : a.candidates) names.push_back(c.individual());
    _ambiguityCandidates = join(names, " ");
    _ambiguityResolved.clear();

    // A Metalaw is an ordinary Law whose target is another Law — here, this
    // channel. Apply exactly those, now, against the ambiguity just exposed.
    std::vector<std::string> resolvers;
    for (const auto& law : laws.getAll()) {
        if (!law || law.get() == this) continue;
        const auto& targets = law->targets().getMembers();
        if (std::find(targets.begin(), targets.end(), static_cast<Singular*>(this)) == targets.end()) {
            continue;
        }
        if (law->applyTo(*this) == Law::ApplicationResult::Applied) {
            resolvers.push_back(law->getIdentifier());
        }
    }
    LawSentence::Resolution r;
    r.chosen = _ambiguityResolved;
    if (!r.chosen.empty()) {
        r.reason = "resolved by Metalaw " + join(resolvers, ", ");
    } else if (resolvers.empty()) {
        r.reason = "no Law targeting @terminal-channel applied";
    } else {
        r.reason = "Metalaw " + join(resolvers, ", ") + " applied but wrote no candidate";
    }
    // The question is answered; leave no standing ambiguity for a WhileTrue
    // Metalaw to keep answering. What was asked and answered stays legible.
    _ambiguitySymbol.clear();
    _ambiguitySlot.clear();
    return r;
}

void TerminalChannel::speak(LawManager& laws, const std::string& text) {
    const LawSentence::Parse p = LawSentence::parse(text, vocabulary(laws));
    _preview = p.preview();
    _openClauses = join(p.openClauses, "; ");

    if (p.search) {
        say(p.candidates.empty() ? "?? nothing matches" : join(p.candidates, "\n"));
        return;
    }
    if (!p.ok) {
        std::string msg = "refused: " + p.error;
        if (!p.candidates.empty()) msg += "\n  candidates: " + join(p.candidates, ", ");
        const std::size_t trimmed = text.find_first_not_of(" \t");
        const std::size_t column = (trimmed == std::string::npos ? 0 : trimmed) + p.errorOffset;
        msg += "\n  " + text + "\n  " + std::string(std::min(column, text.size()), ' ') + "^";
        _status = msg;
        say(msg);
        return;
    }
    const std::string notes = p.notes.empty() ? std::string{} : "\n  " + join(p.notes, "\n  ");
    if (p.previewOnly) {
        _status = "preview";
        say("preview: " + _preview + notes);
        return;
    }

    // Nothing enters the world without an author.
    PropertyValue authorValue;
    std::string authorId;
    if (lawGetValue(*this, PropertyPath::parse(_authorPath), authorValue)) {
        if (const auto* s = std::get_if<std::string>(&authorValue)) authorId = *s;
    }
    Singular* author = findBeing(authorId);
    if (!author) {
        _status = "refused: no author at " + _authorPath + " ('" + authorId +
                  "'); nothing enters the world without one";
        say(_status);
        return;
    }

    const std::string id = mintLawId();
    auto law = std::make_shared<Law>(p.name.empty() ? text : p.name, std::vector<Singular*>{author});
    law->setLawIdentifier(id);
    law->setActivation(p.activation);
    law->setScope(p.scope);
    if (p.condition) law->setConditionModel(*p.condition);
    if (p.action) law->setActionModel(*p.action);
    law->setEnabled(true);
    for (const auto& presetId : p.presetLawIds) {
        if (Law* preset = laws.find(presetId)) {
            law->recordProvenance("branched-from", *law, *preset, true, 1.0f);
        }
    }
    laws.add(law);
    for (const auto& trigger : p.triggers) laws.bindTrigger(id, trigger);

    // Keeping the Law means Zone membership, so Save Zone persists it.
    if (ZoneManager* zones = ZoneManager::live()) {
        if (!zones->adoptLawIntoActiveZone(id)) {
            laws.remove(id);
            _status = "refused: the new Law could not enter the active Zone's authored closure";
            say(_status);
            return;
        }
    }
    _lastCreated = id;
    _status = "authored " + id + " (written by " + author->getIdentifier() + ")";
    say(_status + "\n  " + _preview + notes);
}

void TerminalChannel::say(const std::string& text) { propSetOutput(text); }

void TerminalChannel::propSetOutput(const std::string& v) {
    _output = v;
    if (_sink) {
        _sink(v);
        return;
    }
#ifdef EARTHCALL_TERMINAL_LIBEDIT
    if (_attached && g_attached == this) {
        // Clear the prompt line, print, and redraw the prompt with whatever
        // the Person had typed so far.
        std::fputs("\r\033[K", stdout);
        std::fputs(v.c_str(), stdout);
        std::fputs("\n", stdout);
        if (g_freshLine) {
            // Nothing typed yet: show a clean prompt, then return to column 0 so
            // libedit's own prompt, drawn at the next keystroke, lands on it.
            std::fputs(_prompt.c_str(), stdout);
            std::fputs("\r", stdout);
            std::fflush(stdout);
        } else {
            std::fflush(stdout);
            rl_forced_update_display();            // keep what the Person was typing
        }
        return;
    }
#endif
    std::cout << v << std::endl;
}

void TerminalChannel::buildProperties() {
    registerEnabledProperty();
    using S = std::string;
    const auto writable = [this](const char* name, S TerminalChannel::*member) {
        registerProperty(std::make_unique<PropertyRef<TerminalChannel, S>>(name, this, member));
    };
    const auto level = [this](const char* name, const auto* member) {
        using T = std::remove_cv_t<std::remove_pointer_t<decltype(member)>>;
        registerProperty(std::make_unique<ReadOnlyLevel<T>>(name, member));
    };
    level("lastLine", &_lastLine);
    writable("prompt", &TerminalChannel::_prompt);
    writable("authorPath", &TerminalChannel::_authorPath);
    writable("lexemeRelation", &TerminalChannel::_lexemeRelation);
    writable("scopeBeing", &TerminalChannel::_scopeBeing);
    level("status", &_status);
    level("preview", &_preview);
    level("openClauses", &_openClauses);
    level("lastCreated", &_lastCreated);
    level("ambiguity.symbol", &_ambiguitySymbol);
    level("ambiguity.slot", &_ambiguitySlot);
    level("ambiguity.candidates", &_ambiguityCandidates);
    writable("ambiguity.resolved", &TerminalChannel::_ambiguityResolved);

    registerProperty(std::make_unique<ComputedProperty<TerminalChannel, S>>(
        "output", this, &TerminalChannel::propOutput, &TerminalChannel::propSetOutput));
    registerProperty(std::make_unique<ComputedProperty<TerminalChannel, double>>(
        "speakRequests", this, &TerminalChannel::propSpeakRequests,
        &TerminalChannel::propSetSpeakRequests));
    level("spokenRequests", &_spoken);
    level("linesEntered", &_linesEntered);
    level("attached", &_attached);
}

} // namespace Terminal
} // namespace Singularity
