#include "ArchivePatcher.h"
#include "Common/Logger.h"
#include "replicant/arc.h"
#include "replicant/bxon.h"

#include "replicant/core/io.h"
#include "replicant/tpArchiveFileParam.h"
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <expected>

using enum Logger::LogCategory;

namespace {

    uint32_t CalculateHash(std::string_view s) {
        uint32_t hash = 0x811c9dc5;
        for (char c : s) {
            hash *= 0x01000193;              
            hash ^= static_cast<uint8_t>(c); 
        }
        return hash;
    }
}

std::vector<ModArchive> g_patched_archive_mods;

bool GeneratePatchedIndex() {
    try {
        if (g_patched_archive_mods.empty()) {
            Logger::Log(Verbose) << "No VFS mods detected. Skipping archive patching.";
            return true; 
        }

        auto original_index_data = replicant::ReadFile("data/info.arc");
        if (!original_index_data) {
            Logger::Log(Error) << "Failed to read original info.arc: " << original_index_data.error().message;
            return false;
        }


        auto decompressed_size = replicant::archive::GetDecompressedSize(*original_index_data);
        if (!decompressed_size) {
            Logger::Log(Error) << "Failed to decompress original info.arc: " << decompressed_size.error().code << " - " << decompressed_size.error().message;
            return false;
        }

        auto decompressed_result = replicant::archive::Decompress(*original_index_data, *decompressed_size);
        if (!decompressed_result) {
            Logger::Log(Error) << "Failed to decompress original info.arc: " << decompressed_result.error().message;
            return false;
        }

        auto bxonRes = replicant::ParseBxon(*decompressed_result);
        if (!bxonRes) {
            Logger::Log(Error) << "Failed to parse info.arc BXON", bxonRes.error();
            return false;
        }

        auto& [headerInfo, payload] = *bxonRes;

		auto main_param = replicant::TpArchiveFileParam::Deserialize(payload);
        if (!main_param) {
            Logger::Log(Error) << "Failed to parse original info.arc as BXON: " << main_param.error().message;
            return false;
        }

        std::sort(g_patched_archive_mods.begin(), g_patched_archive_mods.end(), [](const ModArchive& a, const ModArchive& b) {
            return a.id < b.id;
            });

        bool any_mod_was_valid = false;
        std::filesystem::path game_root_path = std::filesystem::current_path();

        for (auto& mod : g_patched_archive_mods) {
            auto mod_index_data = replicant::ReadFile(mod.indexPath);
            if (!mod_index_data) {
                Logger::Log(Warning) << "Could not read mod index '" 
                    << mod.indexPath << "'. Skipping: " << mod_index_data.error().message;
                continue;
            }

            auto mod_decompressed_size = replicant::archive::GetDecompressedSize(*mod_index_data);
            if (!mod_decompressed_size) {
                Logger::Log(Error) << "Failed to decompress mod index '" << mod.indexPath 
                    << "'. Skipping:" << mod_decompressed_size.error().code << " - "
                    << mod_decompressed_size.error().message;
                continue;
            }
            auto mod_decompressed_result = replicant::archive::Decompress(*mod_index_data, *mod_decompressed_size);
            if (!mod_decompressed_result) {
                Logger::Log(Warning) << "Failed to decompress mod index '" << mod.indexPath << "'. Skipping:" 
                    << mod_decompressed_result.error().code 
                    << " - " << mod_decompressed_result.error().message;
                continue;
            }

            auto mod_bxonRes = replicant::ParseBxon(*mod_decompressed_result);
            if (!mod_bxonRes) {
                Logger::Log(Warning) << "Failed to parse mod index '" << mod.indexPath
                    << "' as BXON. Skipping: " << mod_bxonRes.error().message;
                continue;
            }
            auto& [modHeader, modPayload] = *mod_bxonRes;

            auto mod_param = replicant::TpArchiveFileParam::Deserialize(modPayload);
            if (!mod_param) {
                Logger::Log(Warning) << "Failed to parse mod index '" << mod.indexPath 
                    << "' as BXON. Skipping: " << mod_param.error().message;
                continue;
            }
  
            const std::string mod_archive_name = mod_param->archiveEntries[0].filename;
            std::filesystem::path mod_dir = std::filesystem::path(mod.indexPath).parent_path();
            std::filesystem::path full_archive_path = mod_dir / mod_archive_name;

            if (!std::filesystem::exists(full_archive_path)) {
                Logger::Log(Error) << "VFS Error for mod '" << mod.id << "': Index requires archive '" << mod_archive_name << "' but it was not found. Skipping.";
                continue;
            }

            any_mod_was_valid = true;
            std::string final_path_for_index = std::filesystem::relative(full_archive_path, game_root_path / "data").generic_string();
            uint8_t new_archive_index = main_param->addArchiveEntry(final_path_for_index, mod_param->archiveEntries[0].loadType);
            main_param->archiveEntries[new_archive_index].arcOffsetScale = mod_param->archiveEntries[0].arcOffsetScale;

            for (const auto& mod_file_entry : mod_param->fileEntries) {
                auto* game_entry = main_param->findFile(mod_file_entry.name);
                if (game_entry) { 
                    Logger::Log(Verbose) << "Overwriting entry: " << mod_file_entry.name;
                    game_entry->archiveIndex = new_archive_index;
                    game_entry->rawOffset = mod_file_entry.rawOffset;
                    game_entry->size = mod_file_entry.size;
                    game_entry->packFileSerializedSize = mod_file_entry.packFileSerializedSize;
                    game_entry->packFileResourceSize = mod_file_entry.packFileResourceSize;
                }
                else {
                    Logger::Log(Verbose) << "Adding new entry: " << mod_file_entry.name;
                    replicant::FileEntry new_fe = mod_file_entry;
                    new_fe.archiveIndex = new_archive_index;
                    main_param->fileEntries.push_back(new_fe);
                }
            }
        }

        if (!any_mod_was_valid) {
            Logger::Log(Verbose) << "No valid VFS mods were found after scanning. Skipping patch generation.";
            return false;
        }

        auto& all_file_entries = main_param->fileEntries;
        std::sort(all_file_entries.begin(), all_file_entries.end(),
            [](const replicant::FileEntry& a, const replicant::FileEntry& b) {
                return CalculateHash(a.name) < CalculateHash(b.name);
            }
        );

        auto newPayloadRes = (*main_param).Serialize();
        if (!newPayloadRes) {
            Logger::Log(Error) << "Failed to serialize patched index: " << newPayloadRes.error().message;
            return false;
		}
		std::vector<std::byte> newPayload = *newPayloadRes;

        auto newBxonRes = replicant::BuildBxon(
            headerInfo.assetType,
            headerInfo.version,
            headerInfo.projectId,
            newPayload
        );
        if (!newBxonRes) {
            Logger::Log(Error) << "Failed to build patched index BXON: " << newBxonRes.error().message;
            return false;
		}

		std::vector<std::byte> newBxon = *newBxonRes;

        auto compressed_index_result = replicant::archive::Compress(newBxon);
        if (!compressed_index_result) {
            Logger::Log(Error) << "Failed to compress patched index: " << compressed_index_result.error().message;
            return false;
        }

        std::filesystem::create_directories("LunarTear");
        auto writeRes = replicant::WriteFile("LunarTear/LunarTear.arc", *compressed_index_result);
        if (!writeRes) {
            Logger::Log(Error) << "Failed to write output file" << writeRes.error().message;
            return false;
        }


        Logger::Log(Verbose) << "Successfully built sorted and patched LunarTear.arc.";
        return true;
    }
    catch (const std::exception& e) {
        Logger::Log(Error) << "An exception occurred during archive patching: " << e.what();
        return false;
    }
}