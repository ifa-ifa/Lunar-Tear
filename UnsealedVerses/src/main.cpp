#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <iomanip>
#include <optional>
#include <filesystem>
#include <memory>
#include <algorithm>
#include "replicant/patcher.h"
#include "replicant/arc.h"
#include "replicant/bxon.h"
#include "replicant/pack.h"
#include "replicant/dds.h"

namespace util {
    std::expected<std::vector<char>, std::string> readFile(const std::filesystem::path& path) {
        if (!std::filesystem::exists(path)) {
            return std::unexpected("File not found: " + path.string());
        }
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

    size_t align16(size_t size) {
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

        auto dds_file_result = replicant::dds::DDSFile::FromMemory({ dds_data->data(), dds_data->size() });
        if (!dds_file_result) {
            std::cerr << "Error parsing DDS file: " << dds_file_result.error().message << "\n";
            return 1;
        }

        auto texture_result = replicant::bxon::Texture::FromDDS(*dds_file_result);
        if (!texture_result) {
            std::cerr << "Error creating texture from DDS: " << texture_result.error().message << "\n";
            return 1;
        }

        auto bxon_result = texture_result->build(3, 0x2ea74106); // Using version and projectID from game
        if (!bxon_result) {
            std::cerr << "Error building BXON texture: " << bxon_result.error().message << "\n";
            return 1;
        }

        return util::writeFile(output_path, *bxon_result) ? 0 : 1;
    }
};


class FindEntryCommand : public Command {
public:
    FindEntryCommand(std::vector<std::string> args) : Command(std::move(args)) {}
    int execute() override {
        if (m_args.size() != 2) {
            std::cerr << "Error: find-entry mode requires <search_folder> <entry_name_to_find>\n";
            return 1;
        }

        const std::filesystem::path search_path(m_args[0]);
        const std::string entry_name = m_args[1];

        if (!std::filesystem::is_directory(search_path)) {
            std::cerr << "Error: Provided path is not a directory: " << search_path << "\n";
            return 1;
        }

        std::cout << "Searching for entry '" << entry_name << "' in directory '" << search_path.string() << "'...\n";
        int found_count = 0;

        for (const auto& dir_entry : std::filesystem::recursive_directory_iterator(search_path)) {
            if (dir_entry.is_regular_file()) {
                const auto& current_file_path = dir_entry.path();

                std::ifstream file_stream(current_file_path, std::ios::binary);
                if (!file_stream) {
                    continue;
                }

                replicant::pack::PackFile pack_file;
                auto load_result = pack_file.loadHeaderAndEntriesOnly(file_stream);
                if (!load_result) {
                    continue;
                }

                for (const auto& entry : pack_file.getFileEntries()) {
                    if (entry.name == entry_name) {
                        std::filesystem::path relative_path = std::filesystem::relative(current_file_path, search_path);
                        std::cout << "Found in: " << relative_path.generic_string() << "\n";
                        found_count++;
                        break;
                    }
                }
            }
        }

        std::cout << "Search complete. Found " << found_count << " occurrence(s).\n";
        return 0;
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
        const std::filesystem::path output_path(m_args[0]);
        const std::filesystem::path input_path(m_args[1]);
        const std::filesystem::path dds_folder_path(m_args[2]);

        std::cout << "Patching PACK file\n";
        std::cout << "Input PACK:  " << input_path << "\n";
        std::cout << "DDS Folder:  " << dds_folder_path << "\n";
        std::cout << "Output PACK: " << output_path << "\n\n";

        if (!std::filesystem::is_directory(dds_folder_path)) {
            std::cerr << "Error: Provided path for patching is not a directory: " << dds_folder_path << "\n";
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
        std::cout << "Successfully loaded and parsed '" << input_path.string() << "'.\n";

        int patch_count = 0;
        for (const auto& dir_entry : std::filesystem::directory_iterator(dds_folder_path)) {
            if (dir_entry.is_regular_file() && (dir_entry.path().extension() == ".dds" || dir_entry.path().extension() == ".DDS")) {
                const auto& dds_path = dir_entry.path();
                std::string entry_name = dds_path.stem().string() + ".rtex";

                std::cout << "Attempting to patch '" << entry_name << "' with '" << dds_path.filename().string() << "'...\n";

                auto dds_data = util::readFile(dds_path);
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
    std::optional<std::filesystem::path> m_index_new_path;
    std::optional<std::filesystem::path> m_index_patch_path;
    std::optional<std::filesystem::path> m_index_patch_out_path;
    replicant::bxon::ArchiveLoadType m_load_type = replicant::bxon::ArchiveLoadType::PRELOAD_DECOMPRESS;
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
                    m_load_type = static_cast<replicant::bxon::ArchiveLoadType>(type);
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

    bool writeCompressedIndex(const std::filesystem::path& path, std::vector<char>& bxon_data) {
        // padd to 16 bytes before compression
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
        auto params = replicant::bxon::ArchiveParameters::FromArcEntries(entries, 0, arc_filename, m_load_type);

        auto res = params.build(0x20090422, 0x54455354); // constatns from game

        if (!res) {
            std::cerr << "Error building new index: " << res.error().message << "\n";
            return 1;
        }
        std::vector<char> bxon_data = std::move(*res);
        return writeCompressedIndex(*m_index_new_path, bxon_data) ? 0 : 1;
    }

    int handlePatchMode(const std::string& arc_filename, const std::vector<replicant::archive::ArcWriter::Entry>& entries) {
        std::cout << "\nPatching original index: " << *m_index_patch_path << "\n";

        auto read_result = util::readFile(*m_index_patch_path);
        if (!read_result) {
            std::cerr << "Error: " << read_result.error() << "\n";
            return 1;
        }

        auto patch_result = replicant::patcher::patchIndex(*read_result, arc_filename, entries, m_load_type);
        if (!patch_result) {
            std::cerr << "Error patching index: " << patch_result.error().message << "\n";
            return 1;
        }

        std::cout << "Successfully patched index in memory.\n";

        std::vector<char> bxon_data = std::move(*patch_result);
        return writeCompressedIndex(*m_index_patch_out_path, bxon_data) ? 0 : 1;
    }

public:
    ArchiveCommand(std::vector<std::string> args) : Command(std::move(args)) {}
    int execute() override {
        if (!parse_archive_args()) {
            return 1;
        }

        replicant::archive::ArcWriter writer;
        std::cout << " Preparing files for archive: " << m_output_archive_path.string() << " \n";

        for (const auto& input_arg : m_file_inputs) {
            std::filesystem::path input_path(input_arg);

            if (std::filesystem::is_directory(input_path)) {
                std::cout << "Scanning directory '" << input_path.string() << "'...\n";
                for (const auto& entry : std::filesystem::recursive_directory_iterator(input_path)) {
                    if (entry.is_regular_file()) {
                        const auto& file_path = entry.path();
                        std::filesystem::path relative_path = std::filesystem::relative(file_path, input_path);
                        std::string key = relative_path.generic_string();
                        std::replace(key.begin(), key.end(), '\\', '/');
                        if (file_path.extension() != "") {
                            std::cout << "WARNING: You are trying to pack a file with an extension. The game does not expect input PACK files to have any extension.";
                        }
                        std::cout << "Adding '" << key << "' from '" << file_path.string() << "'...\n";
                        if (auto res = writer.addFileFromDisk(key, file_path.string()); !res) {
                            std::cerr << "  - FAILED to add file. Reason: " << res.error().message << "\n";
                            return 1;
                        }
                    }
                }
            }
            else {
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

                std::cout << "Adding '" << key << "' from '" << path_str << "'...\n";
                if (auto res = writer.addFileFromDisk(key, path_str); !res) {
                    std::cerr << "Error: " << res.error().message << "\n";
                    return 1;
                }
            }
        }

        if (writer.getPendingEntryCount() == 0) {
            std::cerr << "Error: No valid input files were found to add to the archive.\n";
            return 1;
        }

        auto build_mode = (m_load_type == replicant::bxon::ArchiveLoadType::PRELOAD_DECOMPRESS)
            ? replicant::archive::ArcBuildMode::SingleStream
            : replicant::archive::ArcBuildMode::ConcatenatedFrames;

        auto build_result = writer.build(build_mode);
        if (!build_result) {
            std::cerr << "Error building archive: " << build_result.error().message << "\n";
            return 1;
        }

        if (!util::writeFile(m_output_archive_path, *build_result)) {
            return 1;
        }

        std::string arc_filename = m_output_archive_path.filename().string();
        if (m_index_new_path) {
            return handleNewIndex(arc_filename, writer.getEntries());
        }
        else if (m_index_patch_path) {
            return handlePatchMode(arc_filename, writer.getEntries());
        }

        return 0;
    }
};

class UnpackCommand : public Command {
public:
    UnpackCommand(std::vector<std::string> args) : Command(std::move(args)) {}
    int execute() override {
        if (m_args.size() != 2) {
            std::cerr << "Error: unpack mode requires <input.xap> <output_folder>\n";
            return 1;
        }
        const std::filesystem::path input_path(m_args[0]);
        const std::filesystem::path output_folder_path(m_args[1]);

        std::cout << "Unpacking PACK file\n";
        std::cout << "Input PACK:  " << input_path << "\n";
        std::cout << "Output Folder: " << output_folder_path << "\n\n";

        if (!std::filesystem::exists(input_path)) {
            std::cerr << "Error: Input file not found: " << input_path << "\n";
            return 1;
        }

        std::filesystem::create_directories(output_folder_path);

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

        for (const auto& entry : pack_file.getFileEntries()) {
            std::filesystem::path out_path = output_folder_path / entry.name;
            std::filesystem::create_directories(out_path.parent_path());
            std::ofstream out_file(out_path, std::ios::binary);
            if (!out_file) {
                std::cerr << "Error: Could not open output file for writing: " << out_path << "\n";
                continue;
            }
            out_file.write(entry.serialized_data.data(), entry.serialized_data.size());
            std::cout << "Extracted: " << entry.name << "\n";
        }

        return 0;
    }
};

class RtexToDdsCommand : public Command {
public:
    RtexToDdsCommand(std::vector<std::string> args) : Command(std::move(args)) {}
    int execute() override {
        if (m_args.size() != 2) {
            std::cerr << "Error: rtex-to-dds mode requires <output.dds> <input.rtex>\n";
            return 1;
        }
        const std::filesystem::path output_path(m_args[0]);
        const std::filesystem::path input_path(m_args[1]);

        std::cout << "Converting BXON Texture to DDS\n";
        std::cout << "Input:  " << input_path << "\n";
        std::cout << "Output: " << output_path << "\n";

        auto rtex_data = util::readFile(input_path);
        if (!rtex_data) {
            std::cerr << "Error: " << rtex_data.error() << "\n";
            return 1;
        }

        replicant::bxon::File bxon_file;
        if (!bxon_file.loadFromMemory(rtex_data->data(), rtex_data->size())) {
            std::cerr << "Error parsing BXON file.\n";
            return 1;
        }

        auto* tex = std::get_if<replicant::bxon::Texture>(&bxon_file.getAsset());
        if (!tex) {
            std::cerr << "Error: BXON file is not a texture.\n";
            return 1;
        }

        auto dds_result = replicant::dds::DDSFile::FromGameTexture(*tex);
        if (!dds_result) {
            std::cerr << "Error creating DDS from game texture: " << dds_result.error().message << "\n";
            return 1;
        }

        auto save_result = dds_result->saveToFile(output_path);
        if (!save_result) {
            std::cerr << "Error saving DDS file: " << save_result.error().message << "\n";
            return 1;
        }

        std::cout << "Successfully converted to " << output_path << "\n";

        return 0;
    }
};

void printUsage() {
    std::cout << "UnsealedVerses Refactored - A tool for manipulating game archives and assets.\n\n";
    std::cout << "Usage: UnsealedVerses <command> [options] <args...>\n\n";
    std::cout << "COMMANDS:\n";
    std::cout << "  archive <output.arc> <inputs...> [options]\n";
    std::cout << "    Builds a game archive from specified files or a directory.\n";
    std::cout << "    Inputs can be individual files like 'key1=path/to/file1.xap' or a single folder path.\n";
    std::cout << "    Options:\n";
    std::cout << "      --index <path>      Generate a new compressed index file (e.g., info.arc).\n";
    std::cout << "      --patch <path>      Patch an existing compressed index file.\n";
    std::cout << "      --out <path>        Output path for the patched index. Overwrites original if not set.\n";
    std::cout << "      --load-type <0|1|2> Set load type (0: Preload, 1: Stream, 2: StreamSpecial).\n\n";
    std::cout << "  texture <output.rtex> <input.dds>\n";
    std::cout << "    Converts a standard DDS texture into a game-ready BXON texture (.rtex).\n\n";
    std::cout << "  unpack <input.xap> <output_folder>\n";
    std::cout << "    Extracts all files from a PACK file (.xap) into a specified folder.\n\n";
    std::cout << "  rtex-to-dds <output.dds> <input.rtex>\n";
    std::cout << "    Converts a game-ready BXON texture (.rtex) back into a standard DDS file.\n\n";
    std::cout << "  pack-patch <out.xap> <in.xap> <dds_folder>\n";
    std::cout << "    Patches textures within a PACK file (.xap) using DDS files from a folder.\n";
    std::cout << "    DDS filenames (without extension) must match the entry names in the PACK file.\n";
    std::cout << "  find-entry <search_folder> <entry_name>\n";
    std::cout << "    Recursively searches a directory for PACK files containing an entry with the given name.\n";
    std::cout << "    Useful for finding which archive contains a specific texture or asset.\n";
    std::cout << "    Note this only scans top level entries within the PACKs. Nedted files (e.g. KPKy files) will be missed.\n";
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
    else if (command_name == "find-entry") {
        command = std::make_unique<FindEntryCommand>(command_args);
    }
    else if (command_name == "unpack") {
        command = std::make_unique<UnpackCommand>(command_args);
    }
    else if (command_name == "rtex-to-dds") {
        command = std::make_unique<RtexToDdsCommand>(command_args);
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
        std::cerr << "An unexpected and fatal error occurred: " << e.what() << "\n";
        return 1;
    }
}