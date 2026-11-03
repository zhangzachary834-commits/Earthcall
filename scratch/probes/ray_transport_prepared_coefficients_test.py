"""Run real-source refusal, detached revision and changing-binding cost witnesses.
CPU only; no native frame/upload or device lifetime claim.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-22.
"""
import argparse,json,subprocess,sys,importlib.util
from pathlib import Path

def load(root,name):
    spec=importlib.util.spec_from_file_location(name,root/'scratch/probes'/f'{name}.py')
    m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m

def source(base,root):
    old=load(root,'ray_transport_gpu_pose_binding_test');original_load=old.load
    old.load=lambda r,name:load(r,'ray_transport_gpu_prepared_coefficients') if name=='ray_transport_gpu_pose_authored_coefficients' else original_load(r,name)
    s=old.source(base,root);q=load(root,'ray_transport_gpu_pose_qualification')
    s=q.once(s,'J checks=J::array();unsigned member=0;','J checks=J::array(),costs=J::array();unsigned member=0;')
    extra=r'''
        {
            auto recipe=deriveOriginalCoefficients(m);auto p=input(m),legacy=input(m);
            bool identical=true;double checksum=0;
            for(int i=0;i<257;++i){const double t=-10.0+i*.137;
                prepareOriginalCoefficients(p,recipe,t);legacyBindOriginalCoefficients(legacy,t);
                identical=identical&&p.alpha==legacy.alpha&&p.color==legacy.color;checksum+=p.color.x;
            }
            if(!identical)throw std::runtime_error("prepared vs old binding changed float coefficients");pass("257 changing bindings agree exactly with independent legacy binder");
            {auto e=*m.emission;auto edited=input(m);edited.emission=&e;
                auto v=std::make_unique<M>();v->op=M::Op::VectorConstruct;
                for(int k=0;k<3;++k)v->children.push_back(leaf(.2));e.children[1]=std::move(v);
                prepareOriginalCoefficients(p,recipe,0);auto oldColor=p.color;
                auto revised=deriveOriginalCoefficients(edited);prepareOriginalCoefficients(edited,revised,0);
                if(edited.color==oldColor)throw std::runtime_error("revision did not change coefficients");
                prepareOriginalCoefficients(p,recipe,0);if(p.color!=oldColor)throw std::runtime_error("source edit mutated owned snapshot");
                pass("explicit revision rebuild changes coefficient and leaves old snapshot historical");
            }
            refusal("nonfinite explicit clock",[&]{prepareOriginalCoefficients(p,recipe,INFINITY);});
            {auto x=*m.sigma;auto edited=input(m);edited.sigma=&x;x.children[0]=leaf(1e-300);
                refusal("backend underflow refuses",[&]{auto rr=deriveOriginalCoefficients(edited);prepareOriginalCoefficients(edited,rr,0);});}
            J rounds=J::array();
            for(int round=0;round<4;++round){double oldMs=0,newMs=0;
                auto run=[&](bool prepared){auto begin=std::chrono::steady_clock::now();
                    for(int i=0;i<64;++i){double t=(round*64+i)*.019;
                        if(prepared)prepareOriginalCoefficients(p,recipe,t);else legacyBindOriginalCoefficients(p,t);
                        checksum+=p.color.y;
                    }
                    auto elapsed=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();
                    if(prepared)newMs=elapsed/64;else oldMs=elapsed/64;
                };
                if(round%2==0){run(false);run(true);}else{run(true);run(false);}
                rounds.push_back({{"old_rederive_ms_per_member",oldMs},{"prepared_ms_per_member",newMs}});
            }
            if(!std::isfinite(checksum))throw std::runtime_error("benchmark output invalid");
            costs.push_back({{"member",member},{"rounds",rounds},{"checksum",checksum}});
        }
'''
    s=q.once(s,'        ++member;',extra+'\n        ++member;')
    s=s.replace('checks.size()==36','checks.size()==52')
    s=q.once(s,'{"checks",checks},{"pass",','{"checks",checks},{"cpu_preparation_cost",costs},{"native_frame_upload_measured",false},{"pass",')
    return s

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--root',type=Path,required=True);p.add_argument('--work',type=Path,required=True);a=p.parse_args()
    root=a.root.resolve();work=a.work.resolve();m=load(root,'ray_transport_gpu_pose_qualification');m.source=lambda base,r:source(base,r)
    old=sys.argv
    try:sys.argv=[old[0],'--root',str(root),'--work',str(work),'--build-only'];m.main()
    finally:sys.argv=old
    output=work/'binding-controls.json';subprocess.run([str(work/'probe'),str(output)],cwd=root,check=True)
    if json.loads(output.read_text())['pass'] is not True:raise ValueError('prepared binding controls failed')
if __name__=='__main__':main()
