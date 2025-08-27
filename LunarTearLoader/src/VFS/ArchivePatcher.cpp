#include "ArchivePatcher.h"
#include "Common/Logger.h"
#include "replicant/arc.h"
#include "replicant/bxon.h"
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <expected>

using enum Logger::LogCategory;

std::vector<ModArchive> g_patched_archive_mods;

namespace {

    std::expected<std::vector<char>, std::string> ReadFile(const std::filesystem::path& path) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file) {
            return std::unexpected("Could not open file: " + path.string());
        }
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        if (size <= 0) {
            return std::unexpected("File is empty or invalid: " + path.string());
        }
        std::vector<char> buffer(static_cast<size_t>(size));
        if (!file.read(buffer.data(), size)) {
            return std::unexpected("Failed to read all data from file: " + path.string());
        }
        return buffer;
    }

    bool WriteFile(const std::filesystem::path& path, const std::vector<char>& data) {
        std::ofstream out_file(path, std::ios::binary);
        if (!out_file) {
            Logger::Log(Error) << "Could not open output file for writing: " << path;
            return false;
        }
        out_file.write(data.data(), data.size());
        return true;
    }

} 

bool GeneratePatchedIndex() {
    try {
        if (g_patched_archive_mods.empty()) {
            Logger::Log(Verbose) << "No VFS mods detected. Skipping archive patching.";
            return true; 
        }

        auto original_index_data = ReadFile("data/info.arc");
        if (!original_index_data) {
            Logger::Log(Error) << "Failed to read original info.arc: " << original_index_data.error();
            return false;
        }


        auto decompressed_size = replicant::archive::get_decompressed_size_zstd(*original_index_data);
        if (!decompressed_size) {
            Logger::Log(Error) << "Failed to decompress original info.arc: " << decompressed_size.error().code << " - " << decompressed_size.error().message;
            return false;
        }

        auto decompressed_result = replicant::archive::decompress_zstd(original_index_data->data(), original_index_data->size(), *decompressed_size);
        if (!decompressed_result) {
            Logger::Log(Error) << "Failed to decompress original info.arc: " << decompressed_result.error().message;
            return false;
        }

        replicant::bxon::File base_bxon_file;
        if (!base_bxon_file.loadFromMemory(decompressed_result->data(), decompressed_result->size())) {
            Logger::Log(Error) << "Failed to parse original info.arc as BXON.";
            return false;
        }

        auto* main_param = std::get_if<replicant::bxon::ArchiveParameters>(&base_bxon_file.getAsset());
        if (!main_param) {
            Logger::Log(Error) << "Original info.arc is not a tpArchiveFileParam BXON.";
            return false;
        }

        std::sort(g_patched_archive_mods.begin(), g_patched_archive_mods.end(), [](const ModArchive& a, const ModArchive& b) {
            return a.id < b.id;
            });

        bool any_mod_was_valid = false;
        std::filesystem::path game_root_path = std::filesystem::current_path();

        for (auto& mod : g_patched_archive_mods) {
            auto mod_index_data = ReadFile(mod.indexPath);
            if (!mod_index_data) {
                Logger::Log(Warning) << "Could not read mod index '" << mod.indexPath << "'. Skipping.";
                continue;
            }

            auto mod_decompressed_size = replicant::archive::get_decompressed_size_zstd(*mod_index_data);
            if (!mod_decompressed_size) {
                Logger::Log(Error) << "Failed to decompress mod index '" << mod.indexPath << "'. Skipping:" << mod_decompressed_size.error().code << " - "
                                   << mod_decompressed_size.error().message;
                continue;
            }
            auto mod_decompressed_result = replicant::archive::decompress_zstd(mod_index_data->data(), mod_index_data->size(), *mod_decompressed_size);
            if (!mod_decompressed_result) {
                Logger::Log(Warning) << "Failed to decompress mod index '" << mod.indexPath << "'. Skipping:" << mod_decompressed_result.error().code 
                                     << " - " << mod_decompressed_result.error().message;
                continue;
            }

            replicant::bxon::File mod_bxon_file;
            if (!mod_bxon_file.loadFromMemory(mod_decompressed_result->data(), mod_decompressed_result->size())) {
                Logger::Log(Warning) << "Failed to parse mod index '" << mod.indexPath << "' as BXON. Skipping.";
                continue;
            }

            auto* mod_param = std::get_if<replicant::bxon::ArchiveParameters>(&mod_bxon_file.getAsset());
            if (!mod_param || mod_param->getArchiveEntries().empty()) {
                Logger::Log(Warning) << "Mod index for '" << mod.id << "' is invalid or contains no archive entries. Skipping.";
                continue;
            }

            const std::string mod_archive_name = mod_param->getArchiveEntries()[0].filename;
            std::filesystem::path mod_dir = std::filesystem::path(mod.indexPath).parent_path();
            std::filesystem::path full_archive_path = mod_dir / mod_archive_name;

            if (!std::filesystem::exists(full_archive_path)) {
                Logger::Log(Error) << "VFS Error for mod '" << mod.id << "': Index requires archive '" << mod_archive_name << "' but it was not found. Skipping.";
                continue;
            }

            any_mod_was_valid = true;
            std::string final_path_for_index = std::filesystem::relative(full_archive_path, game_root_path / "data").generic_string();
            uint8_t new_archive_index = main_param->addArchive(final_path_for_index, mod_param->getArchiveEntries()[0].loadType);

            for (const auto& mod_file_entry : mod_param->getFileEntries()) {
                auto* game_entry = main_param->findFile(mod_file_entry.filePath);
                if (game_entry) { 
                    Logger::Log(Verbose) << "Overwriting entry: " << mod_file_entry.filePath;
                    game_entry->archiveIndex = new_archive_index;
                    game_entry->arcOffset = mod_file_entry.arcOffset;
                    game_entry->compressedSize = mod_file_entry.compressedSize;
                    game_entry->PackSerialisedSize = mod_file_entry.PackSerialisedSize;
                    game_entry->assetsDataSize = mod_file_entry.assetsDataSize;
                }
                else {
                    Logger::Log(Verbose) << "Adding new entry: " << mod_file_entry.filePath;
                    replicant::bxon::FileEntry new_fe = mod_file_entry;
                    new_fe.archiveIndex = new_archive_index;
                    main_param->getFileEntries().push_back(new_fe);
                }
            }
        }

        if (!any_mod_was_valid) {
            Logger::Log(Verbose) << "No valid VFS mods were found after scanning. Skipping patch generation.";
            return true;
        }

        auto& all_file_entries = main_param->getFileEntries();
        std::sort(all_file_entries.begin(), all_file_entries.end(),
            [](const replicant::bxon::FileEntry& a, const replicant::bxon::FileEntry& b) {
                return a.pathHash < b.pathHash;
            }
        );

        auto patched_data_result = main_param->build(base_bxon_file.getVersion(), base_bxon_file.getProjectID());
        if (!patched_data_result) {
            Logger::Log(Error) << "Failed to build patched index BXON: " << patched_data_result.error().message;
            return false;
        }

        auto compressed_index_result = replicant::archive::compress_zstd(patched_data_result->data(), patched_data_result->size());
        if (!compressed_index_result) {
            Logger::Log(Error) << "Failed to compress patched index: " << compressed_index_result.error().message;
            return false;
        }

        std::filesystem::create_directories("LunarTear");
        if (!WriteFile("LunarTear/LunarTear.arc", *compressed_index_result)) {
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