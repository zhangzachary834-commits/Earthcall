#include "ConstructedBeing/Singular/Property/PropertyValueJson.hpp"
#include "Singularity/Terminal/TerminalChannel.hpp"

#include "ConstructedBeing/Singular/Lexeme/Lexeme.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"
#include "ConstructedBeing/Singular/Property/Property.hpp"
#include "ConstructedBeing/Singular/Property/PropertyRef.hpp"
#include "Person/Person.hpp"
#include "Relation/Relation.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "Singularity/Core/StringId.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "Identity/FirstMoverRegister.hpp"
#include "Identity/PersonPresence.hpp"
#include "Singularity/Core/Engine.hpp"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <set>
#include <sstream>
#include <thread>
#include <type_traits>
#include <typeinfo>
#include <unordered_map>
#include <uuid/uuid.h>

#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
#include <csignal>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#define EARTHCALL_TERMINAL_POSIX 1
#endif

namespace Singularity {
namespace Terminal {

namespace {

constexpr const char* kLineEntered = "terminal-line-entered";
// The Person's line history. EARTHCALL_TERMINAL_HISTORY moves it (probes and
// tests must never write into — or clear — the Person's own history).
std::string historyFile() {
    const char* override = std::getenv("EARTHCALL_TERMINAL_HISTORY");
    return override && *override ? override : "saves/logs/terminal-history.txt";
}
constexpr const char* kLogFile = "saves/logs/earthcall-terminal.log";

// The event vocabulary, as it actually occurs: every event type the world has
// published this session, and how often. No central list of event names.
std::map<std::string, int>& heardEvents() {
    static std::map<std::string, int> heard;
    return heard;
}

void listenForEvents() {
    static const bool subscribed = [] {
        Core::EventBus::instance().subscribe<ECA::Event>([](const ECA::Event& e) {
            if (!e.type.empty()) ++heardEvents()[e.type];
        });
        return true;
    }();
    (void)subscribed;
}

// Individuated like a Lexeme (`lexeme_<uuid>`): the display name a sentence
// gives may be shared by many Laws; the identifier never is.
// `enter`, `enter <zone>`: the line's move, readable before any grammar.
bool isEnterLine(const std::string& text) {
    const std::size_t b = text.find_first_not_of(" \t");
    if (b == std::string::npos || text.size() - b < 5) return false;
    for (std::size_t i = 0; i < 5; ++i) {
        if (std::tolower(static_cast<unsigned char>(text[b + i])) != "enter"[i]) return false;
    }
    return text.size() == b + 5 || text[b + 5] == ' ';
}

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

std::string lowerCopy(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

std::string number(double d) {
    char buf[32];
    std::snprintf(buf, sizeof buf, "%g", d);
    return buf;
}

bool isEventPath(const PropertyPath& path) {
    const std::string text = path.toString();
    return text.rfind("@event.", 0) == 0;
}

bool readsEventContext(const ConditionNode& node) {
    if (isEventPath(node.path) || isEventPath(node.operandPath) || isEventPath(node.probe)) return true;
    for (const auto& binding : node.bindings) {
        if (isEventPath(binding.second)) return true;
    }
    for (const auto& child : node.children) {
        if (readsEventContext(child)) return true;
    }
    return false;
}

// What a value looks like in the menu.
std::string showValue(const PropertyValue& v) {
    if (const auto* d = std::get_if<double>(&v)) return number(*d);
    if (const auto* f = std::get_if<float>(&v)) return number(*f);
    if (const auto* i = std::get_if<int>(&v)) return std::to_string(*i);
    if (const auto* l = std::get_if<long>(&v)) return std::to_string(*l);
    if (const auto* b = std::get_if<bool>(&v)) return *b ? "true" : "false";
    if (const auto* s = std::get_if<std::string>(&v)) {
        return "\"" + (s->size() > 24 ? s->substr(0, 23) + "…" : *s) + "\"";
    }
    if (const auto* c = std::get_if<glm::vec3>(&v)) {
        return "(" + number(c->x) + ", " + number(c->y) + ", " + number(c->z) + ")";
    }
    if (std::holds_alternative<std::monostate>(v)) return "";
    if (std::holds_alternative<glm::mat4>(v)) return "transform";
    if (std::holds_alternative<std::shared_ptr<PropertyList>>(v)) return "list";
    if (std::holds_alternative<std::shared_ptr<PropertyDict>>(v)) return "dictionary";
    return "…";
}

// Examples are built from the words THIS world has — a trigger preset if one
// exists, the scoped being's own property, a value word — never a fixed
// script, so they stay true as the vocabulary grows.
std::string exampleTrigger(const LawSentence::Vocabulary& v) {
    for (const auto& w : v.words) {
        if (w.opcode != "preset") continue;
        for (const auto& p : v.presets) {
            if (p.lawId == w.lawId && !p.triggers.empty() && !p.condition && !p.action) return w.symbol;
        }
    }
    // Otherwise the event this world has actually heard most often.
    std::string best;
    int bestCount = -1;
    for (const auto& e : v.events) {
        const auto it = heardEvents().find(e);
        const int count = it == heardEvents().end() ? 0 : it->second;
        if (count > bestCount) {
            best = e;
            bestCount = count;
        }
    }
    return best.empty() ? "on tick" : "on " + best;
}

std::string examplePath(const LawSentence::Vocabulary& v) {
    std::vector<std::string> props;
    if (v.propertiesOf && !v.scopeBeing.empty()) props = v.propertiesOf(v.scopeBeing);
    if (props.empty() || std::find(props.begin(), props.end(), "color") != props.end()) return "color";
    return props.front();
}

std::string exampleValue(const LawSentence::Vocabulary& v, const std::string& path) {
    if (path == "color") {
        for (const auto& w : v.words) {
            if (w.opcode != "value") continue;
            for (const auto& p : v.presets) {
                if (p.lawId == w.lawId && p.value && std::holds_alternative<glm::vec3>(*p.value)) return w.symbol;
            }
        }
        return "1 0 0";
    }
    return "1";
}

std::string exampleAction(const LawSentence::Vocabulary& v) {
    const std::string path = examplePath(v);
    return "then set " + path + " " + exampleValue(v, path);
}

// "e.g." for whatever the sentence needs next.
std::string exampleFor(const std::string& need, const LawSentence::Vocabulary& v) {
    if (need.find("event") != std::string::npos) return exampleTrigger(v);
    if (need.find("action") != std::string::npos) return exampleAction(v).substr(need.rfind("then", 0) == 0 ? 0 : 5);
    if (need.find("condition") != std::string::npos) return "if " + examplePath(v) + " > 2";
    if (need.find("property path") != std::string::npos) return examplePath(v);
    if (need.find("value") != std::string::npos) return exampleValue(v, "color") + "  or  1";
    return {};
}

// A level that laws may read and, when `writable`, write — over a member of
// any type. NO_BLACK_BOX: "readable by law, writable unless genuinely derived".
template <typename T>
class Level : public Property {
public:
    Level(std::string name, T* member, bool writable)
        : _name(std::move(name)), _id(Earthcall::StringInterner::intern(_name)), _member(member),
          _writable(writable) {}
    std::string name() const override { return _name; }
    Earthcall::StringId nameId() const override { return _id; }
    std::string typeName() const override { return typeid(T).name(); }
    PropertyValue value() const override { return PropertyValue(*_member); }
    bool isStructurallyWritable() const override { return _writable; }
    bool setValue(const PropertyValue& v) override {
        if (!_writable) return false;
        if (const auto* t = std::get_if<T>(&v)) {
            *_member = *t;
            return true;
        }
        if constexpr (std::is_arithmetic_v<T>) {
            if (const auto* d = std::get_if<double>(&v)) { *_member = static_cast<T>(*d); return true; }
            if (const auto* i = std::get_if<int>(&v)) { *_member = static_cast<T>(*i); return true; }
            if (const auto* b = std::get_if<bool>(&v)) { *_member = static_cast<T>(*b); return true; }
        }
        return false;
    }
    Singular* asSingular() const override { return nullptr; }

private:
    std::string _name;
    Earthcall::StringId _id;
    T* _member;
    bool _writable;
};

#ifdef EARTHCALL_TERMINAL_POSIX
// ---------------------------------------------------------------------------
// Below the Kernel: the process's terminal. One channel holds it at a time.
// ---------------------------------------------------------------------------
TerminalChannel* g_attached = nullptr;
termios g_savedTermios{};
bool g_haveSavedTermios = false;
int g_tty = -1;        // the real terminal, kept aside while app output is relayed
int g_savedOut = -1;
int g_savedErr = -1;

void writeTty(const std::string& s) {
    if (g_tty < 0) return;
    std::size_t off = 0;
    while (off < s.size()) {
        const ssize_t n = ::write(g_tty, s.data() + off, s.size() - off);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) break;
        off += static_cast<std::size_t>(n);
    }
}

// The app's own output, relayed: a thread only READS the pipe (it never
// touches the terminal, so it cannot tear the line) and keeps every line in
// the session log; the main thread prints them above the prompt each frame.
// Heap-allocated and never freed: the detached reader may outlive statics.
struct LogRelay {
    std::mutex mutex;
    std::deque<std::string> lines;
    int readFd = -1;
};
LogRelay* g_relay = nullptr;

void relayLoop(LogRelay* relay, int fd) {
    std::ofstream log(kLogFile, std::ios::trunc);
    std::string partial;
    char buf[4096];
    while (true) {
        const ssize_t n = ::read(fd, buf, sizeof buf);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) break;
        partial.append(buf, static_cast<std::size_t>(n));
        std::size_t nl;
        while ((nl = partial.find('\n')) != std::string::npos) {
            std::string line = partial.substr(0, nl);
            if (!line.empty() && line.back() == '\r') line.pop_back();
            partial.erase(0, nl + 1);
            if (log) log << line << '\n' << std::flush;
            std::lock_guard<std::mutex> lock(relay->mutex);
            relay->lines.push_back(std::move(line));
        }
    }
    ::close(fd);
}

void restoreTerminal() {
    if (g_haveSavedTermios) tcsetattr(STDIN_FILENO, TCSANOW, &g_savedTermios);
    writeTty("\x1b[?1000l\x1b[?1006l\x1b[?2004l\x1b[?25h");
    std::fflush(stdout);
    std::fflush(stderr);
    if (g_savedOut >= 0) { ::dup2(g_savedOut, STDOUT_FILENO); ::close(g_savedOut); g_savedOut = -1; }
    if (g_savedErr >= 0) { ::dup2(g_savedErr, STDERR_FILENO); ::close(g_savedErr); g_savedErr = -1; }
    g_attached = nullptr;
}

// Ctrl-C twice, a kill, or a crash ends the process without atexit. Give the
// Person their terminal back first (tcsetattr/dup2/write are async-signal-
// safe), then let the signal do exactly what it always did.
extern "C" void restoreOnSignal(int sig) {
    if (g_haveSavedTermios) tcsetattr(STDIN_FILENO, TCSANOW, &g_savedTermios);
    if (g_savedOut >= 0) ::dup2(g_savedOut, STDOUT_FILENO);
    if (g_savedErr >= 0) ::dup2(g_savedErr, STDERR_FILENO);
    static const char reset[] = "\x1b[?1000l\x1b[?1006l\x1b[?2004l\x1b[?25h\r\n";
    (void)!::write(STDERR_FILENO, reset, sizeof reset - 1);
    if (sig != SIGINT) {
        static const char note[] = "(Earthcall stopped; its output is in saves/logs/earthcall-terminal.log)\r\n";
        (void)!::write(STDERR_FILENO, note, sizeof note - 1);
    }
    std::signal(sig, SIG_DFL);
    std::raise(sig);
}
#endif

double secondsNow() {
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}

} // namespace

TerminalChannel::TerminalChannel() : Law("terminal-channel") {
    setName("Terminal Channel");
    listenForEvents();
    if (std::getenv("NO_COLOR")) _color = false;   // the no-color.org convention
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
    _laws = &laws;
#ifdef EARTHCALL_TERMINAL_POSIX
    // Only a real terminal: under ctest, an IDE, or Finder there is no
    // keyboard on stdin, and the channel stays quiet — and says why, once.
    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO) || g_attached) {
        std::fprintf(stderr, "[terminal-channel] the Law Line is not listening: %s\n",
                     g_attached ? "another Terminal channel already holds this terminal"
                                : "stdin/stdout is not an interactive terminal");
        return;
    }
    // Keep the Person's own settings before anything changes them: every exit
    // path — quit, Ctrl-D, Ctrl-C, a crash — puts them back.
    if (tcgetattr(STDIN_FILENO, &g_savedTermios) != 0) return;
    g_haveSavedTermios = true;
    g_attached = this;
    g_tty = ::dup(STDOUT_FILENO);

    // Character mode: every key arrives as it is pressed. ISIG is off so
    // Ctrl-C clears the line instead of killing the world (twice quits).
    termios raw = g_savedTermios;
    raw.c_lflag &= ~static_cast<tcflag_t>(ICANON | ECHO | ISIG | IEXTEN);
    raw.c_iflag &= ~static_cast<tcflag_t>(ICRNL | IXON);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    for (int sig : {SIGTERM, SIGHUP, SIGQUIT, SIGABRT, SIGSEGV, SIGBUS, SIGINT}) {
        std::signal(sig, restoreOnSignal);
    }
    std::atexit(restoreTerminal);

    std::error_code ec;
    std::filesystem::create_directories("saves/logs", ec);
    if (_relayLogs) {
        int fds[2];
        if (::pipe(fds) == 0) {
            std::fflush(stdout);
            std::fflush(stderr);
            g_savedOut = ::dup(STDOUT_FILENO);
            g_savedErr = ::dup(STDERR_FILENO);
            ::dup2(fds[1], STDOUT_FILENO);
            ::dup2(fds[1], STDERR_FILENO);
            ::close(fds[1]);
            std::setvbuf(stdout, nullptr, _IOLBF, 0);
            if (!g_relay) g_relay = new LogRelay;
            std::thread(relayLoop, g_relay, fds[0]).detach();
        }
    }

    // History survives the session.
    std::vector<std::string> history;
    std::ifstream in(historyFile());
    for (std::string line; std::getline(in, line);) {
        if (!line.empty()) history.push_back(line);
    }
    if (history.size() > 1000) history.erase(history.begin(), history.end() - 1000);
    _editor.setHistory(std::move(history));

    _editor.setProviders(
        [this](const std::string& before) {
            if (!awaitingAnswer() && isEnterLine(before)) {
                // `enter <zone>`: the menu offers the Zones the line can enter.
                std::vector<LawSentence::Suggestion> out;
                const std::size_t b = before.find_first_not_of(" \t");
                std::size_t from = b + 5;
                while (from < before.size() && before[from] == ' ') ++from;
                if (from > before.size() || before.size() == b + 5) return out;
                const std::string typed = lowerCopy(before.substr(from));
                if (ZoneManager* zones = ZoneManager::live()) {
                    for (const auto& z : zones->zones()) {
                        if (!z) continue;
                        const std::string name = z->name();
                        if (lowerCopy(name).rfind(typed, 0) != 0 &&
                            lowerCopy(z->getIdentifier()).rfind(typed, 0) != 0) continue;
                        out.push_back({from, z->getIdentifier(), name == z->getIdentifier() ? "a Zone" : name,
                                       "being", 1, "move the line (not your body) into " + name, {}});
                    }
                }
                return out;
            }
            if (!awaitingAnswer() && !lawGrammarHere()) return std::vector<LawSentence::Suggestion>{};
            if (!awaitingAnswer()) {
                long shift = 0;
                const std::string context = blockContext(before, shift);
                auto out = LawSentence::suggest(context, liveVocabulary());
                if (shift == 0) return out;
                std::vector<LawSentence::Suggestion> mapped;
                for (auto& sg : out) {
                    const long from = static_cast<long>(sg.from) - shift;
                    if (from < 0) continue;   // belongs to an earlier line of the block
                    sg.from = static_cast<std::size_t>(from);
                    mapped.push_back(std::move(sg));
                }
                return mapped;
            }
            // Answering a question: the only words are its answers.
            const std::size_t space = before.find_last_of(' ');
            const std::string word = space == std::string::npos ? before : before.substr(space + 1);
            const std::size_t from = before.size() - word.size();
            std::vector<LawSentence::Suggestion> out;
            const auto offer = [&](const std::string& text, const std::string& what) {
                if (lowerCopy(text).rfind(lowerCopy(word), 0) == 0) out.push_back({from, text, what, "value", 1, what, {}});
            };
            if (_pendingTargets.size() > 1) {
                for (std::size_t i = 0; i < _pendingTargets.size(); ++i) {
                    Law* law = _laws ? _laws->find(_pendingTargets[i]) : nullptr;
                    offer(std::to_string(i + 1), law ? law->name() + " · " + lawSummary(*law, *_laws) : _pendingTargets[i]);
                }
            } else {
                offer("yes", "delete it");
            }
            offer("no", "keep it");
            return out;
        },
        [this](const std::string& text) {
            if (isEnterLine(text) || !lawGrammarHere()) return std::vector<LawSentence::Span>{};
            long shift = 0;
            const std::string context = blockContext(text, shift);
            if (shift == 0) return liveParse(context).spans;
            std::vector<LawSentence::Span> mapped;
            for (auto span : liveParse(context).spans) {
                const long start = static_cast<long>(span.start) - shift;
                if (start < 0) continue;
                span.start = static_cast<std::size_t>(start);
                span.end = static_cast<std::size_t>(static_cast<long>(span.end) - shift);
                mapped.push_back(span);
            }
            return mapped;
        },
        [this](const std::string& text) {
            long shift = 0;
            const std::string context = blockContext(text, shift);
            auto status = statusOf(context);
            if (_block.empty()) return status;
            for (auto& line : status) {
                if (line.errorOffset == std::string::npos) continue;
                const long at = static_cast<long>(line.errorOffset) - shift;
                line.errorOffset = at < 0 ? std::string::npos : static_cast<std::size_t>(at);   // caret only on this line
            }
            status.insert(status.begin(), LineEditor::Status{
                "block · line " + std::to_string(_block.size() + 1) + " · an empty line authors it · ctrl-c discards it",
                "note", std::string::npos});
            return status;
        });
    // Enter on a sentence that cannot be authored yet keeps the line and says
    // what is missing, instead of filling the scrollback with refusals.
    _editor.submitGate = [this](const std::string& text) { return submitRefusal(text); };

    writeTty("\x1b[?2004h");   // bracketed paste: a pasted sentence arrives as one
    _attached = true;
    printAbove(std::string(_color ? "\x1b[1;38;5;141m" : "") + "The Law Line is listening." +
               (_color ? "\x1b[0m\x1b[2m" : "") +
               "  Type a sentence; the menu follows you. Enter authors it, a trailing ? previews, "
               "?? searches." +
               (_color ? "\x1b[0m" : ""));
#else
    std::fprintf(stderr, "[terminal-channel] the Law Line is not listening: no POSIX terminal here\n");
#endif
}

void TerminalChannel::detach() {
#ifdef EARTHCALL_TERMINAL_POSIX
    if (_attached && g_attached == this) {
        writeTty(erase());
        _drawn = false;
        restoreTerminal();
    }
#endif
    _attached = false;
}

int TerminalChannel::width() const {
#ifdef EARTHCALL_TERMINAL_POSIX
    winsize ws{};
    if (g_tty >= 0 && ::ioctl(g_tty, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) return ws.ws_col;
#endif
    return 80;
}

std::string TerminalChannel::erase() const {
    if (!_drawn) return {};
    std::string s = "\r";
    if (_cursorRow > 0) s += "\x1b[" + std::to_string(_cursorRow) + "A";
    return s + "\x1b[J";
}

void TerminalChannel::draw() {
#ifdef EARTHCALL_TERMINAL_POSIX
    if (!_attached) return;
    _editor.prompt = _prompt;
    if (!_zone.empty()) {
        // "earthcall> " -> "earthcall[LawLine]> "
        std::string base = _prompt;
        std::string tail;
        const std::size_t gt = base.rfind('>');
        if (gt != std::string::npos) { tail = base.substr(gt); base = base.substr(0, gt); }
        _editor.prompt = base + "[" + zoneLabel() + "]" + tail;
    }
    _editor.secret = _secretStage != 0;
    _editor.submitEmpty = !_block.empty();
    if (!_block.empty()) {
        // Python's continuation prompt, as wide as the real one.
        const std::size_t width = visibleWidth(_editor.prompt);
        _editor.prompt = std::string(width > 4 ? width - 4 : 0, ' ') + "... ";
    }
    if (_secretStage == 1) _editor.prompt = "passphrase (hidden) › ";
    if (_secretStage == 2) _editor.prompt = "choose a passphrase for your new key (hidden) › ";
    if (_secretStage == 3) _editor.prompt = "type it again (hidden) › ";
    if (awaitingAnswer() && _secretStage == 0) {
        // The Metalaw's question, about what was named, as the prompt itself.
        const std::string about = _pendingTargets.size() > 1 ? "one of these" : "“" + _pendingNames + "”";
        _editor.prompt = _question + " " + about + "? " +
                         (_pendingTargets.size() > 1 ? "(1–" + std::to_string(_pendingTargets.size()) + " / no)"
                                                     : "(yes / no)") +
                         " › ";
    }
    bool hears = false;
    _editor.footer = footerText(hears);
    _editor.footerMark = hears ? "32" : "33";
    _editor.menuRows = std::max(1, _menuRows);
    _editor.autoMenu = _autoMenu;
    _editor.color = _color;
    _editor.hints = _hints;
    const int w = width();
    _lastWidth = w;
    const LineEditor::Frame f = _editor.render(w);
    // One write, cursor hidden, inside a synchronized update where supported:
    // the region changes in place without flicker.
    std::string out = "\x1b[?2026h\x1b[?25l" + erase() + f.text;
    const int up = f.rows - 1 - f.cursorRow;
    if (up > 0) out += "\x1b[" + std::to_string(up) + "A";
    out += "\r";
    if (f.cursorCol > 0) out += "\x1b[" + std::to_string(f.cursorCol) + "C";
    out += "\x1b[?25h\x1b[?2026l";
    // Report the mouse only while there is something to point at; the rest
    // of the time the terminal scrolls and selects text as it always does.
    const bool wantMouse = _editor.wantsMouse();
    if (wantMouse != _mouseOn) {
        out += wantMouse ? "\x1b[?1000h\x1b[?1006h" : "\x1b[?1000l\x1b[?1006l";
        _mouseOn = wantMouse;
    }
    if (_mouseOn) {
        out += "\x1b[6n";   // where did the cursor land? (maps clicks to rows)
        _decoder.expectCursorReport();
        _reportRegionRow = f.cursorRow;
    }
    writeTty(out);
    _cursorRow = f.cursorRow;
    _drawn = true;
#endif
}

void TerminalChannel::printAbove(const std::string& text) {
#ifdef EARTHCALL_TERMINAL_POSIX
    std::string body;
    for (char c : text) {
        if (c == '\n') body += "\r\n";
        else body += c;
    }
    writeTty("\x1b[?25l" + erase() + body + "\r\n");
    _drawn = false;
    draw();
#else
    std::cout << text << std::endl;
#endif
}

void TerminalChannel::handleKeys(const std::vector<Key>& keys, double now) {
    for (Key key : keys) {
        if (key.kind == Key::Kind::CursorReport) {
            _reportScreenRow = key.y;   // 1-based screen row of the cursor, drawn at _reportRegionRow
            continue;
        }
        if (key.kind == Key::Kind::Click) {
            if (_reportScreenRow < 0) continue;   // not yet known where the region is
            key.y = key.y - (_reportScreenRow - _reportRegionRow);   // screen row -> region row
            key.x = key.x - 1;                                      // 1-based -> 0-based column
        }
        if (key.kind == Key::Kind::Escape && awaitingAnswer() && _editor.buffer().empty() &&
            !_editor.menuVisible()) {
            cancelDeletion("nothing deleted");   // Esc is a no
            continue;
        }
        switch (_editor.press(key)) {
            case LineEditor::Outcome::Submitted: {
                std::string line = _editor.takeSubmitted();
                if (_secretStage != 0) {
                    // Never echoed, never in history: bullets and a note only.
                    printAbove((_color ? "\x1b[2m" : "") + std::string("(passphrase entered · hidden)") +
                               (_color ? "\x1b[0m" : ""));
                    _pending.push_back(std::move(line));
                    break;
                }
                printAbove(_editor.echo(line));
                std::string t = line;
                t.erase(0, t.find_first_not_of(" \t"));
                while (!t.empty() && t.back() == ' ') t.pop_back();
                if (awaitingAnswer()) {          // the answer to a Metalaw's question
                    _pending.push_back(line);    // answered in sense(); not kept in history
                    break;
                }
                if (t == "help" || t == "?") {   // a reading of the terminal, not a Law
                    showHelp();
                    break;
                }
                _pending.push_back(line);
                std::ofstream(historyFile(), std::ios::app) << line << '\n';
                break;
            }
            case LineEditor::Outcome::Help:
                showHelp();
                break;
            case LineEditor::Outcome::Interrupt:
                if (_secretStage != 0) {
                    cancelSecret("nothing unlocked");
                    break;
                }
                if (awaitingAnswer()) {          // Ctrl-C is a no
                    cancelDeletion("nothing deleted");
                    break;
                }
                if (!_block.empty()) {
                    _block.clear();
                    _editor.prefill("");
                    printAbove("(block discarded; nothing authored)");
                    break;
                }
                if (now - _lastInterrupt < 2.0) {
                    printAbove("(quitting Earthcall)");
#ifdef EARTHCALL_TERMINAL_POSIX
                    std::raise(SIGINT);
#endif
                    return;
                }
                _lastInterrupt = now;
                _editor.setNotice("press ctrl-c again to quit Earthcall · ctrl-d closes only this line");
                break;
            case LineEditor::Outcome::EndOfInput:
                printAbove("(the Law Line is closed; the world keeps running)");
                detach();
                return;
            case LineEditor::Outcome::Redraw:
#ifdef EARTHCALL_TERMINAL_POSIX
                writeTty("\x1b[2J\x1b[H");
#endif
                _drawn = false;
                break;
            case LineEditor::Outcome::None:
                break;
        }
    }
}

void TerminalChannel::sense(LawManager& laws) {
    _laws = &laws;
    ++_frame;
    if (!isEnabled()) return;
    if (!_attachTried) attach(laws);

#ifdef EARTHCALL_TERMINAL_POSIX
    if (_attached && g_attached == this) {
        // Whatever the keyboard has sent, without ever waiting for it.
        std::string bytes;
        char buf[4096];
        while (bytes.size() < 65536) {
            fd_set readable;
            FD_ZERO(&readable);
            FD_SET(STDIN_FILENO, &readable);
            timeval zero{0, 0};
            if (select(STDIN_FILENO + 1, &readable, nullptr, nullptr, &zero) <= 0) break;
            const ssize_t n = ::read(STDIN_FILENO, buf, sizeof buf);
            if (n <= 0) break;
            bytes.append(buf, static_cast<std::size_t>(n));
        }
        const double now = secondsNow();
        std::vector<Key> keys;
        if (!bytes.empty()) keys = _decoder.feed(bytes, now);
        for (auto& k : _decoder.flush(now)) keys.push_back(std::move(k));
        bool dirty = !keys.empty();
        if (!keys.empty()) handleKeys(keys, now);

        // The app's own output, above the line, dimmed.
        if (g_relay && _attached) {
            std::vector<std::string> lines;
            {
                std::lock_guard<std::mutex> lock(g_relay->mutex);
                while (!g_relay->lines.empty() && lines.size() < 200) {
                    lines.push_back(std::move(g_relay->lines.front()));
                    g_relay->lines.pop_front();
                }
            }
            if (!lines.empty()) {
                std::string block;
                for (const auto& l : lines) {
                    if (!block.empty()) block += "\n";
                    block += _color ? "\x1b[2m" + l + "\x1b[0m" : l;
                }
                printAbove(block);
                dirty = false;   // printAbove redrew
            }
        }
        if (_attached && width() != _lastWidth) dirty = true;
        if (_attached) {
            bool hears = false;
            const std::string liveFooter = footerText(hears);
            const std::string liveMark = hears ? "32" : "33";
            if (_editor.footer != liveFooter || _editor.footerMark != liveMark) dirty = true;
        }
        if (_attached && (dirty || !_drawn)) draw();
    }
#endif

    if (_pending.empty()) return;
    placeLine();   // a line typed in the very first frame still has a location
    std::string line = _pending.front();
    _pending.pop_front();
    // A secret line (Identity) is consumed here and nowhere else: never
    // published, never _lastLine, never history, never a property.
    if (_secretStage != 0) {
        takeSecret(line);
        if (_attached) draw();
        return;
    }
    // While a Metalaw's question waits, the next line is its answer — never a
    // new sentence. An empty line answers too: it is not a yes.
    if (awaitingAnswer()) {
        answer(laws, line);
        if (_attached) draw();
        return;
    }
    // A Python-style block: a line ending in ':' opens it, every line joins
    // it, and an empty line folds it into one sentence of the same grammar.
    if (lawGrammarHere()) {
        std::string t = line;
        while (!t.empty() && (t.back() == ' ' || t.back() == '\t')) t.pop_back();
        const bool blank = t.find_first_not_of(" \t") == std::string::npos;
        if (!_block.empty() || (!blank && t.back() == ':')) {
            if (!blank) {
                _block.push_back(t);
                // Auto-indent: under a header, two deeper; otherwise the same.
                const std::size_t indent = t.find_first_not_of(" \t");
                if (_attached) {
                    _editor.prefill(std::string(indent + (t.back() == ':' ? 2 : 0), ' '));
                    draw();
                }
                return;
            }
            std::string error;
            const std::string folded = LawSentence::unfoldBlock(_block, liveVocabulary(), error);
            _block.clear();
            if (!error.empty() || folded.empty()) {
                say("refused: " + (error.empty() ? std::string("the block is empty") : error));
                if (_attached) draw();
                return;
            }
            say("⤷ " + folded);
            line = folded;
        }
    }
    if (line.find_first_not_of(" \t") == std::string::npos) return;
    {
        std::string t = line;
        t.erase(0, t.find_first_not_of(" \t"));
        while (!t.empty() && (t.back() == ' ' || t.back() == '\t')) t.pop_back();
        if (handleEnter(t)) {
            if (_attached) draw();
            return;
        }
    }

    _lastLine = line;
    _linesEntered += 1.0;
    Core::EventBus::instance().publish(
        ECA::Event{kLineEntered, this, nullptr, std::time(nullptr), std::string{}});
    if (!laws.rete().hearsType(kLineEntered)) {
        say("(no Law where the line is hears terminal-line-entered, so nothing will act on that line. "
            "`enter LawLine` to author Laws, `enter Identity` to become present, `enter` to list Zones.)");
    }
}

void TerminalChannel::act(LawManager& laws) {
    _laws = &laws;
    placeLine();
    // A Law asked the kernel to take one secret line (the Identity Zone).
    if (_unlockRequests != _unlocksHandled) {
        _unlocksHandled = _unlockRequests;
        beginSecret();
    }
    // A confirmed deletion: the deleting Metalaw ran this tick (or did not).
    if (!_deletingId.empty()) {
        const bool gone = laws.find(_deletingId) == nullptr && findBeing(_deletingId) == nullptr;
        say(gone ? "deleted " + _deletingName + " (" + _deletingId + ")"
                 : "refused: " + _deletingName + " is still here — no Law in this Zone destroyed it");
        _deletingId.clear();
        _deletingName.clear();
        _vocab.reset();
    }
    // A Metalaw just asked its question: show it as the prompt.
    if (awaitingAnswer() != _wasAwaiting) {
        _wasAwaiting = awaitingAnswer();
        if (_attached) {
            _editor.refresh();
            draw();
        }
    }
    if (_speakRequests == _spoken) return;
    _spoken = _speakRequests;
    speak(laws, _lastLine);
    if (_attached) {
        _vocab.reset();          // the world just changed: a Law was born
        _parse.reset();
        _editor.refresh();
        draw();
    }
}

LawSentence::Vocabulary TerminalChannel::vocabulary(LawManager& laws, const std::string& speakingAuthor) {
    LawSentence::Vocabulary v;
    v.words = LawSentence::canonicalWords();
    v.compileInvocation = [this, &laws](const nlohmann::json& input, bool readOnly) {
        return compileByMetalaw(laws, input, readOnly);
    };
    std::string authorId = speakingAuthor;
    if (authorId.empty()) {
        PropertyValue author;
        if (lawGetValue(*this, PropertyPath::parse(_authorPath), author))
            if (auto id = std::get_if<std::string>(&author)) {
                if (auto being = findBeing(*id)) authorId = being->getIdentifier();
            }
    }

    // Lexeme <--denotes--> Law: the Law holds the opcode, and its name is what
    // the menu says the word means.
    for (Relation* r : Universe::instance().relations()) {
        if (!r || r->type != _lexemeRelation) continue;
        auto* lexeme = dynamic_cast<Singularity::Language::Lexeme*>(r->a());
        auto* law = dynamic_cast<Law*>(r->b());
        if (!lexeme || !law || law == this) continue;
        LawSentence::Preset preset;
        const std::string opcode = LawSentence::classify(*law, laws.triggersOf(law->getIdentifier()), preset);
        if (opcode.empty()) continue;
        // A root pronoun is authored vocabulary too, not a Create special case.
        PropertyValue rootMarker;
        if (preset.value && law->getDynamicProperty("sentence.root", rootMarker) &&
            rootMarker == PropertyValue(true)) {
            if (auto root = std::get_if<std::string>(&*preset.value)) {
                const auto resolved = *root == "$author" ? (authorId.empty() ? "" : "@" + authorId) : *root;
                if (!resolved.empty()) {
                    auto [entry, inserted] = v.pathRoots.emplace(lexeme->getSymbol(), resolved);
                    if (!inserted && entry->second != resolved) entry->second.clear(); // ambiguity refuses; no last-writer selection
                }
            }
        }
        // The menu's detail line says what the denoted Law actually holds.
        std::string detail;
        if (opcode == "value" && preset.value) {
            detail = showValue(*preset.value) + " · \"" + lexeme->getSymbol() + "\" denotes " + law->getIdentifier();
        } else if (opcode == "preset") {
            detail = "fixes: " + lawSummary(*law, laws);
        } else {
            detail = law->name() + " · \"" + lexeme->getSymbol() + "\" denotes " + law->getIdentifier();
        }
        v.words.push_back({lexeme->getSymbol(), opcode, lexeme->getIdentifier(), law->getIdentifier(),
                           law->name(), detail});
        PropertyValue expression;
        if (law->getDynamicProperty("sentence.math", expression))
            if (auto text = std::get_if<std::string>(&expression)) v.words.back().expression = *text;
        PropertyValue arguments;
        if (law->getDynamicProperty("sentence.arguments", arguments))
            if (auto text = std::get_if<std::string>(&arguments)) v.words.back().arguments = *text;
        if (opcode == "preset" || opcode == "value" ||
            (opcode.rfind("action.", 0) == 0 && preset.action)) {
            v.presets.push_back(preset);
        }
    }

    std::set<std::string> events;
    for (const auto& [type, count] : heardEvents()) events.insert(type);
    for (const auto& law : laws.getAll()) {
        if (!law) continue;
        for (const auto& t : laws.triggersOf(law->getIdentifier())) events.insert(t);
        if (law->hasActionModel()) collectPublished(*law->actionModel(), events);
    }
    v.events.assign(events.begin(), events.end());

    // Offer the Event Singular's registered properties from its registry,
    // not a hand-maintained list. The old subject/object spellings remain
    // participant aliases until Event-defining Relations become traversable.
    ECA::Event eventProbe;
    for (Property* property : eventProbe.listProperties()) {
        if (!property || property->name() == "subject" || property->name() == "object") continue;
        v.eventProperties.push_back(property->name());
    }

    // The Laws spoken or authored here, by name — for "delete Blue".
    for (const auto& law : laws.getAll()) {
        if (!law || law->isFirstMover() || law.get() == this || !law->isEnabled()) continue;
        v.laws.push_back({law->getIdentifier(), law->name(), lawSummary(*law, laws)});
    }

    // One walk over the Universe, describing each being as it passes. The menu
    // used to call describeBeing(id) per offered being, and each call rebuilt
    // and scanned the whole Universe: O(N^2) per keystroke after '@' (Zach,
    // 2026-10-05: "whenever I enter @ its really laggy"; 708 beings, ~0.5 s).
    // Only STRINGS are kept, never Singular*: a vocabulary can outlive a tick,
    // and Laws/Relations may leave without a structural-revision bump, so a
    // cached pointer could dangle. A stale string is merely an old label.
    std::set<std::string> beings;
    auto described = std::make_shared<std::unordered_map<std::string, std::string>>();
    for (Singular* being : Universe::instance().beings()) {
        if (!being || being->getIdentifier().empty()) continue;
        if (beings.insert(being->getIdentifier()).second)
            (*described)[being->getIdentifier()] = describeSingular(being);
    }
    v.beings.assign(beings.begin(), beings.end());
    // Bare paths complete against the authored suggestion being — or, when
    // none is set, against whatever the Person last clicked in the world.
    v.scopeBeing = propertySuggestionBeing();

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
    v.describeProperty = [this](const std::string& b, const std::string& p) { return describeProperty(b, p); };
    v.describeBeing = [this, described](const std::string& b) {
        const auto it = described->find(b);
        return it != described->end() ? it->second : describeBeing(b);   // e.g. a Lexeme only a Relation holds
    };
    // A being's properties with their current values, from ONE lookup (the
    // per-property describeProperty re-found the being for every name).
    v.describedPropertiesOf = [](const std::string& id) {
        std::vector<std::pair<std::string, std::string>> out;
        Singular* being = findBeing(id);
        if (!being) return out;
        std::vector<std::string> names;
        for (Property* p : being->listProperties()) if (p) names.push_back(p->name());
        for (const auto& entry : being->dynamicProperties())
            names.push_back(Earthcall::StringInterner::resolve(entry.first));
        std::sort(names.begin(), names.end());
        names.erase(std::unique(names.begin(), names.end()), names.end());
        for (const auto& name : names) {
            PropertyValue value;
            std::string shown;
            if (lawGetValue(*being, PropertyPath::parse(name), value)) {
                shown = showValue(value);
                if (!shown.empty()) shown = "= " + shown;
            }
            out.emplace_back(name, std::move(shown));
        }
        return out;
    };
    // What an event means is AUTHORED, on the Zone: the string property
    // `meaning.<event>` (Zach, 2026-10-05: "just give events authored string
    // property now no new fields"; "The Zone"). Written by an ordinary
    // AddProperty Law, saved with the Zone, and different Zones may mean
    // different things. The line's own Zone speaks first, then the body's.
    auto meanings = std::make_shared<std::map<std::string, std::string>>();
    if (ZoneManager* zones = ZoneManager::live()) {
        std::vector<Zone*> readers;
        if (!_zone.empty()) if (Zone* line = zones->findZone(_zone)) readers.push_back(line);
        if (!zones->zones().empty()) readers.push_back(&zones->active());
        for (const auto& e : v.events) {
            for (Zone* zone : readers) {
                PropertyValue meaning;
                if (lawGetValue(*zone, PropertyPath::parse("meaning." + e), meaning))
                    if (const auto* text = std::get_if<std::string>(&meaning); text && !text->empty()) {
                        (*meanings)[e] = *text;
                        break;
                    }
            }
        }
    }
    v.describeEvent = [meanings](const std::string& e) {
        const auto m = meanings->find(e);
        std::string out = m == meanings->end() ? std::string("event") : "event · " + m->second;
        const auto it = heardEvents().find(e);
        if (it != heardEvents().end()) out += " · heard " + std::to_string(it->second) + "×";
        return out;
    };
    v.resolve = [this, &laws](const LawSentence::Ambiguity& a) { return resolveByMetalaw(laws, a); };
    return v;
}

std::string TerminalChannel::propBlock() const {
    std::string out;
    for (const auto& l : _block) out += (out.empty() ? "" : "\n") + l;
    return out;
}

std::string TerminalChannel::blockContext(const std::string& line, long& shift) {
    shift = 0;
    if (_block.empty() || !lawGrammarHere()) return line;
    const std::size_t lead = line.find_first_not_of(" \t");
    if (lead == std::string::npos) return line;
    std::string own = line.substr(lead);
    if (!own.empty() && own.back() == ':') return line;   // a header in progress reads alone
    std::vector<std::string> lines = _block;
    lines.push_back(line);
    std::string error;
    const std::string folded = LawSentence::unfoldBlock(lines, liveVocabulary(), error);
    // The line's own text, trimmed by the fold, ends the folded sentence.
    while (!own.empty() && (own.back() == ' ' || own.back() == '\t')) own.pop_back();
    if (!error.empty() || folded.size() < own.size() ||
        folded.compare(folded.size() - own.size(), own.size(), own) != 0) return line;
    // Keep the typed trailing space: it means "the word is finished".
    const std::string tail = line.substr(lead + own.size());
    shift = static_cast<long>(folded.size() - own.size()) - static_cast<long>(lead);
    return folded + tail;
}

std::string TerminalChannel::propertySuggestionBeing() const {
    if (!_scopeBeing.empty()) return _scopeBeing;
    PropertyValue focused;
    if (lawGetValue(const_cast<TerminalChannel&>(*this), PropertyPath::parse("@interaction-channel.focusedId"), focused)) {
        if (const auto* id = std::get_if<std::string>(&focused)) return *id;
    }
    return {};
}

std::string TerminalChannel::describeProperty(const std::string& beingId, const std::string& property) const {
    Singular* being = findBeing(beingId);
    if (!being) return {};
    PropertyValue v;
    if (!lawGetValue(*being, PropertyPath::parse(property), v)) return {};
    const std::string shown = showValue(v);
    return shown.empty() ? std::string{} : "= " + shown;
}

std::string TerminalChannel::describeBeing(const std::string& beingId) const {
    Singular* being = findBeing(beingId);
    if (!being) {
        // Lexemes are authored Relation endpoints and need not be registered as
        // standalone Universe beings. They are still individuated beings that
        // Law Line must be able to describe by their authored symbol.
        for (Relation* relation : Universe::instance().relations()) {
            if (!relation) continue;
            for (Singular* endpoint : {relation->a(), relation->b()}) {
                if (endpoint && endpoint->getIdentifier() == beingId &&
                    dynamic_cast<Singularity::Language::Lexeme*>(endpoint)) {
                    being = endpoint;
                    break;
                }
            }
            if (being) break;
        }
    }
    return describeSingular(being);
}

// What the menu says a being is, from the being itself (no lookup).
std::string TerminalChannel::describeSingular(Singular* being) {
    if (!being) return {};
    std::string kind = dynamic_cast<Person*>(being)                              ? "person"
                     : dynamic_cast<Zone*>(being)                                ? "zone"
                     : dynamic_cast<Law*>(being)                                 ? "law"
                     : dynamic_cast<Relation*>(being)                            ? "relation"
                     : dynamic_cast<Singularity::Language::Lexeme*>(being)       ? "lexeme"
                     : dynamic_cast<Object*>(being)                              ? "object"
                                                                                 : "being";
    if (auto* lex = dynamic_cast<Singularity::Language::Lexeme*>(being)) return kind + " · " + lex->getSymbol();
    PropertyValue name;
    if (being->getDynamicProperty("displayName", name)) {
        if (const auto* s = std::get_if<std::string>(&name); s && !s->empty()) return kind + " · " + *s;
    }
    if (auto* law = dynamic_cast<Law*>(being)) return kind + " · " + law->name();
    return kind;
}

const LawSentence::Vocabulary& TerminalChannel::liveVocabulary() {
    if (!_vocab || _vocabFrame != _frame) {
        _vocab = vocabulary(*_laws);
        // Live reading must not act OR pretend to resolve meaning. Preserve
        // every grammar-admissible denotation while the Person is typing; the
        // actual vocabulary() resolver invokes the world's Metalaws only when
        // the sentence is spoken.
        _vocab->compileInvocation = [](const nlohmann::json& input, bool) {
            LawSentence::Compilation placeholder{std::nullopt, "", {}, std::nullopt};
            if (input.value("slot", "") == "condition") placeholder.condition = ConditionNode::all({});
            else placeholder.action = ActionNode::sequence({});
            return placeholder;
        };
        _vocab->resolve = [](const LawSentence::Ambiguity&) {
            return LawSentence::Resolution{"", "a Metalaw decides which when it is spoken"};
        };
        _vocabFrame = _frame;
        _parse.reset();
    }
    return *_vocab;
}

const LawSentence::Parse& TerminalChannel::liveParse(const std::string& text) {
    if (!_parse || _parseText != text) {
        const bool question = !text.empty() && text.find_last_not_of(" \t") != std::string::npos &&
                              text[text.find_last_not_of(" \t")] == '?';
        // Read as a preview while typing: an unfinished sentence is not an error.
        std::string splitError;
        auto parts = LawSentence::sentences(question ? text : text + "?", splitError);
        std::string tail = parts.size() > 1 ? parts.back() : (question ? text : text + "?");
        _parse = LawSentence::parse(tail, liveVocabulary());
        if (parts.size() > 1) {
            auto offset = text.rfind(tail.substr(0, tail.size() - (tail.back() == '?' ? 1 : 0)));
            if (offset != std::string::npos) {
                for (auto& span : _parse->spans) { span.start += offset; span.end += offset; }
            }
        }
        _parseText = text;
    }
    return *_parse;
}

std::vector<LineEditor::Status> TerminalChannel::statusOf(const std::string& text) {
    std::vector<LineEditor::Status> out;
    if (awaitingAnswer()) {
        if (_pendingTargets.size() > 1) {
            out.push_back({"Several share that name — which one?", "question"});
            for (std::size_t i = 0; i < _pendingTargets.size(); ++i) {
                Law* law = _laws ? _laws->find(_pendingTargets[i]) : nullptr;
                out.push_back({std::to_string(i + 1) + ")  " +
                                   (law ? law->name() + " · " + lawSummary(*law, *_laws) : _pendingTargets[i]),
                               "note"});
            }
        }
        out.push_back({"only yes deletes · no, Esc or Ctrl-C keeps it", "note"});
        return out;
    }
    if (isEnterLine(text)) {
        std::string target = text.substr(text.find_first_not_of(" \t") + 5);
        target.erase(0, target.find_first_not_of(" \t"));
        while (!target.empty() && target.back() == ' ') target.pop_back();
        ZoneManager* zones = ZoneManager::live();
        if (target.empty()) {
            out.push_back({"↵ lists the Zones the line can enter", "note"});
        } else if (zones && zones->findZoneIndex(target) != static_cast<size_t>(-1)) {
            out.push_back({"↵ moves the line into " + zones->zones()[zones->findZoneIndex(target)]->name() +
                               " (your body stays where it is)", "preview"});
        } else {
            out.push_back({"no single Zone is named '" + target + "' · `enter` lists them", "error"});
        }
        return out;
    }
    if (!lawGrammarHere()) {
        out.push_back({"the line is in " + zoneLabel() + ": the Law grammar is not read here · "
                       "`enter LawLine` to author Laws", "note"});
        return out;
    }
    if (const std::size_t blank = text.find("\u2039"); blank != std::string::npos) {
        const std::size_t close = text.find("\u203A", blank);
        const std::string name = close == std::string::npos ? "‹…›" : text.substr(blank, close + 3 - blank);
        out.push_back({"fill " + name + " — type to fill it · the menu shows what fits · tab jumps to the next blank",
                       "note"});
        return out;
    }
    if (text.find_first_not_of(" \t") == std::string::npos) {
        const auto& v = liveVocabulary();
        out.push_back({"try:  " + exampleTrigger(v) + " " + exampleAction(v), "note"});
        out.push_back({"shape: [preset] [called <name>] [on <event> | when …] [if <condition>] then <action>",
                       "note"});
        return out;
    }
    const LawSentence::Parse& p = liveParse(text);
    if (p.search) {
        const std::size_t shown = std::min<std::size_t>(p.candidates.size(), 6);
        for (std::size_t i = 0; i < shown; ++i) out.push_back({p.candidates[i], "note"});
        out.push_back({p.candidates.empty() ? "nothing matches"
                                            : std::to_string(p.candidates.size()) + " matches",
                       "preview"});
        return out;
    }
    if (!p.error.empty()) {
        // Ambiguity during live typing is truthful semantic plurality, not a
        // malformed sentence. Keep it visually open until a Metalaw gets the
        // authority to choose when the sentence is actually spoken.
        if (p.error.find("no Metalaw resolves which one") != std::string::npos) {
            out.push_back({"meaning stays open while typing · Metalaw decides when spoken", "preview"});
            if (!p.candidates.empty()) out.push_back({"meanings: " + join(p.candidates, ", "), "note"});
            return out;
        }
        const std::size_t lead = text.find_first_not_of(" \t");
        const std::size_t at = (lead == std::string::npos ? 0 : lead) + p.errorOffset;
        // Still typing is not a mistake: a sentence that merely stops early,
        // or trips on the very word being typed, says what may come next.
        const std::size_t lastWord = text.find_last_of(" \t") == std::string::npos
                                         ? 0 : text.find_last_of(" \t") + 1;
        const bool typingIt = !text.empty() && text.back() != ' ' && at >= lastWord;
        const std::string ends = "the sentence ends where ";
        if (p.error.rfind(ends, 0) == 0) {
            std::string next = p.error.substr(ends.size());
            const std::size_t was = next.rfind(" was expected");
            if (was != std::string::npos) next = next.substr(0, was);
            const std::string eg = exampleFor(next, liveVocabulary());
            out.push_back({"next: " + next + (eg.empty() ? "" : "     e.g.  " + eg), "note"});
            return out;
        }
        if (typingIt) return out;   // the menu is the answer while the word is unfinished
        out.push_back({p.error, "error", at});
        if (!p.candidates.empty()) out.push_back({"candidates: " + join(p.candidates, ", "), "note"});
        return out;
    }
    out.push_back({p.immediate ? "asks a Metalaw first, then deletes " + p.destroyTarget + " only on your yes"
                               : p.preview(),
                   "preview"});
    for (const auto& n : p.notes) out.push_back({n, "note"});
    // "…?": who the IF holds for right now (read-only).
    const std::size_t lastChar = text.find_last_not_of(" \t");
    if (!p.immediate && lastChar != std::string::npos && text[lastChar] == '?') out.push_back({dryRun(p), "note"});
    // What the sentence still needs, with a real example from this world.
    for (const auto& clause : p.openClauses) {
        if (clause.find("(optional)") != std::string::npos) continue;
        const std::string eg = exampleFor(clause, liveVocabulary());
        out.push_back({"next: " + clause + (eg.empty() ? "" : "     e.g.  " + eg), "note"});
        break;
    }
    return out;
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

namespace {
// The terminal is a structured-text channel. Template instantiation is an
// irreducible structural operation: $slot JSON pointers are substituted, with
// an authored $default only when a referenced input is absent.
// It knows no kinds, properties, creation policy, or action lowering rules.
nlohmann::json instantiateSentenceTemplate(const nlohmann::json& pattern,
                                           const nlohmann::json& input, unsigned depth = 0) {
    if (depth >= 32) throw std::runtime_error("template nesting exceeds the channel's 32-level structural bound");
    if (pattern.is_object() && pattern.contains("$slot")) {
        if (pattern.size() > (pattern.contains("$default") ? 2 : 1) || !pattern["$slot"].is_string())
            throw std::runtime_error("$slot must be a JSON-pointer reference with an optional $default");
        const auto pointer = nlohmann::json::json_pointer(pattern["$slot"].get<std::string>());
        if (!input.contains(pointer) && pattern.contains("$default")) return pattern["$default"];
        return input.at(pointer);
    }
    if (pattern.is_array()) {
        auto result = nlohmann::json::array();
        for (const auto& child : pattern) result.push_back(instantiateSentenceTemplate(child, input, depth + 1));
        return result;
    }
    if (pattern.is_object()) {
        auto result = nlohmann::json::object();
        for (auto it = pattern.begin(); it != pattern.end(); ++it)
            result[it.key()] = instantiateSentenceTemplate(it.value(), input, depth + 1);
        return result;
    }
    return pattern;
}

// Syntax data is reflected exactly as a tree. Do not apply the PropertyValue
// codec's vec3/mat4 heuristics to model arrays or interpret a syntax key "t"
// as a type tag: a sixteen-child action list is still a list of syntax nodes.
PropertyValue sentenceSyntaxValue(const nlohmann::json& value, unsigned depth = 0) {
    if (depth >= 32) throw std::runtime_error("syntax reflection exceeds the channel's 32-level structural bound");
    if (value.is_object()) {
        auto dict = std::make_shared<PropertyDict>();
        for (auto it = value.begin(); it != value.end(); ++it)
            dict->elements[it.key()] = sentenceSyntaxValue(it.value(), depth + 1);
        return dict;
    }
    if (value.is_array()) {
        auto list = std::make_shared<PropertyList>();
        for (const auto& item : value) list->elements.push_back(sentenceSyntaxValue(item, depth + 1));
        return list;
    }
    return propertyValueFromJson(value);
}

void validateSentenceAction(const nlohmann::json& model, unsigned depth = 0) {
    if (depth >= 32 || !model.is_object() || !model.contains("kind") || !model["kind"].is_number_integer())
        throw std::runtime_error("compiler output must be an ActionModel with an integer kind (depth < 32)");
    if (model["kind"].is_number_unsigned() &&
        model["kind"].get<unsigned long long>() > static_cast<unsigned>(ActionNode::Kind::CodecTransform))
        throw std::runtime_error("compiler output names an unsupported ActionModel kind");
    const auto rawKind = model["kind"].get<long long>();
    if (rawKind < 0 || rawKind > static_cast<int>(ActionNode::Kind::CodecTransform))
        throw std::runtime_error("compiler output names an unsupported ActionModel kind");
    const auto kind = static_cast<ActionNode::Kind>(rawKind);
    if ((kind == ActionNode::Kind::Set || kind == ActionNode::Kind::Add || kind == ActionNode::Kind::Scale) &&
        (!model.contains("path") || !model["path"].is_string() || model["path"].get<std::string>().empty() || !model.contains("operand")))
        throw std::runtime_error("compiled property action requires a path and operand");
    if (kind == ActionNode::Kind::Map || kind == ActionNode::Kind::Flow) {
        if (!model.contains("path") || !model["path"].is_string() || model["path"].get<std::string>().empty() ||
            !model.contains("function") || !model.contains("bindings") || !model["bindings"].is_object())
            throw std::runtime_error(std::string("compiled ") + ActionNode::kindName(kind) + " requires a path, function, and bindings");
        auto function = OntoMath::Piecewise::fromJson(model["function"]);
        if (function.pieces.empty()) throw std::runtime_error(std::string("compiled ") + ActionNode::kindName(kind) + " has no defined pieces");
        for (const auto& piece : function.pieces) if (piece.mathNode) {
            std::string error;
            if (!piece.mathNode->checkTypes({}, error, nullptr, true)) throw std::runtime_error(error);
        }
    }
    if (model.contains("children")) {
        if (!model["children"].is_array()) throw std::runtime_error("action children must be an array");
        for (const auto& child : model["children"]) validateSentenceAction(child, depth + 1);
    }
}
} // namespace

std::string TerminalChannel::propCompilationResult() const {
    if (_compilationTemplate.empty() || !_compilationInput) return "";
    // Recovered input preserves typed operands as serialization data. The
    // raw JSON mirror is below the Kernel only while substituting this template.
    PropertyValue raw;
    if (!_compilationInput->elements.count("json")) return "";
    raw = _compilationInput->elements.at("json");
    const auto* text = std::get_if<std::string>(&raw);
    if (!text) return "";
    try {
        return instantiateSentenceTemplate(nlohmann::json::parse(_compilationTemplate),
                                           nlohmann::json::parse(*text)).dump();
    } catch (const std::exception& e) {
        return nlohmann::json{{"error", std::string("template refused: ") + e.what()}}.dump();
    }
}

LawSentence::Compilation TerminalChannel::compileByMetalaw(LawManager& laws,
                                                          const nlohmann::json& input,
                                                          bool readOnly, nlohmann::json* document) {
    const bool conditionSlot = input.value("slot", "") == "condition";
    const bool valueSlot = input.value("slot", "") == "value";
    if (readOnly) {
        LawSentence::Compilation placeholder{std::nullopt, "", {}, std::nullopt};
        if (valueSlot) placeholder.value = nlohmann::json{{"value", 0}};
        else if (conditionSlot) placeholder.condition = ConditionNode::all({});
        else placeholder.action = ActionNode::sequence({});
        return placeholder;
    }
    LawSentence::Compilation result;
    PropertyValue sensed;
    try { sensed = sentenceSyntaxValue(input); }
    catch (const std::exception& e) { return {std::nullopt, e.what(), {}}; }
    auto dict = std::get_if<std::shared_ptr<PropertyDict>>(&sensed);
    if (!dict || !*dict) return {std::nullopt, "the invocation has no structural input record", {}};
    _compilationInput = *dict;
    _compilationInput->elements["json"] = input.dump();
    _compilationTemplate.clear(); _compilationError.clear();
    std::optional<nlohmann::json> chosen;
    // Copy the register: compiler applications may legitimately change it.
    const auto candidates = laws.getAll();
    for (const auto& law : candidates) {
        if (!law || law.get() == this) continue;
        const auto& targets = law->targets().getMembers();
        if (std::find(targets.begin(), targets.end(), static_cast<Singular*>(this)) == targets.end()) continue;
        _compilationTemplate.clear(); _compilationError.clear();
        if (law->applyTo(*this) != Law::ApplicationResult::Applied) continue;
        if (!_compilationError.empty()) {
            result.error = "Metalaw " + law->getIdentifier() + " refused: " + _compilationError;
            break;
        }
        if (_compilationTemplate.empty()) continue;
        try {
            auto pattern = nlohmann::json::parse(_compilationTemplate);
            if (input.value("slot", "") == "arguments" || valueSlot) {
                std::set<std::string> fields;
                std::function<void(const nlohmann::json&)> collect = [&](const nlohmann::json& node) {
                    if (node.is_object() && node.contains("$slot") && node["$slot"].is_string()) {
                        auto pointer = node["$slot"].get<std::string>();
                        const std::string prefix = "/arguments/";
                        if (pointer.rfind(prefix, 0) == 0) fields.insert(pointer.substr(prefix.size()).substr(0, pointer.substr(prefix.size()).find('/')));
                    } else if (node.is_structured()) for (const auto& child : node) collect(child);
                };
                collect(pattern);
                for (auto it = input["arguments"].begin(); it != input["arguments"].end(); ++it)
                    if (!fields.count(it.key())) throw std::runtime_error("argument '" + it.key() + "' is not consumed by the authored template");
            }
            auto model = nlohmann::json::parse(propCompilationResult());
            if (model.contains("error")) throw std::runtime_error(model["error"].get<std::string>());
            if (conditionSlot) {
                const auto node = ConditionNode::fromJson(model);
                if (node.kind == ConditionNode::Kind::Unsupported)
                    throw std::runtime_error("the template is not a condition this build can read");
            } else if (valueSlot) {
                if (!model.is_object() || model.size() != 1 ||
                    (!model.contains("value") && !model.contains("literal") && !model.contains("math")))
                    throw std::runtime_error("a value compiler must return exactly one value, literal, or math envelope");
            } else if (!document) validateSentenceAction(model);
            if (chosen && *chosen != model) throw std::runtime_error("multiple Metalaws supplied conflicting compilation models");
            chosen = model;
            result.laws.push_back(law->getIdentifier());
        } catch (const std::exception& e) {
            result.error = "Metalaw " + law->getIdentifier() + " compilation refused: " + e.what();
            break;
        }
    }
    if (result.error.empty() && chosen) {
        try {
            if (document) { *document = *chosen; result.action = ActionNode::sequence({}); }
            else if (valueSlot) result.value = *chosen;
            else if (conditionSlot) result.condition = ConditionNode::fromJson(*chosen);
            else result.action = ActionNode::fromJson(*chosen);
        }
        catch (const std::exception& e) { result.error = std::string("invalid compiled model: ") + e.what(); }
    }
    if (!result.action && !result.condition && !result.value && result.error.empty())
        result.error = "no authored Metalaw compiled " + input.value("slot", "invocation") +
                       " for " + input.value("selector", "") + "; no compiler fallback exists";
    // Input remains legible as the last request; clear the live slot so a
    // continuously evaluated Metalaw cannot answer a request that has ended.
    _compilationInput->elements["slot"] = std::string{};
    _compilationError = result.error;
    return result;
}

std::string TerminalChannel::presenceRefusal() {
    PropertyValue value;
    std::string authorId;
    if (lawGetValue(*this, PropertyPath::parse(_authorPath), value))
        if (const auto* s = std::get_if<std::string>(&value)) authorId = *s;
    Singular* author = findBeing(authorId);
    if (!author)
        return "refused: no author at " + _authorPath + " ('" + authorId + "'); nothing enters the world without one";
    auto* person = dynamic_cast<Person*>(author);
    if (!person)
        return "refused: a typed line authors only as a Person (a human), and " + _authorPath +
               " names a " + describeSingular(author) + " ('" + authorId + "')";
    const bool present = _presenceCheck
        ? _presenceCheck(*person)
        : person->personId().canAuthenticate() &&
              Identity::FirstMoverRegister::instance().isAuthenticatedPerson(person->personId());
    if (present) return {};
    return std::string("refused: you are not present yet, so the line will not author as you. ") +
           (person->personId().canAuthenticate()
                ? "`enter Identity` and type your passphrase, then `enter LawLine` again."
                : "`enter Identity` to take a key (you have none yet), then `enter LawLine` again.") +
           " (Previews with ?, search with ??, and help stay open.)";
}

void TerminalChannel::speak(LawManager& laws, const std::string& text) {
    // Stdin trust (Astra's Crystal §13): whatever types here authors as the
    // Person @interaction-channel.personId names, so it may do so only while
    // that Person is PRESENT. Read-only lines never author and stay open.
    if (!isReadOnlySentence(text)) {
        const std::string refusal = presenceRefusal();
        if (!refusal.empty()) { _status = refusal; say(_status); return; }
    }
    std::string splitError;
    auto parts = LawSentence::sentences(text, splitError);
    if (!splitError.empty()) { _status = "refused: " + splitError; say(_status); return; }
    if (parts.size() > 1) {
        _openClauses.clear();
        const bool preview = isReadOnlySentence(text);
        if (preview && !parts.back().empty() && parts.back().back() == '?') parts.back().pop_back();
        nlohmann::json document;
        LawSentence::Compilation compiled;
        if (!preview) {
            compiled = compileByMetalaw(laws, {{"slot", "sentences"}, {"opcode", "sentence.batch"}, {"sentences", parts}}, false, &document);
            if (!compiled.error.empty()) { _status = "refused: " + compiled.error; say(_status); return; }
            // The channel may register only the exact sensed sentences, once each,
            // in source order. Policy must authorize that order through its template.
            if (!document.is_object() || !document.contains("sentences") || document["sentences"] != nlohmann::json(parts)) {
                _status = "refused: Metalaw must compile every sentence exactly once in source order"; say(_status); return;
            }
        }
        auto words = preview ? liveVocabulary() : vocabulary(laws);
        std::vector<LawSentence::Parse> parsed;
        _preview.clear();
        for (std::size_t i = 0; i < parts.size(); ++i) {
            auto p = LawSentence::parse(parts[i] + (preview ? "?" : ""), words);
            if (!p.ok || p.search || p.immediate || (!preview && p.previewOnly)) {
                _status = "refused: sentence " + std::to_string(i + 1) + ": " +
                          (p.error.empty() ? "batch accepts Law sentences only" : p.error);
                say(_status); return;
            }
            p.presetLawIds.insert(p.presetLawIds.end(), compiled.laws.begin(), compiled.laws.end());
            _preview += (i ? "\n" : "") + std::to_string(i + 1) + ". " + p.preview();
            parsed.push_back(std::move(p));
        }
        if (preview) { _status = "preview"; say("preview: " + _preview); return; }
        PropertyValue value;
        std::string authorId;
        if (lawGetValue(*this, PropertyPath::parse(_authorPath), value))
            if (auto id = std::get_if<std::string>(&value)) authorId = *id;
        auto author = findBeing(authorId);
        if (!author) { _status = "refused: no author at " + _authorPath; say(_status); return; }
        std::size_t registered = 0;
        for (std::size_t i = 0; i < parsed.size(); ++i) {
            auto id = mintLawId();
            std::string persistence;
            auto refusal = enact(laws, parsed[i], parts[i], {author}, id, persistence);
            if (!refusal.empty()) {
                _status = refusal + " · " + std::to_string(registered) + " earlier sentences registered";
                say(_status); return;
            }
            _lastCreated = id;
            ++registered;
            say("authored " + id + " (sentence " + std::to_string(i + 1) + ", written by " + authorId + ")" + persistence);
        }
        _status = "authored " + std::to_string(registered) + " Laws in sentence order";
        say(_status); return;
    }
    const LawSentence::Parse p = LawSentence::parse(text, vocabulary(laws));
    _preview = p.preview();
    _openClauses = join(p.openClauses, "; ");

    if (p.search) {
        say(p.candidates.empty() ? "?? nothing matches" : join(p.candidates, "\n"));
        return;
    }
    if (p.ok && p.immediate && !p.previewOnly) {
        requestDeletion(laws, p.destroyTarget);
        return;
    }
    if (!p.ok) {
        std::string msg = "refused: " + p.error;
        if (!p.candidates.empty()) msg += "\n  candidates: " + join(p.candidates, ", ");
        _status = msg;
        say(msg);
        return;
    }
    const std::string notes = p.notes.empty() ? std::string{} : "\n  " + join(p.notes, "\n  ");
    if (p.previewOnly) {
        _status = "preview";
        say("preview: " + _preview + "\n  " + dryRun(p) + notes);
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
    std::string persistence;
    const std::string refusal = enact(laws, p, text, {author}, id, persistence);
    if (!refusal.empty()) {
        _status = refusal;
        say(_status);
        return;
    }
    _lastCreated = id;
    _status = "authored " + id + " (written by " + author->getIdentifier() + ")" + persistence;
    say(_status + "\n  " + _preview + notes);
}

std::string TerminalChannel::enact(LawManager& laws, const LawSentence::Parse& p,
                                   const std::string& text,
                                   const std::vector<Singular*>& authors,
                                   const std::string& id, std::string& persistence) {
    if (authors.empty()) return "refused: nothing enters the world without an author";
    if (p.compilationDeferred) return "refused: invocation compilation is deferred; preview syntax cannot become a Law";
    if (laws.find(id)) return "refused: a Law named " + id + " already exists";
    auto law = std::make_shared<Law>(p.name.empty() ? text : p.name, authors);
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

    // Keeping the Law means live Zone membership; durable persistence still
    // requires an explicit Zone save. Say both truths instead of letting the
    // green authored check imply that the new Law already survived a restart.
    persistence = " · live for this session (no active Zone to save)";
    if (ZoneManager* zones = ZoneManager::live()) {
        if (!zones->adoptLawIntoActiveZone(id)) {
            laws.remove(id);
            return "refused: the new Law could not enter the active Zone's authored closure";
        }
        persistence = " · live in " + zones->active().name() + " · Save Zone to keep it after restart";
    }
    return {};
}

bool TerminalChannel::isReadOnlySentence(const std::string& text) {
    std::size_t b = text.find_first_not_of(" \t");
    std::size_t e = text.find_last_not_of(" \t\r\n");
    if (b == std::string::npos) return true;
    return text.compare(b, 2, "??") == 0 || text[e] == '?';
}

TerminalChannel::ForeignSentence TerminalChannel::authorForeign(
        LawManager& laws, const std::string& text,
        const std::vector<Singular*>& authors, const std::string& identifier) {
    ForeignSentence out;
    std::string splitError;
    auto parts = LawSentence::sentences(text, splitError);
    if (!splitError.empty() || parts.size() > 1) {
        out.status = "refused";
        out.error = splitError.empty() ? "foreign authoring requires one separately authorized identifier per sentence; submit sentences individually" : splitError;
        return out;
    }
    auto words = vocabulary(laws, authors.size() == 1 && authors.front() ? authors.front()->getIdentifier() : "");
    // Several speakers provide no unique first-person referent. Never borrow
    // the local keyboard Person's root for a foreign writer's "my".
    if (authors.size() != 1 || !authors.front()) words.pathRoots.clear();
    const LawSentence::Parse p = LawSentence::parse(text, words);
    out.preview = p.preview();
    out.openClauses = p.openClauses;
    const std::string notes = p.notes.empty() ? std::string{} : join(p.notes, "; ");

    if (p.search) {
        out.status = "search";
        out.candidates = p.candidates;
        return out;
    }
    if (!p.ok) {
        out.status = "refused";
        out.error = p.error;
        out.candidates = p.candidates;
        return out;
    }
    if (p.previewOnly) {
        out.status = "preview";
        out.detail = dryRun(p) + (notes.empty() ? "" : "; " + notes);
        return out;
    }
    if (p.immediate) {
        out.status = "refused";
        out.error = "an immediate act (\"delete ...\") needs the Terminal's confirming Metalaw; "
                    "a foreign First Mover deletes a Law it may touch with delete_law";
        return out;
    }
    if (identifier.empty()) {
        out.status = "refused";
        out.error = "an identifier is required so the act can be held to the mover's granted scope";
        return out;
    }
    std::string persistence;
    const std::string refusal = enact(laws, p, text, authors, identifier, persistence);
    if (!refusal.empty()) {
        out.status = "refused";
        out.error = refusal;
        return out;
    }
    out.status = "authored";
    out.lawId = identifier;
    out.detail = "written by " + authors.front()->getIdentifier() + persistence +
                 (notes.empty() ? "" : "; " + notes);
    return out;
}

// ---------------------------------------------------------------------------
// Terminal Zones (Zach, 2026-09-30) -- Terminal_Zones.md. ⚠ GATE: nothing is
// to be built on top of these two opcodes until Zach verifies them.
// ---------------------------------------------------------------------------

// Where the line should be, applied: the first visit puts it in the LawLine
// Zone when there is one (the Terminal's purpose, so the Law Line keeps working
// wherever the Person's body stands); a Law's write to `zone` moves it.
void TerminalChannel::placeLine() {
    if (!_zoneBootTried && ZoneManager::live()) {
        _zoneBootTried = true;
        if (_requestedZone.empty() && _zone.empty() &&
            ZoneManager::live()->findZoneIndex("LawLine") != static_cast<size_t>(-1)) {
            _requestedZone = "LawLine";
        }
    }
    if (!_requestedZone.empty() && _requestedZone != _zone) {
        const std::string target = _requestedZone;
        if (!moveLine(target)) _requestedZone = _zone;   // stayed; the reason was said
    }
}

// Why Enter would keep this line instead of sending it ("" = it is sent).
std::string TerminalChannel::submitRefusal(const std::string& text) {
    const std::string t = text.substr(text.find_first_not_of(" \t"));
    if (awaitingAnswer() || t == "help" || t == "help " || t == "?") return {};
    // A block's lines are read together when it ends, never one by one.
    if (lawGrammarHere() && (!_block.empty() || (!t.empty() && t.back() == ':'))) return {};
    // The line's move is never a Law sentence, and outside the Law Line
    // the Law grammar has no say over what may be sent.
    if (isEnterLine(text) || !lawGrammarHere()) return {};
    if (t.rfind("??", 0) == 0 || t.back() == '?') return {};
    std::string splitError;
    auto parts = LawSentence::sentences(text, splitError);
    if (!splitError.empty()) return "not yet — " + splitError;
    for (const auto& part : parts) {
        const auto parsed = LawSentence::parse(part, liveVocabulary());
        if (!parsed.ok && parsed.error.find("Metalaw") == std::string::npos)
            return "not yet — " + parsed.error;
    }
    if (parts.size() > 1) return {};
    const LawSentence::Parse p = LawSentence::parse(text, liveVocabulary());
    if (p.ok || p.error.find("Metalaw") != std::string::npos) return {};   // Metalaws decide when spoken
    if (p.error.rfind("still open:", 0) == 0) {
        for (const auto& clause : p.openClauses) {
            if (clause.find("(optional)") != std::string::npos) continue;
            const std::string eg = exampleFor(clause, liveVocabulary());
            return "not yet — add " + clause + (eg.empty() ? "" : ", e.g.  " + eg) +
                   "   (or end with ? to preview)";
        }
    }
    return "not yet — " + p.error;
}

std::string TerminalChannel::zoneLabel() const {
    if (ZoneManager* zones = ZoneManager::live()) {
        const size_t i = zones->findZoneIndex(_zone);
        if (i != static_cast<size_t>(-1)) return zones->zones()[i]->name();
    }
    return _zone;
}

bool TerminalChannel::handleEnter(const std::string& t) {
    const auto isWord = [&](const char* w) {
        const std::size_t n = std::strlen(w);
        if (t.size() < n) return false;
        for (std::size_t i = 0; i < n; ++i) {
            if (std::tolower(static_cast<unsigned char>(t[i])) != w[i]) return false;
        }
        return t.size() == n || t[n] == ' ';
    };
    if (!isWord("enter")) return false;
    std::string target = t.size() > 5 ? t.substr(6) : std::string{};
    target.erase(0, target.find_first_not_of(" \t"));
    if (target.empty()) {
        ZoneManager* zones = ZoneManager::live();
        if (!zones) { say("(no Zones are live yet)"); return true; }
        std::string out = "the line is in " + (_zone.empty() ? std::string("no Zone") : zoneLabel()) +
                          ". Zones it can enter:";
        for (const auto& z : zones->zones()) {
            if (!z) continue;
            out += "\n  " + z->name() + (z->name() != z->getIdentifier() ? "  (" + z->getIdentifier() + ")" : "");
        }
        say(out);
        return true;
    }
    _requestedZone = target;
    if (!moveLine(target)) _requestedZone = _zone;
    return true;
}

bool TerminalChannel::moveLine(const std::string& target) {
    ZoneManager* zones = ZoneManager::live();
    if (!zones) {
        say("refused: no Zones are live yet, so the line has nowhere to go");
        return false;
    }
    const size_t index = zones->findZoneIndex(target);
    if (index == static_cast<size_t>(-1)) {
        say("refused: no single Zone is named '" + target + "' (type `enter` to list them)");
        return false;
    }
    const std::string id = zones->zones()[index]->getIdentifier();
    if (id == _zone) {
        say("the line is already in " + zoneLabel());
        return true;
    }
    // The same preflight a Person's walk uses: every Law root, author and
    // trigger resolved before anything changes, or nothing changes.
    if (!zones->holdZoneClosure(kLineHolder, index)) {
        say("refused: " + zones->zones()[index]->name() +
            "'s Laws could not be held (see the log above); the line stays in " +
            (_zone.empty() ? std::string("no Zone") : zoneLabel()));
        return false;
    }
    Singular* previous = nullptr;
    if (!_zone.empty()) {
        const size_t pi = zones->findZoneIndex(_zone);
        if (pi != static_cast<size_t>(-1)) previous = zones->zones()[pi].get();
    }
    if (previous) {
        Core::EventBus::instance().publish(
            ECA::Event{"terminal-zone-exited", this, previous, std::time(nullptr), std::string{}});
    }
    _zone = id;
    _requestedZone = id;
    _vocab.reset();
    _parse.reset();
    say("the line is in " + zoneLabel());
    Core::EventBus::instance().publish(
        ECA::Event{"terminal-zone-entered", this, zones->zones()[index].get(), std::time(nullptr),
                   std::string{}});
    if (_laws && !_laws->rete().hearsType(kLineEntered)) {
        say("  (nothing here hears a typed line: it will scroll and do nothing. `enter <zone>` leaves.)");
    }
    return true;
}

void TerminalChannel::beginSecret() {
    Person* person = _presencePerson ? _presencePerson : Core::Engine::instance().getPerson();
    if (!person) {
        say("refused: no Person is loaded to become present");
        return;
    }
    if (person->hasIdentity() &&
        Identity::FirstMoverRegister::instance().isAuthenticatedPerson(person->personId())) {
        say("'" + person->getDisplayName() + "' is already present (" +
            person->personId().abbreviated() + ")");
        return;
    }
    if (person->hasIdentity() || Identity::keyedProfileExists()) {
        _secretStage = 1;
        say("Identity: type your passphrase. It is hidden, never echoed, never kept. Ctrl-C cancels.");
    } else {
        _secretStage = 2;
        say("Identity: '" + person->getDisplayName() + "' has no key yet. Choose a passphrase to KEY your "
            "Person (you will type it twice). There is no recovery if it is lost. Ctrl-C cancels.");
    }
    _awaitingSecretFlag = true;
    _editor.secret = true;
    _editor.refresh();
}

void TerminalChannel::cancelSecret(const std::string& why) {
    std::fill(_pendingSecret.begin(), _pendingSecret.end(), '\0');
    _pendingSecret.clear();
    _secretStage = 0;
    _awaitingSecretFlag = false;
    _editor.secret = false;
    say("Identity: cancelled · " + why);
}

void TerminalChannel::takeSecret(std::string& secret) {
    const auto wipe = [](std::string& v) { std::fill(v.begin(), v.end(), '\0'); v.clear(); };
    Person* person = _presencePerson ? _presencePerson : Core::Engine::instance().getPerson();
    if (!person) {
        wipe(secret);
        cancelSecret("no Person is loaded");
        return;
    }
    if (_secretStage == 1) {
        const auto r = Identity::unlockPresentPerson(*person, secret);
        wipe(secret);
        _secretStage = 0;
        _awaitingSecretFlag = false;
        _editor.secret = false;
        say("Identity: " + r.report);
        return;
    }
    if (_secretStage == 2) {
        _pendingSecret = std::move(secret);
        wipe(secret);
        _secretStage = 3;
        _editor.refresh();
        return;
    }
    // Stage 3: confirmation.
    const bool same = (secret == _pendingSecret);
    Identity::PresenceResult r;
    if (same) r = Identity::keyPresentPerson(*person, _pendingSecret);
    wipe(secret);
    wipe(_pendingSecret);
    _secretStage = 0;
    _awaitingSecretFlag = false;
    _editor.secret = false;
    say(same ? "Identity: " + r.report : "Identity: the two passphrases differ; nothing was keyed");
}

void TerminalChannel::say(const std::string& text) { propSetOutput(text); }

void TerminalChannel::propSetOutput(const std::string& v) {
    _output = v;
    if (_sink) {
        _sink(v);
        return;
    }
    if (!_attached) {
        std::cout << v << std::endl;
        return;
    }
    // Results stay in the scrollback, marked so they read at a glance.
    std::string first = v, rest;
    const std::size_t nl = v.find('\n');
    if (nl != std::string::npos) {
        first = v.substr(0, nl);
        rest = v.substr(nl);
    }
    const auto paint = [&](const std::string& code, const std::string& s) {
        return _color ? "\x1b[" + code + "m" + s + "\x1b[0m" : s;
    };
    std::string shown;
    if (first.rfind("authored", 0) == 0 || first.rfind("deleted", 0) == 0) {
        shown = paint("32", "✓ " + first) + paint("2", rest);
    } else if (first.rfind("kept", 0) == 0) {
        shown = paint("33", "○ " + first) + paint("2", rest);
    }
    else if (first.rfind("authored", 0) == 0) shown = paint("32", "✓ " + first) + paint("2", rest);
    else if (first.rfind("refused", 0) == 0) shown = paint("31", "✗ " + first) + paint("2", rest);
    else if (first.rfind("(", 0) == 0) shown = paint("33", v);
    else shown = v;
    printAbove(shown);
}

std::string TerminalChannel::lawSummary(const Law& law, LawManager& laws) const {
    std::string s;
    switch (law.activation()) {
        case Law::Activation::WhileTrue: s = "every moment"; break;
        case Law::Activation::OnBecomeTrue: s = "when it becomes true"; break;
        case Law::Activation::OnEvent: {
            const auto& t = laws.triggersOf(law.getIdentifier());
            s = t.empty() ? "on <no event>" : "on " + join(t, " or ");
            break;
        }
    }
    if (law.hasConditionModel()) {
        const auto* c = law.conditionModel();
        const bool none = c->kind == ConditionNode::Kind::All && c->children.empty();
        s += none ? " · no condition" : " · if " + c->describe();
    }
    if (law.hasActionModel()) s += " · then " + law.actionModel()->describe();
    s += law.scope() == Law::Scope::Everyone ? " · on everyone" : " · on the event's subject";
    return s;
}

// "…?" also says who the IF holds for right now — read-only: the condition
// is compiled and asked of each present being, nothing is applied.
std::string TerminalChannel::dryRun(const LawSentence::Parse& p) {
    if (!p.condition) return "no IF: it acts on every subject it is given";
    const bool eventRelative = readsEventContext(*p.condition);
    const auto predicate = p.condition->compile();
    std::vector<std::string> names;
    std::size_t count = 0;
    const ECA::Event nothing;
    for (Singular* being : Universe::instance().beings()) {
        if (!being || dynamic_cast<Law*>(being) || dynamic_cast<Relation*>(being)) continue;
        if (!predicate(nothing, *being)) continue;
        ++count;
        if (names.size() < 4) {
            PropertyValue shown;
            std::string label = being->getIdentifier();
            if (being->getDynamicProperty("displayName", shown)) {
                if (const auto* n = std::get_if<std::string>(&shown); n && !n->empty()) label = *n;
            }
            names.push_back(label);
        }
    }
    const std::string caveat = eventRelative ? "hypothetical — no event supplied · " : "";
    if (count == 0) return caveat + "right now the IF holds for nothing here";
    return caveat + "right now the IF holds for " + std::to_string(count) +
           (count == 1 ? " being: " : " beings: ") + join(names, ", ") +
           (count > names.size() ? ", …" : "");
}

std::string TerminalChannel::footerText(bool& hears) {
    hears = _laws && _laws->rete().hearsType(kLineEntered);
    std::string body = "no Zone";
    if (ZoneManager* zones = ZoneManager::live()) {
        if (!zones->zones().empty()) body = zones->active().name();
    }
    // Two presences, both named: where the LINE is, and where the body is.
    const std::string zone = "line: " + (_zone.empty() ? std::string("no Zone") : zoneLabel()) +
                             " · body: " + body;
    std::string author = "nobody";
    PropertyValue who;
    if (lawGetValue(*this, PropertyPath::parse(_authorPath), who)) {
        if (const auto* id = std::get_if<std::string>(&who); id && !id->empty()) author = *id;
    }
    int count = 0;
    if (_laws) {
        for (const auto& law : _laws->getAll()) {
            if (law && !law->isFirstMover() && law->isEnabled()) ++count;
        }
    }
    const std::string scope = _laws ? propertySuggestionBeing() : std::string{};
    return zone + " · " + (hears ? "hears the line" : "does NOT hear the line") +
           (scope.empty() ? "" : " · property suggestions @" + scope) + " · as " + author + (lawGrammarHere() && !presenceRefusal().empty() ? " (not present: enter Identity)" : "") + " · " + std::to_string(count) +
           (count == 1 ? " live law" : " live laws");
}

// ---------------------------------------------------------------------------
// Confirmed deletion. The line never deletes: it names what the Person asked
// to delete and publishes the edge it sensed; a seeded Metalaw asks the
// question (its text lives in Law text); the answer is the Person's; a second
// Metalaw performs the Destroy. Zach, 2026-09-25: "no means no delete and
// requires your yes to delete."
// ---------------------------------------------------------------------------
void TerminalChannel::requestDeletion(LawManager& laws, const std::string& target) {
    const std::string wanted = !target.empty() && target[0] == '@' ? target.substr(1) : target;
    std::vector<Singular*> found;
    for (const auto& law : laws.getAll()) {
        if (!law || law->isFirstMover()) continue;
        if (law->getIdentifier() == wanted || law->name() == wanted) found.push_back(law.get());
    }
    if (found.empty()) {
        if (Singular* being = findBeing(wanted); being && dynamic_cast<Object*>(being)) found.push_back(being);
    }
    if (found.empty()) {
        say("refused: nothing called '" + target + "' is here to delete");
        return;
    }
    if (!laws.rete().hearsType("terminal-deletion-requested")) {
        say("(no Law in this Zone decides deletions, so nothing was deleted. The LawLine Zone carries "
            "the Metalaws that ask and then delete.)");
        return;
    }
    _pendingTargets.clear();
    std::vector<std::string> names;
    for (Singular* s : found) {
        _pendingTargets.push_back(s->getIdentifier());
        auto* law = dynamic_cast<Law*>(s);
        names.push_back(law ? law->name() : describeBeing(s->getIdentifier()));
    }
    _pendingTargetsText = join(_pendingTargets, " ");
    _pendingNames = join(names, " | ");
    _question.clear();
    Core::EventBus::instance().publish(ECA::Event{"terminal-deletion-requested", this,
                                                  found.size() == 1 ? found.front() : nullptr,
                                                  std::time(nullptr), std::string{}});
}

void TerminalChannel::cancelDeletion(const std::string& why) {
    const std::string names = _pendingNames;
    _pendingTargets.clear();
    _pendingTargetsText.clear();
    _pendingNames.clear();
    _question.clear();
    say("kept " + names + " — " + why);
}

void TerminalChannel::answer(LawManager& laws, const std::string& line) {
    std::string a = line;
    a.erase(0, a.find_first_not_of(" \t"));
    while (!a.empty() && (a.back() == ' ' || a.back() == '\t')) a.pop_back();
    std::transform(a.begin(), a.end(), a.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    // Several Laws share the name: a number chooses which one is asked about.
    if (_pendingTargets.size() > 1) {
        char* end = nullptr;
        const long n = std::strtol(a.c_str(), &end, 10);
        if (end && *end == '\0' && n >= 1 && n <= static_cast<long>(_pendingTargets.size())) {
            const std::string chosen = _pendingTargets[static_cast<std::size_t>(n - 1)];
            _pendingTargets = {chosen};
            _pendingTargetsText = chosen;
            Law* law = laws.find(chosen);
            _pendingNames = law ? law->name() + " (" + chosen + ")" : chosen;
            return;   // the same question, now about one
        }
        cancelDeletion("the answer named none of them");
        return;
    }

    // Only a word that reads as true confirms (yes, on, true, …) — read
    // through the world's own value Lexemes, not a list kept here.
    bool yes = a == "true";
    for (const auto& w : liveVocabulary().words) {
        if (w.opcode != "value" || lowerCopy(w.symbol) != a) continue;
        for (const auto& p : liveVocabulary().presets) {
            if (p.lawId == w.lawId && p.value) {
                if (const auto* b = std::get_if<bool>(&*p.value)) yes = *b;
            }
        }
    }
    if (!yes) {
        cancelDeletion("nothing deleted (only yes deletes)");
        return;
    }
    const std::string id = _pendingTargets.front();
    Singular* victim = findBeing(id);
    if (!victim) {
        cancelDeletion("it was already gone");
        return;
    }
    if (!laws.rete().hearsType("terminal-deletion-confirmed")) {
        cancelDeletion("no Law in this Zone performs deletions");
        return;
    }
    _deletingId = id;
    _deletingName = _pendingNames;
    _pendingTargets.clear();
    _pendingTargetsText.clear();
    _pendingNames.clear();
    _question.clear();
    Core::EventBus::instance().publish(
        ECA::Event{"terminal-deletion-confirmed", this, victim, std::time(nullptr), std::string{}});
}

// ---------------------------------------------------------------------------
// help — a page drawn in the panel, never printed: the grammar coloured the
// way the line colours it, examples made from THIS world's words, the keys,
// and where you are. Zach, 2026-09-25: "add a help command and make it
// aesthetic/beautiful".
// ---------------------------------------------------------------------------
namespace {

struct HelpRow {
    std::size_t max;
    bool color;
    std::string out;
    std::size_t used = 0;
    void add(const std::string& text, const std::string& code = "") {
        std::string piece;
        for (std::size_t i = 0; i < text.size() && used < max;) {
            std::size_t n = 1;
            const unsigned char c = static_cast<unsigned char>(text[i]);
            if (c >= 0xF0) n = 4; else if (c >= 0xE0) n = 3; else if (c >= 0xC0) n = 2;
            piece += text.substr(i, n);
            i += n;
            ++used;
        }
        out += (color && !code.empty()) ? "\x1b[" + code + "m" + piece + "\x1b[0m" : piece;
    }
};

// Pad to a VISIBLE width: "·", "‹" and "›" are several bytes but one cell.
std::string padded(std::string text, std::size_t cells) {
    const std::size_t have = visibleWidth(text);
    if (have < cells) text += std::string(cells - have, ' ');
    return text;
}

} // namespace

void TerminalChannel::showHelp() {
    const auto& v = liveVocabulary();
    const int w = std::max(40, std::min(width() - 4, 100));
    const std::size_t inner = static_cast<std::size_t>(w - 4);   // "│ " + content + " │"
    const std::string frame = "38;5;141", label = "1;38;5;183", dim = "2";
    std::vector<std::string> lines;

    const auto boxed = [&](const std::function<void(HelpRow&)>& fill) {
        HelpRow row{inner, _color, {}, 0};
        fill(row);
        std::string s = (_color ? "\x1b[" + frame + "m  │\x1b[0m " : "  │ ") + row.out;
        s += std::string(inner - std::min(inner, row.used), ' ');
        s += _color ? " \x1b[" + frame + "m│\x1b[0m" : " │";
        lines.push_back(s);
    };
    const auto blank = [&] { boxed([](HelpRow&) {}); };
    const auto rule = [&](const std::string& left, const std::string& title, const std::string& right) {
        std::string s = "  " + left;
        std::string t = title.empty() ? "" : "─ " + title + " ";
        std::size_t width = 0;
        for (unsigned char c : t) if ((c & 0xC0) != 0x80) ++width;
        std::string fill;
        for (std::size_t i = width; i < inner + 2; ++i) fill += "─";
        lines.push_back(_color ? "\x1b[" + frame + "m" + s + "\x1b[1;38;5;225m" + t + "\x1b[0m\x1b[" + frame + "m" +
                                     fill + right + "\x1b[0m"
                               : s + t + fill + right);
    };
    const auto section = [&](const std::string& name, const std::function<void(HelpRow&)>& fill) {
        boxed([&](HelpRow& r) {
            r.add(padded(name, 8), label);
            fill(r);
        });
    };
    const auto cont = [&](const std::function<void(HelpRow&)>& fill) {
        boxed([&](HelpRow& r) {
            r.add("        ");
            fill(r);
        });
    };
    const auto colorOf = [](const std::string& role) {
        if (role == "preset") return std::string("1;95");
        if (role == "action") return std::string("32");
        if (role == "operator") return std::string("33");
        if (role == "condition") return std::string("36");
        if (role == "value") return std::string("38;5;215");
        if (role == "event") return std::string("94");
        if (role == "clause") return std::string("35");
        return std::string("97");
    };

    rule("╭", "✦ The Law Line", "╮");
    boxed([&](HelpRow& r) { r.add("Speak a Law in a sentence. The world keeps it.", "3"); });
    blank();
    section("SHAPE", [&](HelpRow& r) {
        r.add("[", dim); r.add("preset", colorOf("preset")); r.add("] [", dim);
        r.add("called", colorOf("clause")); r.add(" ‹name›", dim); r.add("] [", dim);
        r.add("on", colorOf("clause")); r.add(" ‹event›", colorOf("event")); r.add(" | ", dim);
        r.add("when …", colorOf("preset")); r.add("]", dim);
    });
    cont([&](HelpRow& r) {
        r.add("[", dim); r.add("if", colorOf("clause")); r.add(" ‹condition›", colorOf("condition")); r.add("] ", dim);
        r.add("then", colorOf("clause")); r.add(" ‹action›", colorOf("action"));
    });
    blank();
    const std::string trigger = exampleTrigger(v);
    const std::string path = examplePath(v);
    const std::string value = exampleValue(v, path);
    section("TRY", [&](HelpRow& r) {
        r.add(trigger, colorOf("preset")); r.add(" then ", colorOf("clause"));
        r.add("set ", colorOf("action")); r.add(path + " ", "97"); r.add(value, colorOf("value"));
    });
    cont([&](HelpRow& r) {
        r.add("my law called ", colorOf("clause")); r.add("Glow ", "1"); r.add(trigger, colorOf("preset"));
        r.add(" then ", colorOf("clause")); r.add("add ", colorOf("action")); r.add("glow ", "97");
        r.add("by ", colorOf("clause")); r.add("1", colorOf("value"));
    });
    cont([&](HelpRow& r) {
        r.add("on tick ", colorOf("clause")); r.add("if ", colorOf("clause")); r.add("hp ", "97");
        r.add("> ", colorOf("operator")); r.add("2 ", colorOf("value")); r.add("then ", colorOf("clause"));
        r.add("scale ", colorOf("action")); r.add("glow ", "97"); r.add("by ", colorOf("clause")); r.add("0.5", colorOf("value"));
    });
    blank();

    // What this world can say, counted and sampled.
    std::map<std::string, std::vector<std::string>> byRole;
    for (const auto& word : v.words) {
        if (word.lexemeId.empty() && word.description.find("Law Graph only") != std::string::npos) continue;
        const std::string role = word.opcode == "preset" || word.opcode == "value"
                                     ? word.opcode
                                     : word.opcode.substr(0, word.opcode.find('.'));
        auto& list = byRole[role];
        if (std::find(list.begin(), list.end(), word.symbol) == list.end()) list.push_back(word.symbol);
    }
    const auto sample = [&](const char* role, const char* title, const std::string& code) {
        const auto it = byRole.find(role);
        if (it == byRole.end() || it->second.empty()) return;
        cont([&](HelpRow& r) {
            r.add(padded(std::string(title) + " (" + std::to_string(it->second.size()) + ")", 17), dim);
            for (std::size_t i = 0; i < it->second.size() && i < 7; ++i) {
                if (i) r.add(" · ", dim);
                r.add(it->second[i], code);
            }
        });
    };
    section("ZONES", [&](HelpRow& r) {
        r.add("enter <zone>  moves the line (not you): LawLine authors Laws, Identity makes you present, "
              "Quiet hears nothing · `enter` lists", "3");
    });
    section("WORDS", [&](HelpRow& r) { r.add("every word is a Lexeme that denotes a Law — add your own", "3"); });
    sample("preset", "presets", colorOf("preset"));
    sample("action", "actions", colorOf("action"));
    sample("op", "comparisons", colorOf("operator"));
    sample("value", "values", colorOf("value"));
    cont([&](HelpRow& r) {
        r.add(padded("events (" + std::to_string(v.events.size()) + ")", 17), dim);
        for (std::size_t i = 0; i < v.events.size() && i < 5; ++i) {
            if (i) r.add(" · ", dim);
            r.add(v.events[i], colorOf("event"));
        }
    });
    blank();
    const auto key = [&](const std::string& k1, const std::string& d1, const std::string& k2, const std::string& d2) {
        cont([&](HelpRow& r) {
            r.add(padded(k1, 9), "1;97");
            r.add(padded(d1, 26), dim);
            r.add(padded(k2, 11), "1;97");
            r.add(d2, dim);
        });
    };
    section("KEYS", [&](HelpRow& r) { r.add("the menu follows your typing; nothing here is printed", "3"); });
    key("tab", "take · next ‹blank›", "↑↓ wheel", "choose");
    key("→", "take the dim ghost", "PgUp PgDn", "page");
    key("enter", "author (asks if unsure)", "click", "pick a row");
    key("esc", "close · then clear", "ctrl-r", "search history");
    key("ctrl-c", "clear · twice quits", "ctrl-w u k", "delete word/start/end");
    blank();
    section("ALSO", [&](HelpRow& r) { r.add("…?", colorOf("clause")); r.add("        preview, and who the IF holds for right now", dim); });
    cont([&](HelpRow& r) { r.add("?? word", colorOf("clause")); r.add("    search what this world can say", dim); });
    cont([&](HelpRow& r) { r.add("delete ‹law›", colorOf("action")); r.add("  asks first — only yes deletes", dim); });
    cont([&](HelpRow& r) { r.add("help · ? · F1", colorOf("clause")); r.add(" this page", dim); });
    blank();
    bool hears = false;
    const std::string where = footerText(hears);
    section("HERE", [&](HelpRow& r) { r.add("◆ ", hears ? "32" : "33"); r.add(where, dim); });
    rule("╰", "", "╯");

    _editor.showOverlay(std::move(lines));
    if (_attached) draw();
}

void TerminalChannel::buildProperties() {
    registerEnabledProperty();
    const auto writable = [this](const char* name, auto* member) {
        using T = std::remove_pointer_t<decltype(member)>;
        registerProperty(std::make_unique<Level<T>>(name, member, true));
    };
    const auto level = [this](const char* name, auto* member) {
        using T = std::remove_pointer_t<decltype(member)>;
        registerProperty(std::make_unique<Level<T>>(name, member, false));
    };
    level("lastLine", &_lastLine);
    registerProperty(std::make_unique<ComputedProperty<TerminalChannel, std::string>>(
        "block", this, &TerminalChannel::propBlock));
    writable("prompt", &_prompt);
    writable("authorPath", &_authorPath);
    writable("lexemeRelation", &_lexemeRelation);
    writable("scopeBeing", &_scopeBeing);
    level("status", &_status);
    level("preview", &_preview);
    level("openClauses", &_openClauses);
    level("lastCreated", &_lastCreated);
    level("compilation.input", &_compilationInput);
    writable("compilation.template", &_compilationTemplate);
    writable("compilation.error", &_compilationError);
    registerProperty(std::make_unique<ComputedProperty<TerminalChannel, std::string>>(
        "compilation.result", this, &TerminalChannel::propCompilationResult));
    level("ambiguity.symbol", &_ambiguitySymbol);
    level("ambiguity.slot", &_ambiguitySlot);
    level("ambiguity.candidates", &_ambiguityCandidates);
    writable("ambiguity.resolved", &_ambiguityResolved);
    // Confirmed deletion: the question a Metalaw asks, and what awaits the answer.
    writable("question", &_question);
    level("pending.targets", &_pendingTargetsText);
    level("pending.names", &_pendingNames);
    // The line's own settings.
    writable("menuRows", &_menuRows);
    writable("autoMenu", &_autoMenu);
    writable("color", &_color);
    writable("hints", &_hints);
    writable("relayLogs", &_relayLogs);

    registerProperty(std::make_unique<ComputedProperty<TerminalChannel, std::string>>(
        "output", this, &TerminalChannel::propOutput, &TerminalChannel::propSetOutput));
    registerProperty(std::make_unique<ComputedProperty<TerminalChannel, double>>(
        "speakRequests", this, &TerminalChannel::propSpeakRequests,
        &TerminalChannel::propSetSpeakRequests));
    level("spokenRequests", &_spoken);
    // Terminal Zones: where the line is. Writing it asks the line to move
    // (applied in act(), through the same held-closure preflight as `enter`).
    registerProperty(std::make_unique<ComputedProperty<TerminalChannel, std::string>>(
        "zone", this, &TerminalChannel::propZone, &TerminalChannel::propSetZone));
    // Identity: a Law bumps this to ask the kernel for ONE secret line.
    writable("unlockRequests", &_unlockRequests);
    level("unlocksHandled", &_unlocksHandled);
    level("awaitingSecret", &_awaitingSecretFlag);
    level("linesEntered", &_linesEntered);
    level("attached", &_attached);
}

} // namespace Terminal
} // namespace Singularity
