#include "replicant/tpActorAssetParam.h"
#include "replicant/core/reader.h"
#include "replicant/core/writer.h"
#include "replicant/core/common.h"

namespace replicant {

    namespace {

#pragma pack(push, 1)

        struct RawHeader {
            uint32_t entryCount;
            uint32_t offsetToDataStart;

        };

        struct RawEntry {

            uint32_t u32_00;
            uint32_t assetPathHash;
            uint32_t offsetToAssetPath;
            uint32_t u32_0c;
            uint32_t u32_10;
        };

#pragma pack(pop)
    }

    std::vector<ActorAssetEntry> parseActorAssetsInternal(std::span<const std::byte> data) {

        Reader reader(data);
        RawHeader const* header = reader.view<RawHeader>();

        reader.seek(reader.getOffsetPtr(header->offsetToDataStart));

        std::vector<RawEntry> RawEntries;
		std::span<const uint32_t> offsetsToEntries = reader.viewArray<uint32_t>(header->entryCount);

        for (uint32_t offset : offsetsToEntries) {
            reader.seek(reader.getOffsetPtr(offset));
            RawEntries.push_back(*reader.view<RawEntry>());
		}

        std::vector<ActorAssetEntry> entries(header->entryCount);

        for (size_t i = 0; i < header->entryCount; i++) {
            RawEntry const& rawEntry = RawEntries[i];
            ActorAssetEntry& entry = entries[i];
            entry.u32_00 = rawEntry.u32_00;
            entry.assetPath = reader.readStringRelative(rawEntry.offsetToAssetPath);
            entry.u32_0c = rawEntry.u32_0c;
            entry.u32_10 = rawEntry.u32_10;
        }

        return entries;


    }
    std::vector<std::byte> buildActorAssetParamInternal(std::span <const ActorAssetEntry> entries) {

        Writer writer(entries.size() * 20 + 30);
        StringPool stringPool;

        writer.write(static_cast<uint32_t>(entries.size()));
        size_t offsetToDataStartToken = writer.reserveOffset();

        writer.align(16);
        writer.satisfyOffsetHere(offsetToDataStartToken);

		std::vector<uint32_t> entryOffsetTokens;
        for (size_t i = 0; i < entries.size(); ++i) {
            entryOffsetTokens.push_back(writer.reserveOffset());
		}
        writer.align(8);
        for (size_t i = 0; i < entries.size(); ++i) {
            writer.satisfyOffsetHere(entryOffsetTokens[i]);

            const ActorAssetEntry& entry = entries[i];

            writer.write(entry.u32_00);
            writer.write(fnv1_32(entry.assetPath));
            stringPool.add(entry.assetPath, writer.reserveOffset());
            writer.write(entry.u32_0c);
            writer.write(entry.u32_10);
        }

        writer.align(16);
        stringPool.flush(writer);
        writer.align(16);
        return writer.buffer();
    }

    std::expected<std::vector<ActorAssetEntry>, Error> parseActorAssets(std::span<const std::byte> data) {

        try {
            return parseActorAssetsInternal(data);
        }
        catch (const ReaderException& e) {
            return std::unexpected(Error{ ErrorCode::ParseError, e.what() });
        }
        catch (const std::exception& e) {
            return std::unexpected(Error{ ErrorCode::SystemError,
                std::string("Parsing failed: ") + e.what()
                });
        }

    }

    std::expected<std::vector<std::byte>, Error> buildActorAssetParam(std::span<const ActorAssetEntry> entries) {

        try {
            return buildActorAssetParamInternal(entries);
        }
        catch (const std::exception& e) {
            return std::unexpected(Error{ ErrorCode::SystemError,
                std::string("Building failed: ") + std::string(e.what())
                });
        }
    }
}