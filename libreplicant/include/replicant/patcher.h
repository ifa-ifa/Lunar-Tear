#pragma once

#include "replicant/arc.h"
#include "replicant/bxon.h"
#include <expected>
#include <string>
#include <vector>

namespace replicant::patcher {

    enum class PatcherErrorCode {
        Success,
        DecompressionError,
        CompressionError,
        ParseError,
        BuildError,
        AssetTypeError,
        EntryAddNotSupported,
    };

    struct PatcherError {
        PatcherErrorCode code;
        std::string message;
    };

    /*!
     * @brief Patches a compressed tpArchiveFileParam BXON (index file) in memory.
     *
     * @param original_compressed_index The raw byte data of the original compressed index file (info.arc)
     * @param new_arc_filename The filename of the new archive being added 
     * @param new_entries A vector of built entries from an replicant::archive::ArcWriter instance. 
              This contains the metadata for the files inside the new archive.
     * @param load_type The load type for the new archive
     * @return New, uncompressed BXON data
     */
    std::expected<std::vector<char>, PatcherError> patchIndex(
        const std::vector<char>& original_compressed_index,
        const std::string& new_arc_filename,
        const std::vector<archive::ArcWriter::Entry>& new_entries,
        bxon::ArchiveLoadType load_type = bxon::ArchiveLoadType::PRELOAD_DECOMPRESS
    );

}