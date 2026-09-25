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
// This is not a network channel: its author is the Person at the keyboard,
// so ForeignActuationGuard (for foreign movers) does not apply.
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
    LawSentence::Vocabulary vocabulary(LawManager& laws);

    // Test seams: a line as if typed and entered; where output goes.
    void inject(const std::string& line) { _pending.push_back(line); }
    void setSink(Sink sink) { _sink = std::move(sink); }
    bool attached() const { return _attached; }

private:
    void buildProperties() override;
    void speak(LawManager& laws, const std::string& text);
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
    LawSentence::Resolution resolveByMetalaw(LawManager& laws, const LawSentence::Ambiguity& a);
    void attach(LawManager& laws);
    void detach();

    std::string propOutput() const { return _output; }
    void propSetOutput(const std::string& v);
    double propSpeakRequests() const { return _speakRequests; }
    void propSetSpeakRequests(const double& v) { _speakRequests = v; }

    // Law-legible state (NO_BLACK_BOX: every field is a registered path).
    std::string _lastLine;
    std::string _output;
    std::string _prompt = "earthcall> ";
    double _linesEntered = 0.0;
    double _speakRequests = 0.0;
    double _spoken = 0.0;
    std::string _authorPath = "@interaction-channel.personId";
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
    bool _attached = false;
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
};

} // namespace Terminal
} // namespace Singularity
