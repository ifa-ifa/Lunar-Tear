#include "Game/Functions.h"
#include "Common/Logger.h"

int (*ScriptManager_LoadAndRunBuffer)(void*, void*, size_t);

uint64_t(*ScriptManager_registerBindings)(void*, LuaCBinding*);

int (*luaBindingDispatcher)(void*, void*);

int (*lua_gettop)(void*);
void (*lua_settop)(void*, int);

uint64_t(*PrepareScriptFunctionCall)(void* scriptManager, const char* funcname, uint32_t* argcount);
uint64_t(*ExecuteScriptCoroutine)(void* scriptManager, int* param_2);

void* (*GetArgumentPointer)(void*, int);

const char* (*GetArgumentString)(void*);
float (*GetArgumentFloat)(void*);
int   (*GetArgumentInt)(void*);

void* (*SetArgumentFloat)(void*, float);
void* (*SetArgumentInt)(void*, int);
void* (*SetArgumentString)(void*, char*);


bool (*AnyEndingSeen)(EndingsData* endingsData);
bool (*EndingESeen)(EndingsData* endingsData);
bool (*CheckRouteEActive)(EndingsData* endingsData);
char* (*GetLocalizedString)(void* localedata, int strId);

int (*getCurrentLevel)(void);
uint64_t(*getPlayerItemCount)(PlayerSaveData* saveData, unsigned int itemId);
void (*setCharacterLevelInSaveData)(PlayerSaveData* saveData, unsigned int charIndex, int newLevel);
void (*setPlayerItemCount)(PlayerSaveData* saveData, unsigned int itemId, char count);

void (*SpawnEnemyGroup)(int EnemyGroupID, unsigned char param_2, float param_3);

void (*GameFlagOn)(void* unused, unsigned int flagId);
void (*GameFlagOff)(void* unused, unsigned int flagId);
uint64_t(*IsGameFlag)(void* unused, unsigned int flagId);
void (*SnowGameFlagOn)(void* unused, unsigned int flagId);
void (*SnowGameFlagOff)(void* unused, unsigned int flagId);
uint64_t(*IsSnowGameFlag)(void* unused, unsigned int flagId);


void InitialiseGameFunctions() {


	ScriptManager_LoadAndRunBuffer = (int (*)(void*, void*, size_t))(g_processBaseAddress + 0x3F0350);
	ScriptManager_registerBindings = (uint64_t(*)(void*, LuaCBinding*))(g_processBaseAddress + 0x03efea0);
	luaBindingDispatcher = (int (*)(void*, void*))(g_processBaseAddress + 0x3efd80);

	lua_gettop = (int (*)(void*))(g_processBaseAddress + 0x3d68c0);
	lua_settop = (void (*)(void*, int))(g_processBaseAddress + 0x3d7280);
	PrepareScriptFunctionCall = (uint64_t(*)(void*, const char*, uint32_t*))(g_processBaseAddress + 0x3f04e0);
	ExecuteScriptCoroutine = (uint64_t(*)(void*, int*))(g_processBaseAddress + 0x3eff30);
	GetArgumentFloat = (float (*)(void*))(g_processBaseAddress + 0x3efa80);
	GetArgumentInt = (int (*)(void*))(g_processBaseAddress + 0x3efa60);
	GetArgumentPointer = (void* (*)(void*, int))(g_processBaseAddress + 0x3efa40);
	GetArgumentString = (const char* (*)(void*))(g_processBaseAddress + 0x3efab0);
	SetArgumentFloat = (void* (*)(void*, float))(g_processBaseAddress + 0x3ef8d0);
	SetArgumentInt = (void* (*)(void*, int))(g_processBaseAddress + 0x3ef8c0);
	SetArgumentString = (void* (*)(void*, char*))(g_processBaseAddress + 0x3ef8f0);


	AnyEndingSeen = (bool (*)(EndingsData*))(g_processBaseAddress + 0x3b3bc0);
	EndingESeen = (bool (*)(EndingsData*))(g_processBaseAddress + 0x3b3c10);
	CheckRouteEActive = (bool (*)(EndingsData*))(g_processBaseAddress + 0x3b3c00);
	GetLocalizedString = (char* (*)(void*, int))(g_processBaseAddress + 0x0d71d0);
	getCurrentLevel = (int (*)(void))(g_processBaseAddress + 0x3bbef0);
	getPlayerItemCount = (uint64_t(*)(PlayerSaveData*, unsigned int))(g_processBaseAddress + 0x3bbda0);
	SpawnEnemyGroup = (void (*)(int, unsigned char, float))(g_processBaseAddress + 0x425ba0);
	setCharacterLevelInSaveData = (void (*)(PlayerSaveData*, unsigned int, int))(g_processBaseAddress + 0x3be270);
	setPlayerItemCount = (void (*)(PlayerSaveData*, unsigned int, char))(g_processBaseAddress + 0x3be1a0);
	GameFlagOff = (void (*)(void*, unsigned int))(g_processBaseAddress + 0x3b0210);
	GameFlagOn = (void (*)(void*, unsigned int))(g_processBaseAddress + 0x3b0240);
	IsGameFlag = (uint64_t(*)(void*, unsigned int))(g_processBaseAddress + 0x3b0b90);
	SnowGameFlagOn = (void (*)(void*, unsigned int))(g_processBaseAddress + 0x3b1cc0);
	SnowGameFlagOff = (void (*)(void*, unsigned int))(g_processBaseAddress + 0x3b1c90);
	IsSnowGameFlag = (uint64_t(*)(void*, unsigned int))(g_processBaseAddress + 0x3b0cf0);

}