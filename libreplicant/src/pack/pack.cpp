#include "replicant/pack.h"
#include "replicant/bxon/texture_builder.h"
#include <cstring>
#include <set>
#include <map>
#include <numeric>
#include <algorithm>
#include <stdexcept>

namespace {
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
    struct RawFileEntry {
        uint32_t hash;
        uint32_t nameOffset;
        uint32_t fileSize;
        uint32_t contentStartOffset;
        uint32_t assetsDataHash;
    };
#pragma pack(pop)

    size_t align(size_t value, size_t alignment) {
        return (value + alignment - 1) & ~(alignment - 1);
    }

    const char* get_ptr_from_field(const char* base, const void* field_ptr, uint32_t relative_offset) {
        uintptr_t field_address = reinterpret_cast<uintptr_t>(field_ptr);
        uintptr_t base_address = reinterpret_cast<uintptr_t>(base);
        return base + (field_address - base_address) + relative_offset;
    }
}

namespace replicant::pack {

    std::expected<void, PackError> PackFile::loadFromMemory(const char* buffer, size_t size) {
        m_file_entries.clear();
        if (!buffer || size < sizeof(RawPackHeader)) {
            return std::unexpected(PackError{ PackErrorCode::BufferTooSmall, "Buffer too small for PACK header." });
        }

        const auto* header = reinterpret_cast<const RawPackHeader*>(buffer);
        if (strncmp(header->magic, "PACK", 4) != 0) {
            return std::unexpected(PackError{ PackErrorCode::InvalidMagic, "Invalid PACK magic." });
        }
        if (header->totalSize > size) {
            return std::unexpected(PackError{ PackErrorCode::SizeMismatch, "Header total size exceeds buffer size." });
        }

        m_version = header->version;
        m_total_size = header->totalSize;
        m_serialized_size = header->serializedSize;
        m_resource_size = header->resourceSize;

        if (header->fileCount == 0) return {};

        const auto* raw_file_entries = reinterpret_cast<const RawFileEntry*>(
            get_ptr_from_field(buffer, &header->offsetToFiles, header->offsetToFiles)
            );

        m_file_entries.reserve(header->fileCount);
        for (uint32_t i = 0; i < header->fileCount; ++i) {
            const auto& raw_entry = raw_file_entries[i];
            PackFileEntry entry;
            const char* name_ptr = get_ptr_from_field(buffer, &raw_entry.nameOffset, raw_entry.nameOffset);
            entry.name = std::string(name_ptr);
            const char* content_start = get_ptr_from_field(buffer, &raw_entry.contentStartOffset, raw_entry.contentStartOffset);
            entry.serialized_data.assign(content_start, content_start + raw_entry.fileSize);
            entry.assetsDataHash = raw_entry.assetsDataHash;
            m_file_entries.push_back(std::move(entry));
        }

        const char* resource_base = buffer + header->serializedSize;
        size_t current_resource_offset = 0;

        for (auto& entry : m_file_entries) {
            if (entry.assetsDataHash == 0) continue;

            replicant::bxon::File bxon_file;
            if (!bxon_file.loadFromMemory(entry.serialized_data.data(), entry.serialized_data.size())) {
                continue; 
            }

            if (bxon_file.getAssetTypeName() == "tpGxTexHead") {
                current_resource_offset = align(current_resource_offset, 64);

                struct RawBxonHeader { char m[4]; uint32_t v, p, o1, o2; };
                struct RawGxTexHeader { uint32_t w, h, s, m, texSize; };

                const auto* bxon_header_ptr = reinterpret_cast<const RawBxonHeader*>(entry.serialized_data.data());
                const char* asset_data_ptr = get_ptr_from_field(entry.serialized_data.data(), &bxon_header_ptr->o2, bxon_header_ptr->o2);
                const auto* tex_header = reinterpret_cast<const RawGxTexHeader*>(asset_data_ptr);

                size_t tex_data_size = tex_header->texSize;
                if (header->serializedSize + current_resource_offset + tex_data_size > size) {
                    return std::unexpected(PackError{ PackErrorCode::ParseError, "Texture resource for '" + entry.name + "' extends beyond file bounds." });
                }

                const char* tex_data_start = resource_base + current_resource_offset;
                entry.resource_data.assign(tex_data_start, tex_data_start + tex_data_size);
                current_resource_offset += tex_data_size;
            }
        }
        return {};
    }

    PackFileEntry* PackFile::findFileEntryMutable(const std::string& name) {
        for (auto& entry : m_file_entries) {
            if (entry.name == name) {
                return &entry;
            }
        }
        return nullptr;
    }

    std::expected<void, PackError> PackFile::patchFile(const std::string& entry_name, const std::vector<char>& new_dds_data) {
        PackFileEntry* entry_to_patch = findFileEntryMutable(entry_name);
        if (!entry_to_patch) {
            return std::unexpected(PackError{ PackErrorCode::EntryNotFound, "Could not find file '" + entry_name + "' in PACK." });
        }

        auto bxon_result = replicant::bxon::buildTextureFromDDS(new_dds_data);
        if (!bxon_result) {
            return std::unexpected(PackError{ PackErrorCode::BuildError, "Failed to build texture from DDS: " + bxon_result.error().message });
        }
        const std::vector<char>& full_bxon_data = *bxon_result;

        struct RawBxonHeader { char m[4]; uint32_t v, p, o1, o2; };
        struct RawGxTexHeader { uint32_t w, h, s, mipLevels, texSize, u1, fmt, numMips, mipOffset; };

        const auto* raw_bxon_header = reinterpret_cast<const RawBxonHeader*>(full_bxon_data.data());
        const char* asset_data_ptr = get_ptr_from_field(full_bxon_data.data(), &raw_bxon_header->o2, raw_bxon_header->o2);
        const auto* raw_tex_header = reinterpret_cast<const RawGxTexHeader*>(asset_data_ptr);
        const char* mip_array_ptr = get_ptr_from_field(full_bxon_data.data(), &raw_tex_header->mipOffset, raw_tex_header->mipOffset);

                                                                       // sizeof(RawGxTexMipSurface) is 40 bytes
        size_t serialized_part_size = (mip_array_ptr - full_bxon_data.data()) + (raw_tex_header->numMips * 40);

        if (serialized_part_size + raw_tex_header->texSize != full_bxon_data.size()) {
            return std::unexpected(PackError{ PackErrorCode::BuildError, "Internal error: BXON texture size mismatch during splitting." });
        }

        entry_to_patch->serialized_data.assign(full_bxon_data.data(), full_bxon_data.data() + serialized_part_size);
        entry_to_patch->resource_data.assign(full_bxon_data.data() + serialized_part_size, full_bxon_data.data() + full_bxon_data.size());

        entry_to_patch->assetsDataHash = 0;

        return {};
    }

    std::expected<std::vector<char>, PackError> PackFile::build() {
        std::set<std::string> string_set;
        for (const auto& entry : m_file_entries) { string_set.insert(entry.name); }
        std::vector<std::string> string_pool(string_set.begin(), string_set.end());

        size_t string_pool_size = 0;
        for (const auto& s : string_pool) { string_pool_size += s.length() + 1; }

        const size_t header_size = sizeof(RawPackHeader);
        const size_t file_descriptors_size = m_file_entries.size() * sizeof(RawFileEntry);

        size_t current_offset = header_size;
        const size_t file_descriptors_start = current_offset;
        current_offset += file_descriptors_size;

        const size_t string_pool_start = current_offset;
        current_offset += string_pool_size;

        std::vector<size_t> content_starts;
        for (const auto& entry : m_file_entries) {
            current_offset = align(current_offset, 16);
            content_starts.push_back(current_offset);
            current_offset += entry.serialized_data.size();
        }

        const size_t serialized_size = align(current_offset, 64);

        size_t final_resource_size = 0;
        size_t temp_resource_offset = 0;
        for (const auto& entry : m_file_entries) {
            if (!entry.resource_data.empty()) {
                temp_resource_offset = align(temp_resource_offset, 64);
                temp_resource_offset += entry.resource_data.size();
            }
        }
        final_resource_size = temp_resource_offset;

        const size_t total_size = serialized_size + final_resource_size;
        std::vector<char> out_buffer(total_size, 0);
        char* buffer_base = out_buffer.data();

        auto* header = reinterpret_cast<RawPackHeader*>(buffer_base);
        memcpy(header->magic, "PACK", 4);
        header->version = m_version;
        header->totalSize = total_size;
        header->serializedSize = serialized_size;
        header->resourceSize = final_resource_size;
        header->pathCount = 0;
        header->offsetToPaths = 0;
        header->assetPackCount = 0;
        header->offsetToAssetPacks = 0;
        header->fileCount = m_file_entries.size();
        header->offsetToFiles = file_descriptors_start - (reinterpret_cast<char*>(&header->offsetToFiles) - buffer_base);

        std::map<std::string, size_t> string_to_offset;
        size_t current_string_offset = string_pool_start;
        for (const auto& s : string_pool) {
            string_to_offset[s] = current_string_offset;
            memcpy(buffer_base + current_string_offset, s.c_str(), s.length() + 1);
            current_string_offset += s.length() + 1;
        }

        auto* raw_file_entries = reinterpret_cast<RawFileEntry*>(buffer_base + file_descriptors_start);
        for (size_t i = 0; i < m_file_entries.size(); ++i) {
            auto& raw_entry = raw_file_entries[i];
            const auto& source_entry = m_file_entries[i];

            raw_entry.fileSize = source_entry.serialized_data.size();
            raw_entry.assetsDataHash = source_entry.assetsDataHash;
            raw_entry.nameOffset = string_to_offset.at(source_entry.name) - (reinterpret_cast<char*>(&raw_entry.nameOffset) - buffer_base);
            raw_entry.contentStartOffset = content_starts[i] - (reinterpret_cast<char*>(&raw_entry.contentStartOffset) - buffer_base);

            memcpy(buffer_base + content_starts[i], source_entry.serialized_data.data(), source_entry.serialized_data.size());
        }

        size_t current_resource_write_offset = serialized_size;
        for (const auto& entry : m_file_entries) {
            if (!entry.resource_data.empty()) {
                current_resource_write_offset = align(current_resource_write_offset, 64);
                memcpy(buffer_base + current_resource_write_offset, entry.resource_data.data(), entry.resource_data.size());
                current_resource_write_offset += entry.resource_data.size();
            }
        }

        return out_buffer;
    }
}