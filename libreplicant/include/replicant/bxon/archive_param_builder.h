#pragma once

#include "replicant/arc/arc.h"
#include "replicant/bxon/error.h"
#include "replicant/bxon/archive_param.h" 
#include <vector>
#include <string>
#include <expected>

namespace replicant::bxon {

    class ArchiveFileParam; 

    std::expected<std::vector<char>, BxonError> buildArchiveParam(
        const std::vector<archive::ArcWriter::Entry>& entries,
        uint8_t archive_index,
        const std::string& archive_filename,
        ArchiveLoadType load_type = ArchiveLoadType::PRELOAD_DECOMPRESS
    );

    std::expected<std::vector<char>, BxonError> buildArchiveParam(
        const ArchiveFileParam& param,
        uint32_t version,
        uint32_t projectID
    );

} 