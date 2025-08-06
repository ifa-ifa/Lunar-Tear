#pragma once
#include <string>

namespace LuaCommandQueue {
    
    //Thread safe. arguments not supported
    void QueuePhaseScriptCall(const char* functionName);

    // Not thread safe
    void ProcessPhaseScriptQueue(void* scriptManager);
}