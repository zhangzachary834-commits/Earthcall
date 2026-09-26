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

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
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
            if (!awaitingAnswer()) return LawSentence::suggest(before, liveVocabulary());
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
        [this](const std::string& text) { return liveParse(text).spans; },
        [this](const std::string& text) { return statusOf(text); });
    // Enter on a sentence that cannot be authored yet keeps the line and says
    // what is missing, instead of filling the scrollback with refusals.
    _editor.submitGate = [this](const std::string& text) -> std::string {
        const std::string t = text.substr(text.find_first_not_of(" \t"));
        if (awaitingAnswer() || t == "help" || t == "help " || t == "?") return {};
        if (t.rfind("??", 0) == 0 || t.back() == '?') return {};
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
    };

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
    if (awaitingAnswer()) {
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
                const std::string line = _editor.takeSubmitted();
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
                if (awaitingAnswer()) {          // Ctrl-C is a no
                    cancelDeletion("nothing deleted");
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
        if (_attached && (dirty || !_drawn)) draw();
    }
#endif

    if (_pending.empty()) return;
    std::string line = _pending.front();
    _pending.pop_front();
    // While a Metalaw's question waits, the next line is its answer — never a
    // new sentence. An empty line answers too: it is not a yes.
    if (awaitingAnswer()) {
        answer(laws, line);
        if (_attached) draw();
        return;
    }
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

LawSentence::Vocabulary TerminalChannel::vocabulary(LawManager& laws) {
    LawSentence::Vocabulary v;
    v.words = LawSentence::canonicalWords();

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
        if (opcode == "preset" || opcode == "value") v.presets.push_back(preset);
    }

    std::set<std::string> events;
    for (const auto& [type, count] : heardEvents()) events.insert(type);
    for (const auto& law : laws.getAll()) {
        if (!law) continue;
        for (const auto& t : laws.triggersOf(law->getIdentifier())) events.insert(t);
        if (law->hasActionModel()) collectPublished(*law->actionModel(), events);
    }
    v.events.assign(events.begin(), events.end());

    // The Laws spoken or authored here, by name — for "delete Blue".
    for (const auto& law : laws.getAll()) {
        if (!law || law->isFirstMover() || law.get() == this || !law->isEnabled()) continue;
        v.laws.push_back({law->getIdentifier(), law->name(), lawSummary(*law, laws)});
    }

    std::set<std::string> beings;
    for (Singular* being : Universe::instance().beings()) {
        if (being && !being->getIdentifier().empty()) beings.insert(being->getIdentifier());
    }
    v.beings.assign(beings.begin(), beings.end());
    // Bare paths complete against the authored scope being — or, when none
    // is set, against whatever the Person last clicked in the world.
    v.scopeBeing = _scopeBeing;
    if (v.scopeBeing.empty()) {
        PropertyValue focused;
        if (lawGetValue(*this, PropertyPath::parse("@interaction-channel.focusedId"), focused)) {
            if (const auto* id = std::get_if<std::string>(&focused)) v.scopeBeing = *id;
        }
    }

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
    v.describeBeing = [this](const std::string& b) { return describeBeing(b); };
    v.describeEvent = [](const std::string& e) {
        const auto it = heardEvents().find(e);
        return it == heardEvents().end() ? std::string("event")
                                         : "event · heard " + std::to_string(it->second) + "×";
    };
    v.resolve = [this, &laws](const LawSentence::Ambiguity& a) { return resolveByMetalaw(laws, a); };
    return v;
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
    if (!being) return {};
    std::string kind = dynamic_cast<Person*>(being)                              ? "person"
                     : dynamic_cast<Zone*>(being)                                ? "zone"
                     : dynamic_cast<Law*>(being)                                 ? "law"
                     : dynamic_cast<Relation*>(being)                            ? "relation"
                     : dynamic_cast<Singularity::Language::Lexeme*>(being)       ? "lexeme"
                     : dynamic_cast<Object*>(being)                              ? "object"
                                                                                 : "being";
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
        // Live reading must not act: a shared spelling is shown with its first
        // meaning; the Metalaws decide for real when the sentence is spoken.
        _vocab->resolve = [](const LawSentence::Ambiguity& a) {
            return LawSentence::Resolution{a.candidates.front().individual(),
                                           "a Metalaw decides which when it is spoken"};
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
        _parse = LawSentence::parse(question ? text : text + "?", liveVocabulary());
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

void TerminalChannel::speak(LawManager& laws, const std::string& text) {
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
    if (count == 0) return "right now the IF holds for nothing here";
    return "right now the IF holds for " + std::to_string(count) + (count == 1 ? " being: " : " beings: ") +
           join(names, ", ") + (count > names.size() ? ", …" : "");
}

std::string TerminalChannel::footerText(bool& hears) {
    hears = _laws && _laws->rete().hearsType(kLineEntered);
    std::string zone = "no Zone";
    if (ZoneManager* zones = ZoneManager::live()) {
        if (!zones->zones().empty()) zone = zones->active().name();
    }
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
    const std::string scope = _laws ? liveVocabulary().scopeBeing : std::string{};
    return zone + " · " + (hears ? "hears the line" : "does NOT hear the line") +
           (scope.empty() ? "" : " · scope @" + scope) + " · as " + author + " · " + std::to_string(count) +
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
    writable("prompt", &_prompt);
    writable("authorPath", &_authorPath);
    writable("lexemeRelation", &_lexemeRelation);
    writable("scopeBeing", &_scopeBeing);
    level("status", &_status);
    level("preview", &_preview);
    level("openClauses", &_openClauses);
    level("lastCreated", &_lastCreated);
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
    level("linesEntered", &_linesEntered);
    level("attached", &_attached);
}

} // namespace Terminal
} // namespace Singularity
