// The Terminal channel's line editor, held to what Zach asked for
// (2026-09-25): Tab SELECTS, arrows MOVE between choices, and the line shows
// things temporarily instead of printing them as messages. Pure state and a
// rendered frame — no terminal needed.

#include "Singularity/Terminal/LawSentence.hpp"
#include "Singularity/Terminal/LineEditor.hpp"

#include <cassert>
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
    v.words.push_back({"red", "value", "lex_red", "law_red", "means: red"});
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

    // Tab takes the selection, and a space so the sentence flows on.
    press(e, Key::Kind::Tab);
    assert(e.buffer() == "on object-clicked then Set " || e.buffer() == "on object-clicked then set ");

    // Tab on an empty word opens the menu of what may come next; arrows move.
    type(e, "l");   // fuzzy: both "color" and "glow" contain it
    assert(e.menuVisible() && e.suggestions().size() >= 2);
    const std::string first = e.suggestions()[e.selected()].text;
    press(e, Key::Kind::Down);
    assert(e.selected() == 1);
    press(e, Key::Kind::Up);
    press(e, Key::Kind::Up);   // wraps
    assert(e.selected() == static_cast<int>(e.suggestions().size()) - 1);
    press(e, Key::Kind::Down);
    assert(e.suggestions()[e.selected()].text == first);
    press(e, Key::Kind::Enter);   // after navigating, Enter takes the choice, not the line
    assert(e.buffer().find(first + " ") != std::string::npos);

    // Value words: "red" is a Lexeme denoting a value.
    type(e, "re");
    assert(e.menuVisible());
    press(e, Key::Kind::Tab);
    assert(e.buffer().substr(e.buffer().size() - 4) == "red ");

    // Enter authors (submits) when the menu was not navigated.
    assert(e.press(Key{Key::Kind::Enter, {}}) == LineEditor::Outcome::Submitted);
    assert(e.takeSubmitted().find("then Set color red") != std::string::npos ||
           !e.history().empty());
    assert(e.buffer().empty());

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
    assert(f.text.find("tab accept") != std::string::npos);  // the footer tells you how
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

} // namespace

int main() {
    decoding();
    menu();
    noGluing();
    editing();
    rendering();
    std::cout << "line_editor_test: OK\n";
    return 0;
}
