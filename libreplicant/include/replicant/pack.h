#pragma once
#include "replicant/core/common.h"
#include <vector>
#include <string>
#include <cstdint>
#include <expected>
#include <optional>
#include <span>
#include <filesystem>

namespace replicant {

    struct PackHeaderInfo {
        uint32_t version;
    };

    struct ImportEntry {
        uint32_t pathHash;
        std::string path;
    };

    struct AssetPackageEntry {
        uint32_t nameHash;
        std::string name;
        std::vector<std::byte> content;
    };

    struct PackFileEntry {
        uint32_t nameHash = 0;
        std::string name;

        std::vector<std::byte> serializedData;
        std::vector<std::byte> resourceData;

        bool hasResource() const { return !resourceData.empty(); }
    };

    class Pack {
    public:
        PackHeaderInfo info;
        std::vector<ImportEntry> imports;
        std::vector<AssetPackageEntry> assetPackages;
        std::vector<PackFileEntry> files;

        static std::expected<Pack, Error> Deserialize(std::span<const std::byte> data);
        static std::expected<Pack, Error> DeserializeNoResources(const std::filesystem::path& filePath);

        struct PackFileSizes {
            uint32_t serializedSize;
            uint32_t resourceSize;
            uint32_t fileSize;
        };
		static std::expected<PackFileSizes, Error> getFileSizes(const std::filesystem::path& filePath);

        std::expected<std::vector<std::byte>, Error> Serialize() const;

        PackFileEntry* findFile(const std::string& name);
        const PackFileEntry* findFile(const std::string& name) const;

    private:
        std::vector<std::byte> SerializeInternal() const;

    };
}