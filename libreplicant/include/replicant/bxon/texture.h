#pragma once

#include <cstdint>
#include <vector>

namespace replicant::bxon {

    class File;

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

    struct mipSurface {
        uint32_t offset;
        uint32_t unknown_1;
        uint32_t rowPitch_bpr;
        uint32_t unknown_3; // unused
        uint32_t size;
        uint32_t unknown_5; // unused
        uint32_t width;
        uint32_t height;
        uint32_t depthSliceCount; // The depth of the mip level ( for volume textures). 1 for 2D.
        uint32_t rowCount; //  in pixels (uncompressed) or blocks (compressed)
    };

    class tpGxTexHead {
    public:
        uint32_t getWidth() const { return m_width; }
        uint32_t getHeight() const { return m_height; }
        uint32_t getSurfaceCount() const { return m_surfaceCount; }
        uint32_t getMipmapLevels() const { return m_mipmapLevels; }
        XonSurfaceDXGIFormat getFormat() const { return m_format; }
        const std::vector<mipSurface>& getMipSurfaces() const { return m_mipSurfaces; }
        const std::vector<char>& getTextureData() const { return m_textureData; }

        friend class File;

    private:
        uint32_t m_width = 0;
        uint32_t m_height = 0;
        uint32_t m_surfaceCount = 1;
        uint32_t m_mipmapLevels = 0; // Number of mipmap levels PER surface/face.
							    	// For a texture with no mips, this is 1.
							        	// For a full mip chain, this is log2(max(width, height)) + 1.
        uint32_t m_totalTextureSize = 0;
        uint32_t  m_unknownInt1;  // constant: 2147483648

        XonSurfaceDXGIFormat m_format = XonSurfaceDXGIFormat::UNKNOWN;
        uint32_t m_numMipSurfaces = 0;       // The total number of mipmap surfaces in the file.
									          // For a standard 2D texture, this equals mipmapLevels.
						  	           	      // For a cubemap, this is (mipmapLevels * 6).

        std::vector<mipSurface> m_mipSurfaces;
        std::vector<char> m_textureData;
    };




}