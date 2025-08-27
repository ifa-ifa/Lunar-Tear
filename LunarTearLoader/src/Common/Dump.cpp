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

void dumpTexture(const tpgxResTexture* tex) {
    if (!tex || !tex->name || !tex->bxonAssetHeader || !tex->texData) {
        Logger::Log(Error) << "Attempted to dump an invalid texture";
        return;
    }

    tpGxTex_Header* header = tex->bxonAssetHeader;
    mipSurface* game_mips_ptr = (mipSurface*)((uintptr_t)&header->offsetToMipSurfaces + header->offsetToMipSurfaces);

    // Cast to libraries identical type to keep libreplicant out of API headers (hope this is safe?)
    const replicant::bxon::mipSurface* libreplicant_mips_ptr =
        reinterpret_cast<const replicant::bxon::mipSurface*>(game_mips_ptr);

    std::span<const replicant::bxon::mipSurface> mip_surfaces_span(libreplicant_mips_ptr, header->numMipSurfaces);
    std::span<const char> texture_data_span(static_cast<const char*>(tex->texData), tex->texDataSize);

    auto game_tex_result = replicant::bxon::Texture::FromGameData(
        header->texWidth,
        header->texHeight,
        header->texDepth,
        header->numSurfaces,
        (replicant::bxon::XonSurfaceDXGIFormat)header->XonSurfaceFormat,
        header->texSize,
        mip_surfaces_span,
        texture_data_span
    );

    if (!game_tex_result) {
        Logger::Log(Error) << "Failed to create replicant::Texture from game data for '" << tex->name << "': " << game_tex_result.error().message;
        return;
    }

    auto dds_file_result = replicant::dds::DDSFile::FromGameTexture(*game_tex_result);
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

    auto save_result = dds_file_result->saveToFile(out_path);
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