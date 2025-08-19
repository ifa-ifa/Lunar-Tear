#pragma once

#include <string>

namespace replicant::bxon {

    enum class BxonErrorCode {
        Success,
        // Add more specific codes here as needed
        BuildError
    };

    struct BxonError {
        BxonErrorCode code;
        std::string message;
    };

} // namespace replicant::bxon