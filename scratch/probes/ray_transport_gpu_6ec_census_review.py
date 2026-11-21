"""§6ec event admission: explicit route/value provenance and bounded trace replay.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-27.
Counts are observations of this diagnostic, not runtime-cost or irreducibility proofs.
"""
import argparse,array,collections,hashlib,itertools,json,math,struct,sys
from pathlib import Path
SITES=['COARSE_KNOT','INITIAL_MIDPOINT','ADAPTIVE_MIDPOINT','MONOTONE_ISOLATOR','POST_ROOT']
FAMILIES={1:'S1',2:'S2',3:'General'}

def require(ok,message):
    if not ok:raise ValueError(message)

def pct(x,y):return 100*x/y if y else 0

def analyze_events(path,expected_rays=32784,legacy=False):
    data=Path(path).read_bytes();require(len(data)>=16,'short event header')
    count,overflow,capacity,reserved=struct.unpack_from('<4I',data)
    require(reserved==0 and overflow==0 and count<=capacity,'event overflow/header invalid')
    require(len(data)==16+32*count,'event storage length mismatch')
    words=array.array('I');words.frombytes(data[16:])
    if sys.byteorder!='little':words.byteswap()
    # Bounded arrays/indexes rather than millions of Python event dictionaries.
    order=sorted(range(count),key=lambda i:(words[8*i]<<32)|words[8*i+3])
    totals=collections.Counter();sites=collections.Counter();levels=collections.Counter();families=collections.Counter()
    duplicates=collections.Counter();site_dups=collections.Counter();family_dups=collections.Counter();transitions=collections.Counter()
    distance=collections.Counter();lru_hits=collections.Counter();values=collections.Counter()
    require(type(expected_rays) is int and expected_rays>0,'invalid fixture ray count')
    ray_counts=[0]*expected_rays;ray_dups=[0]*expected_rays;best_trace=[];best_ray=None
    demand_hash=hashlib.sha256();run_lengths=[];route_rays=collections.Counter();heuristic_mismatches=0
    for ray,group in itertools.groupby(order,key=lambda i:words[8*i]):
        require(ray<expected_rays,'event ray outside fixture')
        seen={};recent=[];trace=[];route=None;max_medium=0;run_length=0;run_context=None
        for seq,index in enumerate(group):
            i=8*index;r,med,sl,recorded_seq,sbits,vbits,detail,tag=words[i:i+8]
            site=sl>>4;level=sl&15
            require(recorded_seq==seq,'event sequence gap or duplicate')
            require(med<4 and site<len(SITES),'event medium/site outside schema')
            require((site in [0,4] and level==3) or (site in [1,2,3] and level<3),'site/level identity invalid')
            require((sbits&0x7f800000)!=0x7f800000,'non-finite ray parameter')
            if legacy:
                require(tag==0,'unexpected legacy event tag');family='Unrecorded';quantity=3
            else:
                family_code=tag>>8;quantity=tag&255
                require(family_code in FAMILIES and quantity in [0,1,2],'missing/invalid actual route or value kind')
                family=FAMILIES[family_code]
                require(route is None or route==family,'multiple producer families for one ray')
                route=family
                require(quantity!=1 or site==3,'residual value outside isolator')
                require(quantity!=0 or site in [0,1,2],'raw Phi provenance not established at this site')
            require(quantity==2 and vbits==0 or quantity!=2 and (vbits&0x7f800000)!=0x7f800000,'invalid observed value bits')
            demand_hash.update(struct.pack('<7I',r,med,sl,recorded_seq,sbits,detail,0 if legacy else family_code))
            max_medium=max(max_medium,med)
            if site==3:
                if run_length and (detail==0 or run_context!=(med,level)):
                    run_lengths.append(run_length);run_length=0
                require(detail==run_length,'isolator iteration sequence changed')
                run_context=(med,level);run_length+=1
            elif run_length:run_lengths.append(run_length);run_length=0;run_context=None
            sites[SITES[site]]+=1;levels[str(level)]+=1;families[family]+=1
            key=(med,sbits);value_context=(quantity,level if quantity==1 else -1)
            is_dup=key in seen
            if is_dup:
                first=seen[key];duplicates['positions']+=1;site_dups[SITES[site]]+=1;family_dups[family]+=1;ray_dups[ray]+=1
                transitions[SITES[first[1]]+' -> '+SITES[site]]+=1
                delta=seq-first[0];distance[str(next((n for n in [4,8,16,32] if delta<=n),33))]+=1
            else:seen[key]=(seq,site,{})
            if quantity!=2:
                prior=seen[key][2]
                if value_context in prior:
                    values['comparable_repeat_values']+=1
                    values['comparable_repeat_value_bit_mismatches']+=vbits!=prior[value_context]
                else:prior[value_context]=vbits
            values[{0:'raw_phi_observations',1:'residual_observations',2:'argument_only_observations',3:'legacy_untrusted_value_observations'}[quantity]]+=1
            if key in recent:
                stack_distance=recent.index(key)+1
                for cap in [1,4,8,16,32]:lru_hits[str(cap)]+=stack_distance<=cap
                recent.remove(key)
            recent.insert(0,key);del recent[32:]
            trace.append([seq,med,SITES[site],level,detail,sbits,vbits,quantity,is_dup])
        if run_length:run_lengths.append(run_length)
        if not legacy:
            route_rays[route]+=1
            heuristic=('S1' if max_medium==0 else 'S2' if max_medium==1 else 'General')
            heuristic_mismatches+=heuristic!=route
        ray_counts[ray]=len(trace)
        if len(trace)>len(best_trace) or len(trace)==len(best_trace) and (best_ray is None or ray<best_ray):best_ray=ray;best_trace=trace
    require(sum(ray_counts)==count,'ray call totals differ')
    def distribution(values):
        ordered=sorted(values);n=len(ordered)
        return dict(min=ordered[0],median=(ordered[(n-1)//2]+ordered[n//2])/2,mean=sum(ordered)/n,p90=ordered[round(.9*(n-1))],p99=ordered[round(.99*(n-1))],max=ordered[-1])
    return {'scope':'observed diagnostic call positions; no irreducibility or GPU-time inference','legacy_trace_unqualified':legacy,'event_sha256':hashlib.sha256(data).hexdigest(),'expected_rays':expected_rays,'active_event_rays':sum(v>0 for v in ray_counts),'total_evaluations':count,'position_repeats':duplicates['positions'],'position_repeat_percentage':pct(duplicates['positions'],count),'value_observations':dict(values),'site_counts':dict(sites),'level_counts':dict(levels),'actual_route_counts':dict(families),'site_position_repeats':dict(site_dups),'route_position_repeats':dict(family_dups),'first_occurrence_call_gap_histogram':dict(distance),'position_only_LRU_oracle_hits':dict(lru_hits),'LRU_scope':'observed identical per-ray/medium/parameter positions; value/context substitution and lookup economics remain unproved','site_transitions':dict(transitions),'per_ray_calls':distribution(ray_counts),'per_ray_position_repeats':distribution(ray_dups),'representative_ray':{'index':best_ray,'trace_columns':['sequence','medium','site','level','detail','s_bits','value_bits','quantity_0_phi_1_residual_2_none_3_legacy','position_repeat'],'trace':best_trace},'named_consumer_evalPhi_calls':0,'consumer_scope':'no calls to this producer helper; consumer field/Perlin/transport work is outside this event schema','route_counts_unit':'events, not rays','actual_route_ray_counts':dict(route_rays),'max_medium_heuristic_misclassified_event_rays':heuristic_mismatches if not legacy else None,'canonical_demand_sha256':demand_hash.hexdigest(),'observed_isolator_runs':len(run_lengths),'isolator_run_length_histogram':dict(sorted(collections.Counter(run_lengths).items())),'isolator_run_scope':'only runs with at least one recorded Phi evaluation; zero-evaluation decisions and non-Phi work are outside this histogram'}

def manifest_ray_count(manifest):
    meta=json.loads(Path(manifest).read_text());p=Path(meta['rays']);data=p.read_bytes()
    require(hashlib.sha256(data).hexdigest()==meta['rays_sha256'],'fixture ray bytes changed')
    return len(json.loads(data)['rays'])

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--events',type=Path,required=True);p.add_argument('--manifest',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--legacy',action='store_true');a=p.parse_args()
    d=analyze_events(a.events,manifest_ray_count(a.manifest),a.legacy);a.output.write_text(json.dumps(d,indent=2)+'\n');print(json.dumps({k:d[k] for k in ['total_evaluations','position_repeats','position_repeat_percentage','value_observations','actual_route_counts']}))
if __name__=='__main__':main()
