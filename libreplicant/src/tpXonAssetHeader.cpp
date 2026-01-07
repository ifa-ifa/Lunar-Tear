#include "replicant/tpXonAssetHeader.h"
#include "pack.cpp"
#include "replicant/core/reader.h"
#include "replicant/core/writer.h"
#include "replicant/core/common.h"

namespace replicant {

	namespace {

		struct RawXonAssetHeader {
			uint32_t assetsCount;
			uint32_t offsetToAssets;
			uint32_t importsCount;
			uint32_t offsetToImports;
		};

		struct RawXonAssetEntry {
			uint32_t offsetToAssetData;
		};

	}

	std::pair<std::vector<XonAssetEntry>, std::vector<ImportEntry>>
		parseXonAssetHeaderInternal(std::span<const std::byte> data) {

		Reader reader(data);
		const RawXonAssetHeader* header = reader.view<RawXonAssetHeader>();

		reader.seek(reader.getOffsetPtr(header->offsetToAssets));

		std::span<const RawXonAssetEntry> rawXonAssets = reader.viewArray<RawXonAssetEntry>(header->assetsCount);

		reader.seek(reader.getOffsetPtr(header->offsetToImports));

		std::span<const RawImport> rawImports = reader.viewArray<RawImport>(header->importsCount);


		// There is no easy way to know the size of each asset data without parsing them. Similar to resources in PACK files,
		// we will parse until the next offset or the start of the imports.

		std::vector<XonAssetEntry> xonAssets(header->assetsCount);

		for (size_t i = 0; i < header->assetsCount; i++) {
			RawXonAssetEntry const& rawEntry = rawXonAssets[i];
			XonAssetEntry& entry = xonAssets[i];

			// The wrapper is simply a u32 type hash followed by the data
			const char* assetDataWrapperStart = reader.getOffsetPtr(rawEntry.offsetToAssetData);
			
			const char* assetDataEnd = (i + 1 < header->assetsCount)
				? reader.getOffsetPtr(rawXonAssets[i + 1].offsetToAssetData)
				: reinterpret_cast<const char*>(reader.getOffsetPtr(header->offsetToImports));

			if (assetDataEnd < assetDataWrapperStart) {
				throw ReaderException("Xon asset data offsets are out of order");
			}

			entry.assetTypeHash = *(uint32_t*)assetDataWrapperStart;
			entry.assetData.assign(
				reinterpret_cast<const std::byte*>(assetDataWrapperStart+4),
				reinterpret_cast<const std::byte*>(assetDataEnd)
			);
		}

		
		std::vector<ImportEntry> imports(header->importsCount);

		for (size_t i = 0; i < header->importsCount; i++) {
			RawImport const& rawImport = rawImports[i];
			ImportEntry& entry = imports[i];

			entry.pathHash = rawImport.pathHash;
			entry.path = reader.readStringRelative(rawImport.offsetToPath);
		}

		return { xonAssets, imports };


	}



	std::vector<std::byte>
		buildXonAssetHeaderInternal(std::span<const XonAssetEntry> assets, std::span<const ImportEntry> imports) {

		Writer writer(assets.size() * 50 + imports.size() * 20);
		StringPool stringPool;

		writer.write(static_cast<uint32_t>(assets.size()));
		size_t offsetToAssetsToken = writer.reserveOffset();

		writer.write(static_cast<uint32_t>(imports.size()));
		size_t offsetToImportsToken = writer.reserveOffset();

		// Assets

		writer.align(16);
		writer.satisfyOffsetHere(offsetToAssetsToken);
		std::vector<size_t> assetDataOffsets;

		for (const auto& asset : assets) {
			size_t offsetToken = writer.reserveOffset();
			assetDataOffsets.push_back(offsetToken);
		}
		writer.align(16);
		// Asset Data

		for (size_t i = 0; i < assets.size(); ++i) {
			writer.satisfyOffsetHere(assetDataOffsets[i]);
			writer.write(assets[i].assetTypeHash);
			writer.write(assets[i].assetData.data(), assets[i].assetData.size());
			writer.align(16);
		}

		// Imports

		writer.align(16);
		writer.satisfyOffsetHere(offsetToImportsToken);
		for (const auto& imp : imports) {
			writer.write(fnv1_32(imp.path));
			size_t tName = writer.reserveOffset();
			writer.write<uint32_t>(0);
			stringPool.add(imp.path, tName);
		}

		writer.align(16);
		stringPool.flush(writer);
		return writer.buffer();
	}

	std::expected<std::pair<std::vector<XonAssetEntry>, std::vector<ImportEntry>>, Error>
		parseXonAssetHeader(std::span<const std::byte> data) {
	
		try {
			return parseXonAssetHeaderInternal(data);
		}
		catch (const ReaderException& ex) {
			return std::unexpected(Error{ ErrorCode::ParseError, ex.what() });
		}
		catch (const std::exception& ex) {
			return std::unexpected(Error{ ErrorCode::ParseError, ex.what() });
		}
	}

	std::expected<std::vector<std::byte>, Error>
		buildXonAssetHeader(std::span<const XonAssetEntry> entries, std::span<const ImportEntry> imports) {
	
		try {
			return buildXonAssetHeaderInternal(entries, imports);
		}
		catch (const std::exception& ex) {
			return std::unexpected(Error{ ErrorCode::SystemError, ex.what() });
		}
	}
}