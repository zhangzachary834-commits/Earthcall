// Diagnostic-only observation of the native shader compilation boundary.
// Codex / GPT-6.1 Sol / 01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca / 2026-10-12 PDT.
// Zach's exact-fidelity direction; Astra's backend-admission requirement.
// Linked ONLY by scripts/volume_backend_contract_probe.py into a scratch process.
// No options/source/pipeline changes. The concrete device method forwards the
// original arguments once, recording MSL and option values outside the repo.
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <objc/runtime.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <mutex>

namespace {
// Kernel-only diagnostic state: one scratch-process observer; never authored,
// never installed in the app. OS process teardown releases its retained path.
using Compile = id<MTLLibrary> (*)(id, SEL, NSString*, MTLCompileOptions*, NSError**);
Compile original = nullptr;
NSString* directory = nil;
std::mutex recordMutex;
unsigned recordCount = 0;

void writeExclusive(NSString* path, NSData* data) {
    const int fd = open(path.fileSystemRepresentation, O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (fd < 0 || !data) { std::fprintf(stderr, "METAL_OBSERVER cannot create record\n"); std::abort(); }
    const auto* bytes = static_cast<const unsigned char*>(data.bytes);
    size_t remaining = data.length;
    while (remaining) {
        const ssize_t n = write(fd, bytes, remaining);
        if (n <= 0) { std::fprintf(stderr, "METAL_OBSERVER record write failed\n"); std::abort(); }
        bytes += n; remaining -= static_cast<size_t>(n);
    }
    if (close(fd)) std::abort();
}

id<MTLLibrary> observe(id device, SEL selector, NSString* source,
                      MTLCompileOptions* options, NSError** error) {
    @autoreleasepool {
        NSMutableDictionary* record = [NSMutableDictionary dictionary];
        record[@"optionsPresent"] = @(options != nil);
        record[@"deviceName"] = [(id<MTLDevice>)device name];
        record[@"deviceClass"] = NSStringFromClass(object_getClass(device));
        if (options) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
            record[@"fastMathEnabled"] = @(options.fastMathEnabled);
#pragma clang diagnostic pop
            record[@"languageVersion"] = @(options.languageVersion);
            if (@available(macOS 15.0, *)) {
                record[@"mathMode"] = @(options.mathMode);
                record[@"mathFloatingPointFunctions"] = @(options.mathFloatingPointFunctions);
            }
            if (@available(macOS 11.0, *)) record[@"preserveInvariance"] = @(options.preserveInvariance);
        }
        // Observation cannot silently choose a different numerical profile.
        id<MTLLibrary> library = original(device, selector, source, options, error);
        record[@"libraryCreated"] = @(library != nil);
        if (!library && error && *error) record[@"error"] = (*error).localizedDescription;
        std::lock_guard<std::mutex> lock(recordMutex);
        const unsigned index = recordCount++;
        record[@"index"] = @(index);
        NSString* stem = [directory stringByAppendingPathComponent:
                         [NSString stringWithFormat:@"module-%03u", index]];
        writeExclusive([stem stringByAppendingString:@".metal"], [source dataUsingEncoding:NSUTF8StringEncoding]);
        writeExclusive([stem stringByAppendingString:@".json"],
                       [NSJSONSerialization dataWithJSONObject:record options:NSJSONWritingPrettyPrinted error:nil]);
        return library;
    }
}
}

extern "C" void installVolumeMetalCompileObserver(const char* output) {
    @autoreleasepool {
        if (original || !output) std::abort();
        directory = [[NSString alloc] initWithUTF8String:output];
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        if (!device) { std::fprintf(stderr, "METAL_OBSERVER no native device\n"); std::abort(); }
        const Class concrete = object_getClass(device);
        const SEL selector = @selector(newLibraryWithSource:options:error:);
        const Method method = class_getInstanceMethod(concrete, selector);
        if (!method) std::abort();
        original = reinterpret_cast<Compile>(method_getImplementation(method));
        // Do not replace an inherited method on a superclass: install only on
        // this scratch process's concrete device class.
        if (!class_addMethod(concrete, selector, reinterpret_cast<IMP>(observe), method_getTypeEncoding(method)))
            method_setImplementation(method, reinterpret_cast<IMP>(observe));
        std::fprintf(stdout, "METAL_OBSERVER installed for %s; options unchanged\n", class_getName(concrete));
        [device release];
    }
}
