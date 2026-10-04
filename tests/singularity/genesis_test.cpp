#include "catch.hpp"
#include "Singularity/Core/Engine.hpp"
#include "Singularity/Core/CodecChannel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ActionModel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ConditionModel.hpp"
#include "ZonesOfEarth/Zone.hpp"
#include "ConstructedBeing/Universe.hpp"
#include <nlohmann/json.hpp>

using namespace Core;

TEST_CASE("Genesis Bootstrap Flow", "[modality]") {
    Universe::instance().clear();
    auto& c = CodecChannel::instance();
    
    // We mock the seed JSON
    nlohmann::json seed = nlohmann::json::array();
    seed.push_back({
        {"type", "Singular"},
        {"id", "player-1"},
        {"name", "First Mover"},
        {"telos", "lexeme.human"}
    });
    seed.push_back({
        {"type", "Relation"},
        {"source", "world-genesis"},
        {"target", "player-1"},
        {"name", "owns"}
    });

    // Write to codec input to simulate FileRead
    c.propSetInput(seed.dump());
    
    // The load-genesis-file law does: jsonArray = ecgraphToJson (which is just input for now)
    // Wait, ecgraphToJson in CodecChannel.cpp just returns input if it's already JSON?
    // Let's just bypass load-genesis-file and set jsonArray directly.
    c.propSetJsonArray(seed.dump());
    
    // The world-genesis Zone
    auto zone = Universe::instance().zones().front();
    zone->setIdentifier("world-genesis");
    
    // Set phase 0
    PropertyValue phase(0.0);
    lawSetValue(*zone, PropertyPath::parse("state.genesis.phase"), phase);

    // Build the ActionNodes manually to simulate the laws firing
    ActionNode stepAction;
    stepAction.kind = ActionNode::Kind::Sequence;
    
    ActionNode a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12;
    a1.kind = ActionNode::Kind::Set; a1.path = PropertyPath::parse("@codec.codec.arrayShift"); a1.operand = PropertyValue(true);
    a2.kind = ActionNode::Kind::Set; a2.path = PropertyPath::parse("@codec.codec.queryKey"); a2.operand = PropertyValue("type");
    a3.kind = ActionNode::Kind::CodecTransform; a3.propertyName = "jsonExtract"; a3.input = PropertyPath::parse("@codec.codec.shiftItem"); a3.path = PropertyPath::parse("@state.genesis.type");
    a4.kind = ActionNode::Kind::Set; a4.path = PropertyPath::parse("@codec.codec.queryKey"); a4.operand = PropertyValue("id");
    a5.kind = ActionNode::Kind::CodecTransform; a5.propertyName = "jsonExtract"; a5.input = PropertyPath::parse("@codec.codec.shiftItem"); a5.path = PropertyPath::parse("@state.genesis.id");
    a6.kind = ActionNode::Kind::Set; a6.path = PropertyPath::parse("@codec.codec.queryKey"); a6.operand = PropertyValue("name");
    a7.kind = ActionNode::Kind::CodecTransform; a7.propertyName = "jsonExtract"; a7.input = PropertyPath::parse("@codec.codec.shiftItem"); a7.path = PropertyPath::parse("@state.genesis.name");
    a8.kind = ActionNode::Kind::Set; a8.path = PropertyPath::parse("@codec.codec.queryKey"); a8.operand = PropertyValue("source");
    a9.kind = ActionNode::Kind::CodecTransform; a9.propertyName = "jsonExtract"; a9.input = PropertyPath::parse("@codec.codec.shiftItem"); a9.path = PropertyPath::parse("@state.genesis.source");
    a10.kind = ActionNode::Kind::Set; a10.path = PropertyPath::parse("@codec.codec.queryKey"); a10.operand = PropertyValue("target");
    a11.kind = ActionNode::Kind::CodecTransform; a11.propertyName = "jsonExtract"; a11.input = PropertyPath::parse("@codec.codec.shiftItem"); a11.path = PropertyPath::parse("@state.genesis.target");
    a12.kind = ActionNode::Kind::Set; a12.path = PropertyPath::parse("@state.genesis.phase"); a12.operand = PropertyValue(1.0);
    
    stepAction.children = {a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12};
    auto stepExe = stepAction.compile();

    ActionNode singularAction;
    singularAction.kind = ActionNode::Kind::Sequence;
    ActionNode sa1, sa2;
    sa1.kind = ActionNode::Kind::Create; sa1.path = PropertyPath::parse("lexeme.christ"); sa1.id = "@@state.genesis.id"; sa1.name = "@@state.genesis.name";
    sa2.kind = ActionNode::Kind::Set; sa2.path = PropertyPath::parse("@state.genesis.phase"); sa2.operand = PropertyValue(0.0);
    singularAction.children = {sa1, sa2};
    auto singularExe = singularAction.compile();

    ActionNode relationAction;
    relationAction.kind = ActionNode::Kind::Sequence;
    ActionNode ra1, ra2;
    ra1.kind = ActionNode::Kind::AddRelation; ra1.propertyName = "@@state.genesis.name"; ra1.containerToken = "@@state.genesis.source"; ra1.elementToken = "@@state.genesis.target";
    ra2.kind = ActionNode::Kind::Set; ra2.path = PropertyPath::parse("@state.genesis.phase"); ra2.operand = PropertyValue(0.0);
    relationAction.children = {ra1, ra2};
    auto relationExe = relationAction.compile();

    ECA::Event ev;
    
    // Tick 1: Phase 0 pops Singular
    stepExe(ev, *zone);
    
    PropertyValue valType; lawGetValue(*zone, PropertyPath::parse("state.genesis.type"), valType);
    REQUIRE(std::get<std::string>(valType) == "Singular");
    
    // Tick 2: Phase 1 creates Singular
    singularExe(ev, *zone);

    auto player = resolveBeingToken("player-1", *zone);
    REQUIRE(player != nullptr);
    REQUIRE(player->getIdentifier() == "player-1");

    // Tick 3: Phase 0 pops Relation
    stepExe(ev, *zone);
    lawGetValue(*zone, PropertyPath::parse("state.genesis.type"), valType);
    REQUIRE(std::get<std::string>(valType) == "Relation");

    // Tick 4: Phase 1 adds Relation
    relationExe(ev, *zone);
    
    auto world = resolveBeingToken("world-genesis", *zone);
    bool hasRel = Universe::instance().areRelated(world, player, "owns");
    REQUIRE(hasRel == true);
}
