#pragma once
#include <cstdint>
#include <replicant/bxon/texture.h>

#pragma pack(push, 1)


struct tpGxTex_Header {
	uint32_t texWidth;
	uint32_t texHeight;
		  
	uint32_t texDepth;
	uint32_t numSurfaces;         // 1 for 2d, 6 for cubemaps
		  
	uint32_t texSize;
		  
	uint32_t unk2;                // always 2147483648 (2**31)
	replicant::bxon::XonSurfaceDXGIFormat XonSurfaceFormat;

	uint32_t numMipSurfaces;
	uint32_t offsetToMipSurfaces; // offset from this member
};


struct mipSurface {
	uint32_t offset; // From texData
    uint32_t unk3; // always 0
    uint32_t rowPitch;
    uint32_t unk4; // always 0
    uint32_t size;
    uint32_t unk5; // always 0
    uint32_t width;
    uint32_t height;
    uint32_t currentDepth;
    uint32_t heightInBlocks;
};



struct tpgxResTexture {
	int64_t unk0[8];
	char* name;
	int64_t unk1[9];
	void* bxonHeader;
	int64_t unk2;
	void* texData;
	int64_t texDataSize;
	int64_t unk3[3];
	tpGxTex_Header* bxonAssetHeader; // always tpGxTexHead for us
};

#pragma pack(pop)

inline const char* XonFormatToString(replicant::bxon::XonSurfaceDXGIFormat format) {
	switch (format) {
	case replicant::bxon::R8G8B8A8_UNORM_STRAIGHT:   return "R8G8B8A8_UNORM_STRAIGHT";
	case replicant::bxon::R8G8B8A8_UNORM:            return "R8G8B8A8_UNORM";
	case replicant::bxon::R8G8B8A8_UNORM_SRGB:       return "R8G8B8A8_UNORM_SRGB";
	case replicant::bxon::BC1_UNORM:                 return "BC1_UNORM (DXT1)";
	case replicant::bxon::BC1_UNORM_SRGB:            return "BC1_UNORM_SRGB (DXT1 sRGB)";
	case replicant::bxon::BC2_UNORM:                 return "BC2_UNORM (DXT3)";
	case replicant::bxon::BC2_UNORM_SRGB:            return "BC2_UNORM_SRGB (DXT3 sRGB)";
	case replicant::bxon::BC3_UNORM:                 return "BC3_UNORM (DXT5)";
	case replicant::bxon::BC3_UNORM_SRGB:            return "BC3_UNORM_SRGB (DXT5 sRGB)";
	case replicant::bxon::BC4_UNORM:                 return "BC4_UNORM (ATI1)";
	case replicant::bxon::BC5_UNORM:                 return "BC5_UNORM (ATI2)";
	case replicant::bxon::BC7_UNORM:                 return "BC7_UNORM";
	case replicant::bxon::BC7_UNORM_SRGB_VOLUMETRIC: return "BC7_UNORM_SRGB (VOLUMETRIC)";
	case replicant::bxon::R32G32B32A32_FLOAT:        return "R32G32B32A32_FLOAT";
	case replicant::bxon::UNKN_A8_UNORM:             return "UNKN_A8_UNORM";
	default:                                         return "UNKNOWN_FORMAT";
	}
}
