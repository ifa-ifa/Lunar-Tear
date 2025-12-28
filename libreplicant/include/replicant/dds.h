#pragma once
#include "replicant/tpGxTexHead.h"
#include <memory>
#include <filesystem>
#include <vector>
#include <expected>
#include <span>

namespace DirectX { 
    class ScratchImage; 
}

namespace replicant::dds {

    struct DDSError {
        std::string message;
    };

    class DDSFile {
        std::unique_ptr<DirectX::ScratchImage> image_;

    public:
        DDSFile();
        ~DDSFile();
        DDSFile(DDSFile&&) noexcept;
        DDSFile& operator=(DDSFile&&) noexcept;

        static std::expected<DDSFile, DDSError> Load(const std::filesystem::path& path);
        static std::expected<DDSFile, DDSError> LoadFromMemory(std::span<const std::byte> data);

        static std::expected<DDSFile, DDSError> CreateFromGameData(
            const TextureHeader& header,
            std::span<const std::byte> pixelData
        );

        std::expected<void, DDSError> Save(const std::filesystem::path& path);
        std::expected<std::vector<std::byte>, DDSError> SaveToMemory();

        uint32_t getWidth() const;
        uint32_t getHeight() const;
        uint32_t getDepth() const;
        uint32_t getMipLevels() const;
        TextureFormat getFormat() const;

        std::span<const std::byte> getPixelData() const;

        std::pair<TextureHeader, std::vector<std::byte>> ToGameFormat() const;
    };
}