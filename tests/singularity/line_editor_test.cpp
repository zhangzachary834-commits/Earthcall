// The Terminal channel's line editor, held to what Zach asked for
// (2026-09-25): Tab SELECTS, arrows MOVE between choices, and the line shows
// things temporarily instead of printing them as messages. Pure state and a
// rendered frame — no terminal needed.

#include "Singularity/Terminal/LawSentence.hpp"
#include "Singularity/Terminal/LineEditor.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace Singularity::Terminal;
namespace LS = Singularity::Terminal::LawSentence;

namespace {

LS::Vocabulary vocabulary() {
    LS::Vocabulary v;
    v.words = LS::canonicalWords();
    v.words.push_back({"greater than", "op.Gt", "lex_gt", "law_gt", "means: greater than"});
    v.words.push_back({"red", "value", "lex_red", "law_red", "means: red", "(1, 0, 0) · a Lexeme denoting law_red"});
    v.words.push_back({"my event-triggered law", "preset", "lex_evt", "law_evt", "my event-triggered law"});
    LS::Preset red;
    red.lawId = "law_red";
    red.value = PropertyValue(glm::vec3(1, 0, 0));
    LS::Preset evt;
    evt.lawId = "law_evt";
    v.presets = {red, evt};
    v.events = {"object-clicked", "object-hover-entered", "tick"};
    v.beings = {"cube"};
    v.scopeBeing = "cube";
    v.propertiesOf = [](const std::string&) { return std::vector<std::string>{"color", "glow", "hp"}; };
    v.describeProperty = [](const std::string&, const std::string& p) { return p == "hp" ? "= 3" : ""; };
    return v;
}

void type(LineEditor& e, const std::string& text) {
    for (char c : text) e.press(Key{Key::Kind::Text, std::string(1, c)});
}

void press(LineEditor& e, Key::Kind k) { e.press(Key{k, {}}); }

LineEditor editor(const LS::Vocabulary& v) {
    LineEditor e;
    e.setProviders([&v](const std::string& before) { return LS::suggest(before, v); },
                   [&v](const std::string& text) { return LS::parse(text + "?", v).spans; },
                   [&v](const std::string& text) {
                       std::vector<LineEditor::Status> s;
                       const auto p = LS::parse(text + "?", v);
                       if (!p.error.empty()) s.push_back({p.error, "error", p.errorOffset});
                       else if (!text.empty()) s.push_back({p.preview(), "preview"});
                       return s;
                   });
    return e;
}

void decoding() {
    KeyDecoder d;
    using K = Key::Kind;
    auto keys = d.feed("ab\x1b[A\x1b[B\x1b[C\x1b[D\t\x1b[Z\r\x7f\x03\x04\x12", 0.0);
    std::vector<K> kinds;
    for (const auto& k : keys) kinds.push_back(k.kind);
    const std::vector<K> want{K::Text, K::Text, K::Up, K::Down, K::Right, K::Left, K::Tab,
                              K::BackTab, K::Enter, K::Backspace, K::Interrupt, K::EndOfInput,
                              K::HistorySearch};
    assert(kinds == want);
    keys = d.feed("\x14\x1b[116;9u\x1b[116;9:2u\x1b[116;9:3u", 0.0);
    assert(keys.size() == 2 && keys[0].kind == K::Chord && keys[0].text == "ctrl+t" &&
           keys[1].kind == K::Chord && keys[1].text == "cmd+t");
    assert(d.feed("\x1b[999999999;9u\x1b[116;999999999u", 0.0).empty());
    assert(d.feed("\x1b[116;", 0.0).empty());
    keys = d.feed("9:1u", 0.0);
    assert(keys.size() == 1 && keys[0].text == "cmd+t");
    keys = d.feed("\x1b[200~a\x14" "b\x1b[201~", 0.0);
    assert(std::none_of(keys.begin(), keys.end(), [](const Key& key) { return key.kind == K::Chord; }));
    // Word jumps from Option/Ctrl-arrows, Home/End, Delete.
    keys = d.feed("\x1b[1;3D\x1b[1;5C\x1b[H\x1b[F\x1b[3~\x1b" "b\x1b" "f", 0.0);
    assert(keys.size() == 7 && keys[0].kind == K::WordLeft && keys[1].kind == K::WordRight &&
           keys[2].kind == K::Home && keys[3].kind == K::End && keys[4].kind == K::Delete &&
           keys[5].kind == K::WordLeft && keys[6].kind == K::WordRight);
    // A lone ESC waits for more, then resolves to Escape.
    keys = d.feed("\x1b", 1.0);
    assert(keys.empty());
    assert(d.flush(1.01).empty());
    keys = d.flush(1.05);
    assert(keys.size() == 1 && keys[0].kind == K::Escape);
    // A split escape sequence across reads is still one key.
    assert(d.feed("\x1b[", 2.0).empty());
    keys = d.feed("A", 2.0);
    assert(keys.size() == 1 && keys[0].kind == K::Up);
    // UTF-8 arrives whole, even split across reads.
    assert(d.feed("\xe2\x96", 3.0).empty());
    keys = d.feed("\xb8", 3.0);
    assert(keys.size() == 1 && keys[0].text == "\xe2\x96\xb8");
    // Bracketed paste: the text, with line breaks as spaces, between markers.
    keys = d.feed("\x1b[200~on tick\nthen\x1b[201~", 4.0);
    assert(keys.front().kind == K::PasteBegin && keys.back().kind == K::PasteEnd);
    std::string pasted;
    for (const auto& k : keys) if (k.kind == K::Text) pasted += k.text;
    assert(pasted == "on tick then");
}

void menu() {
    const auto v = vocabulary();
    LineEditor e = editor(v);

    // Typing opens the menu by itself; the best reading is first.
    type(e, "on object-clicked then se");
    assert(e.menuVisible());
    assert(e.suggestions().front().text == "set" || e.suggestions().front().text == "Set");
    assert(!e.ghost().empty());   // "t" shown dim after the cursor

    // Tab takes the selection and lays out the word's own blanks, with the
    // first one selected (Zach: "only shows the remaining args").
    press(e, Key::Kind::Tab);
    assert(e.buffer() == "on object-clicked then set \u2039path\u203A to \u2039value\u203A");
    assert(e.onPlaceholder() && e.menuVisible());   // the menu offers what fills ‹path›
    assert(e.press(Key{Key::Kind::Enter, {}}) == LineEditor::Outcome::None);   // blanks remain

    // Arrows move through what may fill the blank; Enter takes the choice.
    const std::string first = e.suggestions()[e.selected()].text;
    assert(e.suggestions().size() >= 2);
    press(e, Key::Kind::Down);
    assert(e.selected() == 1);
    press(e, Key::Kind::Up);
    press(e, Key::Kind::Up);   // wraps
    assert(e.selected() == static_cast<int>(e.suggestions().size()) - 1);
    press(e, Key::Kind::Down);
    press(e, Key::Kind::Enter);
    assert(e.buffer() == "on object-clicked then set " + first + " to \u2039value\u203A");
    assert(e.onPlaceholder());   // filling one blank selected the next

    // Typing replaces the selected blank; value words complete.
    type(e, "re");
    assert(e.buffer() == "on object-clicked then set " + first + " to re");
    assert(e.menuVisible());
    press(e, Key::Kind::Tab);
    assert(e.buffer() == "on object-clicked then set " + first + " to red ");
    assert(!e.hasPlaceholders());

    // Enter authors (submits) now that nothing is blank.
    assert(e.press(Key{Key::Kind::Enter, {}}) == LineEditor::Outcome::Submitted);
    assert(e.takeSubmitted() == "on object-clicked then set " + first + " to red ");
    assert(e.buffer().empty());

    // Tab jumps between blanks when the menu is closed; typing fills.
    type(e, "on tick then set ");
    press(e, Key::Kind::KillToStart);

    // Fuzzy: a subsequence still finds the phrase.
    type(e, "on tick if hp gth");
    bool found = false;
    for (const auto& s : e.suggestions()) found = found || s.text == "greater than";
    assert(found);
    press(e, Key::Kind::Escape);   // closes the menu
    assert(!e.menuVisible());
    press(e, Key::Kind::Escape);   // then clears the line
    assert(e.buffer().empty());

    // With the live menu off, Tab first extends to what every candidate
    // shares ("object-c…" / "object-h…" share "object-"), then opens the menu.
    e.autoMenu = false;
    type(e, "on obj");
    assert(!e.menuVisible());
    press(e, Key::Kind::Tab);
    assert(e.buffer() == "on object-");
    assert(e.menuVisible());
    press(e, Key::Kind::Tab);   // now Tab takes the selection
    assert(e.buffer() == "on object-clicked " || e.buffer() == "on object-hover-entered ");
    press(e, Key::Kind::KillToStart);
    assert(e.buffer().empty());
    e.autoMenu = true;
}

void noGluing() {
    // A next-word suggestion must never be glued onto a finished word
    // (found in the running app: "set co" + Tab became "set cofalse").
    auto v = vocabulary();
    v.scopeBeing.clear();
    for (const auto& s : LS::suggest("on tick then set co", v)) {
        assert(s.text != "false" && s.text != "true" && s.text != "to");
    }
    for (const auto& s : LS::suggest("on tick then set glow 1 0 0", v)) {
        assert(s.text != "then" && s.text != "and");
    }
}

void editing() {
    const auto v = vocabulary();
    LineEditor e = editor(v);
    type(e, "on tick then set glow 1");
    press(e, Key::Kind::WordLeft);
    press(e, Key::Kind::WordLeft);
    assert(e.cursor() == std::string("on tick then set ").size());
    press(e, Key::Kind::KillWord);
    assert(e.buffer() == "on tick then glow 1");
    press(e, Key::Kind::Home);
    press(e, Key::Kind::Delete);
    assert(e.buffer() == "n tick then glow 1");
    press(e, Key::Kind::End);
    press(e, Key::Kind::Backspace);
    assert(e.buffer() == "n tick then glow ");

    // Ctrl-C clears a line; on an empty line it asks the channel to quit.
    assert(e.press(Key{Key::Kind::Interrupt, {}}) == LineEditor::Outcome::None);
    assert(e.buffer().empty());
    assert(e.press(Key{Key::Kind::Interrupt, {}}) == LineEditor::Outcome::Interrupt);
    assert(e.press(Key{Key::Kind::EndOfInput, {}}) == LineEditor::Outcome::EndOfInput);

    // History: ↑ recalls, ↓ returns to the draft; ghost text offers history.
    e.setHistory({"on tick then set glow 1", "on object-clicked then set color red"});
    type(e, "draft");
    press(e, Key::Kind::Up);
    assert(e.buffer() == "on object-clicked then set color red");
    press(e, Key::Kind::Up);
    assert(e.buffer() == "on tick then set glow 1");
    press(e, Key::Kind::Down);
    press(e, Key::Kind::Down);
    assert(e.buffer() == "draft");
    press(e, Key::Kind::KillToStart);
    type(e, "on tick then set gl");
    press(e, Key::Kind::Escape);   // close the menu so history speaks
    assert(e.ghost() == "ow 1");
    press(e, Key::Kind::Right);    // → takes the ghost
    assert(e.buffer() == "on tick then set glow 1");

    // Ctrl-R searches history.
    press(e, Key::Kind::KillToStart);
    press(e, Key::Kind::HistorySearch);
    assert(e.searching());
    type(e, "color");
    press(e, Key::Kind::Enter);
    assert(!e.searching() && e.buffer() == "on object-clicked then set color red");

    // Paste arrives as one edit.
    press(e, Key::Kind::KillToStart);
    press(e, Key::Kind::PasteBegin);
    type(e, "on tick then add hp 1");
    press(e, Key::Kind::PasteEnd);
    assert(e.buffer() == "on tick then add hp 1");
}

void rendering() {
    const auto v = vocabulary();
    LineEditor e = editor(v);
    e.color = false;
    type(e, "on object-clicked then se");
    const auto f = e.render(80);
    assert(f.text.rfind("earthcall> on object-clicked then se", 0) == 0);
    assert(f.text.find("▸") != std::string::npos);          // the selected row
    assert(f.text.find("tab take") != std::string::npos);    // the footer tells you how
    assert(f.cursorRow == 0 && f.cursorCol == static_cast<int>(std::string("earthcall> on object-clicked then se").size()));
    assert(f.rows >= 3);

    // An error is shown live, with a caret under where the reading stopped.
    LineEditor bad = editor(v);
    bad.color = false;
    type(bad, "on tick then frobnicate glow");
    const auto g = bad.render(80);
    assert(g.text.find("✗") != std::string::npos);
    const std::string caretRow = "\r\n" + std::string(std::string("earthcall> on tick then ").size(), ' ') + "^";
    assert(g.text.find(caretRow) != std::string::npos);

    // Colour follows the reading: the action word is green.
    LineEditor colored = editor(v);
    type(colored, "on tick then set glow 1");
    assert(colored.render(80).text.find("\x1b[32mset") != std::string::npos);
    assert(visibleWidth("\x1b[32mset\x1b[0m ▸") == 5);
}

// Zach, 2026-10-14: "make the Law Line ... more beautiful using Lexeme
// symbols or objects with symbolic properties". The editor asks `look` how a
// word, a role or its own chrome appears; unanswered keys keep the built-in.
void appearance() {
    // Cells, not code points: authored glyphs must not shear the panel.
    assert(visibleWidth("\u27E8\u03C8|") == 3);              // ⟨ψ|
    assert(visibleWidth("\u2764") == 1);                      // ❤ text presentation
    assert(visibleWidth("\u2764\uFE0F") == 2);               // ❤️ emoji presentation
    assert(visibleWidth("\u6F22\u5B57") == 4);               // 漢字
    assert(visibleWidth("e\u0301") == 1);                     // é, combining
    assert(visibleWidth("\U0001F54A") == 1);                  // 🕊 defaults to text presentation
    assert(visibleWidth("\U0001F54A\uFE0F") == 2);           // 🕊️
    assert(visibleWidth("\u2728") == 2);                      // ✨ Emoji_Presentation
    assert(visibleWidth("\x1b[1;38;2;1;2;3m\u2726\x1b[0m") == 1);

    const std::string heart = "\u2764\uFE0F";
    const std::string family = "\U0001F468\u200D\U0001F469\u200D\U0001F467";
    const std::string flag = "\U0001F1FA\U0001F1F8";
    assert(visibleWidth(family) == 2 && visibleWidth(flag) == 2);
    assert(visibleWidth("\u2764\x1b[31m\uFE0F\x1b[0m") == 2);
    assert(clipToCells(heart, 1).empty());
    assert(clipToCells(heart + "x", 2, true) == "…");
    assert(clipToCells(family + "x", 2) == family);
    assert(clipToCells("e\u0301x", 1) == "e\u0301");
    for (const std::string bad : {std::string("\nextra-row"), std::string("\x1b[2J"),
                                  std::string("\r"), std::string("\t"), std::string("\xC2\x85"), std::string("\xFF")}) {
        assert(!isPrintableGlyph(bad));
        LineEditor control;
        control.hints = false;
        control.setProviders([](const std::string&) { return std::vector<LS::Suggestion>{{0, "red", "", "value", 1, "", "", ""}}; }, {}, {});
        control.look = [bad](const std::string& key) { return key == "chrome.selected" ? LineEditor::Look{bad, {}, {}, {}} : LineEditor::Look{}; };
        control.prefill("r");
        for (bool color : {false, true}) {
            control.color = color;
            const auto f = control.render(80);
            assert(std::count(f.text.begin(), f.text.end(), '\n') == f.rows - 1);
            assert(f.text.find("\u25B8") != std::string::npos); // printable default survives
        }
    }
    for (const auto& cluster : {heart, family, flag, std::string("e\u0301")}) {
        std::string word;
        for (int i = 0; i < 40; ++i) word += cluster;
        for (bool color : {false, true}) {
            LineEditor menu;
            menu.color = color; menu.hints = false;
            menu.setProviders([word](const std::string&) { return std::vector<LS::Suggestion>{{0, word, "", "value", 1, "", "", ""}}; }, {}, {});
            menu.prefill("x");
            const auto f = menu.render(28);
            std::size_t at = f.text.find("\r\n");
            while (at != std::string::npos) {
                at += 2;
                const auto end = f.text.find("\r\n", at);
                assert(visibleWidth(f.text.substr(at, end == std::string::npos ? end : end - at)) <= 27);
                at = end;
            }
            assert(f.text.find(cluster) != std::string::npos); // never ANSI between base and selector
        }
    }
    LineEditor edit;
    edit.prefill(family + heart);
    press(edit, Key::Kind::Left);
    assert(edit.cursor() == family.size());
    press(edit, Key::Kind::Backspace);
    assert(edit.buffer() == heart && edit.cursor() == 0);

    const auto v = vocabulary();
    LineEditor e = editor(v);
    e.look = [](const std::string& key) {
        if (key == "lexeme.lex_red") return LineEditor::Look{"\u25C9", "38;2;255;64;64", "1", ""};   // ◉
        if (key == "role.action") return LineEditor::Look{"", "", "1", ""};      // style only: keeps green
        if (key == "chrome.selected") return LineEditor::Look{"\u2727", "38;5;220", "", ""};      // ✧
        return LineEditor::Look{};
    };
    type(e, "on tick then set glow to r");
    const auto f = e.render(80);
    assert(f.text.find("\u2727") != std::string::npos);                      // authored selection mark
    assert(f.text.find("\u25B8") == std::string::npos);                      // the built-in one is gone
    assert(f.text.find("\x1b[1;38;2;255;64;64m\u25C9") != std::string::npos);  // the Lexeme's own glyph, in its colour
    assert(f.text.find("\x1b[1;32mset") != std::string::npos);              // style over the role colour

    // The typed word wears its Lexeme's look in the line itself.
    LineEditor typed = editor(v);
    typed.look = e.look;
    type(typed, "on tick then set glow to red");
    assert(typed.render(80).text.find("\x1b[1;38;2;255;64;64mred") != std::string::npos);

    // A swatch: the glyph in the colour the word denotes, the word in its role's.
    LineEditor swatch = editor(v);
    swatch.look = [](const std::string& key) {
        if (key == "lexeme.lex_red") return LineEditor::Look{"\u25CF", "", "", "38;2;255;0;0"};   // ●
        return LineEditor::Look{};
    };
    type(swatch, "on tick then set glow to r");
    const auto sw = swatch.render(80).text;
    assert(sw.find("\x1b[38;2;255;0;0m\u25CF") != std::string::npos);
    assert(sw.find("\x1b[38;5;215mr") != std::string::npos || sw.find("\x1b[38;5;215;1mr") != std::string::npos);

    // Without colour the glyph still shows; nothing else changes.
    LineEditor plain = editor(v);
    plain.look = e.look;
    plain.color = false;
    type(plain, "on tick then set glow to r");
    const auto p = plain.render(80);
    assert(p.text.find("\x1b[") == std::string::npos);
    assert(p.text.find("\u25C9 red") != std::string::npos);

    // Wide glyphs never push a panel row past the terminal's width.
    LineEditor narrow = editor(v);
    narrow.look = [](const std::string& key) {
        if (key == "lexeme.lex_red") return LineEditor::Look{"\u2728", "", "", ""};
        if (key == "chrome.footer") return LineEditor::Look{"\u6F22", "", "", ""};
        return LineEditor::Look{};
    };
    type(narrow, "on tick then set glow to r");
    const auto n = narrow.render(28);
    std::size_t at = 0;
    while (at <= n.text.size()) {
        const std::size_t end = n.text.find("\r\n", at);
        const std::string row = n.text.substr(at, end == std::string::npos ? std::string::npos : end - at);
        if (at > 0) assert(visibleWidth(row) <= 27);
        if (end == std::string::npos) break;
        at = end + 2;
    }
}

} // namespace

// Multi-line blocks: an empty Enter is submitted only while a block is open.
void blocks() {
    LineEditor e;
    assert(e.press(Key{Key::Kind::Enter, {}}) == LineEditor::Outcome::None);   // ignored normally
    e.submitEmpty = true;
    assert(e.press(Key{Key::Kind::Enter, {}}) == LineEditor::Outcome::Submitted);
    assert(e.takeSubmitted().empty());
    assert(e.history().empty());                                               // never into history
    e.prefill("    ");
    assert(e.buffer() == "    ");
    type(e, "hp > 2");
    assert(e.buffer() == "    hp > 2");
}

void rungThree() {
    using K = Key::Kind;
    // The mouse: SGR reports; wheel 64/65; a left press is a click; a cursor
    // report is only read as one when the channel asked for it.
    KeyDecoder d;
    auto keys = d.feed("\x1b[<64;10;5M\x1b[<65;10;5M\x1b[<0;12;7M\x1b[<0;12;7m\x1b[5~\x1b[6~\x1bOP", 0.0);
    assert(keys.size() == 6);
    assert(keys[0].kind == K::WheelUp && keys[1].kind == K::WheelDown);
    assert(keys[2].kind == K::Click && keys[2].x == 12 && keys[2].y == 7);
    assert(keys[3].kind == K::PageUp && keys[4].kind == K::PageDown && keys[5].kind == K::Help);
    assert(d.feed("\x1b[12;3R", 0.0).empty());   // not asked for: not a report
    d.expectCursorReport();
    keys = d.feed("\x1b[12;3R", 0.0);
    assert(keys.size() == 1 && keys[0].kind == K::CursorReport && keys[0].y == 12 && keys[0].x == 3);

    const auto v = vocabulary();
    LineEditor e = editor(v);
    e.color = false;
    type(e, "on tick then ");
    assert(!e.wantsMouse());
    press(e, K::Tab);   // everything that may come next, in sections
    assert(e.menuVisible() && e.wantsMouse());
    auto f = e.render(100);
    assert(f.text.find("Actions") != std::string::npos);

    // Wheel moves the selection; PgDn moves by a page.
    press(e, K::WheelDown);
    assert(e.selected() == 1);
    press(e, K::PageDown);
    assert(e.selected() > 1);
    press(e, K::PageUp);
    press(e, K::PageUp);
    assert(e.selected() == 0);

    // A click on a menu row takes that row: find the row "Add" is drawn on.
    f = e.render(100);
    int row = 0, addRow = -1;
    for (std::size_t at = 0, next; at <= f.text.size(); at = next + 2, ++row) {
        next = f.text.find("\r\n", at);
        const std::string r = f.text.substr(at, next == std::string::npos ? std::string::npos : next - at);
        if (r.find(" Add ") != std::string::npos) addRow = row;
        if (next == std::string::npos) break;
    }
    assert(addRow > 0);
    e.press(Key{K::Click, {}, 6, addRow});
    assert(e.buffer().rfind("on tick then Add \u2039path\u203A by \u2039number\u203A", 0) == 0);

    // A click on the line moves the cursor there.
    press(e, K::KillToStart);
    type(e, "on tick then set glow 1");
    e.press(Key{K::Click, {}, static_cast<int>(std::string("earthcall> on ").size()), 0});
    assert(e.cursor() == std::string("on ").size());

    // Matched letters are bold; the selected entry is explained.
    LineEditor colored = editor(v);
    type(colored, "on tick then se");
    const auto g = colored.render(100);
    assert(g.text.find("\x1b[7;1m") != std::string::npos);    // selected row, matched letters bold
    LineEditor explained = editor(v);
    type(explained, "on tick then set glow re");
    const auto h = explained.render(100);
    assert(h.text.find("\u2937") != std::string::npos && h.text.find("(1, 0, 0)") != std::string::npos);

    // The footer says where you are.
    e.footer = "Law Line · hears ✓ · scope @cube";
    assert(e.render(100).text.find("◆ Law Line · hears ✓") != std::string::npos);

    // Help: an overlay that scrolls and closes, never submitted.
    std::vector<std::string> lines;
    for (int i = 0; i < 40; ++i) lines.push_back("line " + std::to_string(i) + "|");
    e.showOverlay(lines);
    assert(e.overlayVisible() && e.wantsMouse());
    press(e, K::WheelDown);
    assert(e.overlayScroll() == 3);
    press(e, K::PageDown);
    assert(e.overlayScroll() == 3 + e.overlayRows);
    assert(e.render(100).text.find("line 3|") == std::string::npos);
    assert(e.render(100).text.find("line 21|") != std::string::npos);
    press(e, K::Escape);
    assert(!e.overlayVisible());
    assert(e.press(Key{K::Help, {}}) == LineEditor::Outcome::Help);
}

int main() {
    decoding();
    menu();
    noGluing();
    editing();
    rendering();
    rungThree();
    blocks();
    appearance();
    // The actual large authored editor must survive bracketed paste split
    // across TerminalChannel's 64 KiB reads, including the end marker split.
    // A paste newline is text spacing; only the later Enter submits it.
    {
        auto path=std::filesystem::path("examples/law_line_pixel_art_editor.txt");
        if(!std::filesystem::exists(path))path=std::filesystem::path("..")/path;
        std::ifstream in(path);std::string program;std::getline(in,program);
        assert(program.size()>65536);
        KeyDecoder decoder;LineEditor line;
        const std::string bytes="\x1b[200~"+program+"\n\x1b[20";
        for(std::size_t at=0;at<bytes.size();at+=65536)
            for(const auto& key:decoder.feed(bytes.substr(at,65536),double(at)/65536))
                assert(line.press(key)!=LineEditor::Outcome::Submitted);
        // Bracketed paste is explicitly delimited, not an Escape-key timeout.
        assert(decoder.flush(100).empty());
        for(const auto& key:decoder.feed("1~",101))
            assert(line.press(key)!=LineEditor::Outcome::Submitted);
        assert(line.buffer()==program+" ");
        assert(line.press(Key{Key::Kind::Enter,{}})==LineEditor::Outcome::Submitted);
        assert(line.takeSubmitted()==program+" ");
    }
    std::cout << "line_editor_test: OK\n";
    return 0;
}
