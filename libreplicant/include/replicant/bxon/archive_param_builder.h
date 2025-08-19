#pragma once

#include "replicant/arc/arc.h"
#include "replicant/bxon/error.h"
#include <vector>
#include "replicant/bxon.h"
#include <string>
#include <expected>

namespace replicant::bxon {

    class ArchiveFileParam; // Forward declare

    // Builder for creating a new index from scratch
    std::expected<std::vector<char>, BxonError> buildArchiveParam(
        const std::vector<archive::ArcWriter::Entry>& entries,
        uint8_t archive_index,
        const std::string& archive_filename,
        replicant::bxon::ArchiveLoadType load_type = replicant::bxon::ArchiveLoadType::PRELOAD_DECOMPRESS // Add new parameter
    );

    // Builder for serializing/patching an existing index
    // Takes the original version and projectID to ensure they are preserved.
    std::expected<std::vector<char>, BxonError> buildArchiveParam(
        const ArchiveFileParam& param,
        uint32_t version,
        uint32_t projectID
    );

} // namespace replicant::bxon