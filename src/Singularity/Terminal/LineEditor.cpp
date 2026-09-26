#include "Singularity/Terminal/LineEditor.hpp"

#include <algorithm>
#include <cctype>

namespace Singularity {
namespace Terminal {

namespace {

bool isContinuation(unsigned char c) { return (c & 0xC0) == 0x80; }

std::size_t utf8Length(unsigned char lead) {
    if (lead < 0x80) return 1;
    if ((lead >> 5) == 0x6) return 2;
    if ((lead >> 4) == 0xE) return 3;
    if ((lead >> 3) == 0x1E) return 4;
    return 1;
}

std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

bool startsWithCi(const std::string& s, const std::string& prefix) {
    return s.size() >= prefix.size() && lower(s.substr(0, prefix.size())) == lower(prefix);
}

std::size_t prevBoundary(const std::string& s, std::size_t i) {
    if (i == 0) return 0;
    --i;
    while (i > 0 && isContinuation(static_cast<unsigned char>(s[i]))) --i;
    return i;
}

std::size_t nextBoundary(const std::string& s, std::size_t i) {
    if (i >= s.size()) return s.size();
    ++i;
    while (i < s.size() && isContinuation(static_cast<unsigned char>(s[i]))) ++i;
    return i;
}

bool isSpace(char c) { return c == ' ' || c == '\t'; }

// Colour of each reading, so the line shows how it is being understood.
std::string roleColor(const std::string& role) {
    if (role == "clause" || role == "logic" || role == "filler") return "35";
    if (role == "activation" || role == "scope" || role == "preset") return "1;95";
    if (role == "action") return "32";
    if (role == "operator") return "33";
    if (role == "condition" || role == "kind") return "36";
    if (role == "value") return "38;5;215";
    if (role == "path") return "97";
    if (role == "being") return "96";
    if (role == "event") return "94";
    if (role == "name") return "1";
    if (role == "error") return "4;31";
    return "";
}

// One panel line, cut at the terminal's width however its parts are styled.
struct PanelLine {
    std::size_t max;
    bool color;
    std::string out;
    std::size_t used = 0;

    void add(const std::string& text, const std::string& code = "") {
        if (used >= max) return;
        std::string piece;
        std::size_t i = 0;
        while (i < text.size() && used < max) {
            const std::size_t n = std::min(utf8Length(static_cast<unsigned char>(text[i])), text.size() - i);
            piece += text.substr(i, n);
            i += n;
            ++used;
        }
        if (i < text.size() && !piece.empty()) {
            // Mark the cut so a truncated description never reads as whole.
            const std::size_t last = prevBoundary(piece, piece.size());
            piece = piece.substr(0, last) + "…";
        }
        out += (color && !code.empty()) ? "\x1b[" + code + "m" + piece + "\x1b[0m" : piece;
    }
};

} // namespace

std::size_t visibleWidth(const std::string& text) {
    std::size_t width = 0;
    for (std::size_t i = 0; i < text.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(text[i]);
        if (c == 0x1b && i + 1 < text.size() && text[i + 1] == '[') {
            i += 2;
            while (i < text.size() && !(text[i] >= 0x40 && text[i] <= 0x7E)) ++i;
            continue;
        }
        if (!isContinuation(c)) ++width;
    }
    return width;
}

// ---------------------------------------------------------------------------
// KeyDecoder
// ---------------------------------------------------------------------------

std::vector<Key> KeyDecoder::feed(const std::string& bytes, double now) {
    if (_pending.empty()) _pendingSince = now;
    _pending += bytes;
    auto keys = drain(false);
    if (!_pending.empty() && keys.size() > 0) _pendingSince = now;
    return keys;
}

std::vector<Key> KeyDecoder::flush(double now) {
    if (_pending.empty() || now - _pendingSince < escapeDelay) return {};
    return drain(true);
}

std::vector<Key> KeyDecoder::drain(bool final) {
    using K = Key::Kind;
    std::vector<Key> keys;
    const auto push = [&](K kind, std::string text = {}) { keys.push_back(Key{kind, std::move(text), 0, 0}); };
    const auto pushAt = [&](K kind, int x, int y) { keys.push_back(Key{kind, {}, x, y}); };
    // "b;x;y" or "r;c" -> numbers.
    const auto numbers = [](const std::string& text) {
        std::vector<int> out;
        int value = 0;
        bool any = false;
        for (char c : text) {
            if (c >= '0' && c <= '9') { value = value * 10 + (c - '0'); any = true; }
            else if (c == ';') { out.push_back(any ? value : 0); value = 0; any = false; }
        }
        out.push_back(any ? value : 0);
        return out;
    };
    std::size_t i = 0;
    const std::string& p = _pending;
    while (i < p.size()) {
        if (_pasting) {
            static const std::string kEnd = "\x1b[201~";
            const std::size_t end = p.find(kEnd, i);
            const std::size_t stop = end == std::string::npos
                ? (final ? p.size() : (p.size() > i + kEnd.size() ? p.size() - kEnd.size() : i))
                : end;
            while (i < stop) {
                const std::size_t n = std::min(utf8Length(static_cast<unsigned char>(p[i])), stop - i);
                std::string ch = p.substr(i, n);
                if (ch == "\r" || ch == "\n" || ch == "\t") ch = " ";   // one line: breaks become spaces
                if (static_cast<unsigned char>(ch[0]) >= 0x20) push(K::Text, ch);
                i += n;
            }
            if (end == std::string::npos) break;
            i = end + kEnd.size();
            _pasting = false;
            push(K::PasteEnd);
            continue;
        }
        const unsigned char c = static_cast<unsigned char>(p[i]);
        if (c == 0x1b) {
            if (i + 1 >= p.size()) {
                if (!final) break;
                push(K::Escape);
                ++i;
                continue;
            }
            const char next = p[i + 1];
            if (next == '[') {
                std::size_t j = i + 2;
                while (j < p.size() && !(p[j] >= 0x40 && p[j] <= 0x7E)) ++j;
                if (j >= p.size()) {
                    if (!final) break;
                    push(K::Escape);
                    i = p.size();
                    continue;
                }
                const std::string params = p.substr(i + 2, j - i - 2);
                const char fin = p[j];
                // SGR mouse: ESC[<b;x;yM (press) / m (release). Only presses
                // matter: the wheel (64 up, 65 down) and the left button.
                if (!params.empty() && params[0] == '<' && (fin == 'M' || fin == 'm')) {
                    const auto n = numbers(params.substr(1));
                    if (fin == 'M' && n.size() >= 3) {
                        const int b = n[0];
                        if (b & 64) pushAt((b & 1) ? K::WheelDown : K::WheelUp, n[1], n[2]);
                        else if ((b & 3) == 0 && !(b & 32)) pushAt(K::Click, n[1], n[2]);
                    }
                    i = j + 1;
                    continue;
                }
                // The terminal's answer to "where is the cursor?" (ESC[6n).
                if (fin == 'R' && _expectReports > 0) {
                    --_expectReports;
                    const auto n = numbers(params);
                    if (n.size() >= 2) pushAt(K::CursorReport, n[1], n[0]);
                    i = j + 1;
                    continue;
                }
                const bool word = params.find(";5") != std::string::npos || params.find(";3") != std::string::npos;
                switch (fin) {
                    case 'A': push(K::Up); break;
                    case 'B': push(K::Down); break;
                    case 'C': push(word ? K::WordRight : K::Right); break;
                    case 'D': push(word ? K::WordLeft : K::Left); break;
                    case 'H': push(K::Home); break;
                    case 'F': push(K::End); break;
                    case 'Z': push(K::BackTab); break;
                    case '~':
                        if (params == "200") {
                            _pasting = true;
                            push(K::PasteBegin);
                        } else if (params == "3") {
                            push(K::Delete);
                        } else if (params == "5") {
                            push(K::PageUp);
                        } else if (params == "6") {
                            push(K::PageDown);
                        } else if (params == "11") {
                            push(K::Help);
                        } else if (params == "1" || params == "7") {
                            push(K::Home);
                        } else if (params == "4" || params == "8") {
                            push(K::End);
                        }
                        break;
                    default: break;   // an unknown sequence is consumed, never typed
                }
                i = j + 1;
                continue;
            }
            if (next == 'O') {
                if (i + 2 >= p.size()) {
                    if (!final) break;
                    push(K::Escape);
                    i = p.size();
                    continue;
                }
                switch (p[i + 2]) {
                    case 'A': push(K::Up); break;
                    case 'B': push(K::Down); break;
                    case 'C': push(K::Right); break;
                    case 'D': push(K::Left); break;
                    case 'H': push(K::Home); break;
                    case 'F': push(K::End); break;
                    case 'P': push(K::Help); break;   // F1
                    default: break;
                }
                i += 3;
                continue;
            }
            if (next == 'b') { push(K::WordLeft); i += 2; continue; }
            if (next == 'f') { push(K::WordRight); i += 2; continue; }
            if (next == 0x7f) { push(K::KillWord); i += 2; continue; }
            push(K::Escape);
            ++i;
            continue;
        }
        switch (c) {
            case '\r':
                push(K::Enter);
                i += (i + 1 < p.size() && p[i + 1] == '\n') ? 2 : 1;
                continue;
            case '\n': push(K::Enter); break;
            case '\t': push(K::Tab); break;
            case 0x7f: case 0x08: push(K::Backspace); break;
            case 0x01: push(K::Home); break;
            case 0x05: push(K::End); break;
            case 0x02: push(K::Left); break;
            case 0x06: push(K::Right); break;
            case 0x10: push(K::Up); break;
            case 0x0e: push(K::Down); break;
            case 0x03: push(K::Interrupt); break;
            case 0x04: push(K::EndOfInput); break;
            case 0x15: push(K::KillToStart); break;
            case 0x0b: push(K::KillToEnd); break;
            case 0x17: push(K::KillWord); break;
            case 0x0c: push(K::Redraw); break;
            case 0x12: push(K::HistorySearch); break;
            default:
                if (c >= 0x20) {
                    const std::size_t n = utf8Length(c);
                    if (i + n > p.size()) {
                        if (!final) goto done;
                        i = p.size();
                        continue;
                    }
                    push(K::Text, p.substr(i, n));
                    i += n;
                    continue;
                }
                break;   // other controls mean nothing here
        }
        ++i;
    }
done:
    _pending.erase(0, i);
    return keys;
}

// ---------------------------------------------------------------------------
// LineEditor
// ---------------------------------------------------------------------------

void LineEditor::setProviders(SuggestFn suggest, HighlightFn highlight, StatusFn status) {
    _suggest = std::move(suggest);
    _highlight = std::move(highlight);
    _statusFn = std::move(status);
    edited(false);
}

void LineEditor::edited(bool textChanged) {
    if (textChanged) {
        _menuSuppressed = false;
        _historyIndex = -1;
    }
    _navigated = false;
    _selected = 0;
    _suggestions = _suggest ? _suggest(_buffer.substr(0, _cursor)) : std::vector<LawSentence::Suggestion>{};
    _status = _statusFn ? _statusFn(_buffer) : std::vector<Status>{};
    if (_suggestions.empty()) _menuForced = false;
}

std::string LineEditor::currentWord() const {
    std::size_t start = _cursor;
    while (start > 0 && !isSpace(_buffer[start - 1])) --start;
    return _buffer.substr(start, _cursor - start);
}

std::string LineEditor::typedTail(const LawSentence::Suggestion& s) const {
    if (s.from > _cursor) return {};
    return _buffer.substr(s.from, _cursor - s.from);
}

bool LineEditor::menuVisible() const {
    if (_searching || _suggestions.empty()) return false;
    if (_menuForced) return true;
    if (!autoMenu || _menuSuppressed || currentWord().empty()) return false;
    // Nothing to offer when what is typed already IS the best candidate.
    const auto& top = _suggestions.front();
    if (lower(typedTail(top)) == lower(top.text)) return false;
    return true;
}

void LineEditor::insert(const std::string& text) {
    _buffer.insert(_cursor, text);
    _cursor += text.size();
}

// ---------------------------------------------------------------------------
// Blanks. A blank is simply "‹…›" in the text — found by scanning, never
// tracked, so no edit can leave bookkeeping behind. The blank the cursor sits
// on is "selected": typing replaces it, and the menu offers what may fill it.
// ---------------------------------------------------------------------------
namespace {
const std::string kOpen = "\u2039";    // ‹
const std::string kClose = "\u203A";   // ›
}

std::size_t LineEditor::placeholderEnd(std::size_t at) const {
    if (_buffer.compare(at, kOpen.size(), kOpen) != 0) return std::string::npos;
    const std::size_t close = _buffer.find(kClose, at + kOpen.size());
    return close == std::string::npos ? std::string::npos : close + kClose.size();
}

bool LineEditor::onPlaceholder() const { return placeholderEnd(_cursor) != std::string::npos; }

bool LineEditor::hasPlaceholders() const { return _buffer.find(kOpen) != std::string::npos; }

void LineEditor::eraseSelectedPlaceholder() {
    const std::size_t end = placeholderEnd(_cursor);
    if (end != std::string::npos) _buffer.erase(_cursor, end - _cursor);
}

void LineEditor::jumpToPlaceholder() {
    std::size_t from = _cursor;
    if (onPlaceholder()) from = placeholderEnd(_cursor);
    std::size_t at = _buffer.find(kOpen, from);
    if (at == std::string::npos) at = _buffer.find(kOpen);
    if (at == std::string::npos) return;
    _cursor = at;
    edited(false);
    _menuForced = !_suggestions.empty();   // show what may fill this blank
}

void LineEditor::accept(const LawSentence::Suggestion& s) {
    const bool fillingBlank = onPlaceholder();
    eraseSelectedPlaceholder();   // a chosen word fills the selected blank
    const std::size_t from = std::min(s.from, _cursor);
    // Keep the casing the Person typed ("se" + Set -> "set"); only the
    // untyped remainder comes from the candidate.
    const std::string typed = _buffer.substr(from, _cursor - from);
    const std::string text = startsWithCi(s.text, typed) ? typed + s.text.substr(typed.size()) : s.text;
    _buffer.replace(from, _cursor - from, text);
    _cursor = from + text.size();
    // The word's own blanks follow it ("set" -> "set ‹path› to ‹value›"),
    // and the first one is selected — unless blanks already follow.
    if (!s.snippet.empty()) {
        const std::string rest = _buffer.substr(_cursor);
        const std::size_t lead = rest.find_first_not_of(' ');
        if (lead == std::string::npos || rest.compare(lead, kOpen.size(), kOpen) != 0) {
            _buffer.insert(_cursor, " " + s.snippet);
        }
        const std::size_t first = _buffer.find(kOpen, _cursor);
        if (first != std::string::npos) {
            _cursor = first;
            _menuForced = false;
            edited(true);
            _menuForced = !_suggestions.empty();
            return;
        }
    }
    const bool continues = !text.empty() && text.back() != '.';
    if (continues && (_cursor >= _buffer.size() || _buffer[_cursor] != ' ')) {
        _buffer.insert(_cursor, " ");
        ++_cursor;
    }
    _menuForced = false;
    edited(true);
    // Filling one blank moves on to the next, as snippets do in an IDE.
    if (fillingBlank && _buffer.find(kOpen, _cursor) != std::string::npos) jumpToPlaceholder();
}

void LineEditor::clickAt(int row, int col) {
    if (row >= 0 && row < static_cast<int>(_rowItems.size()) && _rowItems[row] >= 0 &&
        _rowItems[row] < static_cast<int>(_suggestions.size())) {
        _selected = _rowItems[row];
        accept(_suggestions[_selected]);
        return;
    }
    if (row < 0 || row >= _inputRows || _searching) return;
    // A click on the line itself moves the cursor there.
    const int cell = row * _width + col - static_cast<int>(visibleWidth(prompt));
    if (cell < 0) return;
    std::size_t at = 0;
    int seen = 0;
    while (at < _buffer.size() && seen < cell) {
        at = nextBoundary(_buffer, at);
        ++seen;
    }
    _cursor = at;
    _menuForced = false;
    edited(false);
}

std::string LineEditor::historyMatch() const {
    if (_buffer.empty()) return {};
    for (auto it = _history.rbegin(); it != _history.rend(); ++it) {
        if (it->size() > _buffer.size() && it->compare(0, _buffer.size(), _buffer) == 0) return *it;
    }
    return {};
}

std::string LineEditor::ghost() const {
    if (_cursor != _buffer.size() || _searching) return {};
    if (menuVisible() && _selected < static_cast<int>(_suggestions.size())) {
        const auto& s = _suggestions[_selected];
        const std::string tail = typedTail(s);
        if (!tail.empty() && s.text.size() > tail.size() && startsWithCi(s.text, tail)) {
            return s.text.substr(tail.size());
        }
        return {};
    }
    const std::string h = historyMatch();
    return h.empty() ? h : h.substr(_buffer.size());
}

void LineEditor::acceptGhost() {
    if (menuVisible()) {
        if (!ghost().empty()) accept(_suggestions[_selected]);
        return;
    }
    const std::string g = ghost();
    if (g.empty()) return;
    insert(g);
    edited(true);
}

void LineEditor::historyStep(int direction) {
    if (_history.empty()) return;
    if (_historyIndex == -1) {
        if (direction > 0) return;
        _draft = _buffer;
        _historyIndex = static_cast<int>(_history.size()) - 1;
    } else {
        _historyIndex += direction;
        if (_historyIndex < 0) _historyIndex = 0;
        if (_historyIndex >= static_cast<int>(_history.size())) {
            _historyIndex = -1;
            _buffer = _draft;
            _cursor = _buffer.size();
            edited(false);
            _menuSuppressed = true;
            return;
        }
    }
    _buffer = _history[_historyIndex];
    _cursor = _buffer.size();
    edited(false);
    // A recalled line is walked with ↑↓; its menu waits until it is edited.
    _menuSuppressed = true;
    _menuForced = false;
}

std::string LineEditor::takeSubmitted() {
    std::string s;
    s.swap(_submitted);
    return s;
}

LineEditor::Outcome LineEditor::press(const Key& key) {
    using K = Key::Kind;
    _notice.clear();

    if (overlayVisible()) {
        const int last = std::max(0, static_cast<int>(_overlay.size()) - overlayRows);
        const auto scrollBy = [&](int d) { _overlayScroll = std::clamp(_overlayScroll + d, 0, last); };
        switch (key.kind) {
            case K::Up: scrollBy(-1); return Outcome::None;
            case K::Down: scrollBy(+1); return Outcome::None;
            case K::WheelUp: scrollBy(-3); return Outcome::None;
            case K::WheelDown: scrollBy(+3); return Outcome::None;
            case K::PageUp: scrollBy(-overlayRows); return Outcome::None;
            case K::PageDown: scrollBy(+overlayRows); return Outcome::None;
            case K::Escape: case K::Enter: case K::Help: case K::Click: case K::Interrupt:
                closeOverlay();
                return Outcome::None;
            default:
                closeOverlay();   // typing goes on where it left off
                break;
        }
    }
    if (key.kind == K::CursorReport) return Outcome::None;
    if (key.kind == K::Help) return Outcome::Help;
    if (key.kind == K::Click) {
        clickAt(key.y, key.x);
        return Outcome::None;
    }

    if (_searching) {
        const auto match = [&]() -> std::string {
            int skip = _searchSkip;
            for (auto it = _history.rbegin(); it != _history.rend(); ++it) {
                if (lower(*it).find(lower(_query)) == std::string::npos) continue;
                if (skip-- == 0) return *it;
            }
            return {};
        };
        switch (key.kind) {
            case K::Text: _query += key.text; _searchSkip = 0; return Outcome::None;
            case K::Backspace:
                _query.erase(prevBoundary(_query, _query.size()));
                _searchSkip = 0;
                return Outcome::None;
            case K::HistorySearch: ++_searchSkip; return Outcome::None;
            case K::Escape: case K::Interrupt: _searching = false; return Outcome::None;
            default: {
                const std::string found = match();
                _searching = false;
                if (!found.empty()) {
                    _buffer = found;
                    _cursor = _buffer.size();
                    edited(true);
                }
                if (key.kind == K::Enter) return Outcome::None;   // Enter takes it; a second Enter speaks it
                return press(key);
            }
        }
    }

    const auto select = [&](int delta) {
        const int n = static_cast<int>(_suggestions.size());
        _selected = ((_selected + delta) % n + n) % n;
        _navigated = true;
    };

    switch (key.kind) {
        case K::Text:
            eraseSelectedPlaceholder();
            insert(key.text);
            if (!_pasting) edited(true);
            return Outcome::None;
        case K::WheelUp:
            if (menuVisible()) select(-1);
            return Outcome::None;
        case K::WheelDown:
            if (menuVisible()) select(+1);
            return Outcome::None;
        case K::PageUp:
        case K::PageDown:
            if (menuVisible()) {
                const int n = static_cast<int>(_suggestions.size());
                const int step = std::max(1, menuRows);
                _selected = std::clamp(_selected + (key.kind == K::PageUp ? -step : step), 0, n - 1);
                _navigated = true;
            }
            return Outcome::None;
        case K::PasteBegin: _pasting = true; return Outcome::None;
        case K::PasteEnd: _pasting = false; edited(true); return Outcome::None;
        case K::Enter:
            if (menuVisible() && _navigated) {
                accept(_suggestions[_selected]);
                return Outcome::None;
            }
            if (_buffer.find_first_not_of(' ') == std::string::npos) return Outcome::None;
            if (hasPlaceholders()) {
                _notice = "fill the ‹blanks› first — tab jumps between them";
                return Outcome::None;
            }
            if (submitGate) {
                const std::string why = submitGate(_buffer);
                if (!why.empty()) {
                    _notice = why;   // the line stays; nothing lands in the scrollback
                    return Outcome::None;
                }
            }
            _submitted = _buffer;
            if (_history.empty() || _history.back() != _buffer) _history.push_back(_buffer);
            _buffer.clear();
            _cursor = 0;
            _menuForced = false;
            edited(true);
            return Outcome::Submitted;
        case K::Tab: {
            if (menuVisible()) {
                accept(_suggestions[_selected]);
                return Outcome::None;
            }
            if (hasPlaceholders()) {   // the menu is closed: go to the next blank
                jumpToPlaceholder();
                return Outcome::None;
            }
            if (_suggestions.empty()) {
                acceptGhost();   // nothing to choose between: Tab takes the ghost
                return Outcome::None;
            }
            if (_suggestions.size() == 1) {
                accept(_suggestions.front());
                return Outcome::None;
            }
            // Extend to what every prefix-matching candidate shares, then
            // open the menu for the choice that remains.
            const auto& top = _suggestions.front();
            const std::string tail = typedTail(top);
            std::string common;
            bool first = true;
            for (const auto& s : _suggestions) {
                if (s.from != top.from || !startsWithCi(s.text, tail)) continue;
                const std::string t = lower(s.text);
                if (first) { common = t; first = false; continue; }
                std::size_t k = 0;
                while (k < common.size() && k < t.size() && common[k] == t[k]) ++k;
                common.resize(k);
            }
            if (common.size() > tail.size() && !first) {
                const std::size_t from = std::min(top.from, _cursor);
                _buffer.replace(from, _cursor - from, top.text.substr(0, common.size()));
                _cursor = from + common.size();
                edited(true);
            }
            _menuForced = true;
            return Outcome::None;
        }
        case K::BackTab:
            if (menuVisible()) select(-1);
            return Outcome::None;
        case K::Up:
            if (menuVisible()) select(-1);
            else historyStep(-1);
            return Outcome::None;
        case K::Down:
            if (menuVisible()) select(+1);
            else historyStep(+1);
            return Outcome::None;
        case K::Left:
            _cursor = prevBoundary(_buffer, _cursor);
            _menuForced = false;
            edited(false);
            return Outcome::None;
        case K::Right:
            if (_cursor < _buffer.size()) {
                _cursor = nextBoundary(_buffer, _cursor);
                _menuForced = false;
                edited(false);
            } else {
                acceptGhost();
            }
            return Outcome::None;
        case K::Home: _cursor = 0; _menuForced = false; edited(false); return Outcome::None;
        case K::End:
            if (_cursor == _buffer.size()) acceptGhost();
            _cursor = _buffer.size();
            edited(false);
            return Outcome::None;
        case K::WordLeft:
            while (_cursor > 0 && isSpace(_buffer[_cursor - 1])) --_cursor;
            while (_cursor > 0 && !isSpace(_buffer[_cursor - 1])) --_cursor;
            edited(false);
            return Outcome::None;
        case K::WordRight:
            while (_cursor < _buffer.size() && isSpace(_buffer[_cursor])) ++_cursor;
            while (_cursor < _buffer.size() && !isSpace(_buffer[_cursor])) ++_cursor;
            edited(false);
            return Outcome::None;
        case K::Backspace: {
            if (onPlaceholder()) {
                eraseSelectedPlaceholder();
                edited(true);
                return Outcome::None;
            }
            if (_cursor == 0) return Outcome::None;
            const std::size_t from = prevBoundary(_buffer, _cursor);
            _buffer.erase(from, _cursor - from);
            _cursor = from;
            edited(true);
            return Outcome::None;
        }
        case K::Delete:
            if (onPlaceholder()) {
                eraseSelectedPlaceholder();
                edited(true);
            } else if (_cursor < _buffer.size()) {
                _buffer.erase(_cursor, nextBoundary(_buffer, _cursor) - _cursor);
                edited(true);
            }
            return Outcome::None;
        case K::KillToStart:
            _buffer.erase(0, _cursor);
            _cursor = 0;
            edited(true);
            return Outcome::None;
        case K::KillToEnd:
            _buffer.erase(_cursor);
            edited(true);
            return Outcome::None;
        case K::KillWord: {
            std::size_t from = _cursor;
            while (from > 0 && isSpace(_buffer[from - 1])) --from;
            while (from > 0 && !isSpace(_buffer[from - 1])) --from;
            _buffer.erase(from, _cursor - from);
            _cursor = from;
            edited(true);
            return Outcome::None;
        }
        case K::Escape:
            if (menuVisible()) {
                _menuSuppressed = true;
                _menuForced = false;
            } else if (!_buffer.empty()) {
                _buffer.clear();
                _cursor = 0;
                edited(true);
            }
            return Outcome::None;
        case K::Interrupt:
            if (!_buffer.empty()) {
                _buffer.clear();
                _cursor = 0;
                edited(true);
                return Outcome::None;
            }
            return Outcome::Interrupt;
        case K::EndOfInput:
            if (_buffer.empty()) return Outcome::EndOfInput;
            if (_cursor < _buffer.size()) {
                _buffer.erase(_cursor, nextBoundary(_buffer, _cursor) - _cursor);
                edited(true);
            }
            return Outcome::None;
        case K::Redraw: return Outcome::Redraw;
        case K::Click: case K::Help: case K::CursorReport: return Outcome::None;   // handled above
        case K::HistorySearch:
            _searching = true;
            _query.clear();
            _searchSkip = 0;
            return Outcome::None;
    }
    return Outcome::None;
}

std::string LineEditor::sgr(const std::string& code, const std::string& text) const {
    if (!color || code.empty() || text.empty()) return text;
    return "\x1b[" + code + "m" + text + "\x1b[0m";
}

std::string LineEditor::styled(const std::string& text, bool markCursor) const {
    // Blanks read as blanks: dim italic, and the selected one inverted.
    std::vector<std::pair<std::size_t, std::size_t>> blanks;
    for (std::size_t at = text.find(kOpen); at != std::string::npos; at = text.find(kOpen, at + 1)) {
        const std::size_t close = text.find(kClose, at);
        if (close == std::string::npos) break;
        blanks.emplace_back(at, close + kClose.size());
    }
    if (!color) return text;
    const auto spans = _highlight ? _highlight(text) : std::vector<LawSentence::Span>{};
    std::string out;
    std::string current;
    for (std::size_t i = 0; i < text.size();) {
        std::string code;
        bool inBlank = false;
        for (const auto& [a, b] : blanks) {
            if (i >= a && i < b) {
                inBlank = true;
                code = (markCursor && a == _cursor) ? "7" : "3;38;5;245";
            }
        }
        if (!inBlank) {
            std::string role;
            for (const auto& s : spans) {
                if (i >= s.start && i < s.end) role = s.role;   // later spans (errors) win
            }
            code = roleColor(role);
        }
        if (code != current) {
            out += "\x1b[0m";
            if (!code.empty()) out += "\x1b[" + code + "m";
            current = code;
        }
        const std::size_t n = std::min(utf8Length(static_cast<unsigned char>(text[i])), text.size() - i);
        out += text.substr(i, n);
        i += n;
    }
    if (!current.empty()) out += "\x1b[0m";
    return out;
}

std::string LineEditor::echo(const std::string& line) const {
    return sgr("2", prompt) + styled(line);
}

namespace {

// The menu's section names when Tab lists everything that may come next.
const char* groupLabel(const std::string& role) {
    if (role == "preset" || role == "activation") return "Presets";
    if (role == "action") return "Actions";
    if (role == "operator") return "Comparisons";
    if (role == "condition" || role == "kind") return "Conditions";
    if (role == "value") return "Values";
    if (role == "event") return "Events";
    if (role == "path") return "Properties";
    if (role == "being") return "Beings & Laws";
    return "Clause words";
}

// Which letters of `candidate` the typed text matched: prefix, then a
// substring, then letters in order — the same order the menu ranks by.
std::vector<bool> matchedLetters(const std::string& candidate, const std::string& typed) {
    std::vector<bool> hit(candidate.size(), false);
    if (typed.empty() || typed.find(' ') != std::string::npos) return hit;
    const std::string c = lower(candidate), t = lower(typed);
    std::size_t at = c.find(t);
    if (at != std::string::npos) {
        for (std::size_t k = 0; k < t.size(); ++k) hit[at + k] = true;
        return hit;
    }
    std::size_t j = 0;
    for (std::size_t i = 0; i < c.size() && j < t.size(); ++i) {
        if (c[i] == t[j]) { hit[i] = true; ++j; }
    }
    if (j < t.size()) std::fill(hit.begin(), hit.end(), false);
    return hit;
}

} // namespace

LineEditor::Frame LineEditor::render(int width) const {
    const std::size_t w = static_cast<std::size_t>(std::max(width, 20));
    const std::size_t max = w - 1;
    _width = static_cast<int>(w);
    Frame frame;

    std::string input;
    std::size_t before = 0;   // visible cells before the cursor
    std::size_t total = 0;
    if (_searching) {
        std::string found;
        int skip = _searchSkip;
        for (auto it = _history.rbegin(); it != _history.rend(); ++it) {
            if (lower(*it).find(lower(_query)) == std::string::npos) continue;
            if (skip-- == 0) { found = *it; break; }
        }
        const std::string label = "history search: ";
        input = sgr("1;33", label) + _query + "  " + sgr("2", found.empty() ? "(no match)" : found);
        before = visibleWidth(label) + visibleWidth(_query);
        total = visibleWidth(input);
    } else {
        const std::string g = ghost();
        input = sgr("1;38;5;141", prompt) + styled(_buffer, true) + sgr("2", g);
        before = visibleWidth(prompt) + visibleWidth(_buffer.substr(0, _cursor));
        total = visibleWidth(prompt) + visibleWidth(_buffer) + visibleWidth(g);
    }
    const int inputRows = total == 0 ? 1 : static_cast<int>((total + w - 1) / w);
    _inputRows = inputRows;
    frame.cursorRow = static_cast<int>(before / w);
    frame.cursorCol = static_cast<int>(before % w);

    std::vector<std::string> panel;
    std::vector<int> items;   // parallel to panel: the menu item on that row, or -1
    const auto line = [&]() { return PanelLine{max, color, {}, 0}; };
    const auto push = [&](const std::string& text, int item = -1) {
        panel.push_back(text);
        items.push_back(item);
    };

    if (overlayVisible()) {
        // Help: a window onto its lines, and how to move it.
        const int n = static_cast<int>(_overlay.size());
        const int rows = std::min(overlayRows, n);
        for (int i = _overlayScroll; i < _overlayScroll + rows && i < n; ++i) push(_overlay[i]);
        if (n > rows) {
            PanelLine l = line();
            l.add("  ↑↓ wheel · PgUp PgDn scroll · esc closes    " + std::to_string(_overlayScroll + 1) + "–" +
                      std::to_string(std::min(n, _overlayScroll + rows)) + " of " + std::to_string(n),
                  "2");
            push(l.out);
        }
    } else {
        // A caret under the character the sentence stumbled on.
        if (!_searching && inputRows == 1) {
            for (const auto& s : _status) {
                if (s.role != "error" || s.errorOffset == std::string::npos) continue;
                const std::size_t col =
                    visibleWidth(prompt) + visibleWidth(_buffer.substr(0, std::min(s.errorOffset, _buffer.size())));
                if (col < max) {
                    PanelLine l = line();
                    l.add(std::string(col, ' '));
                    l.add("^", "1;31");
                    push(l.out);
                }
                break;
            }
        }

        const bool menu = menuVisible();
        if (menu) {
            const int n = static_cast<int>(_suggestions.size());
            // Tab on an empty word lists everything next: show it in sections.
            const bool grouped = typedTail(_suggestions.front()).empty();
            struct Row { bool header; int item; std::string label; };
            std::vector<Row> rows;
            std::string lastGroup;
            for (int i = 0; i < n; ++i) {
                const std::string g = groupLabel(_suggestions[i].role);
                if (grouped && g != lastGroup) {
                    rows.push_back({true, -1, g});
                    lastGroup = g;
                }
                rows.push_back({false, i, {}});
            }
            int selectedRow = 0;
            for (int r = 0; r < static_cast<int>(rows.size()); ++r) {
                if (!rows[r].header && rows[r].item == _selected) selectedRow = r;
            }
            const int total = static_cast<int>(rows.size());
            const int shown = std::min(std::max(menuRows, 1) + (grouped ? 2 : 0), total);
            int start = std::clamp(selectedRow - shown / 2, 0, total - shown);
            if (start > 0 && !rows[start].header && grouped) {
                // Keep a section's header visible above its first shown item.
                for (int r = start; r >= 0; --r) {
                    if (rows[r].header) { if (selectedRow - r < shown) start = r; break; }
                }
            }
            std::size_t textCol = 0;
            for (int r = start; r < start + shown; ++r) {
                if (!rows[r].header) textCol = std::max(textCol, visibleWidth(_suggestions[rows[r].item].text));
            }
            textCol = std::min<std::size_t>(textCol, 30);
            for (int r = start; r < start + shown; ++r) {
                PanelLine l = line();
                if (rows[r].header) {
                    l.add("   " + rows[r].label, "1;38;5;244");
                    push(l.out);
                    continue;
                }
                const int i = rows[r].item;
                const auto& s = _suggestions[i];
                const bool chosen = i == _selected;
                l.add(chosen ? "  ▸ " : "    ", chosen ? "1;36" : "");
                const auto hit = matchedLetters(s.text, typedTail(s));
                const std::string base = chosen ? "7" : roleColor(s.role);
                for (std::size_t k = 0; k < s.text.size();) {
                    const std::size_t len = std::min(utf8Length(static_cast<unsigned char>(s.text[k])), s.text.size() - k);
                    const std::string code = hit[k] ? (base.empty() ? "1" : base + ";1") : base;
                    l.add(s.text.substr(k, len), code);
                    k += len;
                }
                const std::size_t shownWidth = visibleWidth(s.text);
                if (shownWidth < textCol) l.add(std::string(textCol - shownWidth, ' '), chosen ? "7" : "");
                if (!s.description.empty()) {
                    l.add("  ");
                    l.add(s.description, "2");
                }
                push(l.out, i);
            }
            const int moreItems = n - [&] {
                int c = 0;
                for (int r = start; r < start + shown; ++r) if (!rows[r].header) ++c;
                return c;
            }();
            if (moreItems > 0) {
                PanelLine l = line();
                l.add("    " + std::to_string(moreItems) + " more · wheel, ↑↓ or PgUp PgDn · keep typing to narrow", "2");
                push(l.out);
            }
            // The selected entry, explained.
            const auto& sel = _suggestions[std::clamp(_selected, 0, n - 1)];
            const std::string detail = sel.detail.empty() ? std::string{} : sel.detail;
            if (!detail.empty() && detail != sel.description) {
                PanelLine l = line();
                l.add("    ⤷ ", "38;5;141");
                l.add(detail, "3");
                push(l.out);
            }
        }

        for (const auto& s : _status) {
            PanelLine l = line();
            if (s.role == "error") {
                l.add("  ✗ ", "31");
                l.add(s.text, "31");
            } else if (s.role == "preview") {
                l.add("  ↳ ", "2");
                l.add(s.text, "2");
            } else if (s.role == "question") {
                l.add("  ? ", "1;33");
                l.add(s.text, "1;33");
            } else if (s.role == "note") {
                l.add("    " + s.text, "2");
            } else {
                l.add("  " + s.text);
            }
            push(l.out);
        }

        if (!_notice.empty()) {
            PanelLine l = line();
            l.add("  " + _notice, "33");
            push(l.out);
        }
    }

    if (hints || !footer.empty()) {
        std::string h;
        if (overlayVisible()) {
            h = "esc closes help";
        } else if (_searching) {
            h = "type to search history · ctrl-r older · enter take · esc cancel";
        } else if (menuVisible()) {
            h = "tab take · ↑↓ wheel choose · click picks · esc close";
        } else if (hasPlaceholders()) {
            h = "type to fill the blank · tab next blank · enter when all are filled";
        } else if (_buffer.empty()) {
            h = "type a law sentence · tab shows words · help · ↑ history";
        } else {
            h = "tab complete · → take ghost · enter author · end with ? to preview";
        }
        PanelLine l = line();
        if (!footer.empty()) {
            l.add("  ◆ ", footerMark);
            l.add(footer, "2");
            if (hints) l.add("  │  " + h, "2");
        } else {
            l.add("  " + h, "2");
        }
        push(l.out);
    }

    _rowItems.assign(static_cast<std::size_t>(inputRows), -1);
    for (int item : items) _rowItems.push_back(item);

    frame.text = input;
    for (const auto& p : panel) frame.text += "\r\n" + p;
    frame.rows = inputRows + static_cast<int>(panel.size());
    return frame;
}

} // namespace Terminal
} // namespace Singularity
