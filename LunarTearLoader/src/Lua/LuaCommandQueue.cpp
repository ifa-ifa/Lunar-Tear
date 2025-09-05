#include "LuaCommandQueue.h"
#include "Common/Logger.h"
#include "Game/Functions.h"
#include <deque>
#include <mutex>
#include <memory>
#include <string>

using enum Logger::LogCategory;

namespace {
    struct ExecutionRequest {
        std::string script;
        LT_ScriptExecutionCallbackFunc callback;
        void* userData;
    };

    std::deque<std::unique_ptr<ExecutionRequest>> s_pendingQueue;
    std::mutex s_pendingMutex;


    std::unique_ptr<ExecutionRequest> s_activeRequest = nullptr;
    std::mutex s_activeMutex;

    std::deque<std::string> s_commandQueue;
    std::mutex s_commandMutex;
}

namespace LuaCommandQueue {

    void QueuePhaseScriptCall(const char* functionName) {
        std::lock_guard<std::mutex> lock(s_commandMutex);
        s_commandQueue.push_back(functionName);
    }

    void QueuePhaseScriptExecution(const char* script, LT_ScriptExecutionCallbackFunc callback, void* userData) {
        auto request = std::make_unique<ExecutionRequest>();
        request->script = script;
        request->callback = callback;
        request->userData = userData;

        std::lock_guard<std::mutex> lock(s_pendingMutex);
        s_pendingQueue.push_back(std::move(request));
    }

    void ProcessQueue(void* scriptManager) {
        {
            std::lock_guard<std::mutex> lock(s_commandMutex);
            while (!s_commandQueue.empty()) {
                const std::string& funcName = s_commandQueue.front();
                PrepareScriptFunctionCall(scriptManager, funcName.c_str(), 0);
                ExecuteScriptCoroutine(scriptManager, 0);
                s_commandQueue.pop_front();
            }
        }

        if (s_activeRequest || s_pendingQueue.empty()) {
            return;
        }

        PrepareScriptFunctionCall(scriptManager, "ififa_LTCore_scriptDispatch", 0);
        ExecuteScriptCoroutine(scriptManager, 0);
    }

    bool GetNextScript(std::string& outScript) {
        std::lock_guard<std::mutex> pendingLock(s_pendingMutex);
        if (s_pendingQueue.empty()) {
            return false;
        }

        std::lock_guard<std::mutex> activeLock(s_activeMutex);
        s_activeRequest = std::move(s_pendingQueue.front());
        s_pendingQueue.pop_front();
        outScript = s_activeRequest->script;
        return true;
    }

    void ReportResult(const char* result) {
        std::unique_ptr<ExecutionRequest> requestToProcess;
        {
            std::lock_guard<std::mutex> lock(s_activeMutex);
            requestToProcess = std::move(s_activeRequest);
        }

        if (!requestToProcess) {
            Logger::Log(Warning) << "ReportResult called but there was no active script request.";
            return;
        }

        if (requestToProcess->callback) {
            std::string resultStr(result ? result : "runtime_error:null result string");
            LT_LuaResult parsedResult = {}; 
            size_t colonPos = resultStr.find(':');
            if (colonPos == std::string::npos) {
                parsedResult.type = LT_LUA_RESULT_ERROR_RUNTIME;
                parsedResult.value.stringValue = "Malformed result from Lua";
            }
            else {
                std::string type = resultStr.substr(0, colonPos);
                std::string value = resultStr.substr(colonPos + 1);
                try {
                    if (type == "number") {
                        parsedResult.type = LT_LUA_RESULT_NUMBER;
                        parsedResult.value.numberValue = std::stod(value);
                    }
                    else if (type == "string") {
                        parsedResult.type = LT_LUA_RESULT_STRING;
                        parsedResult.value.stringValue = value.c_str();
                    }
                    else if (type == "nil") {
                        parsedResult.type = LT_LUA_RESULT_NIL;
                    }
                    else if (type == "syntax_error") {
                        parsedResult.type = LT_LUA_RESULT_ERROR_SYNTAX;
                        parsedResult.value.stringValue = value.c_str();
                    }
                    else if (type == "runtime_error") {
                        parsedResult.type = LT_LUA_RESULT_ERROR_RUNTIME;
                        parsedResult.value.stringValue = value.c_str();
                    }
                    else {
                        parsedResult.type = LT_LUA_RESULT_ERROR_UNSUPPORTED_TYPE;
                        parsedResult.value.stringValue = value.c_str();
                    }
                }
                catch (const std::exception&) {
                    parsedResult.type = LT_LUA_RESULT_ERROR_RUNTIME;
                    parsedResult.value.stringValue = "Failed to parse numeric result";
                }
            }
            requestToProcess->callback(&parsedResult, requestToProcess->userData);
        }
    }
}