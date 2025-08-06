#include "LuaCommandQueue.h"
#include "Common/Logger.h"
#include "Game/Functions.h"
#include <deque>
#include <mutex>
#include <chrono>
#include <vector>
#include <iterator>

using enum Logger::LogCategory;

namespace {
    struct LuaCommand {
        std::string functionName;
        std::chrono::steady_clock::time_point creationTime;
    };

    constexpr long long COMMAND_TIMEOUT_MS = 2000;

    std::deque<LuaCommand> s_phaseCommandQueue;
    std::mutex s_phaseCommandQueueMutex;
}

namespace LuaCommandQueue {

    void QueuePhaseScriptCall(const char* functionName) {

        std::lock_guard<std::mutex> lock(s_phaseCommandQueueMutex);
        s_phaseCommandQueue.push_back({ functionName, std::chrono::steady_clock::now() });
        Logger::Log(Verbose) << "Queued phase script call: " << functionName;
    }

    void ProcessPhaseScriptQueue(void* scriptManager) {
        if (s_phaseCommandQueue.empty()) {
            return;
        }

        std::vector<LuaCommand> commandsToExecute;
        {
            std::lock_guard<std::mutex> lock(s_phaseCommandQueueMutex);
            if (s_phaseCommandQueue.empty()) {
                return;
            }

            const auto now = std::chrono::steady_clock::now();
            const auto timeout = std::chrono::milliseconds(COMMAND_TIMEOUT_MS);

            // Discard stale commands from the front of the queue
            while (!s_phaseCommandQueue.empty() && (now - s_phaseCommandQueue.front().creationTime > timeout)) {
                Logger::Log(Warning) << "Discarding stale Lua command: " << s_phaseCommandQueue.front().functionName;
                s_phaseCommandQueue.pop_front();
            }

            // Move remaining (valid) commands to a temporary vector to be executed outside the lock
            if (!s_phaseCommandQueue.empty()) {
                std::move(s_phaseCommandQueue.begin(), s_phaseCommandQueue.end(), std::back_inserter(commandsToExecute));
                s_phaseCommandQueue.clear();
            }
        }

        if (commandsToExecute.empty()) {
            return;
        }

        Logger::Log(Verbose) << "Executing " << commandsToExecute.size() << " queued Lua command(s).";

        for (const auto& command : commandsToExecute) {
            Logger::Log(Info) << "Executing Lua function: " << command.functionName;

            PrepareScriptFunctionCall(scriptManager, command.functionName.c_str(), 0);
            ExecuteScriptCoroutine(scriptManager, 0);
        }
    }
} 