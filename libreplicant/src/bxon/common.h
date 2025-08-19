#pragma once

#include <string>
#include <cstdint>
#include <cstddef>

namespace replicant::bxon::detail {

    std::string ReadStringFromOffset(
        const char* buffer_base,
        size_t buffer_size,
        const void* field_ptr,
        uint32_t relative_offset
    );

}