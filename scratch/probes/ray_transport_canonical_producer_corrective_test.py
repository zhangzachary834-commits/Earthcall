"""Execute the shared §6cf corrective C++ contract, including prior counterexamples.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-22.
CPU only. Conditional mathematical bounds are not a Metal execution certificate.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
from ray_transport_canonical_root_contract import CPP_ROOT_CONTRACT, CPP_FIXTURE

CPU_TEST = r'''
#include <fstream>
#include <iostream>
#include "json.hpp"
using J=nlohmann::json;
using namespace canonical_fixture;
int main(int argc,char**argv){
    if(argc!=3)return 2;
    J report;unsigned checks=0;
    auto require=[&](bool pass,const char* label){++checks;if(!pass)throw std::runtime_error(label);};
    auto covers=[](const Roots& r,double x){
        for(auto b:r.brackets)if(b.lo<=x&&x<=b.hi)return true;
        for(auto b:r.unresolved)if(b.lo<=x&&x<=b.hi)return true;
        return false;
    };
    auto dump=[](const Roots& r){
        J a=J::array(),b=J::array();for(auto v:r.brackets)a.push_back({v.lo,v.hi});
        for(auto v:r.unresolved)b.push_back({{"lo",v.lo},{"hi",v.hi},{"reason",v.reason}});
        return J{{"brackets",a},{"unresolved",b},{"nodes",r.nodes}};
    };
    Limits precise;precise.width=1e-9;
    auto cubic=[](double s){return (s-.02)*(s-.05)*(s-.08);};
    auto r=isolate(cubic,0,1,0,6,1e-15,precise);
    require(r.brackets.size()==3&&r.unresolved.empty(),"three roots must be isolated");
    for(double x:{.02,.05,.08})require(covers(r,x),"cubic root lost");
    report["three_root_cubic"]=dump(r);
    auto thin=[](double s){return (s-.0007)*(s-.0007)-1e-10;};
    r=isolate(thin,0,1,0,2,1e-18,precise);
    require(r.brackets.size()==2&&r.unresolved.empty(),"thin pair must resolve twice");
    for(double x:{.00069,.00071})require(covers(r,x),"thin root lost");
    report["thin_pair"]=dump(r);
    Limits capped=precise;capped.maxDepth=0;
    r=isolate(thin,0,1,0,2,1e-18,capped);
    require(!r.unresolved.empty(),"depth exhaustion must remain unresolved");
    require(covers(r,.00069)&&covers(r,.00071),"depth exhaustion dropped support");
    report["depth_exhaustion"]=dump(r);
    capped=precise;capped.maxNodes=0;
    r=isolate(cubic,0,1,0,6,1e-15,capped);
    for(double x:{.02,.05,.08})require(covers(r,x),"node budget dropped support");
    report["node_exhaustion"]=dump(r);
    capped=precise;capped.maxRoots=0;
    r=isolate(cubic,0,1,0,6,1e-15,capped);
    require(r.brackets.empty()&&!r.unresolved.empty(),"capacity must refuse");
    for(double x:{.02,.05,.08})require(covers(r,x),"capacity dropped support");
    report["capacity_exhaustion"]=dump(r);
    auto undefined=isolate([](double){return NAN;},0,1,0,1,0,precise);
    require(undefined.brackets.empty()&&!undefined.unresolved.empty(),"undefined must refuse");
    auto tangent=isolate([](double s){return (s-.3)*(s-.3);},0,1,0,2,1e-15,precise);
    require(covers(tangent,.3),"tangency omitted");
    report["tangency"]=dump(tangent);
    auto lobe=isolate([](double s){return .01-s*s;},-1,1,0,2,1e-15,precise);
    require(lobe.brackets.size()==2&&lobe.unresolved.empty(),"actual lobe root routine failed");
    const double left=(lobe.brackets[0].lo+lobe.brackets[0].hi)*.5;
    const double right=(lobe.brackets[1].lo+lobe.brackets[1].hi)*.5;
    double integral=0;
    for(auto p:std::array<Band,2>{{{left,0},{0,right}}}){
        double h=p.hi-p.lo;
        for(double q:{.5-std::sqrt(3.)/6,.5+std::sqrt(3.)/6}){
            double x=p.lo+q*h;integral+=h*.5*std::max(.01-x*x,0.);
        }
    }
    require(std::abs(integral-.004/3)<1e-12,"rooted lobe integral failed");
    report["actual_narrow_lobe"]={{"roots",dump(lobe)},{"GL2",integral},{"absolute_error",std::abs(integral-.004/3)}};
    double a=0,b=20;require(clipAxis(1.000001,-1e-7,1,a,b)&&a<15&&b>=15,"near-parallel entry lost");
    report["near_parallel_entry_window"]={a,b};
    a=0;b=20;require(!clipAxis(1.000001,0,1,a,b),"parallel outside must be empty");
    a=0;b=20;require(clipAxis(.5,0,1,a,b)&&a==0&&b==20,"parallel inside changed");
    a=0;b=20;require(clipAxis(1,-1e-7,1,a,b),"boundary entry lost");
    bool invalid=false;try{isolate(cubic,0,1,0,-1,0);}catch(const std::invalid_argument&){invalid=true;}
    require(invalid,"invalid curvature accepted");
    std::ifstream input(std::string(argv[1])+"/docs/audits/NORTHERN_VEIL_NATIVE_CF4_PREFLIGHT_2026-10-21_RAYS.json");J rays;input>>rays;
    std::ifstream exFile(std::string(argv[1])+"/docs/audits/NORTHERN_VEIL_GPU_ROOT_PROPOSALS_2026-10-21.json");J ex;exFile>>ex;
    std::array<double,3> ro;for(int k=0;k<3;++k)ro[k]=rays["origin"][k];
    J cases=J::array();unsigned strictMatches=0;
    for(const auto& e:ex["counterexamples_with_complete_member_union"]){
        const auto& row=rays["rays"][e["ray"].get<unsigned>()];
        const auto& m=members[0];
        auto phi=[&](double s){return ro[2]+s*row[4].get<double>()-m.origin[2]+wave(m,ro[0]+s*row[2].get<double>()-m.origin[0]);};
        double dx=row[2];auto roots=isolate(phi,row[5],row[6],0,up(m.M2*dx*dx),1e-10,precise);
        bool match=false;for(auto band:roots.brackets)if(band.lo<=e["hi"].get<double>()&&band.hi>=e["lo"].get<double>())match=true;
        require(match,"saved missed crossing has no strict bracket match");++strictMatches;
        cases.push_back({{"ray",e["ray"]},{"strict_window_match",match},{"unresolved",roots.unresolved.size()}});
    }
    unsigned fallback=0,unresolved=0,rootCount=0,content=0,supportChecks=0;
    for(const auto& row:rays["rays"]){
        std::array<double,3>d{row[2],row[3],row[4]};auto p=prepare(ro,d,row[5],row[6]);
        fallback+=p.fallback;unresolved+=p.unresolved;rootCount+=p.roots;content+=!p.pieces.empty();
        // An independent finite scan of the mathematical fixture, not a
        // same-schedule integration comparator or universal device theorem.
        for(const auto& m:members)for(unsigned sample=0;sample<128;++sample){
            double t=row[5].get<double>()+(row[6].get<double>()-row[5].get<double>())*(sample+.5)/128;
            std::array<double,3> point;bool inside=true;
            for(int k=0;k<3;++k){point[k]=ro[k]+t*d[k]-m.origin[k];inside=inside&&std::abs(point[k])<m.half[k];}
            if(!inside||std::abs(point[0])>=m.Wx||std::abs(point[1])>=m.Wy||std::abs(point[2]+wave(m,point[0]))>=m.wz)continue;
            ++supportChecks;bool retained=p.fallback;
            for(auto piece:p.pieces)retained=retained||(piece.medium==m.id&&piece.lo<=t&&t<=piece.hi);
            require(retained,"finite all-member positive-support sample omitted");
        }
    }
    // Deliberately exhaust every per-member query, and assert the caller requests
    // whole-ray fallback instead of constructing transport from unresolved points.
    Limits noWork;noWork.maxNodes=0;
    bool fallbackWitness=false;
    for(const auto& row:rays["rays"]){
        std::array<double,3>d{row[2],row[3],row[4]};auto p=prepare(ro,d,row[5],row[6],noWork);
        if(p.fallback){require(p.pieces.empty()&&p.unresolved>0,"fallback leaked partial pieces");fallbackWitness=true;break;}
    }
    require(fallbackWitness,"forced fallback witness did not exercise a member");
    report["saved_examples"]={{"strict_matches",strictMatches},{"cases",cases}};
    report["fixture_census"]={{"rays",rays["rays"].size()},{"root_brackets",rootCount},{"unresolved_leaves",unresolved},{"fallback_rays",fallback},{"rays_with_canonical_content",content}};
    report["independent_finite_positive_support_checks"]=supportChecks;
    report["checks"]=checks;report["pass"]=true;
    report["native_omission_admitted"]=false;
    report["scope"]="Executed shared scratch C++ contract with declared double-math assumptions, not a device proof or native speedup";
    std::ofstream output(argv[2]);output<<report.dump(2)<<"\n";
    std::cout<<"PASS "<<checks<<" checks; strict examples="<<strictMatches<<" fallback rays="<<fallback<<" unresolved="<<unresolved<<"\n";
}
'''


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root',type=Path,required=True)
    p.add_argument('--work',type=Path,required=True)
    a=p.parse_args();root=a.root.resolve();work=a.work.resolve()
    if work==root or root in work.parents:raise SystemExit('external work directory required')
    for rel,expected in {
        'saves/zones/Northern Veil/zone.ecform':'6f541ef6818206be7274fd84659f1c0f7870b450ff8e15035ec34051a2605496',
        'docs/audits/NORTHERN_VEIL_NATIVE_CF4_PREFLIGHT_2026-10-21_RAYS.json':'f2d51ac017f27a3b45d7032b9b50313fc24e8c6fa05fe0fe495c236805fa6be7',
        'docs/audits/NORTHERN_VEIL_PROJECTED_REFERENCE_2026-10-21_ROWS.json':'eb76bb86a69571b7bf2927df31d43920d8751d1b96b5b931cd94f55e4f1b76a8',
    }.items():
        if hashlib.sha256((root/rel).read_bytes()).hexdigest()!=expected:raise SystemExit('pinned fixture changed: '+rel)
    work.mkdir(parents=True,exist_ok=False)
    source=CPP_ROOT_CONTRACT+CPP_FIXTURE+CPU_TEST
    (work/'witness.cpp').write_text(source)
    subprocess.run(['c++','-std=c++20','-O2','-I'+str(root/'src'),str(work/'witness.cpp'),'-o',str(work/'witness')],check=True)
    subprocess.run([str(work/'witness'),str(root),str(work/'result.json')],check=True)
    d=json.loads((work/'result.json').read_text())
    d['source_sha256']=hashlib.sha256(source.encode()).hexdigest()
    (work/'result.json').write_text(json.dumps(d,indent=2)+'\n')
    print(work/'result.json')


if __name__=='__main__':main()
