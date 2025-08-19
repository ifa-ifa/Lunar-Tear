#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <iomanip>
#include <optional>
#include <filesystem>
#include <zstd.h>
#include "replicant/arc/arc.h"
#include "replicant/bxon/archive_param_builder.h"
#include "replicant/bxon.h"

void printUsage() {
    std::cout << "Unsealed Verses - Nier Replicant Archive Tool\n\n";
    std::cout << "Usage: UnsealedVerses <output.arc> [options] <key=filepath>...\n\n";
    std::cout << "Options:\n";
    std::cout << "  --index <path>          Generate a new index file from scratch.\n";
    std::cout << "  --patch <original>      Patch an existing index file.\n";
    std::cout << "  --out <patched_path>    Optional with --patch. Specifies the output path.\n";
    std::cout << "  --load-type <0|1|2>     Set the load type for the new archive.\n";
    std::cout << "                          0: Preload (default), 1: Stream, 2: Stream Special.\n\n";
    std::cout << "Modes:\n";
    std::cout << "  1. Create from scratch (with --index):\n";
    std::cout << "     UnsealedVerses my_mod.arc --index my_mod.bin file1=...\n";
    std::cout << "  2. Create and Patch (with --patch and --out):\n";
    std::cout << "     UnsealedVerses my_mod.arc --patch info.arc --out info_mod.arc file1=...\n";
}

uint32_t align16(uint32_t size) {
    return (size + 15) & ~15;
}

std::expected<std::vector<char>, std::string> loadAndDecompressIndex(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) { return std::unexpected("Could not open file: " + path); }
    std::streamsize compressed_size = file.tellg();
    file.seekg(0, std::ios::beg);
    if (compressed_size <= 0) { return std::unexpected("File is empty or invalid."); }
    std::vector<char> compressed_data(compressed_size);
    if (!file.read(compressed_data.data(), compressed_size)) {
        return std::unexpected("Failed to read compressed data from file.");
    }
    unsigned long long const decompressed_size = ZSTD_getFrameContentSize(compressed_data.data(), compressed_data.size());
    if (decompressed_size == ZSTD_CONTENTSIZE_ERROR) {
        return std::unexpected("File is not a valid ZSTD stream.");
    }
    if (decompressed_size == ZSTD_CONTENTSIZE_UNKNOWN || decompressed_size == 0) {
        return std::unexpected("Could not determine a valid decompressed size from ZSTD stream.");
    }
    auto decompress_result = replicant::archive::decompress_zstd(compressed_data.data(), compressed_data.size(), decompressed_size);
    if (!decompress_result) {
        return std::unexpected("Zstd decompression failed: " + decompress_result.error().message);
    }
    return *decompress_result;
}

void printMetadataTable(const std::vector<replicant::archive::ArcWriter::Entry>& entries, replicant::archive::ArcBuildMode mode) {
    std::cout << "\n--- Archive Entry Metadata ---\n";
    const char* offset_header = (mode == replicant::archive::ArcBuildMode::SingleStream) ? "Uncompressed Offset" : "Physical Offset";
    std::cout << std::left << std::setw(40) << "Key" << std::setw(22) << offset_header << std::setw(18) << "Index Offset" << std::setw(18) << "Compressed Size" << std::setw(18) << "Decompressed Size" << "\n";
    std::cout << std::string(116, '-') << "\n";
    for (const auto& entry : entries) {
        uint32_t index_offset = entry.offset / 16;
        std::cout << std::left << std::setw(40) << entry.key << std::setw(22) << entry.offset << std::setw(18) << index_offset << std::setw(18) << entry.compressed_size << std::setw(18) << entry.uncompressed_size << "\n";
    }
    std::cout << "\n";
}

bool writeCompressedIndex(const std::string& path, std::vector<char>& bxon_data) {
    size_t original_size = bxon_data.size();
    size_t padded_size = align16(original_size);
    if (padded_size > original_size) {
        bxon_data.resize(padded_size, 0);
        std::cout << "Padded uncompressed index from " << original_size << " to " << padded_size << " bytes for alignment.\n";
    }
    auto compressed_result = replicant::archive::compress_zstd(bxon_data.data(), bxon_data.size());
    if (!compressed_result) {
        std::cerr << "Error compressing index: " << compressed_result.error().message << "\n";
        return false;
    }
    const auto& final_compressed_data = *compressed_result;
    std::ofstream out_file(path, std::ios::binary);
    if (!out_file) {
        std::cerr << "Error: Could not open index for writing: " << path << "\n";
        return false;
    }
    out_file.write(final_compressed_data.data(), final_compressed_data.size());
    std::cout << "Successfully wrote index to " << path << " (" << final_compressed_data.size() << " bytes total).\n";
    return true;
}


int handleNewIndexMode(const std::string& new_index_path, const std::string& arc_filename, const std::vector<replicant::archive::ArcWriter::Entry>& entries, replicant::bxon::ArchiveLoadType load_type) {
    std::cout << "\n--- Generating new index file... ---\n";
    auto index_data_result = replicant::bxon::buildArchiveParam(entries, 0, arc_filename, load_type);
    if (!index_data_result) {
        std::cerr << "Error building new index: " << index_data_result.error().message << "\n";
        return 1;
    }
    std::vector<char> bxon_data = std::move(*index_data_result);
    if (!writeCompressedIndex(new_index_path, bxon_data)) { return 1; }
    return 0;
}

int handlePatchMode(const std::string& patch_base_path, const std::string& patch_out_path, const std::string& arc_filename, const std::vector<replicant::archive::ArcWriter::Entry>& entries, replicant::bxon::ArchiveLoadType load_type) {
    std::cout << "\n--- Patching original index: " << patch_base_path << " ---\n";
    auto decompressed_result = loadAndDecompressIndex(patch_base_path);
    if (!decompressed_result) {
        std::cerr << "Error: Failed to load and decompress original index.\n  Reason: " << decompressed_result.error() << "\n";
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
        std::cerr << "Error: Decompressed data from '" << patch_base_path << "' is not a tpArchiveFileParam BXON.\n";
        return 1;
    }

    uint8_t new_archive_index = param->addArchive(arc_filename, load_type);
    std::cout << "Using archive '" << arc_filename << "' at index " << (int)new_archive_index << " with LoadType " << (int)load_type << ".\n";

    for (const auto& mod_entry : entries) {
        auto* game_entry = param->findFileEntryMutable(mod_entry.key);
        if (game_entry) {
            std::cout << "Patching entry: " << mod_entry.key << "\n";
            game_entry->archiveIndex = new_archive_index;
            game_entry->arcOffset = mod_entry.offset / 16;
            game_entry->compressedSize = mod_entry.compressed_size;
            game_entry->decompressedSize = mod_entry.uncompressed_size;
            game_entry->bufferSize = align16(mod_entry.uncompressed_size*20);
        }
        else {
            std::cout << "Adding new entry: " << mod_entry.key << "\n";
            replicant::bxon::FileEntry fe;
            fe.filePath = mod_entry.key;
            fe.archiveIndex = new_archive_index;
            fe.arcOffset = mod_entry.offset / 16;
            fe.compressedSize = mod_entry.compressed_size;
            fe.decompressedSize = mod_entry.uncompressed_size;
            fe.bufferSize = align16(mod_entry.uncompressed_size);
            param->getFileEntries().push_back(fe);
        }
    }
    auto patched_data_result = replicant::bxon::buildArchiveParam(*param, bxon_file.getVersion(), bxon_file.getProjectID());
    if (!patched_data_result) {
        std::cerr << "Error building patched index: " << patched_data_result.error().message << "\n";
        return 1;
    }
    std::vector<char> bxon_data = std::move(*patched_data_result);
    if (!writeCompressedIndex(patch_out_path, bxon_data)) { return 1; }
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 3) { printUsage(); return 1; }

    std::string output_arc_path;
    std::optional<std::string> new_index_path, patch_base_path, patch_out_path;
    std::vector<std::string> file_args;
    replicant::bxon::ArchiveLoadType load_type = replicant::bxon::ArchiveLoadType::PRELOAD_DECOMPRESS;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--index") {
            if (i + 1 < argc) { new_index_path = argv[++i]; }
            else { std::cerr << "Error: --index requires a path.\n"; return 1; }
        }
        else if (arg == "--patch") {
            if (i + 1 < argc) { patch_base_path = argv[++i]; }
            else { std::cerr << "Error: --patch requires a path.\n"; return 1; }
        }
        else if (arg == "--out") {
            if (i + 1 < argc) { patch_out_path = argv[++i]; }
            else { std::cerr << "Error: --out requires a path.\n"; return 1; }
        }
        else if (arg == "--load-type") {
            if (i + 1 < argc) {
                try {
                    int type_val = std::stoi(argv[++i]);
                    if (type_val >= 0 && type_val <= 2) {
                        load_type = static_cast<replicant::bxon::ArchiveLoadType>(type_val);
                    }
                    else { std::cerr << "Error: Invalid value for --load-type. Must be 0, 1, or 2.\n"; return 1; }
                }
                catch (const std::exception&) { std::cerr << "Error: Invalid number format for --load-type.\n"; return 1; }
            }
            else { std::cerr << "Error: --load-type requires a value (0, 1, or 2).\n"; return 1; }
        }
        else if (output_arc_path.empty()) {
            output_arc_path = arg;
        }
        else {
            file_args.push_back(arg);
        }
    }

    if (output_arc_path.empty() || file_args.empty()) { std::cerr << "Error: Output .arc path and at least one input file are required.\n\n"; printUsage(); return 1; }
    if (patch_base_path && !patch_out_path) {
        std::cout << "Warning: --out not specified. The original patch file '" << *patch_base_path << "' will be overwritten.\n";
        patch_out_path = *patch_base_path;
    }
    if (new_index_path && patch_base_path) { std::cerr << "Error: Cannot use --index and --patch at the same time.\n\n"; printUsage(); return 1; }

    replicant::archive::ArcWriter writer;
    std::cout << "--- Preparing files for archive: " << output_arc_path << " ---\n";
    for (const auto& arg : file_args) {
        size_t separator_pos = arg.find('=');
        std::string key, filepath;
        if (separator_pos == std::string::npos) { key = arg; filepath = arg; std::cout << "Adding '" << key << "' (key and path are the same)...\n"; }
        else { key = arg.substr(0, separator_pos); filepath = arg.substr(separator_pos + 1); if (key.empty() || filepath.empty()) { std::cerr << "Error: Invalid input format '" << arg << "'.\n"; return 1; } std::cout << "Adding '" << key << "' from '" << filepath << "'...\n"; }
        auto result = writer.addFileFromDisk(key, filepath);
        if (!result) { std::cerr << "Error: Failed to add file '" << filepath << "'.\n  Reason [" << (int)result.error().code << "]: " << result.error().message << "\n"; return 1; }
    }

    replicant::archive::ArcBuildMode build_mode = (load_type == replicant::bxon::ArchiveLoadType::PRELOAD_DECOMPRESS)
        ? replicant::archive::ArcBuildMode::SingleStream
        : replicant::archive::ArcBuildMode::ConcatenatedFrames;

    auto build_result = writer.build(build_mode);
    if (!build_result) { std::cerr << "Error: Failed to build archive.\n  Reason [" << (int)build_result.error().code << "]: " << build_result.error().message << "\n"; return 1; }

    const auto& arc_data = *build_result;
    std::cout << "Build successful. Total archive size: " << arc_data.size() << " bytes.\n";
    std::ofstream arc_out_file(output_arc_path, std::ios::binary);
    if (!arc_out_file) { std::cerr << "Error: Could not write to " << output_arc_path << "\n"; return 1; }
    arc_out_file.write(arc_data.data(), arc_data.size());
    std::cout << "Successfully wrote archive to " << output_arc_path << "\n";

    const auto& entries = writer.getEntries();
    std::string arc_filename = std::filesystem::path(output_arc_path).filename().string();
    int result = 0;
    if (patch_base_path) { result = handlePatchMode(*patch_base_path, *patch_out_path, arc_filename, entries, load_type); }
    else if (new_index_path) { result = handleNewIndexMode(*new_index_path, arc_filename, entries, load_type); }
    if (result == 0) { printMetadataTable(entries, build_mode); }
    return result;
}