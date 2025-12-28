#pragma once
#include "replicant/core/reader.h"
#include "replicant/core/writer.h"
#include <string>
#include <vector>
#include <span >
#include <cstdint>
#include <expected>

namespace replicant {

    struct BxonHeaderInfo {
        uint32_t version;
        uint32_t projectId;
        std::string assetType; 
    };

    class Bxon {
    public:

        static std::expected<std::pair<BxonHeaderInfo, std::span<const std::byte>>, ReaderError>
            Parse(std::span<const std::byte> data);

        static std::vector<std::byte> Build(
            const std::string& assetType,
            uint32_t version,
            uint32_t projectId,
            std::span<const std::byte> payload
        );
    };
}