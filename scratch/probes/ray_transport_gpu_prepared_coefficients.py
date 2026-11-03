"""Consume OntoMath common-factor snapshots; separate admission from preparation.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-22.
Pinned density/root decoder remains a diagnostic. No viewport/frame admission.
"""
import argparse,importlib.util,hashlib
from pathlib import Path

def load(root,name):
    spec=importlib.util.spec_from_file_location(name,root/'scratch/probes'/f'{name}.py')
    m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m

def source(base,root):
    a=load(root,'ray_transport_gpu_pose_authored_coefficients');q=load(root,'ray_transport_gpu_pose_qualification');s=a.source(base,root)
    header_hash=hashlib.sha256((root/'src/Singularity/OntoMath/CommonScalarFactor.hpp').read_bytes()).hexdigest()
    s='// CommonScalarFactor.hpp sha256: '+header_hash+'\n'+s
    s='#include "Singularity/OntoMath/CommonScalarFactor.hpp"\n'+s
    s=q.once(s,'static void bindOriginalCoefficients(Medium& m,double clock){','static void legacyBindOriginalCoefficients(Medium& m,double clock){')
    helper=r'''
static OntoMath::CommonScalarFactor deriveOriginalCoefficients(const Medium& m){
    if(!m.density||!m.sigma||!m.emission)throw std::runtime_error("missing original channels");
    // Screen owns this diagnostic consumer's positive-literal policy, not the
    // generic factor office. No computed divisor/exponent admission is added.
    if(m.density->op!=M::Op::Scale||m.sigma->op!=M::Op::Scale||m.emission->op!=M::Op::Scale||
       m.density->children.size()!=2||m.sigma->children.size()!=2||m.emission->children.size()!=2||
       !m.density->children[0]||!m.sigma->children[0])throw std::runtime_error("transport source shape refused");
    literalCoefficient(*m.density->children[0]);literalCoefficient(*m.sigma->children[0]);
    OntoMath::TypeEnv env={{"p",OntoMath::ValueKind::Vector},{"x",OntoMath::ValueKind::Scalar},
        {"y",OntoMath::ValueKind::Scalar},{"z",OntoMath::ValueKind::Scalar},{OntoMath::kTimeVar,OntoMath::ValueKind::Scalar}};
    std::string why;auto recipe=OntoMath::factorCommonScalar({m.density,m.sigma,m.emission},env,{OntoMath::kTimeVar},&why);
    if(!recipe||recipe->factorChild!=std::vector<unsigned>({1,1,0}))
        throw std::runtime_error("transport factor recipe refused: "+why);
    if(recipe->coefficients[2].typeOf(env).kind!=OntoMath::ValueKind::Vector)
        throw std::runtime_error("transport coefficient must be vector");
    return std::move(*recipe);
}
static void prepareOriginalCoefficients(Medium& m,const OntoMath::CommonScalarFactor& recipe,double clock){
    if(!std::isfinite(clock))throw std::runtime_error("nonfinite explicit clock");
    std::string why;const auto values=OntoMath::evaluateFactorCoefficients(recipe,{{OntoMath::kTimeVar,PropertyValue(clock)}},&why);
    if(!values||values->size()!=3||!std::holds_alternative<glm::vec3>((*values)[2]))
        throw std::runtime_error("coefficient preparation refused: "+why);
    const auto cD=literalCoefficient(recipe.coefficients[0]);const auto cX=literalCoefficient(recipe.coefficients[1]);
    const auto v=std::get<glm::vec3>((*values)[2]);
    const float alpha=float(cX/cD),sigma=float(cX);const auto color=v/sigma;
    if(!(alpha>0)||!std::isfinite(alpha)||!(sigma>0)||!std::isfinite(sigma))throw std::runtime_error("backend scalar conversion refused");
    for(int k=0;k<3;++k)if(!std::isfinite(color[k])||color[k]<0)throw std::runtime_error("backend color conversion refused");
    m.alpha=alpha;m.color=color;
}
// Compatibility entry for the existing refusal witness: fresh admission each
// call. Actual main below keeps owned recipes for separate clock preparation.
static void bindOriginalCoefficients(Medium& m,double clock){
    auto recipe=deriveOriginalCoefficients(m);prepareOriginalCoefficients(m,recipe,clock);
}
'''
    s=q.once(s,'int main(int argc, char** argv) {',helper+'\nint main(int argc, char** argv) {')
    s=q.once(s,'    double coefficientBindingMs=0;','    double coefficientBindingMs=0,coefficientPreparationMs=0;\n    std::vector<OntoMath::CommonScalarFactor> coefficientRecipes;')
    s=q.once(s,'        {auto begin=std::chrono::steady_clock::now();bindOriginalCoefficients(m,clock);coefficientBindingMs+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();}',
        '        {auto begin=std::chrono::steady_clock::now();coefficientRecipes.push_back(deriveOriginalCoefficients(m));coefficientBindingMs+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();begin=std::chrono::steady_clock::now();prepareOriginalCoefficients(m,coefficientRecipes.back(),clock);coefficientPreparationMs+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();}')
    meta=r'''
    report["authored_coefficient_binding"]["derivation_office"]="OntoMath::factorCommonScalar";
    report["authored_coefficient_binding"]["source_snapshots_owned"]=true;
    report["authored_coefficient_binding"]["clock_preparation_ms"]=coefficientPreparationMs;
    report["authored_coefficient_binding"]["production_revision_device_lifetime_qualified"]=false;
    report["authored_coefficient_binding"]["cold_structural_binding_ms"]=coefficientBindingMs;
    report["authored_coefficient_binding"]["full_frame_preparation_upload_in_gpu_timing"]=false;
'''
    meta+='\n    report["authored_coefficient_binding"]["recipe_header_sha256"]="'+header_hash+'";\n'
    return q.once(s,'    std::cout << report.dump(2) << std::endl;',meta+'\n    std::cout << report.dump(2) << std::endl;')

def main():
    p=argparse.ArgumentParser(add_help=False);p.add_argument('--root',type=Path,required=True);a,_=p.parse_known_args()
    root=a.root.resolve();m=load(root,'ray_transport_gpu_pose_qualification');m.source=lambda base,r:source(base,r);m.main()
if __name__=='__main__':main()
