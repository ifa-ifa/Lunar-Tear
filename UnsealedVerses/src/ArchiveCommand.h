#pragma once
#include "Common.h"
#include <filesystem>
#include <iostream>
#include <vector>
#include <string>
#include <optional>
#include "replicant/core/io.h"
#include "replicant/bxon.h"
#include "replicant/arc.h"
#include "replicant/tpArchiveFileParam.h"

// TODO: This command is extremely memory heavy
class ArchiveCommand : public Command {
private:
    std::optional<std::filesystem::path> m_index_new_path;
    std::optional<std::filesystem::path> m_index_patch_path;
    std::optional<std::filesystem::path> m_index_patch_out_path;
    replicant::ArchiveLoadType m_load_type = replicant::ArchiveLoadType::PRELOAD_DECOMPRESS;
    std::filesystem::path m_output_archive_path;
    std::vector<std::string> m_file_inputs;

    bool parse_archive_args() {
        if (m_args.empty()) {
            std::cerr << "Error: 'archive' command requires an output path.\n";
            return false;
        }

        m_output_archive_path = m_args[0];

        for (size_t i = 1; i < m_args.size(); ++i) {
            std::string arg = m_args[i];
            if (arg == "--index" && i + 1 < m_args.size()) {
                m_index_new_path = m_args[++i];
            }
            else if (arg == "--patch" && i + 1 < m_args.size()) {
                m_index_patch_path = m_args[++i];
            }
            else if (arg == "--out" && i + 1 < m_args.size()) {
                m_index_patch_out_path = m_args[++i];
            }
            else if (arg == "--load-type" && i + 1 < m_args.size()) {
                try {
                    int type = std::stoi(m_args[++i]);
                    if (type < 0 || type > 2) { std::cerr << "Error: Invalid load type.\n"; return false; }
                    m_load_type = static_cast<replicant::ArchiveLoadType>(type);
                }
                catch (...) { std::cerr << "Error: Invalid number for load type.\n"; return false; }
            }
            else {
                m_file_inputs.push_back(arg);
            }
        }

        if (m_index_new_path && m_index_patch_path) {
            std::cerr << "Error: Cannot use --index and --patch together.\n"; return false;
        }
        if (m_file_inputs.empty()) {
            std::cerr << "Error: No input files or folder provided for archive.\n"; return false;
        }
        if (m_index_patch_path && !m_index_patch_out_path) {
            std::cout << "Warning: --out not specified. The original patch file '" << m_index_patch_path->string() << "' will be overwritten.\n";
            m_index_patch_out_path = *m_index_patch_path;
        }
        return true;
    }

    void writeCompressedIndex(const std::filesystem::path& path, const std::vector<std::byte>& bxon_data) {

        auto compressed_data = unwrap(replicant::archive::Compress(bxon_data), "Failed to compress index file");
        unwrap(replicant::WriteFile(path, compressed_data), "Failed to write index file");
    }

    void handleNewIndex(const std::string& arc_filename, const std::vector<replicant::archive::ArchiveEntryInfo>& entries) {
        replicant::TpArchiveFileParam params;
        uint8_t arc_index = params.addArchiveEntry(arc_filename, m_load_type);
        params.registerArchive(arc_index, entries);

        std::vector<std::byte> payload = unwrap(params.Serialize(), "Failed to serialise new index");
        std::vector<std::byte> bxon_data = unwrap(replicant::BuildBxon("tpArchiveFileParam", 0x20090422, 0x54455354, payload), "Failed to serialise new index bxon");

        writeCompressedIndex(*m_index_new_path, bxon_data);
    }

    void handlePatchMode(const std::string& arc_filename, const std::vector<replicant::archive::ArchiveEntryInfo>& entries) {
        std::cout << "\nPatching original index: " << *m_index_patch_path << "\n";

        auto compressed_data = unwrap(replicant::ReadFile(*m_index_patch_path), "Failed to read index to patch");

        auto decompressed_size = unwrap(replicant::archive::GetDecompressedSize(compressed_data), "Failed to get decompressed size of index");
        auto bxon_data = unwrap(replicant::archive::Decompress(compressed_data, decompressed_size), "Failed to decompress index");

        auto [bxon_info, payload] = unwrap(replicant::ParseBxon(bxon_data), "Failed to parse index BXON");

        auto params = unwrap(replicant::TpArchiveFileParam::Deserialize(payload), "Failed to deserialize index payload");

        uint8_t arc_index = params.addArchiveEntry(arc_filename, m_load_type);
        params.registerArchive(arc_index, entries);

        std::vector<std::byte> new_payload = unwrap(params.Serialize(), "Failed to serialise new patched index");
        std::vector<std::byte> new_bxon_data = unwrap(replicant::BuildBxon(bxon_info.assetType, bxon_info.version, bxon_info.projectId, new_payload), "failed to build new index bxon");

        writeCompressedIndex(*m_index_patch_out_path, new_bxon_data);
    }

public:
    ArchiveCommand(std::vector<std::string> args) : Command(std::move(args)) {}
    int execute() override {
        if (!parse_archive_args()) {
            return 1;
        }

        std::vector<replicant::archive::ArchiveInput> inputs;
        std::cout << "Preparing files for archive: " << m_output_archive_path.string() << " \n";

        for (const auto& input_arg : m_file_inputs) {
            std::filesystem::path input_path(input_arg);

            // Helper lambda to process a single file
            auto processFile = [&](const std::filesystem::path& path, const std::string& key) {
                std::cout << "Adding '" << key << "' from '" << path.string() << "'...\n";

                auto fileData = unwrap(replicant::ReadFile(path), "Failed to read file " + path.string());

                uint32_t serSize = 0;
                uint32_t resSize = 0;


                if (fileData.size() >= 20 &&
                    fileData[0] == std::byte{ 'P' } && fileData[1] == std::byte{ 'A' } &&
                    fileData[2] == std::byte{ 'C' } && fileData[3] == std::byte{ 'K' })
                {
                    std::memcpy(&serSize, &fileData[12], 4);
                    std::memcpy(&resSize, &fileData[16], 4);
                }
                else
                {
                    // Warn the user if they are packing something that isn't a PACK file
                    std::cerr << "WARNING: File '" << key << "' does not appear to be a PACK file.\n";

                    // Fallback - Treat entire file as serialized data
                    serSize = static_cast<uint32_t>(fileData.size());
                }

                inputs.push_back({
                    .name = key,
                    .data = std::move(fileData),
                    .packSerializedSize = serSize,
                    .packResourceSize = resSize
                    });
                };

            if (std::filesystem::is_directory(input_path)) {
                std::cout << "Scanning directory '" << input_path.string() << "'...\n";
                for (const auto& entry : std::filesystem::recursive_directory_iterator(input_path)) {
                    if (entry.is_regular_file()) {
                        std::string key = std::filesystem::relative(entry.path(), input_path).generic_string();
                        std::replace(key.begin(), key.end(), '\\', '/'); // Standardize paths
                        processFile(entry.path(), key);
                    }
                }
            }
            else {
                // Handle single file argument (potentially with key=path syntax) 
                //TODO: Maybe just remove this
                size_t sep = input_arg.find('=');
                std::string key, path_str;
                if (sep == std::string::npos) {
                    key = std::filesystem::path(input_arg).filename().string();
                    path_str = input_arg;
                }
                else {
                    key = input_arg.substr(0, sep);
                    path_str = input_arg.substr(sep + 1);
                }
                processFile(path_str, key);
            }
        }

        if (inputs.empty()) {
            std::cerr << "Error: No valid input files were found to add to the archive.\n";
            return 1;
        }

        auto build_mode = (m_load_type == replicant::ArchiveLoadType::PRELOAD_DECOMPRESS)
            ? replicant::archive::BuildMode::SingleStream
            : replicant::archive::BuildMode::SeparateFrames;

        auto build_result = unwrap(replicant::archive::Build(inputs, build_mode), "Failed to build archive");

        unwrap(replicant::WriteFile(m_output_archive_path, build_result.arcData), "Failed to write archive file");
        std::cout << "Successfully wrote " << build_result.arcData.size() << " bytes to " << m_output_archive_path << ".\n";

        std::string arc_filename = m_output_archive_path.filename().string();
        if (m_index_new_path) {
            handleNewIndex(arc_filename, build_result.entries);
        }
        else if (m_index_patch_path) {
            handlePatchMode(arc_filename, build_result.entries);
        }

        return 0;
    }
};