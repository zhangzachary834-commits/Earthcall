#pragma once

#include "Singularity/Terminal/LawSentence.hpp"

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// The Terminal channel's line editor: keystrokes in, one redrawable region out.
//
// Zach, 2026-09-25: "tab shouldn't just display a list of all possibilities
// above me as a message. Tab should be able to actually select one and arrow
// keys should be able to move between them … like claude code cli … it can
// show me stuff temporarily without sending as a full message."
//
// libedit could not: its Tab prints matches into the scrollback and it has no
// selectable menu. Every CLI that behaves the way Zach describes (Claude Code
// via Ink, fish, zsh's menu-select) owns a small region under the prompt and
// redraws it in place. This is that region, as pure state + a rendered frame,
// so every behaviour is testable without a terminal. TerminalChannel feeds it
// bytes and writes its frames; it decides nothing about what a word MEANS —
// suggestions, colours, and the live preview come from LawSentence, which
// reads the world's Lexemes and Laws.
//
// Machine mechanism below the Kernel (NO_BLACK_BOX §5): the edit buffer, the
// cursor, the menu selection, history, and the escape-sequence decoder. The
// settings a Person may want to change are registered on TerminalChannel.
// ---------------------------------------------------------------------------

namespace Singularity {
namespace Terminal {

struct Key {
    enum class Kind {
        Text, Enter, Tab, BackTab, Backspace, Delete, Left, Right, Up, Down, Home, End,
        WordLeft, WordRight, Escape, Interrupt, EndOfInput, KillToStart, KillToEnd, KillWord,
        Redraw, HistorySearch, PasteBegin, PasteEnd,
        // Rung 3: the mouse (only reported while a menu or help is open),
        // paging, help, and the terminal's answer to "where is the cursor?".
        WheelUp, WheelDown, Click, PageUp, PageDown, Help, CursorReport
    };
    Kind kind = Kind::Text;
    std::string text;   // Kind::Text: one UTF-8 character
    int x = 0;          // Click / CursorReport: column (Click: region-relative once mapped)
    int y = 0;          // Click / CursorReport: row
};

// Bytes from a terminal in character mode -> keys. A lone ESC is ambiguous
// with the start of a sequence; flush() resolves it once `escapeDelay`
// seconds pass with nothing following.
class KeyDecoder {
public:
    std::vector<Key> feed(const std::string& bytes, double now);
    std::vector<Key> flush(double now);
    double escapeDelay = 0.03;
    // A cursor-position report looks like a key (`ESC[r;cR`); it is only
    // read as one when the channel has just asked for it.
    void expectCursorReport() { ++_expectReports; }

private:
    int _expectReports = 0;
    std::vector<Key> drain(bool final);
    std::string _pending;
    double _pendingSince = 0.0;
    bool _pasting = false;
};

class LineEditor {
public:
    struct Status {
        std::string text;
        std::string role;                         // preview, error, search, note
        std::size_t errorOffset = std::string::npos;   // into the buffer, for the caret
    };
    using SuggestFn = std::function<std::vector<LawSentence::Suggestion>(const std::string& beforeCursor)>;
    using HighlightFn = std::function<std::vector<LawSentence::Span>(const std::string& text)>;
    using StatusFn = std::function<std::vector<Status>(const std::string& text)>;

    enum class Outcome { None, Submitted, Interrupt, EndOfInput, Redraw, Help };

    void setProviders(SuggestFn suggest, HighlightFn highlight, StatusFn status);
    // Asked before Enter submits: "" lets the line go; anything else keeps it
    // in place and shows the answer (what is still missing, with an example).
    std::function<std::string(const std::string& text)> submitGate;
    Outcome press(const Key& key);
    std::string takeSubmitted();
    void setNotice(const std::string& notice) { _notice = notice; }

    // A transient, scrollable overlay in the panel (help). Lines arrive
    // already styled and fitted to the width; wheel/↑↓/PgUp/PgDn scroll,
    // Esc or typing closes it. It never enters the scrollback.
    void showOverlay(std::vector<std::string> lines) { _overlay = std::move(lines); _overlayScroll = 0; }
    void closeOverlay() { _overlay.clear(); }
    bool overlayVisible() const { return !_overlay.empty(); }
    int overlayScroll() const { return _overlayScroll; }
    // Mouse reporting is wanted only while there is something to point at.
    bool wantsMouse() const { return menuVisible() || overlayVisible(); }
    void refresh() { edited(false); }   // the world changed: recompute menu and status

    void setHistory(std::vector<std::string> history) { _history = std::move(history); }
    const std::vector<std::string>& history() const { return _history; }

    // Settings (TerminalChannel registers these as properties).
    std::string prompt = "earthcall> ";
    int menuRows = 8;
    bool autoMenu = true;
    bool color = true;
    bool hints = true;
    int overlayRows = 18;
    // The status footer (plain text; the editor styles it) and the colour of
    // its leading mark — green when this Zone hears the line, yellow if not.
    std::string footer;
    std::string footerMark = "32";

    // One drawable region: the input (which the terminal wraps) and the panel.
    struct Frame {
        std::string text;     // rows joined by "\r\n", starting at column 0
        int rows = 1;
        int cursorRow = 0;
        int cursorCol = 0;
    };
    Frame render(int width) const;

    // The submitted line as it should stay in the scrollback, coloured.
    std::string echo(const std::string& line) const;

    // Inspection (tests).
    const std::string& buffer() const { return _buffer; }
    std::size_t cursor() const { return _cursor; }
    bool menuVisible() const;
    int selected() const { return _selected; }
    const std::vector<LawSentence::Suggestion>& suggestions() const { return _suggestions; }
    std::string ghost() const;
    bool searching() const { return _searching; }
    bool onPlaceholder() const;
    bool hasPlaceholders() const;

private:
    void edited(bool textChanged);
    void insert(const std::string& text);
    void accept(const LawSentence::Suggestion& s);
    void acceptGhost();
    std::string typedTail(const LawSentence::Suggestion& s) const;
    std::string currentWord() const;
    void historyStep(int direction);
    std::string historyMatch() const;
    std::string styled(const std::string& text, bool markCursor = false) const;
    std::size_t placeholderEnd(std::size_t at) const;
    void jumpToPlaceholder();
    void eraseSelectedPlaceholder();
    void clickAt(int row, int col);
    std::string sgr(const std::string& code, const std::string& text) const;

    std::string _buffer;
    std::size_t _cursor = 0;
    std::string _submitted;
    std::string _notice;

    std::vector<LawSentence::Suggestion> _suggestions;
    std::vector<Status> _status;
    int _selected = 0;
    bool _menuForced = false;
    bool _menuSuppressed = false;
    bool _navigated = false;
    bool _pasting = false;

    std::vector<std::string> _history;
    int _historyIndex = -1;   // -1 = editing the draft
    std::string _draft;

    bool _searching = false;
    std::string _query;
    int _searchSkip = 0;

    std::vector<std::string> _overlay;
    int _overlayScroll = 0;
    // What the last frame put on each region row (a menu item's index, or -1)
    // — how a click becomes a choice.
    mutable std::vector<int> _rowItems;
    mutable int _inputRows = 1;
    mutable int _width = 80;

    SuggestFn _suggest;
    HighlightFn _highlight;
    StatusFn _statusFn;
};

// Visible width of text that may contain ANSI SGR sequences and UTF-8.
std::size_t visibleWidth(const std::string& text);

} // namespace Terminal
} // namespace Singularity
