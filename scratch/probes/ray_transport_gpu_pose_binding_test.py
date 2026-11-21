"""Execute the actual authored-coefficient admission helper without a GPU.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-22.
The native camera/clock reference pass is the independent second path.
"""
import argparse,importlib.util,json,subprocess,sys
from pathlib import Path

def load(root,name):
    p=root/'scratch/probes'/f'{name}.py';spec=importlib.util.spec_from_file_location(name,p)
    m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m

def source(base,root):
    a=load(root,'ray_transport_gpu_pose_authored_coefficients');s=a.source(base,root)
    if s.count('int main(int argc, char** argv) {')!=1:raise ValueError('test entry hook')
    s=s.replace('int main(int argc, char** argv) {','int unusedGpuMain(int argc, char** argv) {')
    return s+r'''
int main(int argc,char**argv){
    std::ifstream in("saves/zones/Northern Veil/zone.ecform",std::ios::binary);
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)),{});
    auto zone=J::parse(J::from_msgpack(bytes).at("MigrationRoot").get<std::string>());
    J checks=J::array();unsigned member=0;
    auto pass=[&](const char* name){checks.push_back({{"member",member},{"case",name},{"pass",true}});};
    auto refusal=[&](const char* name,auto fn){bool refused=false;try{fn();}catch(const std::runtime_error&){refused=true;}
        if(!refused)throw std::runtime_error(std::string("accepted invalid binding: ")+name);pass(name);};
    auto leaf=[](double c){auto n=std::make_unique<OntoMath::MathNode>();n->op=M::Op::ScalarLeaf;n->scalarForm=OntoMath::ScalarForm::constant(c);return n;};
    auto input=[](const Medium& m){Medium p;p.density=m.density;p.sigma=m.sigma;p.emission=m.emission;return p;};
    for(const auto& j:zone.at("spatialFields")){
        auto field=geom::FieldNode::fromJson(j);Rendering::VolumeDensityBinding b;
        if(!field||!Rendering::readVolumeDensity(*field,0,0,b))throw std::runtime_error("fixture binding");
        Medium m;m.owner=field;m.density=single(b.densityExpr);m.sigma=single(b.extinctionExpr);m.emission=single(b.emissionExpr);
        bindOriginalCoefficients(m,0);const auto c0=m.color;pass("original factor/time binding");
        bindOriginalCoefficients(m,7.25);if(glm::length(m.color-c0)<.001f)throw std::runtime_error("clock-specific coefficients frozen");pass("authored clock changes coefficient");
        {auto x=*m.sigma;auto p=input(m);p.sigma=&x;x.children[1]->children.push_back(leaf(0));refusal("unshared original factor",[&]{bindOriginalCoefficients(p,0);});}
        {auto x=*m.sigma;auto p=input(m);p.sigma=&x;x.children[0]=leaf(0);refusal("zero extinction factor",[&]{bindOriginalCoefficients(p,0);});}
        {auto d=*m.density;auto p=input(m);p.density=&d;d.children[0]=leaf(-1);refusal("negative density factor",[&]{bindOriginalCoefficients(p,0);});}
        {auto x=*m.sigma;auto p=input(m);p.sigma=&x;auto n=std::make_unique<M>();n->op=M::Op::Add;n->children.push_back(leaf(.05));n->children.push_back(leaf(.05));x.children[0]=std::move(n);refusal("computed coefficient is not a literal",[&]{bindOriginalCoefficients(p,0);});}
        {auto e=*m.emission;auto p=input(m);p.emission=&e;e.children[1]=leaf(.1);refusal("scalar emission coefficient",[&]{bindOriginalCoefficients(p,0);});}
        {auto e=*m.emission;auto p=input(m);p.emission=&e;auto v=std::make_unique<M>();v->op=M::Op::VectorConstruct;for(int k=0;k<3;++k)v->children.push_back(leaf(.1));v->children[0]->scalarForm.terms[0].factors["x"]=1;e.children[1]=std::move(v);refusal("spatial emission coefficient",[&]{bindOriginalCoefficients(p,0);});}
        {auto x=*m.sigma;auto p=input(m);p.sigma=&x;x.children[0]=leaf(INFINITY);refusal("nonfinite coefficient",[&]{bindOriginalCoefficients(p,0);});}
        ++member;
    }
    J result={{"scope","Actual C++ structural binding helper CPU/refusal witnesses; no GPU or native omission admission"},{"members",member},{"checks",checks},{"pass",member==4&&checks.size()==36}};
    if(argc<2)throw std::runtime_error("output required");std::ofstream out(argv[1]);out<<result.dump(2)<<"\n";
    std::cout<<"PASS "<<checks.size()<<" actual binding/refusal witnesses\n";return result["pass"].get<bool>()?0:3;
}
'''

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--root',type=Path,required=True);p.add_argument('--work',type=Path,required=True)
    a=p.parse_args();root=a.root.resolve();work=a.work.resolve();m=load(root,'ray_transport_gpu_pose_qualification')
    m.source=lambda base,r:source(base,r);old=sys.argv
    try:sys.argv=[old[0],'--root',str(root),'--work',str(work),'--build-only'];m.main()
    finally:sys.argv=old
    output=work/'binding-controls.json';subprocess.run([str(work/'probe'),str(output)],cwd=root,check=True)
    if json.loads(output.read_text())['pass'] is not True:raise ValueError('binding controls failed')

if __name__=='__main__':main()
