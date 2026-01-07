#pragma once
#include <expected>
#include <string>
#include <span>
#include <vector>

#include "replicant/core/common.h"
#include "replicant/pack.h"

namespace replicant {

	struct XonAssetEntry {
		uint32_t assetTypeHash;
		std::vector<std::byte> assetData;
	};

	std::expected<std::pair<std::vector<XonAssetEntry>, std::vector<ImportEntry>>, Error> 
		parseXonAssetHeader(std::span<const std::byte> data);

	std::expected<std::vector<std::byte>, Error> 
		buildXonAssetHeader(std::span<const XonAssetEntry> entries, std::span<const ImportEntry> imports);
}