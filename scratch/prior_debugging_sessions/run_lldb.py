import lldb
import os

debugger = lldb.SBDebugger.Create()
debugger.SetAsync(False)
target = debugger.CreateTarget("build/object_concept_test")
if target:
    process = target.LaunchSimple(None, None, os.getcwd())
    if process:
        state = process.GetState()
        print("Process state: ", lldb.SBDebugger.StateAsCString(state))
        for thread in process:
            print("Thread: ", thread.GetThreadID())
            for frame in thread:
                print(frame)
