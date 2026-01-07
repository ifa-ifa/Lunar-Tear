#pragma once
#include <expected>
#include <string>
#include <span>
#include <vector>

#include "replicant/core/common.h"

namespace replicant{

	struct ActorAssetEntry {
		uint32_t u32_00 = 0;
		std::string assetPath;
		uint32_t u32_0c;
		uint32_t u32_10;
	};

	std::expected<std::vector<ActorAssetEntry>, Error> parseActorAssets(std::span<const std::byte> data);
	std::expected<std::vector<std::byte>, Error> buildActorAssetParam(std::span<const ActorAssetEntry> entries);
}
