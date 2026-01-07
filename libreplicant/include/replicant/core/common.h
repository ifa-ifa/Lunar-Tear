#pragma once
#include <string>
#include <format>

namespace replicant {

    inline uint32_t fnv1_32(std::string_view s) {
        uint32_t hash = 0x811c9dc5;
        for (char c : s) {
            hash *= 0x01000193;
            hash ^= static_cast<uint8_t>(c);
        }
        return hash;
    }

    enum class ErrorCode {
        IoError,
        ParseError,
        InvalidArguments,
        SystemError,
        UnsupportedFeature
    };

    struct Error {
        ErrorCode code;
        std::string message;

        std::string toString() const {
            const char* typeStr = "Unknown";
            switch (code) {
            case ErrorCode::IoError: typeStr = "I/O Error"; break;
            case ErrorCode::ParseError: typeStr = "Corrupt Data"; break;
            case ErrorCode::InvalidArguments: typeStr = "Invalid Arguments"; break;
            case ErrorCode::UnsupportedFeature: typeStr = "Unsupported"; break;
            }
            return std::format("[{}] {}", typeStr, message);
        }
    };
}