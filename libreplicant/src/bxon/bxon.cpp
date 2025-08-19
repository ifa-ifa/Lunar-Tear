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
} // namespace

namespace replicant::bxon {

    void parseArchiveFileParam(ArchiveFileParam& asset, const char* buffer, size_t size, const char* asset_base);

    bool File::loadFromFile(const std::string& filepath) {
        std::ifstream file(filepath, std::ios::binary);
        if (!file) {
            // std::cerr << "Debug: Failed to open file: " << filepath << std::endl;
            return false;
        }

        file.seekg(0, std::ios::end);
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        if (size <= 0) {
            // std::cerr << "Debug: File is empty or cannot determine size: " << filepath << std::endl;
            return false;
        }

        std::vector<char> buffer(size);
        if (!file.read(buffer.data(), size)) {
            // std::cerr << "Debug: Failed to read file data: " << filepath << std::endl;
            return false;
        }
        return loadFromMemory(buffer.data(), buffer.size());
    }

    bool File::parseAsset(const char* buffer, size_t size, uintptr_t asset_data_offset) {
        if (asset_data_offset >= size) {
            // std::cerr << "Debug: Asset data offset is out of bounds.\n";
            return false;
        }
        const char* asset_base = buffer + asset_data_offset;

        if (m_assetTypeName == "tpArchiveFileParam") {
            ArchiveFileParam& param = m_asset.emplace<ArchiveFileParam>();
            parseArchiveFileParam(param, buffer, size, asset_base);
            return true;
        }

        // std::cout << "Debug: Unsupported asset type '" << m_assetTypeName << "'.\n";
        return false;
    }

    bool File::loadFromMemory(const char* buffer, size_t size) {
        m_asset.emplace<std::monostate>();

        if (!buffer || size < sizeof(RawBxonHeader)) {
            // std::cerr << "Debug: Buffer is null or too small for a BXON header.\n";
            return false;
        }
        const auto* header = reinterpret_cast<const RawBxonHeader*>(buffer);

        if (strncmp(header->magic, "BXON", 4) != 0) {
            // std::cerr << "Debug: Invalid BXON magic bytes.\n";
            return false;
        }

        m_version = header->version;
        m_projectID = header->projectID;

        m_assetTypeName = detail::ReadStringFromOffset(buffer, size, &header->offsetToAssetTypeName, header->offsetToAssetTypeName);
        if (m_assetTypeName.empty()) {
            // std::cerr << "Debug: Could not read asset type name.\n";
            return false;
        }

        const uint32_t asset_data_field_absolute_pos = (uintptr_t)&header->offsetToAssetTypeData - (uintptr_t)buffer;
        const uint32_t asset_data_absolute_pos = asset_data_field_absolute_pos + header->offsetToAssetTypeData;

        return parseAsset(buffer, size, asset_data_absolute_pos);
    }
} // namespace replicant::bxon