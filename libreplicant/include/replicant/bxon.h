#pragma once
#include "replicant/core/common.h"
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

    std::expected<std::pair<BxonHeaderInfo, std::span<const std::byte>>, Error>
            ParseBxon(std::span<const std::byte> data);

    std::expected<std::vector<std::byte>, Error> BuildBxon(
            const std::string& assetType,
            uint32_t version,
            uint32_t projectId,
            std::span<const std::byte> payload
        );
    
}