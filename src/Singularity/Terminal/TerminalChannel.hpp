#pragma once

#include "Singularity/Terminal/LawSentence.hpp"
#include "Singularity/Terminal/LineEditor.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"

#include <cstdint>
#include <deque>
#include <optional>
#include <functional>
#include <string>
#include <vector>

class Person;

namespace Singularity {
namespace Terminal {

// First-mover channel: the Mac Terminal's command line — the window
// `Run Earthcall.command` opened — as a modality of the running world.
//
// Zach, 2026-09-25: "implement this in the terminal first … Make something
// like a TerminalChannel in the Singularity … its just really cool to be able
// to control WORLD from the MAC COMMAND CENTER". The legacy `earthcall_terminal`
// (src/terminal_entry.cpp) is a separate program and is left exactly as it is.
//
// SENSE. Each finished line becomes the level `lastLine` and the edge
// `terminal-line-entered` (subject: this channel). The event is named here,
// where it is sensed — as `locomotion-started` lives in LocomotionChannel.
//
// DECIDE — authored, not here. Whether a line becomes a Law is the world's
// call: the LawLine seed authors
//     on terminal-line-entered -> Publish "law-sentence-spoken"
//     on law-sentence-spoken  -> Add @terminal-channel.speakRequests 1
// so "law-sentence-spoken" is an authored event, named only in law text.
//
// ACT. When `speakRequests` advances, the channel reads `lastLine` through
// the LawSentence grammar (the terminal's own reading; nothing in-world
// parses text) and authors the Law, recorded as written by the being at
// `authorPath` — the Person at the machine by default. Its answer is written
// to `output`, which the channel prints. No law in this Zone hears the line?
// The channel says so rather than staying silent.
//
// AMBIGUITY. Shared spellings are legitimate. When two meanings stay
// admissible, the channel sets `ambiguity.symbol / .slot / .candidates`,
// applies every Law that targets it (its Metalaws), and takes whichever
// candidate they wrote to `ambiguity.resolved`. None did -> refused.
//
// THE LINE ITSELF (2026-09-25, Zach: "tab should … actually select one and
// arrow keys should … move between them … show me stuff temporarily"): the
// channel owns a LineEditor region under the prompt — a live menu, ghost
// text, the sentence coloured by how it is read, a transient preview/error
// panel, history — redrawn in place. The app's own stdout/stderr are relayed
// above that region (dimmed, and kept in saves/logs/earthcall-terminal.log)
// so they never tear the line being typed. Settings a Person may change are
// registered: menuRows, autoMenu, color, hints, relayLogs, prompt.
//
// AUTHORSHIP — AN OPEN GAP, NOT A GUARANTEE. Every spoken Law is recorded as
// written by whoever `authorPath` names (the Person present). But this channel
// cannot tell a Person typing from any process writing to its stdin — a test
// harness, a script, an agent. Astra's warning ("a string saying Terminal
// supplies transport context, not proof that Zach authored an utterance") holds
// here: a non-Person writer should arrive as a registered First Mover under a
// Person's grant, as MCP does via ForeignActuationGuard. That door now exists
// (authorForeign below, reached by the socket's `law_sentence`, 2026-09-30);
// the stdin path itself still trusts its writer -- see Law_Line.md, "The line
// trusts its stdin, and stdin can lie".
class TerminalChannel : public Law {
public:
    using Sink = std::function<void(const std::string&)>;

    TerminalChannel();
    ~TerminalChannel() override;

    bool isFirstMover() const override { return true; }
    std::string getIdentifier() const override { return "terminal-channel"; }

    static void syncRegister(LawManager& laws);
    static TerminalChannel* find(LawManager& laws);

    // Once per frame BEFORE LawManager::tick(): read the keyboard, publish at
    // most one line (so the laws answer each line before the next arrives).
    void sense(LawManager& laws);
    // Once per frame AFTER LawManager::tick(): speak if the laws asked.
    void act(LawManager& laws);

    // The live vocabulary: structural words, the engine's opcode spellings,
    // every Lexeme that denotes a Law, heard events, present beings.
    LawSentence::Vocabulary vocabulary(LawManager& laws, const std::string& authorId = "");

    // The Law Line for a writer who is NOT the Person at the keyboard: a
    // foreign First Mover (MCP) that has already proved its key and been
    // admitted by ForeignActuationGuard. Same grammar, same vocabulary, same
    // Law construction as a typed line -- only the author differs, and it is
    // the mover, never @interaction-channel.personId. Closes the gap named
    // above ("a non-Person writer should arrive as a registered First
    // Mover"). A trailing '?' previews and a leading '??' searches; both are
    // read-only and take no author. Immediate acts ("delete Blue") are
    // refused here: they need the channel's own confirming Metalaw.
    struct ForeignSentence {
        std::string status;                   // authored | preview | search | refused
        std::string lawId;
        std::string preview;                  // WHEN ... -> IF ... -> THEN ...
        std::string detail;                   // dry run / persistence / notes
        std::string error;
        std::vector<std::string> candidates;
        std::vector<std::string> openClauses;
    };
    static bool isReadOnlySentence(const std::string& text);
    ForeignSentence authorForeign(LawManager& laws, const std::string& text,
                                  const std::vector<Singular*>& authors,
                                  const std::string& identifier);

    // ------------------------------------------------------------------
    // Terminal Zones (Zach, 2026-09-30; Terminal_Zones.md). The line has a
    // LOCATION: a real Zone whose Laws hear what is typed there. The Person's
    // body stays where it is; only the line moves. `enter <zone>` is the one
    // move opcode and works in every Zone (a Zone that decided whether you
    // may leave it could trap the line). A Law moves it by writing `zone`.
    // ⚠ GATE: build nothing on top of these opcodes until Zach verifies them
    // as minimal-maximal invariants.
    // ------------------------------------------------------------------
    static constexpr const char* kLineHolder = "terminal-channel";
    const std::string& zone() const { return _zone; }
    bool awaitingSecret() const { return _secretStage != 0; }
    // Why Enter would KEEP this line rather than send it ("" = sent). The
    // editor's submit gate; public so tests witness what a Person's Enter does.
    std::string submitRefusal(const std::string& text);
    // Test seam: the Person the Identity Zone makes present (default: the
    // Engine's Person).
    void setPresencePerson(Person* person) { _presencePerson = person; }
    // Why a typed line may not author right now ("" = it may). Kernel guard,
    // never a property: stdin authors only as a Person who is PRESENT, i.e.
    // proved their key to this process this session (Identity unlock), the
    // same root MCP grants terminate in. Zach, 2026-10-05: "Require presence".
    std::string presenceRefusal();
    // Test seam (C++ only, like setPresencePerson): replaces the register
    // check so an isolated harness, which holds no real private key, can
    // stand in for a present Person. Never reachable from Law or a socket.
    void setPresenceCheckForTests(std::function<bool(const Person&)> check) { _presenceCheck = std::move(check); }

    // Test seams: a line as if typed and entered; where output goes.
    void inject(const std::string& line) { _pending.push_back(line); }
    void setSink(Sink sink) { _sink = std::move(sink); }
    bool attached() const { return _attached; }

private:
    void buildProperties() override;
    void speak(LawManager& laws, const std::string& text);
    void placeLine();
    bool moveLine(const std::string& target);         // false = stayed, and said why
    bool handleEnter(const std::string& trimmed);      // true = the line was `enter ...`
    void beginSecret();
    void takeSecret(std::string& secret);
    void cancelSecret(const std::string& why);
    std::string zoneLabel() const;
    // The Law Line grammar (gate, menu, colouring) reads the line only where
    // the line is in the Law Line -- or nowhere yet (legacy). Elsewhere a line
    // is sent as typed and that Zone's Laws decide what it means.
    bool lawGrammarHere() const { return _zone.empty() || _zone == "LawLine"; }
    // The one place a parsed sentence becomes a live Law (speak + authorForeign).
    // Returns "" on success, else the refusal.
    std::string enact(LawManager& laws, const LawSentence::Parse& p, const std::string& text,
                      const std::vector<Singular*>& authors, const std::string& id,
                      std::string& persistence);
    void say(const std::string& text);
    const LawSentence::Vocabulary& liveVocabulary();
    const LawSentence::Parse& liveParse(const std::string& text);
    std::vector<LineEditor::Status> statusOf(const std::string& text);
    void handleKeys(const std::vector<Key>& keys, double now);
    void draw();
    void printAbove(const std::string& text);
    std::string erase() const;
    int width() const;
    std::string describeProperty(const std::string& beingId, const std::string& property) const;
    std::string describeBeing(const std::string& beingId) const;
    static std::string describeSingular(Singular* being);
    std::string propertySuggestionBeing() const;
    // Rung 3 (Zach, 2026-09-25): the footer, help, confirmed deletion, dry run.
    std::string footerText(bool& hears);
    void showHelp();
    void requestDeletion(LawManager& laws, const std::string& target);
    void answer(LawManager& laws, const std::string& line);
    void cancelDeletion(const std::string& why);
    bool awaitingAnswer() const { return !_pendingTargets.empty() && !_question.empty(); }
    std::string dryRun(const LawSentence::Parse& p);
    std::string lawSummary(const Law& law, LawManager& laws) const;
    LawSentence::Resolution resolveByMetalaw(LawManager& laws, const LawSentence::Ambiguity& a);
    LawSentence::Compilation compileByMetalaw(LawManager& laws, const nlohmann::json& input, bool readOnly, nlohmann::json* document = nullptr);
    std::string propCompilationResult() const;
    void attach(LawManager& laws);
    void detach();

    std::string propOutput() const { return _output; }
    std::string propZone() const { return _zone; }
    void propSetZone(const std::string& v) { _requestedZone = v; }
    void propSetOutput(const std::string& v);
    double propSpeakRequests() const { return _speakRequests; }
    void propSetSpeakRequests(const double& v) { _speakRequests = v; }

    // Law-legible state (NO_BLACK_BOX: every field is a registered path).
    std::string _lastLine;
    // Multi-line block (Python-style) being typed: its lines so far. Folded
    // into one sentence by LawSentence::unfoldBlock when an empty line ends it.
    // Registered read-only as `block` (lines joined by newlines).
    std::vector<std::string> _block;
    std::string propBlock() const;
    // The line being typed, read in its open block's context: the folded text
    // and how far the line's own offsets shift inside it (0 = no context).
    std::string blockContext(const std::string& line, long& shift);
    std::string _output;
    std::string _prompt = "earthcall> ";
    double _linesEntered = 0.0;
    double _speakRequests = 0.0;
    double _spoken = 0.0;
    std::string _authorPath = "@interaction-channel.personId";
    // Terminal Zones: where the line is, where a Law asked it to go.
    std::string _zone;
    std::string _requestedZone;
    bool _zoneBootTried = false;
    // Identity: a Law asks (unlockRequests), the kernel takes one secret line.
    double _unlockRequests = 0.0;
    double _unlocksHandled = 0.0;
    bool _awaitingSecretFlag = false;          // registered mirror of _secretStage
    // NOT registered, named per NO_BLACK_BOX §5 (secrets beneath the Kernel):
    // _secretStage (0 none, 1 unlock, 2 new passphrase, 3 confirm) and
    // _pendingSecret (the first entry while keying, wiped after confirmation).
    int _secretStage = 0;
    std::string _pendingSecret;
    Person* _presencePerson = nullptr;          // test seam; null = Engine's Person
    std::function<bool(const Person&)> _presenceCheck;   // test seam; empty = FirstMoverRegister
    std::string _lexemeRelation = "denotes";
    std::string _scopeBeing;
    std::string _status;
    std::string _preview;
    std::string _openClauses;
    std::string _lastCreated;
    std::string _ambiguitySymbol;
    std::string _ambiguitySlot;
    std::string _ambiguityCandidates;
    std::string _ambiguityResolved;
    // Generic invocation protocol. Input is channel-sensed syntax; templates
    // and refusals are authored by Metalaws targeting this channel. The result
    // is derived solely by structural JSON substitution, not by opcode lowering.
    std::shared_ptr<PropertyDict> _compilationInput = std::make_shared<PropertyDict>();
    std::string _compilationTemplate;
    std::string _compilationError;
    bool _attached = false;
    // Confirmed deletion (registered): the question a Metalaw asks, and what
    // is waiting for the Person's answer.
    std::string _question;
    std::string _pendingTargetsText;
    std::string _pendingNames;
    // Settings of the line (registered).
    int _menuRows = 8;
    bool _autoMenu = true;
    bool _color = true;
    bool _hints = true;
    bool _relayLogs = true;

    // Below the Kernel — machine mechanism, not world state (named per
    // NO_BLACK_BOX §5): the queue between libedit's callback and the frame,
    // whether attachment was tried, where printed text goes, and libedit's
    // own terminal state (process-global, owned by the C library).
    std::deque<std::string> _pending;
    bool _attachTried = false;
    Sink _sink;
    LawManager* _laws = nullptr;
    // The drawn region: the editor's state, the key decoder, where the cursor
    // sits inside the region, and per-frame caches of the vocabulary and the
    // live parse (rebuilt every frame, so they never outlive a world change).
    LineEditor _editor;
    KeyDecoder _decoder;
    bool _drawn = false;
    int _cursorRow = 0;
    int _lastWidth = 0;
    double _lastInterrupt = -10.0;
    std::uint64_t _frame = 0;
    std::uint64_t _vocabFrame = 0;
    std::optional<LawSentence::Vocabulary> _vocab;
    std::string _parseText;
    std::optional<LawSentence::Parse> _parse;
    // Mouse reporting is on only while a menu or help is open; a cursor
    // report after each draw says which screen row the region starts on.
    bool _mouseOn = false;
    bool _wasAwaiting = false;
    int _reportScreenRow = -1;
    int _reportRegionRow = 0;
    // The Law(s) a deletion request names, and the one being deleted now.
    std::vector<std::string> _pendingTargets;
    std::string _deletingId;
    std::string _deletingName;
};

} // namespace Terminal
} // namespace Singularity
