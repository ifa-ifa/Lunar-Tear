#pragma once
#include <Windows.h>
#include "Game/Types.h"
#include "Game/Globals.h"
#include <cstdint>

void InitialiseGameFunctions();

extern int (*ScriptManager_LoadAndRunBuffer)(void*, void*, size_t) ;

extern uint64_t(*ScriptManager_registerBindings)(void*, LuaCBinding*);

extern int (*lua_gettop)(void*);
extern void (*lua_settop)(void*, int);

extern uint64_t(*PrepareScriptFunctionCall)(void* scriptManager, const char* funcname, uint32_t* argcount);
extern uint64_t(*ExecuteScriptCoroutine)(void* scriptManager, int* param_2);

extern int (*luaBindingDispatcher)(void*, void*);

extern void* (*GetArgumentPointer)(void*, int);

extern const char* (*GetArgumentString)(void*);
extern float (*GetArgumentFloat)(void*);
extern int   (*GetArgumentInt)(void*);

extern void* (*SetArgumentFloat)(void*, float);
extern void* (*SetArgumentInt)(void*, int);
extern void* (*SetArgumentString)(void*, char*);


extern bool (*AnyEndingSeen)(EndingsData* endingsData);
extern bool (*EndingESeen)(EndingsData* endingsData);
extern bool (*CheckRouteEActive)(EndingsData* endingsData);

extern char* (*GetLocalizedString)(void* localedata, int strId);

extern int (*getCurrentLevel)(void);
extern uint64_t(*getPlayerItemCount)(PlayerSaveData* saveData, unsigned int itemId);
extern void (*setCharacterLevelInSaveData)(PlayerSaveData* saveData, unsigned int charIndex, int newLevel);
extern void (*setPlayerItemCount)(PlayerSaveData* saveData, unsigned int itemId, char count);

extern void (*SpawnEnemyGroup)(int EnemyGroupID, unsigned char param_2, float param_3);

extern void (*GameFlagOn)(void* unused, unsigned int flagId);
extern void (*GameFlagOff)(void* unused, unsigned int flagId);
extern uint64_t(*IsGameFlag)(void* unused, unsigned int flagId);
extern void (*SnowGameFlagOn)(void* unused, unsigned int flagId);
extern void (*SnowGameFlagOff)(void* unused, unsigned int flagId);
extern uint64_t(*IsSnowGameFlag)(void* unused, unsigned int flagId);