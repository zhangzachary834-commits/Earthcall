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
constexpr const char* kHistoryFile = "saves/logs/terminal-history.txt";
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
    writeTty("\x1b[?2004l\x1b[?25h");
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
    static const char reset[] = "\x1b[?2004l\x1b[?25h\r\n";
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
    std::ifstream in(kHistoryFile);
    for (std::string line; std::getline(in, line);) {
        if (!line.empty()) history.push_back(line);
    }
    if (history.size() > 1000) history.erase(history.begin(), history.end() - 1000);
    _editor.setHistory(std::move(history));

    _editor.setProviders(
        [this](const std::string& before) { return LawSentence::suggest(before, liveVocabulary()); },
        [this](const std::string& text) { return liveParse(text).spans; },
        [this](const std::string& text) { return statusOf(text); });

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
    for (const Key& key : keys) {
        switch (_editor.press(key)) {
            case LineEditor::Outcome::Submitted: {
                const std::string line = _editor.takeSubmitted();
                printAbove(_editor.echo(line));
                _pending.push_back(line);
                std::ofstream(kHistoryFile, std::ios::app) << line << '\n';
                break;
            }
            case LineEditor::Outcome::Interrupt:
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
        v.words.push_back({lexeme->getSymbol(), opcode, lexeme->getIdentifier(), law->getIdentifier(), law->name()});
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
    if (text.find_first_not_of(" \t") == std::string::npos) return out;
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
            out.push_back({"next: " + next, "note"});
            return out;
        }
        if (typingIt) return out;   // the menu is the answer while the word is unfinished
        out.push_back({p.error, "error", at});
        if (!p.candidates.empty()) out.push_back({"candidates: " + join(p.candidates, ", "), "note"});
        return out;
    }
    out.push_back({p.preview(), "preview"});
    for (const auto& n : p.notes) out.push_back({n, "note"});
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
    if (first.rfind("authored", 0) == 0) shown = paint("32", "✓ " + first) + paint("2", rest);
    else if (first.rfind("refused", 0) == 0) shown = paint("31", "✗ " + first) + paint("2", rest);
    else if (first.rfind("(", 0) == 0) shown = paint("33", v);
    else shown = v;
    printAbove(shown);
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
