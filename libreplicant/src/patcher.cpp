#include "replicant/patcher.h"
#include "replicant/bxon/archive_param.h"
#include <zstd.h>

namespace replicant::patcher {

    std::expected<std::vector<char>, PatcherError> patchIndex(
        const std::vector<char>& original_compressed_index,
        const std::string& new_arc_filename,
        const std::vector<archive::ArcWriter::Entry>& new_entries,
        bxon::ArchiveLoadType load_type)
    {
        unsigned long long const decompressed_size = ZSTD_getFrameContentSize(original_compressed_index.data(), original_compressed_index.size());
        if (decompressed_size == ZSTD_CONTENTSIZE_ERROR || decompressed_size == ZSTD_CONTENTSIZE_UNKNOWN || decompressed_size == 0) {
            return std::unexpected(PatcherError{ PatcherErrorCode::DecompressionError, "Could not determine a valid decompressed size from ZSTD stream." });
        }

        auto decompress_result = archive::decompress_zstd(original_compressed_index.data(), original_compressed_index.size(), decompressed_size);
        if (!decompress_result) {
            return std::unexpected(PatcherError{ PatcherErrorCode::DecompressionError, "ZSTD decompression failed: " + decompress_result.error().message });
        }
        std::vector<char> decompressed_data = std::move(*decompress_result);

        bxon::File bxon_file;
        if (!bxon_file.loadFromMemory(decompressed_data.data(), decompressed_data.size())) {
            return std::unexpected(PatcherError{ PatcherErrorCode::ParseError, "Failed to parse the decompressed index data as a BXON file." });
        }

        auto* param = std::get_if<bxon::ArchiveParameters>(&bxon_file.getAsset());
        if (!param) {
            return std::unexpected(PatcherError{ PatcherErrorCode::AssetTypeError, "Decompressed data is not a tpArchiveFileParam BXON." });
        }

        uint8_t new_archive_index = param->addArchive(new_arc_filename, load_type);

        for (const auto& mod_entry : new_entries) {
            auto* game_entry = param->findFile(mod_entry.key);
            if (game_entry) {
                game_entry->archiveIndex = new_archive_index;
                const auto& archive_entries = param->getArchiveEntries();
                uint32_t scale = archive_entries[new_archive_index].offsetScale;
                game_entry->arcOffset = mod_entry.offset >> scale;
                game_entry->compressedSize = mod_entry.compressed_size;
                game_entry->PackSerialisedSize = mod_entry.PackSerialisedSize;
                game_entry->assetsDataSize = mod_entry.assetsDataSize;
            }
            else {
                // Adding new entries is not yet supported 
                return std::unexpected(PatcherError{ PatcherErrorCode::EntryAddNotSupported, "Support for adding new file entries to an index is not implemented. Could not find: " + mod_entry.key });
            }
        }

        auto patched_data_result = param->build(bxon_file.getVersion(), bxon_file.getProjectID());
        if (!patched_data_result) {
            return std::unexpected(PatcherError{ PatcherErrorCode::BuildError, "Failed to build patched index: " + patched_data_result.error().message });
        }

        return std::move(*patched_data_result);
    }

}