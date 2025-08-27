#include "replicant/bxon.h"
#include "common.h"
#include <fstream>
#include <vector>
#include <iostream>
#include <cstring>

namespace {
#pragma pack(push, 1)
    struct RawBxonHeader {
        char     magic[4];
        uint32_t version;
        uint32_t projectID;
        uint32_t offsetToAssetTypeName;
        uint32_t offsetToAssetTypeData;
    };
#pragma pack(pop)
} 

namespace replicant::bxon {

    void parseArchiveParameters(ArchiveParameters& asset, const char* buffer, size_t size, const char* asset_base);

    bool File::loadFromFile(const std::string& filepath) {
        std::ifstream file(filepath, std::ios::binary);
        if (!file) {
            return false;
        }
        file.seekg(0, std::ios::end);
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        if (size <= 0) {
            return false;
        }
        std::vector<char> buffer(size);
        if (!file.read(buffer.data(), size)) {
            return false;
        }
        return loadFromMemory(buffer.data(), buffer.size());
    }

    bool File::parseAsset(const char* buffer, size_t size, uintptr_t asset_data_offset) {
        if (asset_data_offset >= size) {
            return false;
        }
        const char* asset_base = buffer + asset_data_offset;

        if (m_assetTypeName == "tpArchiveFileParam") {
            ArchiveParameters& param = m_asset.emplace<ArchiveParameters>();
            parseArchiveParameters(param, buffer, size, asset_base);
            return true;
        }
        else if (m_assetTypeName == "tpGxTexHead") {
            Texture& tex = m_asset.emplace<Texture>();
            return tex.parse(buffer, size, asset_base);
        }
        return false;
    }


    bool File::loadFromMemory(const char* buffer, size_t size) {
        m_asset.emplace<std::monostate>();

        if (!buffer || size < sizeof(RawBxonHeader)) {
            return false;
        }
        const auto* header = reinterpret_cast<const RawBxonHeader*>(buffer);

        if (strncmp(header->magic, "BXON", 4) != 0) {
            return false;
        }

        m_version = header->version;
        m_projectID = header->projectID;

        m_assetTypeName = detail::ReadStringFromOffset(buffer, size, &header->offsetToAssetTypeName, header->offsetToAssetTypeName);
        if (m_assetTypeName.empty()) {
            return false;
        }

        const uint32_t asset_data_field_absolute_pos = (uintptr_t)&header->offsetToAssetTypeData - (uintptr_t)buffer;
        const uint32_t asset_data_absolute_pos = asset_data_field_absolute_pos + header->offsetToAssetTypeData;

        return parseAsset(buffer, size, asset_data_absolute_pos);
    }
} 