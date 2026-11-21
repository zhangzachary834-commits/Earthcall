"""Bounded scratch WGSL producer with explicit whole-ray fallback.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-22.
The numerical margin is a diagnostic model, not certified Metal omission authority.
"""

WGSL = r'''
struct CurtainSpec {origin:vec4f,half:vec4f,limits:vec4f,curv:vec4f,amps:vec4f,freqs:vec4f,phases:vec4f};
struct Ray {ro_clipA:vec4f,rd_clipB:vec4f,counts:vec4u};
struct Subpanel {span:vec2f,info:vec2u};
struct Overlap {span:vec2f,info:vec2u};
struct Config {rayCount:u32,maxSubPerRay:u32,maxOverPerRay:u32,budget:u32};
@group(0) @binding(0) var<uniform> cfg:Config;
@group(0) @binding(1) var<storage,read> curtains:array<CurtainSpec,4>;
@group(0) @binding(2) var<storage,read_write> rays:array<Ray>;
@group(0) @binding(3) var<storage,read_write> subpanels:array<Subpanel>;
@group(0) @binding(4) var<storage,read_write> overlaps:array<Overlap>;
const FALLBACK:u32=0x80000000u;
fn evalWave(m:CurtainSpec,x:f32)->f32 {
 return m.amps.x*sin(m.freqs.x*x+m.phases.x)+m.amps.y*cos(m.freqs.y*x+m.phases.y)+m.amps.z*sin(m.freqs.z*x+m.phases.z);
}
fn evalPhi(m:CurtainSpec,o:vec3f,d:vec3f,s:f32)->f32{return o.z+s*d.z+evalWave(m,o.x+s*d.x);}
fn putBand(lo:f32,hi:f32,bounds:ptr<function,array<f32,96>>,nb:ptr<function,u32>,
 bands:ptr<function,array<vec2f,32>>,nr:ptr<function,u32>,reason:ptr<function,u32>){
 if(*nb+2u>92u || *nr>=32u){*reason|=2u;return;}
 (*bounds)[*nb]=lo;(*bounds)[*nb+1u]=hi;*nb+=2u;
 (*bands)[*nr]=vec2f(lo,hi);*nr+=1u;
}
// The curvature/endpoint model is explicit. A bin must be monotone before
// bisection can resolve it. Exhaustion never becomes an isolated midpoint.
fn roots(m:CurtainSpec,o:vec3f,d:vec3f,lo:f32,hi:f32,c:f32,
 bounds:ptr<function,array<f32,96>>,nb:ptr<function,u32>,bands:ptr<function,array<vec2f,32>>,
 nr:ptr<function,u32>,reason:ptr<function,u32>,work:ptr<function,u32>){
 let M=m.curv.x*d.x*d.x*1.00002;
 let err=m.curv.y;
 var stack:array<vec3f,20>;var top=0u;
 for(var bin=0u;bin<8u;bin++){
  var a=lo+(hi-lo)*f32(bin)/8.0;var b=lo+(hi-lo)*f32(bin+1u)/8.0;
  if(bin==0u){a=lo;}if(bin==7u){b=hi;}
  stack[0]=vec3f(a,b,0);top=1u;
  loop {
   if(top==0u || *reason!=0u){break;}
   top--;let cell=stack[top];a=cell.x;b=cell.y;let h=b-a;
   if(*work>=cfg.budget){*reason|=1u;break;}*work+=1u;
   if(!(h>0.0)){*reason|=4u;break;}
   let va=evalPhi(m,o,d,a)-c;let vb=evalPhi(m,o,d,b)-c;
   let scale=1.0+abs(va)+abs(vb)+abs(a)+abs(b);
   let arithmetic=0.000001*scale;
   let error=err+arithmetic;
   let bow=0.125*M*h*h+error;
   if(min(va,vb)>bow || max(va,vb)<-bow){continue;}
   let slope=(vb-va)/h;let radius=0.5*M*h+2.0*error/h;
   let minSlope=abs(slope)-radius;
   if(minSlope>0.0){
    // Endpoint uncertainty is retained as a full possible-root band.
    if(abs(va)<=error){let w=2.0*error/minSlope;putBand(a,min(b,a+w),bounds,nb,bands,nr,reason);}
    if(abs(vb)<=error){let w=2.0*error/minSlope;putBand(max(a,b-w),b,bounds,nb,bands,nr,reason);}
    if((va>error&&vb < -error)||(va < -error&&vb>error)){
     var l=a;var r=b;var left=va;
     for(var iter=0u;iter<32u;iter++){
      if(*work>=cfg.budget){*reason|=1u;break;}*work+=1u;
      let mid=l+0.5*(r-l);
      if(mid<=l || mid>=r || r-l<=0.0001){break;}
      let v=evalPhi(m,o,d,mid)-c;
      if(abs(v)<=error){let w=2.0*error/minSlope;l=max(l,mid-w);r=min(r,mid+w);break;}
      if((left<0.0&&v>0.0)||(left>0.0&&v<0.0)){r=mid;}else{l=mid;left=v;}
     }
     putBand(l,r,bounds,nb,bands,nr,reason);
    }
    continue;
   }
   if(cell.z>=14.0 || top+2u>20u){*reason|=4u;break;}
   let mid=a+0.5*h;if(mid<=a||mid>=b){*reason|=4u;break;}
   stack[top]=vec3f(mid,b,cell.z+1.0);stack[top+1u]=vec3f(a,mid,cell.z+1.0);top+=2u;
  }
  if(*reason!=0u){break;}
 }
}
@compute @workgroup_size(64)
fn producerMain(@builtin(global_invocation_id) gid:vec3u){
 let id=gid.x;if(id>=cfg.rayCount){return;}
 let input=rays[id];let ro=input.ro_clipA.xyz;let d=input.rd_clipB.xyz;
 let subBase=id*cfg.maxSubPerRay;let overBase=id*cfg.maxOverPerRay;
 var reason=0u;var work=0u;var count=0u;
 var pieces:array<vec4f,48>;
 if(cfg.budget==0u){rays[id].counts=vec4u(subBase,0,overBase,FALLBACK|1u);return;}
 for(var member=0u;member<4u;member++){
  let m=curtains[member];let o=ro-m.origin.xyz;
  var a=input.ro_clipA.w;var b=input.rd_clipB.w;var hit=true;
  for(var axis=0u;axis<3u;axis++){
   var half=m.half[axis];
   if(axis==0u){half=min(half,m.limits.x);}if(axis==1u){half=min(half,m.limits.y);}
   // Only exact zero is a constant coordinate; tiny nonzero rays may enter.
   if(d[axis]==0.0){if(abs(o[axis])>half+m.curv.y){hit=false;break;}}
   else {
    let q0=(-half-m.curv.y-o[axis])/d[axis];let q1=(half+m.curv.y-o[axis])/d[axis];
    a=max(a,min(q0,q1));b=min(b,max(q0,q1));
   }
  }
  if(!hit || !(b>a)){continue;}
  var bounds:array<f32,96>;var nb=1u;bounds[0]=a;
  var bands:array<vec2f,32>;var nr=0u;
  roots(m,o,d,a,b,-m.limits.w,&bounds,&nb,&bands,&nr,&reason,&work);
  if(reason!=0u){break;}
  roots(m,o,d,a,b,0.0,&bounds,&nb,&bands,&nr,&reason,&work);
  if(reason!=0u){break;}
  roots(m,o,d,a,b,m.limits.w,&bounds,&nb,&bands,&nr,&reason,&work);
  if(reason!=0u){break;}
  if(d.x!=0.0){let x=-o.x/d.x;if(x>a&&x<b){bounds[nb]=x;nb++;}}
  if(d.y!=0.0){
   let y=-o.y/d.y;if(y>a&&y<b){bounds[nb]=y;nb++;}
   let k=(m.limits.z-o.y)/d.y;if(k>a&&k<b){bounds[nb]=k;nb++;}
  }
  bounds[nb]=b;nb++;
  for(var i=1u;i<nb;i++){
   let v=bounds[i];var j=i;loop{if(j==0u||bounds[j-1u]<=v){break;}bounds[j]=bounds[j-1u];j--;}
   bounds[j]=v;
  }
  for(var i=1u;i<nb;i++){
   let l=bounds[i-1u];let r=bounds[i];if(!(r>l)){continue;}
   let mid=l+0.5*(r-l);var uncertain=false;
   for(var k=0u;k<nr;k++){uncertain=uncertain||(l<bands[k].y&&r>bands[k].x);}
   if(uncertain||abs(evalPhi(m,o,d,mid))<=m.limits.w+m.curv.y){
    if(count>=48u){reason|=8u;break;}
    pieces[count]=vec4f(l,r,f32(member),0);count++;
   }
  }
  if(reason!=0u){break;}
 }
 var events:array<f32,96>;var ne=0u;
 for(var k=0u;k<count;k++){events[ne]=pieces[k].x;events[ne+1u]=pieces[k].y;ne+=2u;}
 for(var i=1u;i<ne;i++){
  let v=events[i];var j=i;loop{if(j==0u||events[j-1u]<=v){break;}events[j]=events[j-1u];j--;}
  events[j]=v;
 }
 var singles=0u;var overs=0u;
 for(var i=1u;i<ne&&reason==0u;i++){
  let a=events[i-1u];let b=events[i];if(!(b>a)){continue;}
  let mid=a+0.5*(b-a);var mask=0u;
  for(var k=0u;k<count;k++){if(mid>=pieces[k].x&&mid<=pieces[k].y){mask|=1u<<u32(pieces[k].z);}}
  if(mask==0u){continue;}
  if(countOneBits(mask)==1u){
   let n=max(1u,u32(ceil((b-a)/4.0)));
   if(singles+n>cfg.maxSubPerRay){reason|=16u;break;}
   let member=firstTrailingBit(mask);
   for(var k=0u;k<n;k++){
    let l=select(a+f32(k)*(b-a)/f32(n),a,k==0u);let r=select(a+f32(k+1u)*(b-a)/f32(n),b,k+1u==n);
    subpanels[subBase+singles]=Subpanel(vec2f(l,r),vec2u(member,0));singles++;
   }
  }else{
   if(overs>=cfg.maxOverPerRay){reason|=32u;break;}
   overlaps[overBase+overs]=Overlap(vec2f(a,b),vec2u(mask,0));overs++;
  }
 }
 if(reason!=0u){rays[id].counts=vec4u(subBase,0,overBase,FALLBACK|reason);}
 else {rays[id].counts=vec4u(subBase,singles,overBase,overs);}
}
'''
