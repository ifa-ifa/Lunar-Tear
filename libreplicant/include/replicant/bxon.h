#pragma once

#include <string>
#include <vector>
#include <variant>
#include <optional>
#include <cstdint>
#include "replicant/bxon/archive_param.h"

namespace replicant::bxon {

    using AssetData = std::variant<
        std::monostate,
        ArchiveFileParam
    >;

    class File {
    public:
        bool loadFromFile(const std::string& filepath);
        bool loadFromMemory(const char* buffer, size_t size);

        uint32_t getVersion() const { return m_version; }
        uint32_t getProjectID() const { return m_projectID; }
        const std::string& getAssetTypeName() const { return m_assetTypeName; }

        const AssetData& getAsset() const { return m_asset; }
        AssetData& getAsset() { return m_asset; }

    private:
        bool parseAsset(const char* buffer, size_t size, uintptr_t asset_data_offset);

        uint32_t m_version = 0;
        uint32_t m_projectID = 0;
        std::string m_assetTypeName;
        AssetData m_asset;
    };

} // namespace replicant::bxon