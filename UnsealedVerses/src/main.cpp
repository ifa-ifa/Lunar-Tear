#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <iomanip>
#include <optional>
#include <filesystem>
#include <memory>
#include <algorithm>
#include <zstd.h>
#include "replicant/arc/arc.h"
#include "replicant/bxon/archive_param_builder.h"
#include "replicant/bxon.h"
#include "replicant/pack.h"
#include "replicant/bxon/texture_builder.h"

namespace util {
    std::expected<std::vector<char>, std::string> readFile(const std::filesystem::path& path) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file) {
            return std::unexpected("Could not open file: " + path.string());
        }
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        if (size <= 0) {
            return std::unexpected("File is empty or invalid: " + path.string());
        }
        std::vector<char> buffer(size);
        if (!file.read(buffer.data(), size)) {
            return std::unexpected("Failed to read all data from file: " + path.string());
        }
        return buffer;
    }

    bool writeFile(const std::filesystem::path& path, const std::vector<char>& data) {
        std::ofstream out_file(path, std::ios::binary);
        if (!out_file) {
            std::cerr << "Error: Could not open output file for writing: " << path << "\n";
            return false;
        }
        out_file.write(data.data(), data.size());
        std::cout << "Successfully wrote " << data.size() << " bytes to " << path << ".\n";
        return true;
    }

    uint32_t align16(uint32_t size) {
        return (size + 15) & ~15;
    }
}

class Command {
public:
    virtual ~Command() = default;
    virtual int execute() = 0; 
protected:
    Command(std::vector<std::string> args) : m_args(std::move(args)) {}
    std::vector<std::string> m_args;
};

class TextureCommand : public Command {
public:
    TextureCommand(std::vector<std::string> args) : Command(std::move(args)) {}
    int execute() override {
        if (m_args.size() != 2) {
            std::cerr << "Error: Texture mode requires <output.rtex> <input.dds>\n";
            return 1;
        }
        const std::filesystem::path output_path(m_args[0]);
        const std::filesystem::path input_path(m_args[1]);

        std::cout << "Converting DDS to BXON Texture\n";
        std::cout << "Input:  " << input_path << "\n";
        std::cout << "Output: " << output_path << "\n";

        auto dds_data = util::readFile(input_path);
        if (!dds_data) {
            std::cerr << "Error: " << dds_data.error() << "\n";
            return 1;
        }

        auto bxon_result = replicant::bxon::buildTextureFromDDS(*dds_data);
        if (!bxon_result) {
            std::cerr << "Error building BXON texture: " << bxon_result.error().message << "\n";
            return 1;
        }

        return util::writeFile(output_path, *bxon_result) ? 0 : 1;
    }
};

class PackPatchCommand : public Command {
public:
    PackPatchCommand(std::vector<std::string> args) : Command(std::move(args)) {}
    int execute() override {
        if (m_args.size() != 3) {
            std::cerr << "Error: Pack patch mode requires <output.xap> <input.xap> <dds_folder>\n";
            return 1;
        }
        const std::string& output_path = m_args[0];
        const std::string& input_path = m_args[1];
        const std::filesystem::path dds_folder_path(m_args[2]);

        std::cout << "Patching PACK file\n";
        std::cout << "Input PACK:  " << input_path << "\n";
        std::cout << "DDS Folder:  " << dds_folder_path << "\n";
        std::cout << "Output PACK: " << output_path << "\n\n";

        if (!std::filesystem::is_directory(dds_folder_path)) {
            std::cerr << "Error: Provided path for patching is not a directory: " << dds_folder_path.string() << "\n";
            return 1;
        }

        auto pack_data = util::readFile(input_path);
        if (!pack_data) {
            std::cerr << "Error: " << pack_data.error() << "\n";
            return 1;
        }

        replicant::pack::PackFile pack_file;
        if (auto res = pack_file.loadFromMemory(pack_data->data(), pack_data->size()); !res) {
            std::cerr << "Error parsing PACK file: " << res.error().message << "\n";
            return 1;
        }
        std::cout << "Successfully loaded and parsed '" << input_path << "'.\n";

        int patch_count = 0;
        for (const auto& dir_entry : std::filesystem::directory_iterator(dds_folder_path)) {
            if (dir_entry.is_regular_file() && (dir_entry.path().extension() == ".dds" || dir_entry.path().extension() == ".DDS")) {
                const auto& dds_path = dir_entry.path();
                std::string entry_name = dds_path.stem().string() + ".rtex";

                std::cout << "Attempting to patch '" << entry_name << "' with '" << dds_path.filename().string() << "'...\n";

                auto dds_data = util::readFile(dds_path.string());
                if (!dds_data) {
                    std::cerr << "  - Error reading DDS file: " << dds_data.error() << "\n";
                    continue;
                }

                auto patch_result = pack_file.patchFile(entry_name, *dds_data);
                if (!patch_result) {
                    if (patch_result.error().code == replicant::pack::PackErrorCode::EntryNotFound) {
                        std::cout << "  - Entry not found in PACK. Skipping.\n";
                    }
                    else {
                        std::cerr << "  - Failed to apply patch: " << patch_result.error().message << "\n";
                    }
                }
                else {
                    std::cout << "  + Successfully patched.\n";
                    patch_count++;
                }
            }
        }

        if (patch_count == 0) {
            std::cout << "\nNo matching textures were found to patch. Output file will be identical to input.\n";
        }

        std::cout << "\nRebuilding PACK file...\n";
        auto build_result = pack_file.build();
        if (!build_result) {
            std::cerr << "Error rebuilding PACK file: " << build_result.error().message << "\n";
            return 1;
        }

        return util::writeFile(output_path, *build_result) ? 0 : 1;
    }
};

class ArchiveCommand : public Command {
private:
    std::optional<std::filesystem::path> index_new_path;
    std::optional<std::filesystem::path> index_patch_path;
    std::optional<std::filesystem::path> index_patch_out_path;
    replicant::bxon::ArchiveLoadType load_type = replicant::bxon::ArchiveLoadType::PRELOAD_DECOMPRESS;
    std::filesystem::path output_archive_path;
    std::vector<std::string> file_inputs;

    bool parse_archive_args() {
        if (m_args.empty()) {
            std::cerr << "Error: 'archive' command requires an output path.\n";
            return false;
        }

        output_archive_path = m_args[0];

        for (size_t i = 1; i < m_args.size(); ++i) {
            std::string arg = m_args[i];
            if (arg == "--index" && i + 1 < m_args.size()) {
                index_new_path = m_args[++i];
            }
            else if (arg == "--patch" && i + 1 < m_args.size()) {
                index_patch_path = m_args[++i];
            }
            else if (arg == "--out" && i + 1 < m_args.size()) {
                index_patch_out_path = m_args[++i];
            }
            else if (arg == "--load-type" && i + 1 < m_args.size()) {
                try {
                    int type = std::stoi(m_args[++i]);
                    if (type < 0 || type > 2) { std::cerr << "Error: Invalid load type.\n"; return false; }
                    load_type = static_cast<replicant::bxon::ArchiveLoadType>(type);
                }
                catch (...) { std::cerr << "Error: Invalid number for load type.\n"; return false; }
            }
            else {
                file_inputs.push_back(arg);
            }
        }

        if (index_new_path && index_patch_path) {
            std::cerr << "Error: Cannot use --index and --patch together.\n";
            return false;
        }
        if (file_inputs.empty()) {
            std::cerr << "Error: No input files or folder provided for archive.\n";
            return false;
        }
        if (index_patch_path && !index_patch_out_path) {
            std::cout << "Warning: --out not specified. The original patch file '" << index_patch_path->string() << "' will be overwritten.\n";
            index_patch_out_path = *index_patch_path;
        }
        return true;
    }

    std::expected<std::vector<char>, std::string> loadAndDecompressIndex(const std::filesystem::path& path) {
        auto read_result = util::readFile(path);
        if (!read_result) {
            return std::unexpected(read_result.error());
        }
        const auto& compressed_data = *read_result;

        unsigned long long const decompressed_size = ZSTD_getFrameContentSize(compressed_data.data(), compressed_data.size());
        if (decompressed_size == ZSTD_CONTENTSIZE_ERROR) {
            return std::unexpected("File is not a valid ZSTD stream: " + path.string());
        }
        if (decompressed_size == ZSTD_CONTENTSIZE_UNKNOWN || decompressed_size == 0) {
            return std::unexpected("Could not determine a valid decompressed size from ZSTD stream: " + path.string());
        }
        auto decompress_result = replicant::archive::decompress_zstd(compressed_data.data(), compressed_data.size(), decompressed_size);
        if (!decompress_result) {
            return std::unexpected("Zstd decompression failed: " + decompress_result.error().message);
        }
        return *decompress_result;
    }

    bool writeCompressedIndex(const std::filesystem::path& path, std::vector<char>& bxon_data) {
        size_t original_size = bxon_data.size();
        size_t padded_size = util::align16(original_size);
        if (padded_size > original_size) {
            bxon_data.resize(padded_size, 0);
        }
        auto compressed_result = replicant::archive::compress_zstd(bxon_data.data(), bxon_data.size());
        if (!compressed_result) {
            std::cerr << "Error compressing index: " << compressed_result.error().message << "\n";
            return false;
        }
        return util::writeFile(path, *compressed_result);
    }

    int handleNewIndex(const std::string& arc_filename, const std::vector<replicant::archive::ArcWriter::Entry>& entries) {
        auto res = replicant::bxon::buildArchiveParam(entries, 0, arc_filename, load_type);
        if (!res) {
            std::cerr << "Error building new index: " << res.error().message << "\n";
            return 1;
        }
        std::vector<char> bxon_data = std::move(*res);
        return writeCompressedIndex(*index_new_path, bxon_data) ? 0 : 1;
    }

    int handlePatchMode(const std::string& arc_filename, const std::vector<replicant::archive::ArcWriter::Entry>& entries) {
        std::cout << "\n Patching original index: " << *index_patch_path << "\n";
        auto decompressed_result = loadAndDecompressIndex(*index_patch_path);
        if (!decompressed_result) {
            std::cerr << "Error: " << decompressed_result.error() << "\n";
            return 1;
        }
        std::vector<char> decompressed_data = std::move(*decompressed_result);
        std::cout << "Successfully decompressed original index (" << decompressed_data.size() << " bytes).\n";

        replicant::bxon::File bxon_file;
        if (!bxon_file.loadFromMemory(decompressed_data.data(), decompressed_data.size())) {
            std::cerr << "Error: Failed to parse the decompressed index data as a BXON file.\n";
            return 1;
        }

        auto* param = std::get_if<replicant::bxon::ArchiveFileParam>(&bxon_file.getAsset());
        if (!param) {
            std::cerr << "Error: Decompressed data from '" << *index_patch_path << "' is not a tpArchiveFileParam BXON.\n";
            return 1;
        }

        uint8_t new_archive_index = param->addArchive(arc_filename, load_type);
        std::cout << "Using archive '" << arc_filename << "' at index " << (int)new_archive_index << " with LoadType " << (int)load_type << ".\n";

        for (const auto& mod_entry : entries) {
            auto* game_entry = param->findFileEntryMutable(mod_entry.key);
            if (game_entry) {
                std::cout << "Patching entry: " << mod_entry.key << "\n";
                game_entry->archiveIndex = new_archive_index;

                const auto& archive_entries = param->getArchiveEntries();
                uint32_t scale = archive_entries[new_archive_index].offsetScale;
                game_entry->arcOffset = mod_entry.offset >> scale;

                game_entry->compressedSize = mod_entry.compressed_size;
                game_entry->everythingExceptAssetsDataSize = mod_entry.everythingExceptAssetsDataSize;
                game_entry->assetsDataSize = mod_entry.assetsDataSize;
            }
            else {
                std::cerr << "Error: Support for adding new file entries to an index is not implemented. (Did you misspell the asset path '" << mod_entry.key << "'?)\n";
                return 1;
            }
        }

        auto patched_data_result = replicant::bxon::buildArchiveParam(*param, bxon_file.getVersion(), bxon_file.getProjectID());
        if (!patched_data_result) {
            std::cerr << "Error building patched index: " << patched_data_result.error().message << "\n";
            return 1;
        }
        std::vector<char> bxon_data = std::move(*patched_data_result);
        return writeCompressedIndex(*index_patch_out_path, bxon_data) ? 0 : 1;
    }

public:
    ArchiveCommand(std::vector<std::string> args) : Command(std::move(args)) {}
    int execute() override {
        if (!parse_archive_args()) return 1;

        replicant::archive::ArcWriter writer;
        std::cout << "--- Preparing files for archive: " << output_archive_path.string() << " ---\n";

        for (const auto& input_arg : file_inputs) {
            std::filesystem::path input_path(input_arg);

            if (std::filesystem::is_directory(input_path)) {
                // --- FOLDER SCANNING LOGIC ---
                std::cout << "Scanning directory '" << input_path.string() << "'...\n";
                for (const auto& entry : std::filesystem::recursive_directory_iterator(input_path)) {
                    if (entry.is_regular_file()) {
                        const auto& file_path = entry.path();
                        std::filesystem::path relative_path = std::filesystem::relative(file_path, input_path);
                        std::string key = relative_path.generic_string();
                        std::replace(key.begin(), key.end(), '\\', '/');
                        std::cout << "Adding '" << key << "' from '" << file_path.string() << "'...\n";
                        auto res = writer.addFileFromDisk(key, file_path.string());
                        if (!res) {
                            std::cerr << "  - FAILED to add file. Reason: " << res.error().message << "\n";
                            return 1;
                        }
                    }
                }
            }
            else {
                // --- FILE-BY-FILE LOGIC ---
                size_t sep = input_arg.find('=');
                std::string key = (sep == std::string::npos) ? input_arg : input_arg.substr(0, sep);
                std::filesystem::path path((sep == std::string::npos) ? input_arg : input_arg.substr(sep + 1));

                std::cout << "Adding '" << key << "' from '" << path.string() << "'...\n";
                if (auto res = writer.addFileFromDisk(key, path.string()); !res) {
                    std::cerr << "Error: " << res.error().message << "\n";
                    return 1;
                }
            }
        }

        if (writer.getPendingEntryCount() == 0) {
            std::cerr << "Error: No valid input files were found to add to the archive.\n";
            return 1;
        }

        auto build_mode = (load_type == replicant::bxon::ArchiveLoadType::PRELOAD_DECOMPRESS)
            ? replicant::archive::ArcBuildMode::SingleStream
            : replicant::archive::ArcBuildMode::ConcatenatedFrames;

        auto build_result = writer.build(build_mode);
        if (!build_result) {
            std::cerr << "Error building archive: " << build_result.error().message << "\n";
            return 1;
        }

        if (!util::writeFile(output_archive_path, *build_result)) return 1;

        std::string arc_filename = output_archive_path.filename().string();
        if (index_new_path) {
            return handleNewIndex(arc_filename, writer.getEntries());
        }
        else if (index_patch_path) {
            return handlePatchMode(arc_filename, writer.getEntries());
        }

        return 0;
    }
};

void printUsage() {
    std::cout << "Usage: UnsealedVerses <command> [options] <args...>\n\n";
    std::cout << "COMMANDS:\n";
    std::cout << "  archive <output.arc> <inputs...> [options]   Build a game archive.\n";
    std::cout << "    Inputs: <key=filepath>... OR a single <folder_path> to scan for .xap files.\n"; 
    std::cout << "    Options:\n";
    std::cout << "      --index <path>      Generate a new index file.\n";
    std::cout << "      --patch <path>      Patch an existing index file.\n";
    std::cout << "      --out <path>        Output for patched index. Overwrites original if not set.\n";
    std::cout << "      --load-type <0|1|2> Set load type (0=Preload, 1=Stream, 2=StreamSpecial).\n\n";
    std::cout << "  texture <output.rtex> <input.dds>             Convert a DDS texture.\n\n";
    std::cout << "  pack-patch <out.xap> <in.xap> <dds_folder>    Patch textures in a PACK file.\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage();
        return 1;
    }

    std::string command_name = argv[1];
    std::vector<std::string> command_args(argv + 2, argv + argc);
    std::unique_ptr<Command> command;

    if (command_name == "archive") {
        command = std::make_unique<ArchiveCommand>(command_args);
    }
    else if (command_name == "texture") {
        command = std::make_unique<TextureCommand>(command_args);
    }
    else if (command_name == "pack-patch") {
        command = std::make_unique<PackPatchCommand>(command_args);
    }
    else {
        std::cerr << "Error: Unknown command '" << command_name << "'\n\n";
        printUsage();
        return 1;
    }

    try {
        return command->execute();
    }
    catch (const std::exception& e) {
        std::cerr << "An unexpected error occurred: " << e.what() << "\n";
        return 1;
    }
}