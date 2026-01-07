#include "replicant/core/reader.h"
#include "replicant/core/writer.h"
#include "replicant/weapon.h"
#include <fstream>
#include <cstring>
#include <map>
#include <set>
#include <algorithm>

using namespace replicant::raw;

namespace replicant::weapon {

    std::vector<WeaponEntry> openWeaponSpecsInternal(std::span<const std::byte> data) {
        
        Reader reader(data);
        const RawHeader* header = reader.view<RawHeader>();
        const char* entriesStart = reader.getOffsetPtr(header->offsetToDataStart);

        reader.seek(entriesStart);

        std::vector<WeaponEntry> entries(header->entryCount);
		auto raw_entries = reader.viewArray<RawWeaponEntry>(header->entryCount);

        for (int i = 0; i < header->entryCount; i++) {

			RawWeaponEntry const& raw_entry = raw_entries[i];
			WeaponEntry& entry = entries[i];

            entry.uint32_0x00_entry = raw_entry.uint32_0x00;

			std::string internalNameHeader = reader.readStringRelative(raw_entry.offsetInternalNameHead);

			entry.internalNameHeader = internalNameHeader;

            entry.uint32_0x00 = raw_entry.body.uint32_0x00;
            entry.uint32_0x04 = raw_entry.body.uint32_0x04;

			std::string jpName = reader.readStringRelative(raw_entry.body.offestToJPName);

			entry.jpName = jpName;

			std::string internalNameBody = reader.readStringRelative(raw_entry.body.offsetToInternalWeaponName);

			entry.internalNameBody = internalNameBody;

            entry.weaponID = raw_entry.body.weaponID;
            entry.nameStringID = raw_entry.body.nameStringID;
            entry.descStringID = raw_entry.body.descStringID;
            entry.unkStringID1 = raw_entry.body.unkStringID1;
            entry.unkStringID2 = raw_entry.body.unkStringID2;
            entry.unkStringID3 = raw_entry.body.unkStringID3;
            entry.storyStringID = raw_entry.body.storyStringID;

            entry.uint32_0x2C = raw_entry.body.uint32_0x2C;
            entry.uint32_0x30 = raw_entry.body.uint32_0x30;
            entry.uint32_0x34 = raw_entry.body.uint32_0x34;
            entry.listOrder = raw_entry.body.listOrder;

            entry.float_0x3C = raw_entry.body.float_0x3C;
            entry.uint32_0x40 = raw_entry.body.uint32_0x40;
            entry.uint32_0x44 = raw_entry.body.uint32_0x44;

            entry.float_0x48 = raw_entry.body.float_0x48;
            entry.uint32_0x4C = raw_entry.body.uint32_0x4C;
            entry.uint32_0x50 = raw_entry.body.uint32_0x50;

            entry.HeightOnBack = raw_entry.body.float_0x54;
            entry.uint32_0x58 = raw_entry.body.uint32_0x58;
            entry.uint32_0x5C = raw_entry.body.uint32_0x5C;

            entry.float_0x60 = raw_entry.body.float_0x60;
            entry.uint32_0x64 = raw_entry.body.uint32_0x64;
            entry.uint32_0x68 = raw_entry.body.uint32_0x68;

            entry.InHandDisplacment = raw_entry.body.float_0x6C;
            entry.uint32_0x70 = raw_entry.body.uint32_0x70;
            entry.uint32_0x74 = raw_entry.body.uint32_0x74;

            entry.float_0x78 = raw_entry.body.float_0x78;
            entry.uint32_0x7C = raw_entry.body.uint32_0x7C;
            entry.uint32_0x80 = raw_entry.body.uint32_0x80;

            entry.int32_0x84 = raw_entry.body.int32_0x84;
            entry.shopPrice = raw_entry.body.shopPrice;
            entry.knockbackPercent = raw_entry.body.knockbackPercent;
            entry.uint32_0x90 = raw_entry.body.uint32_0x90;

            entry.level1Stats = raw_entry.body.level1Stats;
            entry.level2Stats = raw_entry.body.level2Stats;
            entry.level2Recipe = raw_entry.body.level2Recipe;
            entry.level3Stats = raw_entry.body.level3Stats;
            entry.level3Recipe = raw_entry.body.level3Recipe;
            entry.level4Stats = raw_entry.body.level4Stats;
            entry.level4Recipe = raw_entry.body.level4Recipe;

            entry.uint32_0x168 = raw_entry.body.uint32_0x168;

            entry.uint8_0x16C = raw_entry.body.uint8_0x16C;
            entry.uint8_0x16D = raw_entry.body.uint8_0x16D;
            entry.uint8_0x16E = raw_entry.body.uint8_0x16E;
            entry.uint8_0x16F = raw_entry.body.uint8_0x16F;

            entry.float_0x170 = raw_entry.body.float_0x170;
        }
        return entries;
	}

    std::vector<std::byte> serialiseWeaponSpecsInternal(std::span<const WeaponEntry> entries) {

		Writer writer(entries.size() * 380 + 500);
		StringPool stringPool;
        
        writer.write(static_cast<uint32_t>(entries.size()));
        size_t offsetToDataStartToken = writer.reserveOffset();
		writer.align(16);
		writer.satisfyOffsetHere(offsetToDataStartToken);

        for (size_t i = 0; i < entries.size(); i++) {

            const WeaponEntry& entry = entries[i];

            writer.write(entry.uint32_0x00_entry);
			stringPool.add(entry.internalNameHeader, writer.reserveOffset()); 
            
            writer.write(entry.uint32_0x00);
            writer.write(entry.uint32_0x04);

			stringPool.add(entry.jpName, writer.reserveOffset());
			stringPool.add(entry.internalNameBody, writer.reserveOffset());

			writer.write(entry.weaponID);
			writer.write(entry.nameStringID);
			writer.write(entry.descStringID);
			writer.write(entry.unkStringID1);
			writer.write(entry.unkStringID2);
			writer.write(entry.unkStringID3);
			writer.write(entry.storyStringID);
			writer.write(entry.uint32_0x2C);
			writer.write(entry.uint32_0x30);
			writer.write(entry.uint32_0x34);
			writer.write(entry.listOrder);
			writer.write(entry.float_0x3C);
			writer.write(entry.uint32_0x40);
			writer.write(entry.uint32_0x44);
			writer.write(entry.float_0x48);
			writer.write(entry.uint32_0x4C);  
			writer.write(entry.uint32_0x50);
			writer.write(entry.HeightOnBack);
			writer.write(entry.uint32_0x58);
			writer.write(entry.uint32_0x5C);
			writer.write(entry.float_0x60);
			writer.write(entry.uint32_0x64);
			writer.write(entry.uint32_0x68);
			writer.write(entry.InHandDisplacment);
			writer.write(entry.uint32_0x70);
			writer.write(entry.uint32_0x74);
			writer.write(entry.float_0x78);
			writer.write(entry.uint32_0x7C);
			writer.write(entry.uint32_0x80);
			writer.write(entry.int32_0x84);
			writer.write(entry.shopPrice);
			writer.write(entry.knockbackPercent);
			writer.write(entry.uint32_0x90);
			writer.write(entry.level1Stats);
			writer.write(entry.level2Stats);
			writer.write(entry.level2Recipe);
			writer.write(entry.level3Stats);
			writer.write(entry.level3Recipe);
			writer.write(entry.level4Stats);
			writer.write(entry.level4Recipe);
			writer.write(entry.uint32_0x168);
			writer.write(entry.uint8_0x16C);
			writer.write(entry.uint8_0x16D);
			writer.write(entry.uint8_0x16E);
			writer.write(entry.uint8_0x16F);
			writer.write(entry.float_0x170);
        }
		writer.align(16);
		stringPool.flush(writer);
        return writer.buffer();
    }

    std::expected<std::vector<WeaponEntry>, Error> openWeaponSpecs(std::span<const std::byte> data) {
        try {
            return openWeaponSpecsInternal(data);
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

    std::expected<std::vector<std::byte>, Error> serialiseWeaponSpecs(std::span<const WeaponEntry> entries) {
        try {
            return serialiseWeaponSpecsInternal(entries);
        }
        catch (const std::exception& e) {
            return std::unexpected(Error{ ErrorCode::SystemError,
                std::string("Serialization failed: ") + e.what()
                });
        }

    }
        


        
}

