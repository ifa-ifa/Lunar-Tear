#define ZSTD_STATIC_LINKING_ONLY
#include <zstd.h>
#include <iostream>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include "replicant/arc.h"
#include "replicant/pack.h"

#include "replicant/core/io.h"


namespace replicant::archive {

    namespace {
        constexpr size_t SECTOR_ALIGNMENT = 16;
        constexpr size_t STREAM_BUFFER_SIZE = 128 * 1024;

        size_t align_to(size_t value, size_t alignment) {
            return (value + alignment - 1) & ~(alignment - 1);
        }

    }

    std::expected<std::vector<std::byte>, Error> Compress(std::span<const std::byte> data, CompressionConfig config) {
        if (data.empty()) return std::vector<std::byte>{};
        size_t bound = ZSTD_compressBound(data.size());
        std::vector<std::byte> buffer(bound);

        ZSTD_CCtx* cctx = ZSTD_createCCtx();
        if (!cctx) return std::unexpected(Error{ ErrorCode::SystemError, "Failed to create ZSTD Context" });

        ZSTD_CCtx_setParameter(cctx, ZSTD_c_compressionLevel, config.level);
        ZSTD_CCtx_setParameter(cctx, ZSTD_c_windowLog, config.windowLog);

        size_t cSize = ZSTD_compress2(
            cctx,
            buffer.data(),
            bound,
            data.data(),
            data.size()
        );

        ZSTD_freeCCtx(cctx);

        if (ZSTD_isError(cSize)) return std::unexpected(Error{ ErrorCode::SystemError, ZSTD_getErrorName(cSize) });
        buffer.resize(cSize);
        return buffer;
    }


    std::expected<std::vector<std::byte>, Error> Decompress(std::span<const std::byte> data, size_t decompressedSize) {
        if (data.empty()) return std::vector<std::byte>{};

        if (decompressedSize > 0) {
            std::vector<std::byte> output(decompressedSize);
            size_t dSize = ZSTD_decompress(output.data(), decompressedSize, data.data(), data.size());

            if (ZSTD_isError(dSize)) {
                return std::unexpected(Error{ ErrorCode::SystemError, ZSTD_getErrorName(dSize) });
            }
            if (dSize != decompressedSize) {
                return std::unexpected(Error{ ErrorCode::InvalidArguments, "Decompressed size mismatch" });
            }
            return output;
        }

        // Streaming decompression fallback if size is unknown (0)
        ZSTD_DCtx* dctx = ZSTD_createDCtx();
        if (!dctx) return std::unexpected(Error{ ErrorCode::SystemError, "Failed to create ZSTD DCtx" });

        std::vector<std::byte> output;
        output.reserve(data.size() * 4); // General heuristic

        ZSTD_inBuffer input = { data.data(), data.size(), 0 };
        size_t const buffOutSize = ZSTD_DStreamOutSize();
        std::vector<std::byte> outBuf(buffOutSize);

        while (input.pos < input.size) {
            ZSTD_outBuffer out = { outBuf.data(), outBuf.size(), 0 };
            size_t const ret = ZSTD_decompressStream(dctx, &out, &input);

            if (ZSTD_isError(ret)) {
                ZSTD_freeDCtx(dctx);
                return std::unexpected(Error{ ErrorCode::SystemError, ZSTD_getErrorName(ret) });
            }
            output.insert(output.end(), outBuf.data(), outBuf.data() + out.pos);
        }

        // Check if there's any pending output buffered when input is exhausted
        while (true) {
            ZSTD_outBuffer out = { outBuf.data(), outBuf.size(), 0 };
            ZSTD_inBuffer empty_input = { nullptr, 0, 0 };
            size_t const ret = ZSTD_decompressStream(dctx, &out, &empty_input);

            if (ZSTD_isError(ret)) {
                ZSTD_freeDCtx(dctx);
                return std::unexpected(Error{ ErrorCode::SystemError, ZSTD_getErrorName(ret) });
            }
            output.insert(output.end(), outBuf.data(), outBuf.data() + out.pos);
            if (out.pos == 0) break;
        }

        ZSTD_freeDCtx(dctx);
        return output;

    }

    std::expected<size_t, Error> GetDecompressedSize(std::span<const std::byte> data) {
        unsigned long long size = ZSTD_getFrameContentSize(data.data(), data.size());
        if (size == ZSTD_CONTENTSIZE_ERROR || size == ZSTD_CONTENTSIZE_UNKNOWN) {
            return std::unexpected(Error{ ErrorCode::SystemError, "Could not determine frame content size" });
        }
        return static_cast<size_t>(size);
    }

    std::expected<ArchiveResult, Error> Build(
        const std::filesystem::path& outputPath,
        const std::vector<ArchiveInput>& inputs,
        BuildMode mode,
        CompressionConfig config
    ) {
        ArchiveResult result;
        result.entries.reserve(inputs.size());

        std::ofstream outArc(outputPath, std::ios::binary | std::ios::trunc);
        if (!outArc) return std::unexpected(Error{ ErrorCode::IoError, "Failed to open output archive: " + outputPath.string() });

        if (mode == BuildMode::SingleStream) {
            // PRELOAD_DECOMPRESS (Type 0)
            ZSTD_CStream* cstream = ZSTD_createCStream();
            if (!cstream) return std::unexpected(Error{ ErrorCode::SystemError, "Failed to create ZSTD Stream" });

            size_t totalPledgedSize = 0;
            for (const auto& input : inputs) {
                std::error_code ec;
                uintmax_t physicalSize = std::filesystem::file_size(input.fullPath, ec);
                if (ec) {
                    ZSTD_freeCStream(cstream);
                    return std::unexpected(Error{ ErrorCode::IoError, std::format("Could not determine file size for pledge: {}", input.fullPath.string()) });
                }

                totalPledgedSize = align_to(totalPledgedSize, SECTOR_ALIGNMENT);
                totalPledgedSize += physicalSize;
            }
            ZSTD_CCtx_setPledgedSrcSize(cstream, totalPledgedSize);
            ZSTD_CCtx_setParameter(cstream, ZSTD_c_compressionLevel, config.level);
            ZSTD_CCtx_setParameter(cstream, ZSTD_c_windowLog, config.windowLog);

            std::vector<char> inBuf(STREAM_BUFFER_SIZE);
            std::vector<char> outBuf(ZSTD_CStreamOutSize());

            size_t currentUncompressedOffset = 0;

            for (const auto& input : inputs) {
                std::error_code ec;
                uintmax_t physicalSize = std::filesystem::file_size(input.fullPath, ec);
                if (ec) {
                    return std::unexpected(Error{ ErrorCode::IoError, std::format("Could not determine file size for pledge: {}", input.fullPath.string()) });
                }
                auto sizesRes = replicant::Pack::getFileSizes(input.fullPath);
                if (!sizesRes) { ZSTD_freeCStream(cstream); return std::unexpected(sizesRes.error()); }
                auto sizes = *sizesRes;

                size_t targetOffset = align_to(currentUncompressedOffset, SECTOR_ALIGNMENT);
                size_t padding_needed = targetOffset - currentUncompressedOffset;
                if (padding_needed > 0) {
                    std::vector<char> zeros(padding_needed, 0);
                    ZSTD_inBuffer zIn = { zeros.data(), zeros.size(), 0 };

                    while (zIn.pos < zIn.size) {
                        ZSTD_outBuffer zOut = { outBuf.data(), outBuf.size(), 0 };
                        size_t ret = ZSTD_compressStream(cstream, &zOut, &zIn);
                        if (ZSTD_isError(ret)) { ZSTD_freeCStream(cstream); return std::unexpected(Error{ ErrorCode::SystemError, ZSTD_getErrorName(ret) }); }
                        outArc.write(outBuf.data(), zOut.pos);
                    }
                    currentUncompressedOffset = targetOffset;
                }

                ArchiveEntryInfo entry;
                entry.name = input.name;
                entry.offset = currentUncompressedOffset; // Offset in DECOMPRESSED stream
                entry.compressedSize = 0; // Type 0 uses 0 here
                entry.packSerializedSize = sizes.serializedSize;
                entry.packResourceSize = sizes.resourceSize;
                result.entries.push_back(entry);

                // 3. Feed File Content to Compressor
                std::ifstream inFile(input.fullPath, std::ios::binary);
                if (!inFile) { ZSTD_freeCStream(cstream); return std::unexpected(Error{ ErrorCode::IoError, "Failed to read input: " + input.fullPath.string() }); }

                while (inFile) {
                    inFile.read(inBuf.data(), inBuf.size());
                    std::streamsize bytesRead = inFile.gcount();
                    if (bytesRead == 0) break;

                    ZSTD_inBuffer zIn = { inBuf.data(), static_cast<size_t>(bytesRead), 0 };
                    while (zIn.pos < zIn.size) {
                        ZSTD_outBuffer zOut = { outBuf.data(), outBuf.size(), 0 };
                        size_t ret = ZSTD_compressStream(cstream, &zOut, &zIn);
                        if (ZSTD_isError(ret)) { ZSTD_freeCStream(cstream); return std::unexpected(Error{ ErrorCode::SystemError, ZSTD_getErrorName(ret) }); }
                        outArc.write(outBuf.data(), zOut.pos);
                    }
                }
                currentUncompressedOffset += physicalSize;
            }

            // 4. Finish Stream
            ZSTD_outBuffer zOut = { outBuf.data(), outBuf.size(), 0 };
            size_t ret;
            do {
                zOut.pos = 0;
                ret = ZSTD_endStream(cstream, &zOut);
                if (ZSTD_isError(ret)) { ZSTD_freeCStream(cstream); return std::unexpected(Error{ ErrorCode::SystemError, ZSTD_getErrorName(ret) }); }
                outArc.write(outBuf.data(), zOut.pos);
            } while (ret > 0);

            ZSTD_freeCStream(cstream);
        }
        else {
            // STREAM (Type 1/2)
            // Compress each -> Align -> Record Offsets
            for (const auto& input : inputs) {
                auto infoRes = Pack::getFileSizes(input.fullPath);
                if (!infoRes) {
                    std::cerr << "Failed to get file sizes for " << input.fullPath << ": " << infoRes.error().toString() << "\n";
                    return std::unexpected(infoRes.error());
                };
                auto info = *infoRes;

                // Read file
                auto fileBlob = ReadFile(input.fullPath);
                if (!fileBlob) return std::unexpected(Error{ ErrorCode::IoError, "Failed to read " + input.fullPath.string() });

                // Compress
                auto compressRes = Compress(*fileBlob, config);
                if (!compressRes) return std::unexpected(compressRes.error());

                // Release input memory immediately
                fileBlob->clear();
                fileBlob->shrink_to_fit();

                size_t cSize = compressRes->size();
                size_t alignedSize = align_to(cSize, SECTOR_ALIGNMENT);
                size_t padding = alignedSize - cSize;

                ArchiveEntryInfo entry;
                entry.name = input.name;
                entry.offset = static_cast<uint64_t>(outArc.tellp());
                entry.compressedSize = static_cast<uint32_t>(cSize);
                entry.packSerializedSize = info.serializedSize;
                entry.packResourceSize = info.resourceSize;
                result.entries.push_back(entry);

                // Write compressed data
                outArc.write(reinterpret_cast<const char*>(compressRes->data()), cSize);

                // Write padding
                if (padding > 0) {
                    std::vector<char> pad(padding, 0);
                    outArc.write(pad.data(), padding);
                }
            }
        }

        return result;
    }
}