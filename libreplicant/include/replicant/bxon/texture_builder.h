#pragma once

#include "replicant/bxon/error.h"
#include <vector>
#include <string>
#include <expected>

namespace replicant::bxon {


    std::expected<std::vector<char>, BxonError> buildTextureFromDDS(
        const std::vector<char>& dds_data,
        uint32_t version = 3,
        uint32_t projectID = 0x2ea74106 // copied from game // TODO: MAKE IT COPY FROM ORIGINAL PACK!
    );

}