"""Bind admitted common-factor transport coefficients from original MathNodes.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-22.
Scratch structural admission only: D=cD*B, X=cX*B, E=B*V(t), exact shared
subtree identity and time-only V. Numeric backend reassociation stays unproved;
original-AST/native reference gates remain mandatory. No new source admission.
"""
import argparse,importlib.util
from pathlib import Path

def load(root):
    p=root/'scratch/probes/ray_transport_gpu_pose_qualification.py'
    spec=importlib.util.spec_from_file_location('authored_coefficients_base',p)
    m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m

def source(base,root):
    m=load(root);s=m.source(base,root)
    helper=r'''
// Scratch admission for an original shared mathematical factor. No spatial
// samples are used to infer proportionality. Future reusable derivation belongs
// to OntoMath; these pinned backend diagnostics grant no omission authority.
static double literalCoefficient(const OntoMath::MathNode& n){
    if(n.op!=OntoMath::MathNode::Op::ScalarLeaf||n.unsupported||!n.children.empty()||n.scalarForm.terms.size()!=1)
        throw std::runtime_error("coefficient is not a literal scalar");
    const auto& term=n.scalarForm.terms[0];
    if(!term.factors.empty()||!term.trans.empty()||!std::isfinite(term.coefficient)||!(term.coefficient>0))
        throw std::runtime_error("coefficient requires a positive finite literal");
    return term.coefficient;
}
static void bindOriginalCoefficients(Medium& m,double clock){
    for(auto n:{m.density,m.sigma,m.emission})
        if(!n||n->op!=OntoMath::MathNode::Op::Scale||n->unsupported||n->children.size()!=2||!n->children[0]||!n->children[1])
            throw std::runtime_error("transport common-factor shape refused");
    const double cD=literalCoefficient(*m.density->children[0]);
    const double cX=literalCoefficient(*m.sigma->children[0]);
    const auto B=m.density->children[1]->toJson();
    if(B!=m.sigma->children[1]->toJson()||B!=m.emission->children[0]->toJson())
        throw std::runtime_error("original common-factor subtrees differ");
    const auto& V=*m.emission->children[1];std::set<std::string> deps;V.collectDependencies(deps);
    for(const auto& dep:deps)if(dep!=OntoMath::kTimeVar)
        throw std::runtime_error("emission coefficient is not time-only");
    const auto value=V.evaluate({{OntoMath::kTimeVar,PropertyValue(clock)}});
    if(!value||!std::holds_alternative<glm::vec3>(*value))throw std::runtime_error("time-only vector evaluation refused");
    const auto v=std::get<glm::vec3>(*value);
    for(int k=0;k<3;++k)if(!std::isfinite(v[k])||v[k]<0)throw std::runtime_error("emission coefficient invalid");
    m.alpha=float(cX/cD);m.color=v/float(cX);
}
'''
    s=m.once(s,'int main(int argc, char** argv) {',helper+'\nint main(int argc, char** argv) {')
    s=m.once(s,'    std::vector<float> globalParamPool;','    double coefficientBindingMs=0;\n    std::vector<float> globalParamPool;')
    s=m.once(s,'        m.alpha = alphas[idx];\n        m.color = colors[idx];',
        '        {auto begin=std::chrono::steady_clock::now();bindOriginalCoefficients(m,clock);coefficientBindingMs+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();}')
    meta=r'''
    J coefficients=J::array();
    for(const auto& m:media)coefficients.push_back({{"member",m.id},{"extinction_over_density",m.alpha},
        {"emission_over_extinction",{m.color.x,m.color.y,m.color.z}}});
    report["authored_coefficient_binding"]={{"literal_clock_specific_color_tables_used",false},
        {"shared_original_subtree_identity_checked",true},{"emission_coefficient_time_only_checked",true},
        {"clock",clock},{"members",coefficients},{"general_native_execution_certificate",false},{"cold_structural_binding_ms",coefficientBindingMs},{"binding_in_gpu_timing",false},{"dynamic_frame_lifetime_qualified",false}};
'''
    return m.once(s,'    std::cout << report.dump(2) << std::endl;',meta+'\n    std::cout << report.dump(2) << std::endl;')

def main():
    p=argparse.ArgumentParser(add_help=False);p.add_argument('--root',type=Path,required=True)
    a,_=p.parse_known_args();root=a.root.resolve();m=load(root);m.source=lambda base,r:source(base,r);m.main()

if __name__=='__main__':main()
