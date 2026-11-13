// Read-only host range census. No renderer/device construction, proof upload,
// live save loading or save writes. Uses the legacy builder solely to choose
// the current grid and compares explicit host analysis at its enlarged leaves.
// Codex / GPT-6.1 Sol / session 01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca / 2026-10-15.
#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"
#include "Singularity/Screen/VolumeZeroProof.hpp"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <random>

using RV=OntoMath::MathNode::RangeValue;
using I=OntoMath::Interval;
int main(int argc,char** argv) {
    if(argc!=2) {std::fprintf(stderr,"usage: volume_euclidean_range_probe <frozen zone fixture>\n");return 2;}
    std::ifstream in(argv[1],std::ios::binary);
    if(!in)return 2;
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)),{});
    if(bytes.empty())return 2;
    const auto data=bytes.front()=='{' ? nlohmann::json::parse(bytes) :
        nlohmann::json::parse(nlohmann::json::from_msgpack(bytes).at("MigrationRoot").get<std::string>());
    nlohmann::json report={{"scope","host analysis only; no native skip or performance claim"},{"media",nlohmann::json::array()}};
    const bool payloadOnly=std::getenv("EC_RANGE_PAYLOAD_ONLY")!=nullptr;
    report["payload_only"]=payloadOnly;
    uint32_t failures=0;
    std::mt19937 rng(20261015u);std::uniform_real_distribution<float> unit(0,1);
    for(const auto& saved:data.at("spatialFields")) {
        auto field=geom::FieldNode::fromJson(saved);
        if(!field || !field->volumeDensity || field->volumeDensity->pieces.empty())continue;
        const auto half=glm::abs(field->scale);
        const auto proof=Rendering::buildVolumeZeroProof(*field->volumeDensity,half);
        if(!proof.totalCells)continue;
        const auto cell=2.0f*half/glm::vec3(proof.dims);
        uint32_t analysisEmpty=0,legacyLeafEmpty=0,checked=0,positive=0;
        uint64_t payload=14695981039346656037ull;
        for(float word:proof.words) {
            const uint32_t v=uint32_t(word);
            for(int i=0;i<4;++i){payload^=(v>>(8*i))&255u;payload*=1099511628211ull;}
        }
        if(!payloadOnly) for(uint32_t z=0;z<proof.dims.z;++z)for(uint32_t y=0;y<proof.dims.y;++y)for(uint32_t x=0;x<proof.dims.x;++x) {
            const auto lo=-half+glm::vec3(x,y,z)*cell-cell*.01f;
            const auto hi=-half+glm::vec3(x+1,y+1,z+1)*cell+cell*.01f;
            const std::map<std::string,RV> vars{{"p",RV::makeVector(I(lo.x,hi.x),I(lo.y,hi.y),I(lo.z,hi.z))},
                {"x",RV::makeScalar(I(lo.x,hi.x))},{"y",RV::makeScalar(I(lo.y,hi.y))},{"z",RV::makeScalar(I(lo.z,hi.z))}};
            bool empty=true;
            for(const auto& piece:field->volumeDensity->pieces) {
                if(piece.guard || piece.whereLEZero || piece.call || piece.fold){empty=false;break;}
                if(!piece.mathNode)continue;
                const auto range=piece.mathNode->evalMathematicalRange(vars);
                if(!range || range->kind!=OntoMath::ValueKind::Scalar || !(range->scalar.hi<=0)){empty=false;break;}
                if(!piece.hasLo && !piece.hasHi)break;
            }
            if(Rendering::densityProvenNonPositive(*field->volumeDensity,lo,hi))++legacyLeafEmpty;
            if(!empty)continue;
            ++analysisEmpty;
            // Numerical corroboration, never the authority for the enclosure.
            if((analysisEmpty%17u)==1u)for(int sample=0;sample<4;++sample) {
                const auto p=lo+(hi-lo)*glm::vec3(unit(rng),unit(rng),unit(rng));
                auto v=field->volumeDensity->evaluate({{"p",p},{"x",double(p.x)},{"y",double(p.y)},{"z",double(p.z)},{"t",7.25}});
                double density=0;
                if(!v || !propertyValueToNumber(*v,density) || !(density<=0))++positive;
                ++checked;
            }
        }
        nlohmann::json bands=nlohmann::json::array();
        for(const auto& b:proof.supportBands)bands.push_back({b.x,b.y,b.z,b.w});
        report["media"].push_back({{"id",field->getIdentifier()},{"dims",{proof.dims.x,proof.dims.y,proof.dims.z}},
            {"total_cells",proof.totalCells},{"legacy_builder_empty",proof.provenCells},{"legacy_leaf_empty",legacyLeafEmpty},
            {"legacy_payload_fnv1a64",std::to_string(payload)},
            {"legacy_words",proof.words},{"legacy_support_min",{proof.supportMin.x,proof.supportMin.y,proof.supportMin.z}},
            {"legacy_support_max",{proof.supportMax.x,proof.supportMax.y,proof.supportMax.z}},
            {"legacy_support_axis",proof.supportAxis},{"legacy_support_bands",bands},{"analysis_empty",analysisEmpty},
            {"corroborating_samples",checked},{"nonzero_or_unknown_samples",positive}});
        if(positive)++failures;
    }
    std::puts(report.dump(2).c_str());return failures || report["media"].empty()?1:0;
}
