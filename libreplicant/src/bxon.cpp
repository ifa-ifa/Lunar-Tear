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

    std::expected<std::pair<BxonHeaderInfo, std::span<const std::byte>>, ReaderError>
        Bxon::Parse(std::span<const std::byte> data) {
        Reader reader(data);

        auto headerRes = reader.view<RawBxonHeader>();
        if (!headerRes) return std::unexpected(headerRes.error());
        const auto* raw = *headerRes;

        if (std::strncmp(raw->magic, "BXON", 4) != 0) {
            return std::unexpected(ReaderError{ ReaderErrorCode::InvalidPointer, "Invalid Magic: Not a BXON file" });
        }

        auto typeRes = reader.readStringRelative(raw->offsetToAssetType);
        if (!typeRes) return std::unexpected(typeRes.error());

        auto payloadPtrRes = reader.getOffsetPtr(raw->offsetToAssetData);
        if (!payloadPtrRes) return std::unexpected(payloadPtrRes.error());

        const std::byte* payloadStart = reinterpret_cast<const std::byte*>(*payloadPtrRes);
        const std::byte* fileEnd = data.data() + data.size();

        if (payloadStart > fileEnd) {
            return std::unexpected(ReaderError{ ReaderErrorCode::InvalidOffset, "Asset data starts beyond file end" });
        }

        size_t payloadSize = fileEnd - payloadStart;

        BxonHeaderInfo info;
        info.version = raw->version;
        info.projectId = raw->projectId;
        info.assetType = std::move(*typeRes);

        return std::make_pair(info, std::span<const std::byte>(payloadStart, payloadSize));
    }

    std::vector<std::byte> Bxon::Build(
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

        return writer.buffer();
    }
}