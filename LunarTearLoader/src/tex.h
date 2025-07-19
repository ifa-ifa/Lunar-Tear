#pragma once
#include <cstdint>



struct BXON_Header {
	char    magic[4];
	uint32_t  version;
	uint32_t projectID;
	uint32_t  offsetToAssetTypeName;  // offset from this member // type name is "tpGxTexHead" for textures
	uint32_t  offsetToAssetTypeData;  // offset from this member // offset to tpGxTexHead
};

enum XonSurfaceDXGIFormat {
	UNKNOWN = 0,
	R8G8B8A8_UNORM_STRAIGHT = 0x00010700,
	R8G8B8A8_UNORM = 0x00010800,
	R8G8B8A8_UNORM_SRGB = 0x00010B00,
	BC1_UNORM = 0x00010F00,
	BC1_UNORM_SRGB = 0x00011000,
	BC2_UNORM = 0x00011100,
	BC2_UNORM_SRGB = 0x00011200,
	BC3_UNORM = 0x00011300,
	BC3_UNORM_SRGB = 0x00011400,
	BC4_UNORM = 0x00011500,
	BC5_UNORM = 0x00011600,
	BC7_UNORM = 0x00011900,
	R32G32B32A32_FLOAT = 0x00030000,
	UNKN_A8_UNORM = 0x00031700,
};

struct tpGxTex_Header {
	uint32_t texWidth;
	uint32_t texHeight;
	uint32_t surfaces;
	uint32_t unknownInt0;
	uint32_t texSize;
	uint32_t unknownInt1;
	XonSurfaceDXGIFormat XonSurfaceFormat;
	uint32_t numMipSurfaces;
	uint32_t offsetToMipSurfaces; // offset from this member
};

struct mipSurface {
	uint32_t offset;
	uint32_t unknown_1;
	uint32_t unknown_2;
	uint32_t unknown_3;
	uint32_t size;
	uint32_t unknown_5;
	uint32_t width;
	uint32_t height;
	uint32_t unknown_8;
	uint32_t unknown_9;
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

inline const char* XonFormatToString(XonSurfaceDXGIFormat format) {
	switch (format) {
	case R8G8B8A8_UNORM_STRAIGHT: return "R8G8B8A8_UNORM_STRAIGHT";
	case R8G8B8A8_UNORM:          return "R8G8B8A8_UNORM";
	case R8G8B8A8_UNORM_SRGB:     return "R8G8B8A8_UNORM_SRGB";
	case BC1_UNORM:               return "BC1_UNORM (DXT1)";
	case BC1_UNORM_SRGB:          return "BC1_UNORM_SRGB (DXT1 sRGB)";
	case BC2_UNORM:               return "BC2_UNORM (DXT3)";
	case BC2_UNORM_SRGB:          return "BC2_UNORM_SRGB (DXT3 sRGB)";
	case BC3_UNORM:               return "BC3_UNORM (DXT5)";
	case BC3_UNORM_SRGB:          return "BC3_UNORM_SRGB (DXT5 sRGB)";
	case BC4_UNORM:               return "BC4_UNORM (ATI1)";
	case BC5_UNORM:               return "BC5_UNORM (ATI2)";
	case BC7_UNORM:               return "BC7_UNORM";
	case R32G32B32A32_FLOAT:      return "R32G32B32A32_FLOAT";
	case UNKN_A8_UNORM:           return "UNKN_A8_UNORM";
	default:                      return "UNKNOWN_FORMAT";
	}
}



#pragma pack(push, 1)
struct DDS_HEADER {
	uint32_t magic;                
	uint32_t dwSize;               
	uint32_t dwFlags;              
	uint32_t dwHeight;             
	uint32_t dwWidth;              
	uint32_t dwPitchOrLinearSize;  
	uint32_t dwDepth;              
	uint32_t dwMipMapCount;        
	uint32_t dwReserved1[11];      
	uint32_t ddspf_dwSize;         
	uint32_t ddspf_dwFlags;        
	uint32_t ddspf_dwFourCC;       
	uint32_t ddspf_dwRGBBitCount;  
	uint32_t ddspf_dwRBitMask;     
	uint32_t ddspf_dwGBitMask;     
	uint32_t ddspf_dwBBitMask;     
	uint32_t ddspf_dwABitMask;     
	uint32_t dwCaps;               
	uint32_t dwCaps2;              
	uint32_t dwCaps3;              
	uint32_t dwCaps4;             
	uint32_t dwReserved2;          
};

#pragma pack(push, 1)
struct DDS_HEADER_DXT10 {
	uint32_t dxgiFormat;          // A DXGI_FORMAT enum value
	uint32_t resourceDimension;
	uint32_t miscFlag;
	uint32_t arraySize;
	uint32_t miscFlags2;
};
#pragma pack(pop)

enum DXGI_FORMAT {
	DXGI_FORMAT_BC1_UNORM = 71,
	DXGI_FORMAT_BC1_UNORM_SRGB = 72,
	DXGI_FORMAT_BC2_UNORM = 74,
	DXGI_FORMAT_BC2_UNORM_SRGB = 75,
	DXGI_FORMAT_BC3_UNORM = 77,
	DXGI_FORMAT_BC3_UNORM_SRGB = 78,
	DXGI_FORMAT_BC4_UNORM = 80,
	DXGI_FORMAT_BC5_UNORM = 83,
	DXGI_FORMAT_BC7_UNORM = 98,
	DXGI_FORMAT_BC7_UNORM_SRGB = 99,
};

inline XonSurfaceDXGIFormat DXGIFormatToXonFormat(uint32_t dxgiFormat) {
	switch (dxgiFormat) {
	case DXGI_FORMAT_BC1_UNORM:         return BC1_UNORM;
	case DXGI_FORMAT_BC1_UNORM_SRGB:    return BC1_UNORM_SRGB;
	case DXGI_FORMAT_BC2_UNORM:         return BC2_UNORM;
	case DXGI_FORMAT_BC2_UNORM_SRGB:    return BC2_UNORM_SRGB;
	case DXGI_FORMAT_BC3_UNORM:         return BC3_UNORM;
	case DXGI_FORMAT_BC3_UNORM_SRGB:    return BC3_UNORM_SRGB;
	case DXGI_FORMAT_BC4_UNORM:         return BC4_UNORM;
	case DXGI_FORMAT_BC5_UNORM:         return BC5_UNORM;
	case DXGI_FORMAT_BC7_UNORM:         return BC7_UNORM;
	default:                            return UNKNOWN;
	}
}

inline XonSurfaceDXGIFormat FourCCToXonFormat(uint32_t fourCC) {
	switch (fourCC) {
	case '1TXD': return BC1_UNORM;       // DXT1
	case '1CB\0': return BC1_UNORM;      // BC1
	case '3TXD': return BC2_UNORM;       // DXT3
	case '2CB\0': return BC2_UNORM;      // BC2
	case '5TXD': return BC3_UNORM;       // DXT5
	case '3CB\0': return BC3_UNORM;      // BC3
	case '1ITA': return BC4_UNORM;       // ATI1
	case '4CB\0': return BC4_UNORM;      // BC4
	case '2ITA': return BC5_UNORM;       // ATI2
	case '5CB\0': return BC5_UNORM;      // BC5
	case '7CB\0': return BC7_UNORM;      // BC7
	default:     return UNKNOWN;
	}
}

