#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <expected>
#include <array>
#include <span>

#include "replicant/core/common.h"

namespace replicant::weapon {


    #pragma pack(push, 1)
    struct WeaponStats {
        uint32_t attack = 0;
        uint32_t magicPower = 0;
        uint32_t guardBreak = 0;
        uint32_t armourBreak = 0;
        float float_0x10 = 0.0f;
        uint32_t weight = 0;
        float float_0x18 = 0.0f;
        float float_0x1c = 0.0f;
    };

    struct WeaponUpgradeRecipe {
        uint32_t upgradeCost = 0;
        int32_t ingredientId1 = 0;
        uint32_t ingredientCount1 = 0;
        int32_t ingredientId2 = 0;
        uint32_t ingredientCount2 = 0;
        int32_t ingredientId3 = 0;
        uint32_t ingredientCount3 = 0;
    };
    #pragma pack(pop)

    struct WeaponEntry {
    public:

        uint32_t uint32_0x00_entry = 0;

        uint32_t uint32_0x00 = 0;
        uint32_t uint32_0x04 = 0;

        std::string internalNameHeader;
        std::string jpName;
        std::string internalNameBody;

        uint32_t weaponID = 0;
        uint32_t nameStringID = 0;
        uint32_t descStringID = 0;
        uint32_t unkStringID1 = 0;
        uint32_t unkStringID2 = 0;
        uint32_t unkStringID3 = 0;
        uint32_t storyStringID = 0;

        uint32_t uint32_0x2C = 0;
        uint32_t uint32_0x30 = 0;
        uint32_t uint32_0x34 = 0;
        uint32_t listOrder = 0;

        float float_0x3C = 0;
        uint32_t uint32_0x40 = 0;
        uint32_t uint32_0x44 = 0;

        float float_0x48 = 0;
        uint32_t uint32_0x4C = 0;
        uint32_t uint32_0x50 = 0;

        float HeightOnBack = 0; 
        uint32_t uint32_0x58 = 0;
        uint32_t uint32_0x5C = 0;

        float float_0x60;
        uint32_t uint32_0x64;
        uint32_t uint32_0x68;

        float InHandDisplacment; 
        uint32_t uint32_0x70;
        uint32_t uint32_0x74;

        float float_0x78;
        uint32_t uint32_0x7C;
        uint32_t uint32_0x80;

        int32_t int32_0x84;
        uint32_t shopPrice = 0;
        float knockbackPercent = 0.0f;
        uint32_t uint32_0x90;

        WeaponStats level1Stats;
        WeaponStats level2Stats;
        WeaponUpgradeRecipe level2Recipe;
        WeaponStats level3Stats;
        WeaponUpgradeRecipe level3Recipe;
        WeaponStats level4Stats;
        WeaponUpgradeRecipe level4Recipe;

        uint32_t uint32_0x168;
        uint8_t  uint8_0x16C;
        uint8_t  uint8_0x16D;
        uint8_t  uint8_0x16E;
        uint8_t  uint8_0x16F;
        float float_0x170;
    };

    std::expected<std::vector<WeaponEntry>, Error> openWeaponSpecs(std::span<const std::byte> data);
    std::expected<std::vector<std::byte>, Error> serialiseWeaponSpecs (std::span<const WeaponEntry> entries);
}


namespace replicant::raw {
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