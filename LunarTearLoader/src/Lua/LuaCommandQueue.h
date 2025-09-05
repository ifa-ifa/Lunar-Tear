#pragma once
#include <string>
#include <API/LunarTear.h>

namespace LuaCommandQueue {
    
    //Thread safe. arguments and returns not supported. For external use its not reccomended anymore, use new QueuePhaseScriptExecution instead
    void QueuePhaseScriptCall(const char* functionName);

    // Thread safe. Pass entire script.
    void QueuePhaseScriptExecution(const char* script, LT_ScriptExecutionCallbackFunc callback, void* userData);


    // Tries to get the next script. Returns true if a script was available
    bool GetNextScript(std::string& outScript);

    // Reports the result for the script that was just processed.
    void ReportResult(const char* result);

    void ProcessQueue(void* scriptManager);
}
