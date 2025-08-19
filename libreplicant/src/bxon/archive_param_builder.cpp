#include "replicant/bxon/archive_param_builder.h"
#include "replicant/bxon/archive_param.h"
#include <vector>
#include <string>
#include <map>
#include <set>
#include <cstring>
#include <numeric>

namespace {
    // Correct FNV-1 hash
    uint32_t fnv1_32(const std::string& s) {
        uint32_t hash = 0x811c9dc5;
        for (char c : s) {
            hash *= 0x01000193; // Multiply first
            hash ^= static_cast<uint8_t>(c); // XOR second
        }
        return hash;
    }

    // Generic alignment helper function
    size_t align_to(size_t value, size_t alignment) {
        return (value + alignment - 1) & ~(alignment - 1);
    }

    // Aligns a size up to the nearest multiple of 16
    uint32_t align16(uint32_t size) {
        return (size + 15) & ~15;
    }

#pragma pack(push, 1)
    struct RawBxonHeader {
        char     magic[4];
        uint32_t version;
        uint32_t projectID;
        uint32_t offsetToAssetTypeName;
        uint32_t offsetToAssetTypeData;
    };
    struct RawArchiveEntry {
        uint32_t offsetToFilename;
        uint32_t unknown_1;
        replicant::bxon::ArchiveLoadType loadType;
        uint8_t  padding[3];
    };
    struct RawFileEntry {
        uint32_t pathHash;
        uint32_t nameOffset;
        uint32_t arcOffset;
        uint32_t compressedSize;
        uint32_t decompressedSize;
        uint32_t bufferSize;
        uint8_t  archiveIndex;
        uint8_t  flags;
        uint8_t  padding[2];
    };
    struct RawArchiveFileParam {
        uint32_t numArchives;
        uint32_t offsetToArray;
        uint32_t fileEntryCount;
        uint32_t offsetToFileTable;
    };
#pragma pack(pop)

    static_assert(sizeof(RawBxonHeader) == 20, "RawBxonHeader size must be 20 bytes");
    static_assert(sizeof(RawArchiveFileParam) == 16, "RawArchiveFileParam size must be 16 bytes");
    static_assert(sizeof(RawArchiveEntry) == 12, "RawArchiveEntry size must be 12 bytes");
    static_assert(sizeof(RawFileEntry) == 28, "RawFileEntry size must be 28 bytes");

} // namespace

namespace replicant::bxon {

    // FROM SCRATCH BUILDER
    std::expected<std::vector<char>, BxonError> buildArchiveParam(
        const std::vector<archive::ArcWriter::Entry>& entries,
        uint8_t archive_index,
        const std::string& archive_filename,
        ArchiveLoadType load_type) // Add new parameter
    {
        ArchiveFileParam param;
        param.addArchive(archive_filename, load_type); // Use the load_type here
        for (const auto& entry : entries) {
            FileEntry fe;
            fe.filePath = entry.key;
            fe.arcOffset = entry.offset / 16;
            fe.compressedSize = entry.compressed_size;
            fe.decompressedSize = entry.uncompressed_size;
            fe.bufferSize = align16(entry.uncompressed_size);
            fe.archiveIndex = archive_index;
            param.getFileEntries().push_back(fe);
        }
        return buildArchiveParam(param, 0x20090422, 0x54455354); // "TEST"
    }



    // FROM EXISTING DATA BUILDER
    std::expected<std::vector<char>, BxonError> buildArchiveParam(
        const ArchiveFileParam& param,
        uint32_t version,
        uint32_t projectID)
    {
        const std::string asset_type_name = "tpArchiveFileParam";
        constexpr size_t ALIGNMENT = 16;

        // Step 1: Prepare the ASSET's string pool
        std::set<std::string> asset_string_set;
        for (const auto& arc : param.getArchiveEntries()) { asset_string_set.insert(arc.filename); }
        for (const auto& file : param.getFileEntries()) { asset_string_set.insert(file.filePath); }
        const std::vector<std::string> asset_string_pool(asset_string_set.begin(), asset_string_set.end());
        size_t asset_string_pool_size = 0;
        for (const auto& s : asset_string_pool) { asset_string_pool_size += s.length() + 1; }

        // Step 2: Define the file layout
        const uint32_t header_start = 0;
        const uint32_t asset_type_name_start = align_to(header_start + sizeof(RawBxonHeader), ALIGNMENT);
        const uint32_t asset_param_start = align_to(asset_type_name_start + asset_type_name.length() + 1, ALIGNMENT);
        const uint32_t archive_array_start = asset_param_start + sizeof(RawArchiveFileParam);
        const uint32_t file_table_start = archive_array_start + (sizeof(RawArchiveEntry) * param.getArchiveEntries().size());
        const uint32_t asset_string_pool_start = file_table_start + (sizeof(RawFileEntry) * param.getFileEntries().size());

        const size_t total_size = asset_string_pool_start + asset_string_pool_size;
        std::vector<char> buffer(total_size, 0);

        // Step 3: Populate the asset string offset map
        std::map<std::string, uint32_t> asset_string_absolute_offsets;
        uint32_t current_string_offset = asset_string_pool_start;
        for (const auto& s : asset_string_pool) {
            asset_string_absolute_offsets[s] = current_string_offset;
            current_string_offset += s.length() + 1;
        }

        // Step 4: Write all data blocks
        auto* header = reinterpret_cast<RawBxonHeader*>(buffer.data() + header_start);
        memcpy(header->magic, "BXON", 4);
        header->version = version;
        header->projectID = projectID;
        uint32_t field_pos_asset_type_name = header_start + offsetof(RawBxonHeader, offsetToAssetTypeName);
        header->offsetToAssetTypeName = asset_type_name_start - field_pos_asset_type_name;
        uint32_t field_pos_asset_data = header_start + offsetof(RawBxonHeader, offsetToAssetTypeData);
        header->offsetToAssetTypeData = asset_param_start - field_pos_asset_data;

        memcpy(buffer.data() + asset_type_name_start, asset_type_name.c_str(), asset_type_name.length() + 1);

        auto* asset_header = reinterpret_cast<RawArchiveFileParam*>(buffer.data() + asset_param_start);
        asset_header->numArchives = param.getArchiveEntries().size();
        uint32_t field_pos_offset_to_array = asset_param_start + offsetof(RawArchiveFileParam, offsetToArray);
        asset_header->offsetToArray = archive_array_start - field_pos_offset_to_array;
        asset_header->fileEntryCount = param.getFileEntries().size();
        uint32_t field_pos_offset_to_table = asset_param_start + offsetof(RawArchiveFileParam, offsetToFileTable);
        asset_header->offsetToFileTable = file_table_start - field_pos_offset_to_table;

        for (size_t i = 0; i < param.getArchiveEntries().size(); ++i) {
            const auto& entry = param.getArchiveEntries()[i];
            uint32_t current_arc_entry_start = archive_array_start + (i * sizeof(RawArchiveEntry));
            auto* arc_entry_ptr = reinterpret_cast<RawArchiveEntry*>(buffer.data() + current_arc_entry_start);
            arc_entry_ptr->loadType = entry.loadType;
            arc_entry_ptr->unknown_1 = entry.unknown_1;
            uint32_t field_pos_filename = current_arc_entry_start + offsetof(RawArchiveEntry, offsetToFilename);
            arc_entry_ptr->offsetToFilename = asset_string_absolute_offsets.at(entry.filename) - field_pos_filename;
        }

        for (size_t i = 0; i < param.getFileEntries().size(); ++i) {
            const auto& entry = param.getFileEntries()[i];
            uint32_t current_file_entry_start = file_table_start + (i * sizeof(RawFileEntry));
            auto* file_entry_ptr = reinterpret_cast<RawFileEntry*>(buffer.data() + current_file_entry_start);
            file_entry_ptr->pathHash = fnv1_32(entry.filePath);
            file_entry_ptr->arcOffset = entry.arcOffset;
            file_entry_ptr->compressedSize = entry.compressedSize;
            file_entry_ptr->decompressedSize = entry.decompressedSize;
            file_entry_ptr->bufferSize = entry.bufferSize; // This should already be aligned
            file_entry_ptr->archiveIndex = entry.archiveIndex;
            file_entry_ptr->flags = entry.flags;
            uint32_t field_pos_name_offset = current_file_entry_start + offsetof(RawFileEntry, nameOffset);
            file_entry_ptr->nameOffset = asset_string_absolute_offsets.at(entry.filePath) - field_pos_name_offset;
        }

        for (const auto& [str, abs_offset] : asset_string_absolute_offsets) {
            memcpy(buffer.data() + abs_offset, str.c_str(), str.length() + 1);
        }

        return buffer;
    }
}