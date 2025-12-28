#include <Common/Patch.h>
#include <Game/Globals.h>

#pragma pack(push, 1)

struct WeaponUpgradeRecepie {

    uint32_t upgradeCost;
    int32_t ingredientId1;
    uint32_t ingredientCount1;
    int32_t ingredientId2;
    uint32_t ingredientCount2;
    int32_t ingredientId3;
    uint32_t ingredientCount3;

} ;

struct WeaponStats {

    uint32_t attack;
    uint32_t magicPower;
    uint32_t guardBreak;
    uint32_t armourBreak;
    float float_0x10;
    uint32_t weight;
    float float_0x18;
    float float_0x1c;

} ;


typedef struct {

    uint32_t uint32_0x00;
    uint32_t uint32_0x04;
          
    uint32_t offestToJPName;
    uint32_t offsetToInternalWeaponName; // Offset from this field in memory
          
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
    float float_0x64;
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

    WeaponStats level1Stats;
    WeaponStats level2Stats;
    WeaponUpgradeRecepie level2Recepie;
    WeaponStats level3Stats;
    WeaponUpgradeRecepie level3Recepie;
    WeaponStats level4Stats;
    WeaponUpgradeRecepie level4Recepie;

    uint32_t uint32_0x168;

    uint8_t  uint8_0x16C;
    uint8_t  uint8_0x16D;
    uint8_t  uint8_0x16E;
    uint8_t  uint8_0x16F;

    uint32_t uint32_0x170;

} WeaponSpecBody;

#pragma pack(pop) 

void ApplyWeaponHooks() {
    
    WeaponSpecBody** specs = (WeaponSpecBody**)(g_processBaseAddress + 0x47c2670);



    char* weaponBuffer = new char[50000];

    
	memcpy(weaponBuffer, *specs, sizeof(WeaponSpecBody)); // copy the weapon data of the first weapon
    memcpy((char*)weaponBuffer + sizeof(WeaponSpecBody), "lwsord010", 10);
    memcpy((char*)weaponBuffer+10 + sizeof(WeaponSpecBody), u8"カイネの剣", 40);





	((WeaponSpecBody*)weaponBuffer)->offsetToInternalWeaponName = (uint32_t)((char*)weaponBuffer + sizeof(WeaponSpecBody)) - (uint32_t) & ((WeaponSpecBody*)weaponBuffer)->offsetToInternalWeaponName;
    ((WeaponSpecBody*)weaponBuffer)->offestToJPName = (uint32_t)((char*)weaponBuffer + sizeof(WeaponSpecBody)) + 10 - (uint32_t) & ((WeaponSpecBody*)weaponBuffer)->offestToJPName;
	((WeaponSpecBody*)weaponBuffer)->weaponID = 17; // assign unused weapon ID
    ((WeaponSpecBody*)weaponBuffer)->listOrder = 39;
    ((WeaponSpecBody*)weaponBuffer)->nameStringID = 1110200;


    playerSaveData->weaponLevels[17] = 2; // give the player the weapon
    specs[17] = (WeaponSpecBody*)weaponBuffer; // assign the spec data globally (weapon ID 17 is unused)

}
