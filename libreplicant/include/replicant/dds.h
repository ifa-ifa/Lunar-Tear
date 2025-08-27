#pragma once

#include <replicant/bxon/texture.h>
#include <expected>
#include <string>
#include <filesystem>
#include <span>
#include <memory> 

namespace DirectX {
    struct ScratchImage;
}

namespace replicant::bxon {
    class Texture;
    
}

namespace replicant::dds {


    enum class DDSErrorCode {
        Success,
        FileReadError,
        FileWriteError,
        InvalidMagic,
        InvalidHeader,
        UnsupportedFormat,
        BufferTooSmall,
        BuildError,
        LibraryError 
    };

    struct DDSError {
        DDSErrorCode code;
        std::string message;

    };

    class DDSFile {
        std::unique_ptr<DirectX::ScratchImage> m_image;

        DDSFile();

    public:
        ~DDSFile();
        DDSFile(DDSFile&& other) noexcept;
        DDSFile& operator=(DDSFile&& other) noexcept;

        DDSFile(const DDSFile& other) = delete; 
        DDSFile& operator=(const DDSFile& other) = delete;

        static std::expected<DDSFile, DDSError> FromFile(const std::filesystem::path& path);
        static std::expected<DDSFile, DDSError> FromMemory(std::span<const char> data);
        static std::expected<DDSFile, DDSError> FromGameTexture(const replicant::bxon::Texture& tex);

        std::expected<void, DDSError> saveToFile(const std::filesystem::path& path) const;
        std::expected<std::vector<char>, DDSError> saveToMemory() const;

        uint32_t getWidth() const noexcept;
        uint32_t getHeight() const noexcept;
        uint32_t getDepth() const noexcept;

        uint32_t getMipLevels() const noexcept;
        replicant::bxon::XonSurfaceDXGIFormat getFormat() const noexcept;

        // @warning only valid for the lifetime of this DDSFile object.
        std::span<const char> getPixelData() const noexcept;


    };
}