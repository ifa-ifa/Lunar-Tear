#pragma once
#include <string>

namespace replicant::archive {

    enum class ArcErrorCode {
        Success,
        ZstdCompressionError,
        ZstdDecompressionError,
        DecompressionSizeMismatch,
        EmptyInput,
        DuplicateKey,
        FileReadError,
        Unimplemented
    };

    struct ArcError {
        ArcErrorCode code;
        std::string message;
    };

}