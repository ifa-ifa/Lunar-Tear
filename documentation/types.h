struct ActorFactoryData {
    int actor_id;
    float fUnknown_0;
    float fUnknown_1;
    float fUnknown_2; /* often  Pi */
    float fUnknown_3;
    int iUnknown_4;
    void *func_slot_0;
    void *func_slot_1;
    void *func_slot_2;
};


struct ActorHandle {
    int index;
    int generation_count; /* incremented each time a slot is reused to that stale pointers can be invalidated */
};


struct ActorInfo {
    char name[64];
    int actor_id;
    int iUnknown_0;
    char *pUnknown_1;
    int iUnknown_2;
    int iUnknown_3;
    byte auUnknown_4[40];
};



struct CActor_vftable {
    void *field0_0x0[4];
    void *vfunc_Initialize;
    void *field2_0x28[157];
    void *vfunc_InitializePhysicsAndAI;
    void *field4_0x518[20];
    void *field5_0x5b8[16];
    void *vfunc_UpdateTransform;
};

struct CParamSetForVehicle {
    int64_t vtable_pointer; /* 0xae65a8 */
    byte unknown_0[24];
    int cparamsetforvehcile_id;
    int32_t unknown_1;
    byte unknown_2[168];
    int32_t unknown_3;
    int32_t unknown_4;
    int64_t unknown_5[5];
    int32_t unknown_6;
    int32_t unknown_7;
    int32_t unknown_8;
    int32_t unknown_9;
    int64_t unknown_10[14];
    int32_t unknown_11[2];
    int64_t unknown_12[8];
    int16_t unknown_13[4];
    int32_t unknown_14[4];
    int64_t unknown_15[33];
    int32_t unknown_16;
    int health;
    int64_t unknown_17;
    int64_t unknown_18[5];
    float x_pos;
    float y_pos;
    float z_pos;
};

struct ActorTransform {
    float right_x;
    float up_x;
    float forward_x;
    float pos_x;
    float right_y;
    float up_y;
    float forward_y;
    float pos_y;
    float right_z;
    float up_z;
    float forward_z;
    float pos_z;
    float right_w;
    float up_w;
    float forward_w;
    float pos_w;
};

struct cActor {
    struct CActor_vftable *vtable_pointer;
    struct cActor *prev_actor_primary; /* primary linked list (see linkedlistpari_ */
    struct cActor *next_actor_primary;
    struct cActor *prev_actor_secondary; /* secondary linked list */
    struct cActor *next_actor_secondary;
    byte auUnknown_0[168];
    struct ActorHandle self_handle;
    byte uUnknown_1[8];
    struct tp_effect_lib_CRingBuff *ringBuffComponent;
    union Matrix4x4 *pTransformBuffer; /* contains pos, rotations */
    byte auUnknown_2[232];
    int actor_id;
    byte auUnknown_3[132];
    struct tp_mot_CDampNo *pDampNoComponent;
    byte auUnknown_4[82152];
    struct CParamSetForVehicle *cparams;
};

struct ActorListController { /* Linked list */
    struct cActor *head;
    struct cActor *tail;
    INT64 count;
};

struct tp_mot_CDampNo {
    void *vtable_pointer;
    byte padding[56];
};

struct tp_effect_lib_CRingBuff {
    void *vtable_pointer;
    byte padding[168];
};

struct Vector4 {
    float x;
    float y;
    float z;
    float w;
};

union Matrix4x4 {
    float f[16];
    float m[4][4];
    struct Vector4 c[4];
    struct ActorTransform a;
};

struct ActorListControllerPair { /* stores primary and secondary controllers */
    struct ActorListController primary_list;
    struct ActorListController secondary_list;
};

struct cActorAnimal {
    struct cActor actor;
};

struct cActorPlayable {
    struct cActor actor;
};

struct CHandleManager {
    void *vftable_pointer;
    int next_available_entity_id; /* entity ids are not freed when an entity is destroyed/unloaded */
    byte padding_0[4];
    struct cActor *entity_pointer_pool[4096]; /* pool is populated in REVERSE ORDER! */
    int generation_id_table[4096]; /* saftey check - returned as part of handle, incremented each time a new object is put in that place */
    int free_list_indicies[4096]; /* list of indicies that are free and available to store an object */
    int free_list_count; /* amount of free indicies */
};


struct CPlayerParam_vtable {
    void *field0_0x0;
    void *field1_0x8;
    void *field2_0x10;
    void *field3_0x18;
    void *field4_0x20;
    void *field5_0x28;
    void *field6_0x30;
    void *field7_0x38;
    void *field8_0x40;
    void *field9_0x48;
    void *field10_0x50;
    void *field11_0x58;
    void *field12_0x60;
    void *field13_0x68;
    void *field14_0x70;
    void *field15_0x78;
    void *field16_0x80;
    void *field17_0x88;
    void *field18_0x90;
    void *field19_0x98;
    void *field20_0xa0;
    void *changeMaxHP;
    void *field22_0xb0;
    void *field23_0xb8;
    void *field24_0xc0;
    void *field25_0xc8;
    void *field26_0xd0;
    void *field27_0xd8;
    void *field28_0xe0;
};

struct CPlayerParam {
    struct CPlayerParam_vtable *pVtable;
    byte field1_0x8;
    byte field2_0x9;
    byte field3_0xa;
    byte field4_0xb;
    byte field5_0xc;
    byte field6_0xd;
    byte field7_0xe;
    byte field8_0xf;
    int charIndex; /* 0 = player / player kaine, 1 = npc kaine, 2 = npc emil */
    byte field10_0x14;
    byte field11_0x15;
    byte field12_0x16;
    byte field13_0x17;
    int maxHP;
    float maxMP;
    byte field16_0x20[28];
    int attack_stat;
    int magickAttack_stat;
    byte field19_0x44[8];
    int defense_stat;
    int magickDefense_stat;
    byte field22_0x54[4];
    void *statusSpecData;
};

struct EndingsData {
    int unknown; /* only ever seen it as 6E */
    int endingsBitflags; /* flags that shows which endings are complete. see docs */
    char old_name[32]; /* name entered in the save that was wiped for ending e */
};

typedef enum GameDifficultyMode {
    easy=0,
    medium=1,
    hard=2
} GameDifficultyMode;


struct KPK_Header { /* also known as PAK. this structure is impossible to display as a struct becuase it is dynamic. */
    char magic[4];
    int fileCount;
};

struct Lua_LoadS {
    char *s;
    size_t size;
};

struct LuaState {
    void *next;
    byte tt;
    byte marked;
    byte padding_0[6];
    void *top;
    void *base;
    void *global_state;
    void *callInfo;
    void *stack_last;
    void *stacksize;
    byte padding_1[4];
    void *end_ci;
    byte *base_ci;
    uint16 size_ci;
    uint16 nCcalls;
    byte hookmask;
    byte allowhook;
    byte hookinit;
    byte padding_3;
    int basehookcount;
    int hookcount;
    void *hook;
    byte _gt[16];
    void *openupval;
    void *gclist;
    void *errorJmp;
    void *errfunc;
};

struct MBuffer {
    char *buffer;
    SIZE_T buffsize;
};


struct tp_fnd_TextImpl_char_32 { /* small string optimisation class */
    void *pVftable;
    char internal_buffer[32];
    char *external_buffer;
    int64_t size;
};

struct tp::mem::TpAllocator::vftable {
    void *tp_mem_TpAllocator_vfunc_allocate;
    void *tp_mem_TpAllocator_vfunc_deallocate;
};

struct MemoryManager {
    void *pVtable;
    struct tp::mem::TpAllocator *pAllocator_Default;
    struct tp::mem::TpAllocator *pAllocator_Actor;
    struct tp::mem::TpAllocator *pAllocator_unknown0;
    struct tp::mem::TpAllocator *pAllocator_Motion;
    struct tp::mem::TpAllocator *pAllocator_unknown1;
    void *pAllocator_from_func_1;
    void *pAllocator_from_func_2;
    void *pAllocator_from_func_3;
    void *pAllocator_from_func_4;
    void *pAllocator_from_func_5;
    void *pAllocator_from_func_6;
    struct tp::mem::TpAllocator *pAllocator_Block;
    struct tp::mem::TpAllocator *pAllocator_GISys;
    void *pAllocator_from_func_7;
    void *pAllocator_from_func_8;
};

struct tp::mem::TpAllocator {
    struct tp::mem::TpAllocator::vftable *pVtable;
    char *name;
    void *heap;
    void *unknown;
    struct tp_fnd_TextImpl_char_32 field4_0x20;
    byte padding[8];
};

struct ScriptState {
    void *vTable;
    undefined field1_0x8[80];
};

struct ScriptManager {
    void *pVtable;
    byte auUnk_0[192];
    void *luaState;
    void *pUnk_1;
    struct ScriptState scriptState;
};

struct PhaseScriptManager {
    void *pVtable;
    struct ScriptManager scriptManager;
};

struct PlayerSaveData { /* player data */
    int unknown_id_0;
    char map_string[20]; /* multiple strings, first is map name, second is possibly something to do with the entrance that was used */
    undefined unknown[20];
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
    byte field16_0x7c[36];
    int ability_bottom_left;
    int ability_bottom_right;
    int ability_top_left;
    int ability_top_right;
    byte field21_0xb0[12];
    int gold; /* money */
    INT8 Inventory[768];
    byte field24_0x3c0[224];
    double total_play_time;
    byte field26_0x4a8[2440];
    BOOL isKainePlayer;
    int kaine_player_current_hp;
    int kaine_player_current_mp; /* unused in gameplay */
    int kaine_player_current_level; /* 0-indexed */
    int kaine_player_current_xp;
};


struct snow::GameDifficult {
    float field0_0x0_(1.5);
    float mp_regen_easy_(3.0);
    float field2_0x8;
    float field3_0xc;
    float field4_0x10_(1.0);
    float field5_0x14;
    float field6_0x18;
    float field7_0x1c;
    float field8_0x20;
    float field9_0x24;
    float field10_0x28;
    float field11_0x2c;
    float field12_0x30;
    float field13_0x34;
    float field14_0x38;
    float field15_0x3c;
    float field16_0x40;
    float field17_0x44;
    float field18_0x48;
    float field19_0x4c;
    float field20_0x50;
    float field21_0x54;
    float field22_0x58;
    float field23_0x5c;
    float field24_0x60;
    float field25_0x64;
    float field26_0x68;
    float field27_0x6c;
    float field28_0x70;
    float mp_regen_medium_(1.0);
    float field30_0x78;
    float field31_0x7c;
    float field32_0x80_(1.0);
    float field33_0x84;
    float field34_0x88;
    float field35_0x8c;
    float field36_0x90;
    float field37_0x94;
    float field38_0x98;
    float field39_0x9c;
    float field40_0xa0;
    float field41_0xa4;
    float field42_0xa8;
    float field43_0xac;
    float field44_0xb0;
    float field45_0xb4;
    float field46_0xb8;
    float field47_0xbc;
    float field48_0xc0;
    float field49_0xc4;
    float field50_0xc8;
    float field51_0xcc;
    float field52_0xd0;
    float field53_0xd4;
    float field54_0xd8;
    float field55_0xdc;
    float MaxHpFactorHard_(0.7);
    float mp_regen_hard_(1.0);
    float field58_0xe8;
    float field59_0xec_(3.0);
    float field60_0xf0_(1.0);
    float field61_0xf4;
    float field62_0xf8;
    float field63_0xfc_(5.0f);
    float field64_0x100_(15.0);
    float field65_0x104;
    float field66_0x108;
    float field67_0x10c;
    float field68_0x110;
    float field69_0x114;
    float field70_0x118;
    float field71_0x11c;
    float field72_0x120;
    float field73_0x124;
    float field74_0x128;
    float field75_0x12c;
    float field76_0x130;
    float field77_0x134;
    float field78_0x138;
    float field79_0x13c;
    float field80_0x140;
    float field81_0x144;
    float field82_0x148;
    float field83_0x14c;
    float field84_0x150_(1.0);
    float field85_0x154_(1.0);
    float field86_0x158;
    float field87_0x15c_(0.2);
    float field88_0x160_(0.2);
    float field89_0x164;
    float field90_0x168;
    float field91_0x16c_(1.0);
    float field92_0x170_(1.0);
};

struct snow::GameDifficult::Singleton {
    void *pVtable;
    struct snow::GameDifficult *pGameDifficult;
};

struct snow::PlayableManager {
    void *pVtable;
    byte unknown[4448];
};


struct ZIO {
    size_t n; /* bytes still unread */
    char *p; /* current position in buffer */
    char * (*reader)(void *, void *, size_t *);
    void *data; /* additional data  */
    char *name;
};

struct SParser {
    struct ZIO *z;
    struct MBuffer buff;
    int isBytecode;
};



struct STBL_Controller { /* High-level controller stored in static memory for a loaded STBL file.  holds convenient pointers to its various data sections (primary data, extended params, strings) and cached metadata (counts). */
    struct STBL_FileHeader *header;
    INT64 spatialEntityCount;
    struct STBL_SpatialEntity *spatialEntities;
    INT64 unknown_0;
    struct STBL_TableDescriptor *table_descriptors;
    INT64 table_count;
    struct STBL_TableDescriptor *table_descriptors_copy;
    void *unknown_1;
    INT64 unknown_2;
    void *unknown_3;
    INT64 unknown_4;
    void *unknown_5;
};

struct STBL_SpatialEntity {
    struct Vector4 position;
    struct Vector4 volume_data;
    char name[32];
    int32_t unknown_params[4];
    byte padding[16];
};

struct STBL_TableDescriptor {
    char tableName[32];
    int rowCount;
    int rowSizeInFields; /* number of Fields per row */
    int offsetToTableData; /* from start of header */
    byte padding_0[12];
    byte padding_1[8];
};

struct STBL_FileHeader {
    char magic_bytes[4]; /* "STBL" */
    int version;
    byte padding_0[4];
    int spatialEntityCount;
    int header_size;
    byte padding_1[4];
    int table_descriptor_offset; /* from start of header */
    int table_count;
    int table_descriptor_offset_copy;
    int unknown[11];
};


typedef enum STBL_FieldType {
    FIELD_TYPE_DEFAULT =0,
    FIELD_TYPE_INT =1,
    FIELD_TYPE_FLOAT =2,
    FIELD_TYPE_STRING_OFFSET=3,
    FIELD_TYPE_UNKNOWN=4,
    FIELD_TYPE_ERROR=5
} STBL_FieldType;


union STBL_FieldData {
    float as_float;
    int32_t as_int;
    int offset_to_string;
    byte rawdata[4];
};

struct STBL_Field {
    enum STBL_FieldType field0_0x0;
    union STBL_FieldData data; /* int, offset to string ,or float */
};


struct tp::res::ResFile {
    void *pVtable;
};


struct tp::res::ResourcePack {
};

