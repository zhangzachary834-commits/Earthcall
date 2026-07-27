// Offscreen WebGPU smoke: device -> clear render pass -> read pixels back.
// Proves the core rendering path (the substance of drawMesh) works headless.
#include <webgpu/webgpu.h>
#include <webgpu/wgpu.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>

static WGPUStringView sv(const char* s){ return WGPUStringView{ s, s?strlen(s):0 }; }

struct AdapterR { WGPUAdapter a=nullptr; bool done=false; };
static void onAdapter(WGPURequestAdapterStatus s, WGPUAdapter a, WGPUStringView, void* u, void*){
    auto* r=(AdapterR*)u; if(s==WGPURequestAdapterStatus_Success) r->a=a; r->done=true; }
struct DeviceR { WGPUDevice d=nullptr; bool done=false; };
static void onDevice(WGPURequestDeviceStatus s, WGPUDevice d, WGPUStringView, void* u, void*){
    auto* r=(DeviceR*)u; if(s==WGPURequestDeviceStatus_Success) r->d=d; r->done=true; }
struct MapR { bool ok=false, done=false; };
static void onMap(WGPUMapAsyncStatus s, WGPUStringView, void* u, void*){
    auto* r=(MapR*)u; r->ok=(s==WGPUMapAsyncStatus_Success); r->done=true; }

int main(){
    WGPUInstance inst = wgpuCreateInstance(nullptr);
    if(!inst){ printf("FAIL: instance\n"); return 1; }

    AdapterR ar; WGPURequestAdapterCallbackInfo acb={};
    acb.mode=WGPUCallbackMode_AllowProcessEvents; acb.callback=onAdapter; acb.userdata1=&ar;
    wgpuInstanceRequestAdapter(inst, nullptr, acb);
    while(!ar.done) wgpuInstanceProcessEvents(inst);
    if(!ar.a){ printf("FAIL: adapter\n"); return 1; }

    DeviceR dr; WGPURequestDeviceCallbackInfo dcb={};
    dcb.mode=WGPUCallbackMode_AllowProcessEvents; dcb.callback=onDevice; dcb.userdata1=&dr;
    wgpuAdapterRequestDevice(ar.a, nullptr, dcb);
    while(!dr.done) wgpuInstanceProcessEvents(inst);
    if(!dr.d){ printf("FAIL: device\n"); return 1; }
    WGPUDevice dev=dr.d; WGPUQueue q=wgpuDeviceGetQueue(dev);

    WGPUTextureDescriptor td={}; td.label=sv("target");
    td.usage=WGPUTextureUsage_RenderAttachment|WGPUTextureUsage_CopySrc;
    td.dimension=WGPUTextureDimension_2D; td.size={4,4,1};
    td.format=WGPUTextureFormat_RGBA8Unorm; td.mipLevelCount=1; td.sampleCount=1;
    WGPUTexture tex=wgpuDeviceCreateTexture(dev,&td);
    WGPUTextureView view=wgpuTextureCreateView(tex,nullptr);

    WGPUBufferDescriptor bd={}; bd.label=sv("readback");
    bd.usage=WGPUBufferUsage_CopyDst|WGPUBufferUsage_MapRead; bd.size=256*4;
    WGPUBuffer buf=wgpuDeviceCreateBuffer(dev,&bd);

    WGPUCommandEncoder enc=wgpuDeviceCreateCommandEncoder(dev,nullptr);
    WGPURenderPassColorAttachment ca={};
    ca.view=view; ca.depthSlice=WGPU_DEPTH_SLICE_UNDEFINED;
    ca.loadOp=WGPULoadOp_Clear; ca.storeOp=WGPUStoreOp_Store;
    ca.clearValue={0.25,0.5,0.75,1.0};
    WGPURenderPassDescriptor rp={}; rp.colorAttachmentCount=1; rp.colorAttachments=&ca;
    WGPURenderPassEncoder pass=wgpuCommandEncoderBeginRenderPass(enc,&rp);
    wgpuRenderPassEncoderEnd(pass);
    wgpuRenderPassEncoderRelease(pass);

    WGPUTexelCopyTextureInfo src={}; src.texture=tex; src.mipLevel=0; src.origin={0,0,0}; src.aspect=WGPUTextureAspect_All;
    WGPUTexelCopyBufferInfo dst={}; dst.buffer=buf; dst.layout.offset=0; dst.layout.bytesPerRow=256; dst.layout.rowsPerImage=4;
    WGPUExtent3D cs={4,4,1};
    wgpuCommandEncoderCopyTextureToBuffer(enc,&src,&dst,&cs);
    WGPUCommandBuffer cmd=wgpuCommandEncoderFinish(enc,nullptr);
    wgpuQueueSubmit(q,1,&cmd);

    MapR mr; WGPUBufferMapCallbackInfo mcb={};
    mcb.mode=WGPUCallbackMode_AllowProcessEvents; mcb.callback=onMap; mcb.userdata1=&mr;
    wgpuBufferMapAsync(buf, WGPUMapMode_Read, 0, 256*4, mcb);
    while(!mr.done) wgpuDevicePoll(dev, true, nullptr);
    if(!mr.ok){ printf("FAIL: map\n"); return 1; }

    const unsigned char* px=(const unsigned char*)wgpuBufferGetConstMappedRange(buf,0,256*4);
    printf("first pixel RGBA = (%d, %d, %d, %d)\n", px[0],px[1],px[2],px[3]);
    bool ok = abs(px[0]-64)<3 && abs(px[1]-128)<3 && abs(px[2]-191)<3 && px[3]==255;
    printf(ok ? "OK: WebGPU cleared an offscreen frame and read it back\n"
              : "FAIL: wrong clear color\n");
    wgpuBufferUnmap(buf);
    return ok?0:1;
}
