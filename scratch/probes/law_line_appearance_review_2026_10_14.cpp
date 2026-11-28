// Review-only counterexamples for 07435d0ce; no app data or engine startup.
// Codex / GPT-6.1 Sol / 01a10992-828e-7e80-890c-c64b09141e18 / 2026-10-14 PDT.
#include "Singularity/Terminal/LineEditor.hpp"
#include "Singularity/Storage/Serialization/Relation/FormationSerialization.hpp"
#include "Singularity/Storage/Serialization/ZonesOfEarth/ZoneSerialization.hpp"
#include "Singularity/Language/LanguageSystem.hpp"
#include "ConstructedBeing/Singular/Lexeme/Lexeme.hpp"
#include "ConstructedBeing/Singular/Property/PropertyValueJson.hpp"
#include "json.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/SaveContext.hpp"
#include "Person/Person.hpp"
#include "Person/Soul/Soul.hpp"
#include "Singularity/Input/Mouse/MouseHandler.hpp"
#include "Singularity/Screen/Camera.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include <filesystem>
#include <chrono>
#include <algorithm>
#include <iostream>
#include <string>
using namespace Singularity::Terminal;

LineEditor candidate(const std::string& word) {
    LineEditor editor;
    editor.prompt = "> "; editor.color = false; editor.hints = false;
    editor.setProviders([word](const std::string&) {
        return std::vector<LawSentence::Suggestion>{{0, word, "", "value", 1, "", "", ""}};
    }, {}, {});
    editor.prefill("x");
    return editor;
}
int main() {
    nlohmann::json out;
    std::string hearts;
    for (int i=0;i<40;++i) hearts += "\u2764\uFE0F";
    auto editor = candidate(hearts);
    auto frame = editor.render(28);
    size_t at=0, maximum=0;
    while (at < frame.text.size()) {
        auto end=frame.text.find("\r\n",at);
        if (at>0) maximum=std::max(maximum,visibleWidth(frame.text.substr(at,end==std::string::npos ? end : end-at)));
        if (end==std::string::npos) break;
        at=end+2;
    }
    out["variation_selector_menu"]={{"terminal_width",28},{"panel_budget",27},{"maximum_panel_cells",maximum}};
    auto control = candidate("red");
    control.look=[](const std::string& key) {
        return key=="chrome.selected" ? LineEditor::Look{"\nextra-row",{},{},{}} : LineEditor::Look{};
    };
    frame=control.render(80);
    out["control_glyph"]={{"reported_rows",frame.rows},{"actual_line_feeds",std::count(frame.text.begin(),frame.text.end(),'\n')},
                           {"literal_extra_row_emitted",frame.text.find("\nextra-row")!=std::string::npos}};
    Zone zone("review-live-zone","review-live-zone");
    auto word=Singularity::Language::LanguageSystem::instance().intern("ReviewWord","lexeme.review.live");
    zone.addToFormation(word.get());
    word->setDynamicProperty("appearance.glyph",std::string("saved-glyph"));
    const auto old=zoneToJson(zone);
    word->setDynamicProperty("appearance.glyph",std::string("unsaved-live-glyph"));
    applyZoneJson(zone,old,false);
    PropertyValue actual;
    word->getDynamicProperty("appearance.glyph",actual);
    out["preserve_live_zone_merge"]={{"expected","unsaved-live-glyph"},{"actual",std::get<std::string>(actual)}};
    const auto sandbox=std::filesystem::temp_directory_path()/
        ("earthcall-appearance-review-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(sandbox);
    SaveSystem::setSaveRoot(sandbox.string());
    {
        Soul soul("ReviewPerson"); Body body("humanoid","default");
        Person person(std::move(soul),std::move(body),"default");
        Core::Camera camera; MouseHandler mouse; LawManager laws;
        float color[3]={1,1,1}; double worldTime=1;
        SaveContext ctx;ctx.person=&person;ctx.camera=&camera;ctx.mouseHandler=&mouse;
        ctx.lawManager=&laws;ctx.currentColor=color;ctx.worldTime=&worldTime;
        ZoneManager manager;
        auto live=std::make_shared<Zone>("review-manager-zone","review-manager-zone");
        live->addToFormation(word.get());manager.addZone(live);
        word->setDynamicProperty("appearance.glyph",std::string("snapshot-glyph"));
        auto session=(sandbox/"old-session.json").string();manager.saveState(session,ctx);
        word->setDynamicProperty("appearance.glyph",std::string("current-unsaved-glyph"));
        manager.loadState(session,ctx);
        word->getDynamicProperty("appearance.glyph",actual);
        out["actual_session_load"]={{"expected","current-unsaved-glyph"},{"actual",std::get<std::string>(actual)},
            {"reported_unsaved_preserved",manager.getSaveLoadState().lastLoadReport.find("Unsaved work preserved")!=std::string::npos}};
    }
    SaveSystem::setSaveRoot(""); std::filesystem::remove_all(sandbox);
    auto error = candidate("red");error.color=true;
    error.setProviders({}, {}, [](const std::string&) { return std::vector<LineEditor::Status>{{"refused-example","error",0}}; });
    error.look=[](const std::string& key) {
        return key=="chrome.error" ? LineEditor::Look{"SUCCESS", "32",{}, {}} : LineEditor::Look{};
    };
    const auto errorFrame=error.render(80).text;
    out["plan_error_guard"]={{"green_authored_error_mark_emitted",errorFrame.find("\x1b[32m  SUCCESS ")!=std::string::npos},
                              {"built_in_cross_present",errorFrame.find("\u2717")!=std::string::npos}};
    std::cout << out.dump(2) << '\n';
}
