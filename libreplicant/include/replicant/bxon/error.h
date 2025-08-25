#pragma once
#include <string>

namespace replicant::bxon {

    enum class BxonErrorCode {
        Success,
        BuildError
    };

    struct BxonError {
        BxonErrorCode code;
        std::string message;
    };
} 