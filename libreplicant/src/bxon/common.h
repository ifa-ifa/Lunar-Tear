#pragma once

#include <string>
#include <cstdint>
#include <cstddef>

namespace replicant::bxon::detail {

    // This is the declaration for our shared helper function.
    // It's in a 'detail' namespace to signify it's for internal use only.
    std::string ReadStringFromOffset(
        const char* buffer_base,
        size_t buffer_size,
        const void* field_ptr,
        uint32_t relative_offset
    );

} // namespace replicant::bxon::detail