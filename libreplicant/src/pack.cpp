#include "replicant/pack.h"
#include "replicant/core/writer.h"
#include "replicant/core/reader.h"

#include <cstring>
#include <algorithm>

namespace replicant {

    namespace {
#pragma pack(push, 1)
        struct RawPackHeader {
            char     magic[4]; 
            uint32_t version;
            uint32_t totalSize;
            uint32_t serializedSize;
            uint32_t resourceSize;

            uint32_t importsCount;
            uint32_t offsetToImports;

            uint32_t assetPackagesCount;
            uint32_t offsetToAssetPackages;

            uint32_t filesCount;
            uint32_t offsetToFiles;
        };

        struct RawImport {
            uint32_t pathHash;
            uint32_t offsetToPath;
            uint32_t unknown0;
        };

        struct RawAssetPackage {
            uint32_t nameHash;
            uint32_t offsetToName;
            uint32_t contentSize;
            uint32_t offsetToContentStart;
            uint32_t offsetToContentEnd;
        };

        struct DataOffset {
            uint32_t offset : 31;
            uint32_t has_data : 1;
        };

        struct RawFile {
            uint32_t nameHash;
            uint32_t offsetToName;
            uint32_t contentSize;
            uint32_t offsetToContent;
            DataOffset dataOffset;
        };
#pragma pack(pop)

        struct ResourceInfo {
            uint32_t offset;
            size_t fileIndex;
        };
    }

    Pack DeserializeInternal(std::span<const std::byte> data) {
        Reader reader(data);

        // Header

        const RawPackHeader* rawHeader = reader.view<RawPackHeader>();

        if (std::strncmp(rawHeader->magic, "PACK", 4) != 0) {
			throw ReaderException("Invalid PACK magic");
        }

        Pack pack;
        pack.info.version = rawHeader->version;

        if (rawHeader->totalSize > data.size()) {
			throw ReaderException("TotalSize exceeds file size");
        }

        const std::byte* resourceBlockBase = data.data() + rawHeader->serializedSize;
        if (resourceBlockBase > data.data() + data.size()) {
            throw ReaderException("SerializedSize exceeds file size");
        }

        // Imports

        if (rawHeader->importsCount > 0) {
            auto importsPtr = reader.getOffsetPtr(rawHeader->offsetToImports);

            const RawImport* rawImports = reinterpret_cast<const RawImport*>(importsPtr);
            // trust getOffsetPtr puts us in valid range becuase Reader::viewArray logic is complex with getOffsetPtr

            pack.imports.reserve(rawHeader->importsCount);
            for (uint32_t i = 0; i < rawHeader->importsCount; i++) {
                ImportEntry entry;
                entry.pathHash = rawImports[i].pathHash;

                entry.path = reader.readStringRelative(rawImports[i].offsetToPath);

                pack.imports.push_back(entry);
            }
        }

        // Asset Packages
        if (rawHeader->assetPackagesCount > 0) {
            auto apsPtr = reader.getOffsetPtr(rawHeader->offsetToAssetPackages);
            const RawAssetPackage* rawAP = reinterpret_cast<const RawAssetPackage*>(apsPtr);

            pack.assetPackages.reserve(rawHeader->assetPackagesCount);
            for (uint32_t i = 0; i < rawHeader->assetPackagesCount; i++) {
                AssetPackageEntry entry;
                entry.nameHash = rawAP[i].nameHash;

                entry.name = reader.readStringRelative(rawAP[i].offsetToName);

                auto contentPtrRes = reader.getOffsetPtr(rawAP[i].offsetToContentStart);
                if (contentPtrRes) {
                    const std::byte* start = reinterpret_cast<const std::byte*>(*contentPtrRes);
                    if (start >= data.data() && start + rawAP[i].contentSize <= data.data() + data.size()) {
                        entry.content.assign(start, start + rawAP[i].contentSize);
                    }
                }
                pack.assetPackages.push_back(entry);
            }
        }

        // Files
        if (rawHeader->filesCount > 0) {
            auto filesPtr = reader.getOffsetPtr(rawHeader->offsetToFiles);

            const RawFile* rawFiles = reinterpret_cast<const RawFile*>(*filesPtr);

            std::vector<ResourceInfo> resources;
            pack.files.reserve(rawHeader->filesCount);

            for (uint32_t i = 0; i < rawHeader->filesCount; i++) {
                PackFileEntry entry;
                entry.nameHash = rawFiles[i].nameHash;

                entry.name = reader.readStringRelative(rawFiles[i].offsetToName);

                // Serialized Data
                auto contentPtrRes = reader.getOffsetPtr(rawFiles[i].offsetToContent);
                if (contentPtrRes) {
                    const std::byte* start = reinterpret_cast<const std::byte*>(*contentPtrRes);
                    if (start >= data.data() && start + rawFiles[i].contentSize <= data.data() + data.size()) {
                        entry.serializedData.assign(start, start + rawFiles[i].contentSize);
                    }
                }

                // Resource Data
                if (rawFiles[i].dataOffset.has_data) {
                    resources.push_back({ rawFiles[i].dataOffset.offset, i });
                }

                pack.files.push_back(entry);
            }

			// There is no way to know the size of each resource chunk from the PACK, that information is supposed to be in
			// tpArchiveFileParam, or in the content. But becuase we want this to be a dumb parser, we can only infer from
			// looking at the next offset. We will end up taking some padding bytes between the chunks but that should be fine

            if (!resources.empty()) {

                std::sort(resources.begin(), resources.end(), [](const ResourceInfo& a, const ResourceInfo& b) {
                    return a.offset < b.offset;
                    });

                size_t maxResourceSize = rawHeader->resourceSize;

                for (size_t i = 0; i < resources.size(); ++i) {
                    uint32_t currentOffset = resources[i].offset;

                    // The end of this file is the start of the next, or the end of the block
                    uint32_t nextOffset = (i + 1 < resources.size())
                        ? resources[i + 1].offset
                        : static_cast<uint32_t>(maxResourceSize);

                    if (nextOffset < currentOffset) {
						throw ReaderException("Resource offsets are out of order");
                    }

                    size_t size = nextOffset - currentOffset;

                    if (resourceBlockBase + currentOffset + size > data.data() + data.size()) {
						throw ReaderException("Resource data exceeds file size");
                    }

                    pack.files[resources[i].fileIndex].resourceData.assign(
                        resourceBlockBase + currentOffset,
                        resourceBlockBase + currentOffset + size
                    );
                }
            }
        }

        return pack;
    }

    std::expected<Pack, Error> Pack::Deserialize(std::span<const std::byte> data) {
        try {
            return DeserializeInternal(data);
        }
        catch (const ReaderException& ex) {
            return std::unexpected(Error{ ErrorCode::ParseError, ex.what()});
        }
        catch (const std::exception& ex) {
            return std::unexpected(Error{ ErrorCode::ParseError, ex.what() });
        }
    }

    std::vector<std::byte> Pack::SerializeInternal() const {
        Writer writer;
        StringPool stringPool;

        // Header
        writer.write("PACK", 4);
        writer.write(info.version);
        size_t tokenTotalSize = writer.reserveOffset();
        size_t tokenSerializedSize = writer.reserveOffset();
        size_t tokenResourceSize = writer.reserveOffset();

        writer.write<uint32_t>(static_cast<uint32_t>(imports.size()));
        size_t tokenImports = writer.reserveOffset();

        writer.write<uint32_t>(static_cast<uint32_t>(assetPackages.size()));
        size_t tokenAssetPacks = writer.reserveOffset();

        writer.write<uint32_t>(static_cast<uint32_t>(files.size()));
        size_t tokenFiles = writer.reserveOffset();

        // Imports
        writer.align(16);
        writer.satisfyOffsetHere(tokenImports);
        for (const auto& imp : imports) {
            writer.write(imp.pathHash);
            size_t tName = writer.reserveOffset();
            writer.write<uint32_t>(0);
            stringPool.add(imp.path, tName);
        }

        // Asset Packages
        writer.align(16);
        writer.satisfyOffsetHere(tokenAssetPacks);

        std::vector<std::pair<size_t, size_t>> apContentTokens;
        apContentTokens.reserve(assetPackages.size());

        for (const auto& ap : assetPackages) {
            writer.write(ap.nameHash);
            size_t tName = writer.reserveOffset();
            writer.write<uint32_t>(static_cast<uint32_t>(ap.content.size()));

            size_t start = writer.reserveOffset();
            size_t end = writer.reserveOffset();
            apContentTokens.emplace_back(start, end);

            stringPool.add(ap.name, tName);
        }

        // Files
        writer.align(16);
        writer.satisfyOffsetHere(tokenFiles);

        struct FilePatchTokens {
            size_t content;
            size_t dataOffsetField; 
        };
        std::vector<FilePatchTokens> fileTokens;
        fileTokens.reserve(files.size());

        for (const auto& f : files) {
            writer.write(f.nameHash);
            size_t tName = writer.reserveOffset();
            writer.write<uint32_t>(static_cast<uint32_t>(f.serializedData.size()));

            FilePatchTokens ft;
            ft.content = writer.reserveOffset();
            ft.dataOffsetField = writer.reserveOffset();
            fileTokens.push_back(ft);

            stringPool.add(f.name, tName);
        }

        writer.align(16);
        stringPool.flush(writer);

        // Content (Asset Packages)
        writer.align(16);
        for (size_t i = 0; i < assetPackages.size(); ++i) {
            writer.satisfyOffsetHere(apContentTokens[i].first); 
            writer.write(assetPackages[i].content.data(), assetPackages[i].content.size());
            writer.satisfyOffsetHere(apContentTokens[i].second); 
            writer.align(16);
        }

        // Content (Files Serialized)
        writer.align(16);
        for (size_t i = 0; i < files.size(); ++i) {
            writer.align(16);
            writer.satisfyOffsetHere(fileTokens[i].content);
            writer.write(files[i].serializedData.data(), files[i].serializedData.size());
        }

        // Content (Files Resources)
        writer.align(16);
        size_t serializedSizeVal = writer.tell();
        {
            uint32_t val = static_cast<uint32_t>(serializedSizeVal);
            if (tokenSerializedSize + 4 <= writer.buffer().size()) {
                std::memcpy(const_cast<std::byte*>(writer.buffer().data()) + tokenSerializedSize, &val, 4);
            }
        }

        for (size_t i = 0; i < files.size(); ++i) {
            DataOffset dof = { 0, 0 };

            if (files[i].hasResource()) {
                writer.align(16);

                size_t resPos = writer.tell();
                size_t relativeOffset = resPos - serializedSizeVal;

                dof.offset = static_cast<uint32_t>(relativeOffset);
                dof.has_data = 1;

                writer.write(files[i].resourceData.data(), files[i].resourceData.size());
            }

            if (fileTokens[i].dataOffsetField + 4 <= writer.buffer().size()) {
                std::memcpy(const_cast<std::byte*>(writer.buffer().data()) + fileTokens[i].dataOffsetField, &dof, 4);
            }
        }

        size_t finalSize = writer.tell();
        size_t resourceSizeVal = finalSize - serializedSizeVal;

        if (tokenTotalSize + 4 <= writer.buffer().size()) {
            uint32_t val = static_cast<uint32_t>(finalSize);
            std::memcpy(const_cast<std::byte*>(writer.buffer().data()) + tokenTotalSize, &val, 4);
        }
        if (tokenResourceSize + 4 <= writer.buffer().size()) {
            uint32_t val = static_cast<uint32_t>(resourceSizeVal);
            std::memcpy(const_cast<std::byte*>(writer.buffer().data()) + tokenResourceSize, &val, 4);
        }

        return writer.buffer();
    }

    std::expected<std::vector<std::byte>, Error> Pack::Serialize() const {
        try {
            return SerializeInternal();
        }
        catch (const std::exception& ex) {
            return std::unexpected(Error{ ErrorCode::SystemError, ex.what() });
		}
    }

    PackFileEntry* Pack::findFile(const std::string& name) {
        for (auto& f : files) {
            if (f.name == name) return &f;
        }
        return nullptr;
    }

    const PackFileEntry* Pack::findFile(const std::string& name) const {
        for (const auto& f : files) {
            if (f.name == name) return &f;
        }
        return nullptr;
    }
}