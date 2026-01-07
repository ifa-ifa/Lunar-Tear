#include "replicant/bxon.h"
#include "replicant/core/writer.h"
#include "replicant/core/reader.h"
#include <cstring>

namespace replicant {

    namespace {
#pragma pack(push, 1)
        struct RawBxonHeader {
            char     magic[4]; 
            uint32_t version;
            uint32_t projectId;
            uint32_t offsetToAssetType;
            uint32_t offsetToAssetData;
        };
#pragma pack(pop)
    }

    std::pair<BxonHeaderInfo, std::span<const std::byte>>
        ParseBxonInternal(std::span<const std::byte> data) {

        Reader reader(data);

        const RawBxonHeader* rawHeader = reader.view<RawBxonHeader>();

        if (std::strncmp(rawHeader->magic, "BXON", 4) != 0) {
			throw ReaderException("Invalid BXON magic");
        }

        std::string type = reader.readStringRelative(rawHeader->offsetToAssetType);

        const char* payloadPtr = reader.getOffsetPtr(rawHeader->offsetToAssetData);

        const std::byte* payloadStart = reinterpret_cast<const std::byte*>(payloadPtr);
        const std::byte* fileEnd = data.data() + data.size();

        if (payloadStart > fileEnd) {
			throw ReaderException("BXON asset data offset is out of bounds");
        }

        size_t payloadSize = fileEnd - payloadStart;

        BxonHeaderInfo info;
        info.version = rawHeader->version;
        info.projectId = rawHeader->projectId;
        info.assetType = std::move(type);

        return std::make_pair(info, std::span<const std::byte>(payloadStart, payloadSize));
    }

    std::vector<std::byte> BuildBxonInternal(
        const std::string& assetType,
        uint32_t version,
        uint32_t projectId,
        std::span<const std::byte> payload
    ) {

        Writer writer(64 + payload.size());

        writer.write("BXON", 4);
        writer.write(version);
        writer.write(projectId);

        size_t tokenAssetType = writer.reserveOffset();
        size_t tokenAssetData = writer.reserveOffset();

        writer.satisfyOffsetHere(tokenAssetType);
        writer.write(assetType.c_str(), assetType.length() + 1);

        writer.align(16);

        writer.satisfyOffsetHere(tokenAssetData);
        writer.write(payload.data(), payload.size());

        writer.align(16);

        return writer.buffer();
    }

    std::expected<std::pair<BxonHeaderInfo, std::span<const std::byte>>, Error>
        ParseBxon(std::span<const std::byte> data) {
        try {
            return ParseBxonInternal(data);
        }
        catch (const ReaderException& ex) {
            return std::unexpected(Error{ ErrorCode::ParseError, ex.what() });
		}
        catch (const std::exception& ex) {
            return std::unexpected(Error{ ErrorCode::SystemError, ex.what() });
        }
    }

    std::expected<std::vector<std::byte>, Error> BuildBxon(
        const std::string& assetType,
        uint32_t version,
        uint32_t projectId,
        std::span<const std::byte> payload
    ) {
        try {
            return BuildBxonInternal(assetType, version, projectId, payload);
        }
        catch (const std::exception& ex) {
            return std::unexpected(Error{ ErrorCode::SystemError, ex.what() });
		}

    }
}