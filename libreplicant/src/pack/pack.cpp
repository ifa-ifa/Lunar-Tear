#include "replicant/pack.h"
#include <cstring> 

namespace {
    // This struct directly maps to the on-disk header of a PACK file
#pragma pack(push, 1)
    struct RawPackHeader {
        char     magic[4];
        uint32_t version;
        uint32_t totalSize;
        uint32_t serializedSize;
        uint32_t resourceSize;
        uint32_t pathCount;
        uint32_t offsetToPaths;
        uint32_t assetPackCount;
        uint32_t offsetToAssetPacks;
        uint32_t fileCount;
        uint32_t offsetToFiles;
    };
#pragma pack(pop)

    static_assert(sizeof(RawPackHeader) == 44, "RawPackHeader size must be 44  bytes");
}

namespace replicant::pack {

    bool PackFile::loadFromMemory(const char* buffer, size_t size) {
        if (!buffer || size < sizeof(RawPackHeader)) {
            return false;
        }

        const auto* header = reinterpret_cast<const RawPackHeader*>(buffer);

        if (strncmp(header->magic, "PACK", 4) != 0) {
            return false;
        }

        if (header->totalSize > size) {
            return false;
        }

        m_version = header->version;
        m_total_size = header->totalSize;
        m_serialized_size = header->serializedSize;
        m_resource_size = header->resourceSize;

        // We don't need to parse the rest of the file for now, implement later
        

        return true;
    }

} 