#pragma once
#include <cstdint>

#pragma pack(push, 1)
struct ScriptState {
    void** vtable;
    char field1_0x8[8];
    char argBuffer[3760];
    int returnArgCount;
    char padding[4];
    char returnBuffer[576];
};

// Arbritrary, for any point
struct ScriptContextManager {
    void* vtable;
    char scriptManager[2000];
};

struct PhaseScriptManager {
    void* vtable;
    char scriptManager[2000];
};
struct RootScriptManager {
    void* vtable;
    char scriptManager[2000];
};
struct GameScriptManager {
    void* vtable;
    char scriptManager[2000];
};

struct LuaCBinding {
    const char* name;
    void* func;
};

struct PlayerSaveData { 
    int unknown_id_0;
    char current_phase[20]; /* multiple strings, first is map name, second is possibly something to do with the entrance that was used */
    char field2_0x18[12];
    int LastPhaseInPoint;
    char field4_0x28[4];
    char player_name[32];
    int current_hp;
    int kaine_npc_current_hp; /* hp of party member kaine. no effect in actual gameplay, as party members cant take damage. likely that mechanic was cut. */
    int emil_npc_current_hp; /* see above */
    float current_mp;
    float kaine_npc_current_mp; /* always maxed out at some big ass number */
    float emil_npc_current_mp;
    int current_level; /* 0-indexed */
    int kaine_npc_current_level; /* always the same as current_level */
    int emil_npc_current_level; /* always the same as current_level */
    int current_xp;
    int kaine_npc_current_xp;
    int emil_npc_current_xp;
    char field18_0x7c[12];
    int currentWeapon;
    char field20_0x8c[20];
    int ability_bottom_left;
    int ability_bottom_right;
    int ability_top_left;
    int ability_top_right;
    char field25_0xb0[12];
    int gold;
    char Inventory[768];
    char field28_0x3c0[224];
    double total_play_time;
    char field30_0x4a8[4];
    char weaponLevels[64];
    char field32_0x4ec[2372];
    uint32_t isKainePlayer;
    int kaine_player_current_hp;
    int kaine_player_current_mp; /* unused in gameplay */
    int kaine_player_current_level; /* 0-indexed */
    int kaine_player_current_xp;
};

struct EndingsData {
    int unknown; /* only ever seen it as 6E */
    int endingsBitflags; /* flags that shows which endings are complete. see docs */
    char old_name[32]; /* name entered in the save that was wiped for ending e */
};


// 1.2.1

struct CPlayerParam {
    void** pVtable;
    int64_t field1_0x8;
    int charIndex; // 0 = player / player kaine, 1 = npc kaine, 2 = npc emil 
    int32_t field3_0x14;
    int maxHP;
    float maxMP;
    int64_t field6_0x20[3];
    int32_t field7_0x38;
    int attack_stat;
    int magickAttack_stat;
    int64_t field10_0x44;
    int defense_stat;
    int magickDefense_stat;
    int32_t field13_0x54;
    void* statusSpecData;
};


struct PlayableManager {
    void* pVtable;
    int64_t unk[533];
    struct ActorPlayable* actorPlayable;
    struct ActorWhiteBook* actorWhiteBook;
};

struct ActorWhiteBook {
    void** vtable;
};

struct ActorPlayable {
    void** vtable;
    int64_t field1_0x8[18];
    float rotationVal3;
    float posX;
    float field4_0xa0;
    float field5_0xa4;
    float rotationVal1;
    float posY;
    float field8_0xb0;
    float field9_0xb4;
    float rotationVal4;
    float posZ;
    float field12_0xc0;
    float field13_0xc4;
    float rotationVal2;
    float field15_0xcc;
};

#pragma pack(pop) 