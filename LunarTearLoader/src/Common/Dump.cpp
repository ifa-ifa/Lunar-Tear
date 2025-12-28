#define NOMINMAX
#include "dump.h"
#include <fstream>
#include <vector>
#include "Game/TextureFormats.h"
#include <filesystem>
#include <replicant/stbl.h>
#include <replicant/dds.h>
#include "Common/Logger.h"

constexpr size_t MAX_STBL_SIZE = 16 * 1000 * 1000;

using enum Logger::LogCategory;

void dumpScript(const std::string& name, const std::vector<char>& data) {
	if (!std::filesystem::exists("LunarTear/dump/scripts/")) {
		std::filesystem::create_directories("LunarTear/dump/scripts/");
	}
	std::ofstream f("LunarTear/dump/scripts/" + name, std::ios::binary);


	f.write(data.data(), data.size());
    Logger::Log(Verbose) << "Dumped script: " << name;

}

void dumpTexture(const tpGxResTexture* tex) {
    if (!tex || !tex->name || !tex->bxonAssetHeader || !tex->texData) {
        Logger::Log(Error) << "Attempted to dump an invalid texture";
        return;
    }

    std::vector<std::byte> pixel_data = std::vector<std::byte>(
        static_cast<std::byte*>(tex->texData),
        static_cast<std::byte*>(tex->texData) + tex->texDataSize
    );

    auto rawHeader = reinterpret_cast<replicant::raw::RawTexHeader*>(tex->bxonAssetHeader);


    uintptr_t headerBaseAddr = reinterpret_cast<uintptr_t>(tex->bxonAssetHeader);

    uintptr_t offsetFieldAddr = headerBaseAddr + offsetof(replicant::raw::RawTexHeader, offsetToSubresources);

    uintptr_t mipTableStartAddr = offsetFieldAddr + rawHeader->offsetToSubresources;

    size_t mipTableSize = rawHeader->subresourcesCount * sizeof(replicant::raw::RawMipSurface);
    uintptr_t mipTableEndAddr = mipTableStartAddr + mipTableSize;

    size_t totalHeaderSize = mipTableEndAddr - headerBaseAddr;
    std::span<const std::byte> headerSpan(
        reinterpret_cast<const std::byte*>(tex->bxonAssetHeader),
        totalHeaderSize
    );

    auto game_tex = replicant::Texture::DeserializeHeader(headerSpan);    if (!game_tex) {
        Logger::Log(Error) << "Failed to deserialize game texture header for texture '" << tex->name << "': " << game_tex.error().message;
        return;
	}

    auto dds_file_result = replicant::dds::DDSFile::CreateFromGameData(*game_tex, pixel_data);
    if (!dds_file_result) {
        Logger::Log(Error) << "Failed to create DDS from game texture '" << tex->name << "': " << dds_file_result.error().message;
        return;
    }

    std::filesystem::path out_path("LunarTear/dump/textures/");
    std::filesystem::path tex_filename(tex->name);
    out_path /= tex_filename.replace_extension(".dds");

    if (!std::filesystem::exists(out_path.parent_path())) {
        std::filesystem::create_directories(out_path.parent_path());
    }

    auto save_result = dds_file_result->Save(out_path);
    if (!save_result) {
        Logger::Log(Error) << "Failed to save dumped texture '" << tex->name << "' to '" << out_path.string() << "': " << save_result.error().message;
        return;
    }

    Logger::Log(Verbose) << "Dumped texture '" << tex->name << "' to '" << out_path.string() << "'";
}


void dumpTable(const std::string& name, const char* data) {
    size_t inferred_size = StblFile::InferSize(data, MAX_STBL_SIZE);

    if (inferred_size == 0) {
        Logger::Log(Error) << "Failed to infer size for table dump: " << name;
        return;
    }

    if (!std::filesystem::exists("LunarTear/dump/tables/")) {
        std::filesystem::create_directories("LunarTear/dump/tables/");
    }

    std::ofstream f("LunarTear/dump/tables/" + name, std::ios::binary);
    
    f.write(data, inferred_size);
    Logger::Log(Verbose) << "Dumped table '" << name << "' (" << inferred_size << " bytes)";
    

}