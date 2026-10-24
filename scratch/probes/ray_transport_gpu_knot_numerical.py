"""Numerical compact-event candidate, distinct from refused exact compression.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-22.
Zach permits current-or-better authored-reference accuracy. Keep exact fallback,
unchanged original-AST1e-5 and two-resolution six-metric gates. Record all f32
changes to the prior experimental candidate; do not claim bit-lossless lowering.
"""
import argparse,importlib.util
from pathlib import Path

def load(root,name):
 spec=importlib.util.spec_from_file_location(name,root/'scratch/probes'/f'{name}.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m

def source(base,root):
 q=load(root,'ray_transport_gpu_pose_qualification');s=load(root,'ray_transport_gpu_knot_fma').source(base,root)
 s=q.once(s,'unsigned fallbacks=0,capacityFallbacks=0,expanded=0,hostEndpointDifferences=0;', 'unsigned fallbacks=0,capacityFallbacks=0,expanded=0,hostEndpointDifferences=0,changedOutputs=0,changedEndpoints=0;double maxRGBChange=0,maxTChange=0,maxEndpointChange=0,maxAST_RGB=0,maxAST_T=0;')
 before='''for(int k=0;k<4;++k)if(!std::isfinite(result[i].rgb_T[k])||!same(result[i].rgb_T[k],expected[i].rgb_T[k]))refuse("knot output changed or missed",{{"ray",i},{"channel",k},{"actual",result[i].rgb_T[k]},{"expected",expected[i].rgb_T[k]},{"flags",flag}});'''
 after=r'''
                bool outputChanged=false;
                for(int k=0;k<4;++k){
                    if(!std::isfinite(result[i].rgb_T[k])||(refused&&!same(result[i].rgb_T[k],expected[i].rgb_T[k])))refuse("numerical knot nonfinite or inexact native fallback",{{"ray",i},{"channel",k},{"flags",flag}});
                    outputChanged|=!same(result[i].rgb_T[k],expected[i].rgb_T[k]);
                    const double delta=std::abs(double(result[i].rgb_T[k])-expected[i].rgb_T[k]);
                    if(k<3)maxRGBChange=std::max(maxRGBChange,delta);else maxTChange=std::max(maxTChange,delta);
                }
                changedOutputs+=outputChanged;
'''
 s=q.once(s,before,after)
 before='''if(!same(native.s0,want.s0)||!same(native.s1,want.s1)||native.mediumIndex!=want.mediumIndex||member!=want.mediumIndex)refuse("GPU knot changed panel endpoint bits",{{"ray",i},{"panel",single-1},{"packet_span",{a,b}},{"span_index",j},{"panel_in_span",k},{"panel_count",panels},{"GPU_decompressed",{native.s0,native.s1}},{"original_GPU",{want.s0,want.s1}}});'''
 after=r'''
                            if(native.mediumIndex!=want.mediumIndex||member!=want.mediumIndex||!std::isfinite(native.s0)||!std::isfinite(native.s1)||!(native.s1>native.s0))throw std::runtime_error("numerical knot panel structure");
                            changedEndpoints+=!same(native.s0,want.s0)||!same(native.s1,want.s1);
                            maxEndpointChange=std::max({maxEndpointChange,std::abs(double(native.s0)-want.s0),std::abs(double(native.s1)-want.s1)});
'''
 s=q.once(s,before,after)
 before='''if(!same(a,want.s0)||!same(b,want.s1)||mask!=want.activeMask||!same(native.s0,want.s0)||!same(native.s1,want.s1)||native.activeMask!=want.activeMask)throw std::runtime_error("GPU knot changed overlap");'''
 after=r'''
                        if(mask!=want.activeMask||native.activeMask!=want.activeMask||!same(a,native.s0)||!same(b,native.s1)||!(native.s1>native.s0))throw std::runtime_error("numerical knot overlap structure");
                        changedEndpoints+=!same(native.s0,want.s0)||!same(native.s1,want.s1);
                        maxEndpointChange=std::max({maxEndpointChange,std::abs(double(native.s0)-want.s0),std::abs(double(native.s1)-want.s1)});
'''
 s=q.once(s,before,after)
 ast=r'''
                if(!refused){
                    // Original AST at this candidate's actual same-device panels.
                    const glm::vec3 ro(gpuRays[i].ro[0],gpuRays[i].ro[1],gpuRays[i].ro[2]),rd(gpuRays[i].rd[0],gpuRays[i].rd[1],gpuRays[i].rd[2]);
                    glm::vec4 state(0,0,0,1);unsigned si=0,oi=0;float last=-INFINITY;
                    while(si<single||oi<overlap){
                        const bool over=oi<overlap&&(si>=single||decompressedOverlaps[i*maxOverPerRay+oi].s0<decompressedSingles[i*maxSubPerRay+si].s0);
                        if(over){
                            const auto& o=decompressedOverlaps[i*maxOverPerRay+oi++];if(o.s0<last)throw std::runtime_error("numerical knot overlap order");last=o.s1;
                            const float h=(o.s1-o.s0)*.5f,aa=.25f-std::sqrt(3.f)/6.f,bb=.5f-aa;
                            for(int k=0;k<2;++k){const float lo=o.s0+k*h;
                                const auto u=astAt(ro,rd,lo+(.5f-std::sqrt(3.f)/6.f)*h,o.activeMask),v=astAt(ro,rd,lo+(.5f+std::sqrt(3.f)/6.f)*h,o.activeMask);
                                state=cpuFlow(state,bb*u+aa*v,h);state=cpuFlow(state,aa*u+bb*v,h);
                            }
                        }else{
                            const auto& p=decompressedSingles[i*maxSubPerRay+si++];if(p.s0<last)throw std::runtime_error("numerical knot panel order");last=p.s1;const float h=p.s1-p.s0;
                            const auto u=astAt(ro,rd,p.s0+(.5f-std::sqrt(3.f)/6.f)*h,1u<<p.mediumIndex),v=astAt(ro,rd,p.s0+(.5f+std::sqrt(3.f)/6.f)*h,1u<<p.mediumIndex);
                            state=cpuFlow(state,.5f*(u+v),h);
                        }
                    }
                    maxAST_RGB=std::max({maxAST_RGB,std::abs(double(state.x)-result[i].rgb_T[0]),std::abs(double(state.y)-result[i].rgb_T[1]),std::abs(double(state.z)-result[i].rgb_T[2])});maxAST_T=std::max(maxAST_T,std::abs(double(state.w)-result[i].rgb_T[3]));
                    if(maxAST_RGB>=1e-5||maxAST_T>=1e-5)throw std::runtime_error("numerical knot original AST agreement refused");
                }
'''
 s=q.once(s,'rows.push_back({R[i][0],R[i][1],result[i].rgb_T[0],result[i].rgb_T[1],result[i].rgb_T[2],result[i].rgb_T[3]});',ast+'\n                rows.push_back({R[i][0],R[i][1],result[i].rgb_T[0],result[i].rgb_T[1],result[i].rgb_T[2],result[i].rgb_T[3]});')
 s=q.once(s,'{"host_endpoint_differences",hostEndpointDifferences}', '{"host_endpoint_differences",hostEndpointDifferences},{"changed_output_rays",changedOutputs},{"changed_endpoint_packets",changedEndpoints},{"max_RGB_change_vs_bulk",maxRGBChange},{"max_T_change_vs_bulk",maxTChange},{"max_endpoint_change_vs_bulk",maxEndpointChange},{"original_AST_max_RGB",maxAST_RGB},{"original_AST_max_T",maxAST_T}')
 s=q.once(s,'{"same_device_decompression_bit_checked",true}', '{"same_device_decompression_bit_checked",false},{"numerical_candidate",true},{"numeric_checks",normal}')
 # Avoid a duplicate nested copy of the entire output/packet arrays.
 s=q.once(s,'{"numeric_checks",normal}', '{"numeric_checks",J{{"changed_output_rays",normal["changed_output_rays"]},{"changed_endpoint_packets",normal["changed_endpoint_packets"]},{"max_RGB_change_vs_bulk",normal["max_RGB_change_vs_bulk"]},{"max_T_change_vs_bulk",normal["max_T_change_vs_bulk"]},{"max_endpoint_change_vs_bulk",normal["max_endpoint_change_vs_bulk"]},{"original_AST_max_RGB",normal["original_AST_max_RGB"]},{"original_AST_max_T",normal["original_AST_max_T"]}}}')
 s=q.once(s,'{"eligible_bit_identical_to_bulk_candidate",true}', '{"eligible_bit_identical_to_bulk_candidate",false}')
 s=q.once(s,'{"full_precision_intervals_and_original_panel_nodes",true}', '{"full_precision_intervals_and_original_panel_nodes",false},{"original_panel_rule_and_active_member_structure",true}')
 s=q.once(s,'J rows=J::array(),packetRows=J::array(),expandedRows=J::array(),hostExamples=J::array();','J rows=J::array(),astRows=J::array(),packetRows=J::array(),expandedRows=J::array(),hostExamples=J::array();')
 s=q.once(s,'if(!refused){\n                    // Original AST', 'glm::vec4 astState(resCurrent[i].rgb_T[0],resCurrent[i].rgb_T[1],resCurrent[i].rgb_T[2],resCurrent[i].rgb_T[3]);\n                if(!refused){\n                    // Original AST')
 s=q.once(s,'if(maxAST_RGB>=1e-5||maxAST_T>=1e-5)', 'astState=state;\n                    if(maxAST_RGB>=1e-5||maxAST_T>=1e-5)')
 s=q.once(s,'rows.push_back({R[i][0],R[i][1],result[i].rgb_T[0],result[i].rgb_T[1],result[i].rgb_T[2],result[i].rgb_T[3]});', 'astRows.push_back({R[i][0],R[i][1],astState.x,astState.y,astState.z,astState.w});\n                rows.push_back({R[i][0],R[i][1],result[i].rgb_T[0],result[i].rgb_T[1],result[i].rgb_T[2],result[i].rgb_T[3]});')
 s=q.once(s,'{"rows",rows},{"packets",packetRows}', '{"rows",rows},{"original_AST_rows",astRows},{"packets",packetRows}')
 s=q.once(s,'{"normal_packets",normal["packets"]}', '{"normal_packets",normal["packets"]},{"original_AST_rows",normal["original_AST_rows"]}')
 s=q.once(s,'Explicit fma(k,(b-a)/n,a); full same-device endpoint/output bits remain mandatory', 'Explicit FMA; f32 differences recorded; independent authored-reference gates, no bit-lossless claim')
 s=q.once(s,'    std::cout << report.dump(2) << std::endl;', '    report["connected_measured_scope"]["scope"]="Same "+std::to_string(count)+" distinct native rays; original bulk control; no full viewport or frame qualification";\n    std::cout << report.dump(2) << std::endl;')
 return s

def main():
 p=argparse.ArgumentParser(add_help=False);p.add_argument('--root',type=Path,required=True);a,_=p.parse_known_args();root=a.root.resolve();q=load(root,'ray_transport_gpu_pose_qualification');wide=load(root,'ray_transport_gpu_wide_stream');q.source=lambda base,r:source(base,r);q.verify_case=lambda r,p,z:wide.verify_case(r,p,z);q.independent_check=wide.independent_check;q.main()
 import sys
 if '--build-only' not in sys.argv:load(root,'ray_transport_knot_evidence').finish(root)
if __name__=='__main__':main()
