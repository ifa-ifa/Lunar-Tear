#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <expected>

namespace replicant::pack {

    enum class PackErrorCode {
        Success,
        InvalidMagic,
        BufferTooSmall,
        SizeMismatch
    };

    struct PackError {
        PackErrorCode code;
        std::string message;
    };

    class PackFile {
    public:

        bool loadFromMemory(const char* buffer, size_t size);

        uint32_t getVersion() const { return m_version; }
        uint32_t getTotalSize() const { return m_total_size; }
        uint32_t getSerializedSize() const { return m_serialized_size; }
        uint32_t getResourceSize() const { return m_resource_size; }

    private:
        uint32_t m_version = 0;
        uint32_t m_total_size = 0;
        uint32_t m_serialized_size = 0; 
        uint32_t m_resource_size = 0;  
    };

} 