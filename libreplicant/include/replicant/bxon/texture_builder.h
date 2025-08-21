#pragma once

#include "replicant/bxon/error.h"
#include <vector>
#include <string>
#include <expected>

namespace replicant::bxon {


    std::expected<std::vector<char>, BxonError> buildTextureFromDDS(
        const std::vector<char>& dds_data,
        uint32_t version = 0x20090422,
        uint32_t projectID = 0x54455354 // "TEST"   
    );

}