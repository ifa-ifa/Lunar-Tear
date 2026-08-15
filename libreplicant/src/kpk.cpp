#include "replicant/kpk.h"
#include "replicant/core/reader.h"
#include "replicant/core/writer.h"
#include <cstring>

namespace replicant {

    namespace {
#pragma pack(push, 1)
        struct RawKpkHeader {
            char magic[4];
            uint32_t fileCount;
            uint32_t unknown;
        };
#pragma pack(pop)
    }

    KpkFile DeserializeInternal(std::span<const std::byte> data) {
        Reader reader(data);

        const RawKpkHeader* header = reader.view<RawKpkHeader>();

        // Magic can be either "KPK\x7F" or "KPKy"
        if (std::strncmp(header->magic, "KPK\x7F", 4) != 0 && std::strncmp(header->magic, "KPKy", 4) != 0) {
            throw ReaderException("Invalid KPK magic");
        }

        KpkFile file;
        std::memcpy(file.magic.data(), header->magic, 4);
        file.unknown = header->unknown;

        auto offsets = reader.viewArray<uint32_t>(header->fileCount);
        auto sizes = reader.viewArray<uint32_t>(header->fileCount);

        uint32_t fileNameSize = *reader.view<uint32_t>();

        std::span<const char> names;
        if (fileNameSize > 0) {
            names = reader.viewArray<char>(static_cast<size_t>(header->fileCount) * fileNameSize);
        }

        file.entries.reserve(header->fileCount);

        for (uint32_t i = 0; i < header->fileCount; i++) {
            KpkEntry entry;

            if (fileNameSize > 0) {
                const char* namePtr = names.data() + (static_cast<size_t>(i) * fileNameSize);
                size_t len = strnlen(namePtr, fileNameSize);
                entry.name = std::string(namePtr, len);
            }

            // Read Data
            if (offsets[i] > 0) {
                // KPK uses absolute offsets relative to the start of the file
                if (offsets[i] + sizes[i] > data.size()) {
                    throw ReaderException("KPK entry data exceeds file bounds");
                }
                const std::byte* entryData = data.data() + offsets[i];
                entry.data.assign(entryData, entryData + sizes[i]);
            }

            file.entries.push_back(std::move(entry));
        }

        return file;
    }

    std::expected<KpkFile, Error> KpkFile::Deserialize(std::span<const std::byte> data) {
        try {
            return DeserializeInternal(data);
        }
        catch (const ReaderException& ex) {
            return std::unexpected(Error{ ErrorCode::ParseError, ex.what() });
        }
        catch (const std::exception& ex) {
            return std::unexpected(Error{ ErrorCode::SystemError, ex.what() });
        }
    }

    std::vector<std::byte> KpkFile::SerializeInternal() const {
        Writer writer;

        writer.write(magic.data(), 4);
        uint32_t fileCount = static_cast<uint32_t>(entries.size());
        writer.write(fileCount);
        writer.write(unknown);

        size_t offsetsPos = writer.tell();
        for (uint32_t i = 0; i < fileCount; i++) {
            writer.write<uint32_t>(0);
        }

        for (uint32_t i = 0; i < fileCount; i++) {
            writer.write<uint32_t>(static_cast<uint32_t>(entries[i].data.size()));
        }

        uint32_t fileNameSize = 0;
        for (const auto& entry : entries) {
            if (entry.name.length() + 1 > fileNameSize) {
                fileNameSize = static_cast<uint32_t>(entry.name.length() + 1);
            }
        }

        if (fileNameSize > 0) {
            fileNameSize = (fileNameSize + 3) & ~3;
        }

        writer.write(fileNameSize);

        if (fileNameSize > 0) {
            for (const auto& entry : entries) {
                std::vector<char> nameBuf(fileNameSize, 0);
                std::memcpy(nameBuf.data(), entry.name.c_str(), entry.name.length());
                writer.write(nameBuf.data(), fileNameSize);
            }
        }

        writer.align(16);

        std::vector<uint32_t> actualOffsets(fileCount, 0);

        for (size_t i = 0; i < fileCount; i++) {
            if (entries[i].data.empty()) {
                actualOffsets[i] = 0;
                continue;
            }

            actualOffsets[i] = static_cast<uint32_t>(writer.tell());
            writer.write(entries[i].data.data(), entries[i].data.size());
            writer.align(16);
        }

        for (size_t i = 0; i < fileCount; i++) {
            std::memcpy(const_cast<std::byte*>(writer.buffer().data()) + offsetsPos + (i * 4), &actualOffsets[i], 4);
        }

        return writer.buffer();
    }

    std::expected<std::vector<std::byte>, Error> KpkFile::Serialize() const {
        try {
            return SerializeInternal();
        }
        catch (const std::exception& ex) {
            return std::unexpected(Error{ ErrorCode::SystemError, ex.what() });
        }
    }

    KpkEntry* KpkFile::findFile(const std::string& name) {
        for (auto& entry : entries) {
            if (entry.name == name) return &entry;
        }
        return nullptr;
    }

    const KpkEntry* KpkFile::findFile(const std::string& name) const {
        for (const auto& entry : entries) {
            if (entry.name == name) return &entry;
        }
        return nullptr;
    }

}