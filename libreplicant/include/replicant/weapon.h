#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <expected>
#include <array>

namespace replicant::weapon {

    enum class WeaponErrorCode {
        Success,
        ParseError,
        BuildError
    };

    struct WeaponError {
        WeaponErrorCode code;
        std::string message;
    };

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

    std::expected<std::vector<WeaponEntry>, WeaponError> openWeaponSpecs(std::vector<std::byte>& data);
    std::expected<std::vector<std::byte>, WeaponError> serialiseWeaponSpecs (const std::vector<WeaponEntry>& entries);
}