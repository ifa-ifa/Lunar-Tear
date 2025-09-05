#include "CallbackQueue.h"
#include "Common/Logger.h"
#include <deque>
#include <mutex>
#include <vector>
#include <map>

using enum Logger::LogCategory;

namespace {
    struct QueuedCallback {
        LT_UpdateFunc function;
        void* userData;
    };

    static std::map<UpdateCallbackQueue::UpdateLoopType, std::deque<QueuedCallback>> s_callbackQueues;
    static std::mutex s_queueMutex;
}

namespace UpdateCallbackQueue {

    void QueueCallback(UpdateLoopType loop, LT_UpdateFunc func, void* userData) {
        if (!func) return;
        std::lock_guard<std::mutex> lock(s_queueMutex);
        s_callbackQueues[loop].push_back({ func, userData });
    }

    void ProcessCallbacks(UpdateLoopType loop) {
        if (s_callbackQueues.empty() || s_callbackQueues.find(loop) == s_callbackQueues.end()) {
            return;
        }

        std::deque<QueuedCallback> callbacksToExecute;
        {
            std::lock_guard<std::mutex> lock(s_queueMutex);
            auto& queue = s_callbackQueues[loop];
            if (queue.empty()) {
                return;
            }
            // Move all current callbacks to a temporary queue to be executed outside the lock
            callbacksToExecute.swap(queue);
        }

        if (callbacksToExecute.empty()) {
            return;
        }

        for (const auto& callback : callbacksToExecute) {
            try {
                callback.function(callback.userData);
            }
            catch (const std::exception& e) {
                Logger::Log(Error) << "Exception in update callback: " << e.what();
            }
            catch (...) {
                Logger::Log(Error) << "Unknown exception in update callback.";
            }
        }
    }
}