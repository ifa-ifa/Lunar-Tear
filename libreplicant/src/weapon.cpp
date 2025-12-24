#include "replicant/reader.h"
#include "replicant/writer.h"
#include "replicant/weapon.h"
#include <fstream>
#include <cstring>
#include <map>
#include <set>
#include <algorithm>

namespace {

    #pragma pack(push, 1)
    struct RawWeaponStats {
        uint32_t attack = 0;
        uint32_t magicPower = 0;
        uint32_t guardBreak = 0;
        uint32_t armourBreak = 0;
        float float_0x10 = 0.0f;
        uint32_t weight = 0;
        float float_0x18 = 0.0f;
        float float_0x1c = 0.0f;
    };
    struct RawWeaponUpgradeRecipe {
        uint32_t upgradeCost = 0;
        int32_t ingredientId1 = 0;
        uint32_t ingredientCount1 = 0;
        int32_t ingredientId2 = 0;
        uint32_t ingredientCount2 = 0;
        int32_t ingredientId3 = 0;
        uint32_t ingredientCount3 = 0;
    };
    struct RawHeader {
        uint32_t entryCount;
        uint32_t offsetToDataStart;
    };
    struct RawWeaponBody {
        uint32_t uint32_0x00;
        uint32_t uint32_0x04;    

        uint32_t offestToJPName;
        uint32_t offsetToInternalWeaponName;
              
        uint32_t weaponID;
        uint32_t nameStringID;
        uint32_t descStringID;
        uint32_t unkStringID1;
        uint32_t unkStringID2;
        uint32_t unkStringID3;
        uint32_t storyStringID;
              
        uint32_t uint32_0x2C;
        uint32_t uint32_0x30;
        uint32_t uint32_0x34;
        uint32_t listOrder;

        float float_0x3C;
        uint32_t uint32_0x40;
        uint32_t uint32_0x44;

        float float_0x48;
        uint32_t uint32_0x4C;
        uint32_t uint32_0x50;

        float float_0x54;
        uint32_t uint32_0x58;
        uint32_t uint32_0x5C;

        float float_0x60;
        uint32_t uint32_0x64;
        uint32_t uint32_0x68;

        float float_0x6C;
        uint32_t uint32_0x70;
        uint32_t uint32_0x74;

        float float_0x78;
        uint32_t uint32_0x7C;
        uint32_t uint32_0x80;

        int32_t int32_0x84;
        uint32_t shopPrice;
        float knockbackPercent;
        uint32_t uint32_0x90;

        replicant::weapon::WeaponStats level1Stats;
        replicant::weapon::WeaponStats level2Stats;
        replicant::weapon::WeaponUpgradeRecipe level2Recipe;
        replicant::weapon::WeaponStats level3Stats;
        replicant::weapon::WeaponUpgradeRecipe level3Recipe;
        replicant::weapon::WeaponStats level4Stats;
        replicant::weapon::WeaponUpgradeRecipe level4Recipe;

        uint32_t uint32_0x168;
        uint8_t  uint8_0x16C;
        uint8_t  uint8_0x16D;
        uint8_t  uint8_0x16E;
        uint8_t  uint8_0x16F;
        float float_0x170;
    };
    struct RawWeaponEntry {
        uint32_t uint32_0x00;
        uint32_t offsetInternalNameHead; 
        RawWeaponBody body;
    };
    #pragma pack(pop)
}

namespace replicant::weapon {

    std::expected<std::vector<WeaponEntry>, WeaponError> openWeaponSpecs(std::vector<std::byte>& data) {

        Reader reader(data);
		auto header_res = reader.view<RawHeader>();
        if (!header_res) {
            return std::unexpected(WeaponError{ WeaponErrorCode::ParseError, "Failed to read weapon specs header: " + header_res.error().message });
		}
		auto header = *header_res;

        std::vector<WeaponEntry> entries(header->entryCount);
		auto raw_entries_res = reader.viewArray<RawWeaponEntry>(header->entryCount);
        if (!raw_entries_res) {
            return std::unexpected(WeaponError{ WeaponErrorCode::ParseError, "Failed to read weapon specs entries: " + raw_entries_res.error().message });
        }
		auto raw_entries = *raw_entries_res;


        for (int i = 0; i < header->entryCount; i++) {

			RawWeaponEntry const& raw_entry = raw_entries[i];
			WeaponEntry& entry = entries[i];

            entry.uint32_0x00_entry = raw_entry.uint32_0x00;

			auto internalNameHeader = reader.readStringRelative(raw_entry.offsetInternalNameHead);
            if (!internalNameHeader) {
                return std::unexpected(WeaponError{ WeaponErrorCode::ParseError, "Failed to read weapon internal name header: " + internalNameHeader.error().message });
			}
			entry.internalNameHeader = *internalNameHeader;

            entry.uint32_0x00 = raw_entry.body.uint32_0x00;
            entry.uint32_0x04 = raw_entry.body.uint32_0x04;

			auto jpName = reader.readStringRelative(raw_entry.body.offestToJPName);
            if (!jpName) {
                return std::unexpected(WeaponError{ WeaponErrorCode::ParseError, "Failed to read weapon JP name: " + jpName.error().message });
			}
			entry.jpName = *jpName;

			auto internalNameBody = reader.readStringRelative(raw_entry.body.offsetToInternalWeaponName);
            if (!internalNameBody) {
                return std::unexpected(WeaponError{ WeaponErrorCode::ParseError, "Failed to read weapon internal name body: " + internalNameBody.error().message });
            }
			entry.internalNameBody = *internalNameBody;

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

    std::expected<std::vector<std::byte>, WeaponError> serialiseWeaponSpecs(const std::vector<WeaponEntry>& entries) {

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
}