#include "replicant/tpArchiveFileParam.h"
#include "replicant/core/writer.h"
#include <cstring>
#include <string_view>
#include <algorithm>

namespace replicant {

    namespace {

#pragma pack(push, 1)
        struct RawHeader {
            uint32_t numArchives;
            uint32_t offsetToArray;
            uint32_t fileEntryCount;
            uint32_t offsetToFileTable;
        };

        struct RawArchiveEntry {
            uint32_t offsetToFilename;
            uint32_t arcOffsetScale;
            ArchiveLoadType loadType;
            uint8_t  padding[3];
        };

        struct RawFileEntry {
            uint32_t pathHash;
            uint32_t nameOffset;
            uint32_t scaledOffset; 
            uint32_t compressedSize;
            uint32_t packFileSerializedSize;
            uint32_t packFileResourceSize;
            uint8_t  archiveIndex;
            uint8_t  flags;
            uint8_t  padding[2];
        };
#pragma pack(pop)

        uint32_t fnv1_32(std::string_view s) {
            uint32_t hash = 0x811c9dc5;
            for (char c : s) {
                hash *= 0x01000193;         // Multiply First
                hash ^= static_cast<uint8_t>(c); // XOR Second
            }
            return hash;
        }
    }

    std::expected<TpArchiveFileParam, ReaderError> TpArchiveFileParam::Deserialize(std::span<const std::byte> payload) {
        Reader reader(payload);

        auto headerRes = reader.view<RawHeader>();
        if (!headerRes) return std::unexpected(headerRes.error());
        const auto* raw = *headerRes;

        TpArchiveFileParam asset;

        if (raw->numArchives > 0) {
            auto arcArrayRes = reader.getOffsetPtr(raw->offsetToArray);
            if (!arcArrayRes) return std::unexpected(arcArrayRes.error());

            const std::byte* start = reinterpret_cast<const std::byte*>(*arcArrayRes);
            const std::byte* end = payload.data() + payload.size();
            if (start + (raw->numArchives * sizeof(RawArchiveEntry)) > end) {
                return std::unexpected(ReaderError{ ReaderErrorCode::OutOfBounds, "Archive array exceeds buffer size" });
            }

            const RawArchiveEntry* rawArcs = reinterpret_cast<const RawArchiveEntry*>(*arcArrayRes);
            asset.archiveEntries.reserve(raw->numArchives);

            for (uint32_t i = 0; i < raw->numArchives; i++) {
                ArchiveEntry entry;
                entry.arcOffsetScale = rawArcs[i].arcOffsetScale;
                entry.loadType = rawArcs[i].loadType;

                auto strRes = reader.readStringRelative(rawArcs[i].offsetToFilename);
                if (strRes) entry.filename = *strRes;

                asset.archiveEntries.push_back(std::move(entry));
            }
        }

        if (raw->fileEntryCount > 0) {
            auto fileTableRes = reader.getOffsetPtr(raw->offsetToFileTable);
            if (!fileTableRes) return std::unexpected(fileTableRes.error());

            const std::byte* start = reinterpret_cast<const std::byte*>(*fileTableRes);
            const std::byte* end = payload.data() + payload.size();
            if (start + (raw->fileEntryCount * sizeof(RawFileEntry)) > end) {
                return std::unexpected(ReaderError{ ReaderErrorCode::OutOfBounds, "File table exceeds buffer size" });
            }

            const RawFileEntry* rawFiles = reinterpret_cast<const RawFileEntry*>(*fileTableRes);
            asset.fileEntries.reserve(raw->fileEntryCount);

            for (uint32_t i = 0; i < raw->fileEntryCount; i++) {
                FileEntry entry;

                entry.size = rawFiles[i].compressedSize;
                entry.packFileSerializedSize = rawFiles[i].packFileSerializedSize;
                entry.packFileResourceSize = rawFiles[i].packFileResourceSize;
                entry.archiveIndex = rawFiles[i].archiveIndex;
                entry.flags = rawFiles[i].flags;

                auto strRes = reader.readStringRelative(rawFiles[i].nameOffset);
                if (strRes) entry.name = *strRes;

                uint32_t scale = 0;
                if (entry.archiveIndex < asset.archiveEntries.size()) {
                    scale = asset.archiveEntries[entry.archiveIndex].arcOffsetScale;
                }
                entry.rawOffset = static_cast<uint64_t>(rawFiles[i].scaledOffset) << scale;

                asset.fileEntries.push_back(std::move(entry));
            }
        }

        return asset;
    }


    std::vector<std::byte> TpArchiveFileParam::Serialize() const {

        size_t estimatedSize = 32 + (archiveEntries.size() * 24) + (fileEntries.size() * 48) + 1024;
        Writer writer(estimatedSize);
        StringPool stringPool;

        writer.write<uint32_t>(static_cast<uint32_t>(archiveEntries.size()));
        size_t tokenArcArray = writer.reserveOffset();

        writer.write<uint32_t>(static_cast<uint32_t>(fileEntries.size()));
        size_t tokenFileTable = writer.reserveOffset();

        if (!archiveEntries.empty()) {
            writer.align(16);
            writer.satisfyOffsetHere(tokenArcArray);

            for (const auto& entry : archiveEntries) {
                size_t tokenName = writer.reserveOffset();
                writer.write(entry.arcOffsetScale);
                writer.write(entry.loadType);

                writer.write<uint8_t>(0);
                writer.write<uint8_t>(0);
                writer.write<uint8_t>(0);

                stringPool.add(entry.filename, tokenName);
            }
        }

        if (!fileEntries.empty()) {
            writer.align(16);
            writer.satisfyOffsetHere(tokenFileTable);

            for (const auto& entry : fileEntries) {
                uint32_t hash = fnv1_32(entry.name);
                writer.write(hash);

                size_t tokenName = writer.reserveOffset();

                uint32_t scale = 0;
                if (entry.archiveIndex < archiveEntries.size()) {
                    scale = archiveEntries[entry.archiveIndex].arcOffsetScale;
                }
                uint32_t scaled = static_cast<uint32_t>(entry.rawOffset >> scale);
                writer.write(scaled);

                writer.write(entry.size); 
                writer.write(entry.packFileSerializedSize);
                writer.write(entry.packFileResourceSize);
                writer.write(entry.archiveIndex);
                writer.write(entry.flags);

                writer.write<uint8_t>(0);
                writer.write<uint8_t>(0);

                stringPool.add(entry.name, tokenName);
            }
        }

        writer.align(16);
        stringPool.flush(writer);

        return writer.buffer();
    }

    uint8_t TpArchiveFileParam::addArchiveEntry(const std::string& filename, ArchiveLoadType type) {
        for (size_t i = 0; i < archiveEntries.size(); ++i) {
            if (archiveEntries[i].filename == filename) {

                return static_cast<uint8_t>(i);
            }
        }

        ArchiveEntry entry;
        entry.filename = filename;
        entry.loadType = type;
        entry.arcOffsetScale = 4; 
        archiveEntries.push_back(entry);

        return static_cast<uint8_t>(archiveEntries.size() - 1);
    }

    FileEntry* TpArchiveFileParam::findFile(const std::string& name) {
        for (auto& entry : fileEntries) {
            if (entry.name == name) return &entry;
        }
        return nullptr;
    }

    void TpArchiveFileParam::registerArchive(
        uint8_t archiveIndex,
        const std::vector<archive::ArchiveEntryInfo>& builtEntries
    ) {
        for (const auto& built : builtEntries) {
            FileEntry* existing = findFile(built.name);

            if (existing) {
                existing->archiveIndex = archiveIndex;
                existing->rawOffset = built.offset;
                existing->size = built.compressedSize;
                existing->packFileSerializedSize = built.packSerializedSize;
                existing->packFileResourceSize = built.packResourceSize;
            }
            else {
                FileEntry entry;
                entry.name = built.name;
                entry.archiveIndex = archiveIndex;
                entry.rawOffset = built.offset;
                entry.size = built.compressedSize; 
                entry.packFileSerializedSize = built.packSerializedSize;
                entry.packFileResourceSize = built.packResourceSize;
                entry.flags = 0;

                fileEntries.push_back(std::move(entry));
            }
        }
    }

}