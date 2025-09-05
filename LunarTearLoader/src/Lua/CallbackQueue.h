#pragma once
#include "API/LunarTear.h"

namespace UpdateCallbackQueue {

    typedef void (*LT_UpdateFunc)(void* userData);

    enum class UpdateLoopType {
        Phase,
        Game, 
        Root  
    };

    // Thread safe
    void QueueCallback(UpdateLoopType loop, LT_UpdateFunc func, void* userData);

    // Not thread safe. Must be called from the appropriate game thread
    void ProcessCallbacks(UpdateLoopType loop);
}