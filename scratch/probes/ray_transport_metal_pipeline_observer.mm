// Process-local diagnostic only; forwards original calls/options without changes.
// Kernel state below is native observation data, never an authored setting.
// Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-25.
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <objc/runtime.h>
#include <mutex>
#include <set>
#include <cstdio>
#include <cstdlib>
#import <CommonCrypto/CommonDigest.h>
#include <string>

static std::mutex lock;
static std::set<std::string> seen;
static thread_local std::string contextEntry, contextHash;
static id<MTLDevice> captureDevice=nil;
extern "C" void rtMetalSetContext(const char* entry,const char* code,size_t n){
 contextEntry=entry;unsigned char hash[CC_SHA256_DIGEST_LENGTH];CC_SHA256(code,(CC_LONG)n,hash);char hex[65];for(int i=0;i<32;++i)sprintf(hex+i*2,"%02x",hash[i]);hex[64]=0;contextHash=hex;
}
static void emit(NSDictionary* row) {
    const char* path=getenv("EC_RAY_PIPELINE_FACTS");
    if(!path)return;
    NSData* data=[NSJSONSerialization dataWithJSONObject:row options:0 error:nil];
    if(!data)return;
    std::lock_guard<std::mutex> guard(lock);
    FILE* f=fopen(path,"a");if(!f)abort();
    fwrite(data.bytes,1,data.length,f);fputc('\n',f);fclose(f);
}
static void observe(id<MTLComputePipelineState> p, id<MTLFunction> f, NSString* method) {
    if(!p)return;
    {std::lock_guard<std::mutex> guard(lock);if(!seen.insert(std::to_string((uintptr_t)(__bridge void*)p)+contextHash).second)return;}
    emit(@{@"kind":@"pipeline",@"function":f.name?:@"unknown",@"label":p.label?:@"",
           @"factory":method,@"wgslEntry":@(contextEntry.c_str()),@"wgslSHA256":@(contextHash.c_str()),@"threadExecutionWidth":@(p.threadExecutionWidth),
           @"maxTotalThreadsPerThreadgroup":@(p.maxTotalThreadsPerThreadgroup),
           @"staticThreadgroupMemoryLength":@(p.staticThreadgroupMemoryLength)});
}
using Simple=id(*)(id,SEL,id,NSError**);
using FunctionOptions=id(*)(id,SEL,id,MTLPipelineOption,MTLComputePipelineReflection**,NSError**);
using Descriptor=id(*)(id,SEL,MTLComputePipelineDescriptor*,MTLPipelineOption,MTLComputePipelineReflection**,NSError**);
static Simple originalSimple=nullptr;
static FunctionOptions originalOptions=nullptr;
static Descriptor originalDescriptor=nullptr;
static id simple(id self,SEL sel,id<MTLFunction> f,NSError** e){id p=originalSimple(self,sel,f,e);observe(p,f,@"function");return p;}
static id options(id self,SEL sel,id<MTLFunction> f,MTLPipelineOption o,MTLComputePipelineReflection** r,NSError** e){id p=originalOptions(self,sel,f,o,r,e);observe(p,f,@"function-options");return p;}
static id descriptor(id self,SEL sel,MTLComputePipelineDescriptor* d,MTLPipelineOption o,MTLComputePipelineReflection** r,NSError** e){captureDevice=self;id p=originalDescriptor(self,sel,d,o,r,e);observe(p,d.computeFunction,@"descriptor-options");return p;}
static IMP replace(Class cls,SEL sel,IMP imp){
    Method inherited=class_getInstanceMethod(cls,sel);if(!inherited)return nullptr;
    IMP old=method_getImplementation(inherited);
    if(!class_addMethod(cls,sel,imp,method_getTypeEncoding(inherited)))method_setImplementation(class_getInstanceMethod(cls,sel),imp);
    return old;
}
extern "C" void rtInstallMetalObserver(){@autoreleasepool{
    id<MTLDevice> device=MTLCreateSystemDefaultDevice();
    if(!device){emit(@{@"kind":@"unavailable",@"reason":@"MTLCreateSystemDefaultDevice returned nil"});return;}
    NSMutableArray* counters=[NSMutableArray array];
    for(id<MTLCounterSet> set in device.counterSets){NSMutableArray* names=[NSMutableArray array];for(id<MTLCounter> c in set.counters)[names addObject:c.name];[counters addObject:@{@"set":set.name,@"counters":names}];}
    emit(@{@"kind":@"device",@"name":device.name,@"registryID":@(device.registryID),@"runtimeClass":NSStringFromClass([device class]),
           @"maxThreadsPerThreadgroup":@[@(device.maxThreadsPerThreadgroup.width),@(device.maxThreadsPerThreadgroup.height),@(device.maxThreadsPerThreadgroup.depth)],
           @"maxThreadgroupMemoryLength":@(device.maxThreadgroupMemoryLength),@"counterSets":counters,
           @"gpuTraceDocumentSupported":@([[MTLCaptureManager sharedCaptureManager] supportsDestination:MTLCaptureDestinationGPUTraceDocument]),
           @"timestampSamplingRequested":@NO,@"counterSamplingRequested":@NO});
    Class cls=[device class];
    originalSimple=(Simple)replace(cls,@selector(newComputePipelineStateWithFunction:error:),(IMP)simple);
    originalOptions=(FunctionOptions)replace(cls,@selector(newComputePipelineStateWithFunction:options:reflection:error:),(IMP)options);
    originalDescriptor=(Descriptor)replace(cls,@selector(newComputePipelineStateWithDescriptor:options:reflection:error:),(IMP)descriptor);
    emit(@{@"kind":@"observer",@"simpleFactoryHooked":@(originalSimple!=nullptr),@"optionsFactoryHooked":@(originalOptions!=nullptr),@"descriptorFactoryHooked":@(originalDescriptor!=nullptr),@"scope":@"Only this child process; original arguments/options/return objects forwarded unchanged"});
}}
#ifdef EC_OBSERVER_STANDALONE
int main(){rtInstallMetalObserver();}
#endif

extern "C" void rtBeginCapture(const char* path){@autoreleasepool{
 MTLCaptureDescriptor* d=[MTLCaptureDescriptor new];d.captureObject=captureDevice;d.destination=MTLCaptureDestinationGPUTraceDocument;d.outputURL=[NSURL fileURLWithPath:@(path)];NSError* error=nil;BOOL ok=[[MTLCaptureManager sharedCaptureManager] startCaptureWithDescriptor:d error:&error];
 emit(@{@"kind":@"capture-start",@"path":@(path),@"success":@(ok),@"error":error.localizedDescription?:@""});if(!ok)abort();
}}
extern "C" void rtEndCapture(){[[MTLCaptureManager sharedCaptureManager] stopCapture];emit(@{@"kind":@"capture-end"});}
