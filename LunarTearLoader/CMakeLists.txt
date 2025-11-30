set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project ("LunarTearLoader" LANGUAGES C CXX ASM_NASM)

find_package(minhook REQUIRED CONFIG)
find_package(unofficial-inih REQUIRED CONFIG)
find_package(crc32c REQUIRED CONFIG)
find_package(zstd REQUIRED CONFIG)
find_package(nlohmann_json REQUIRED CONFIG)


set_source_files_properties(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/API/api.cpp"
    PROPERTIES INTERPROCEDURAL_OPTIMIZATION FALSE
)

add_library(LunarTearLoader SHARED
    "src/DLLMain.cpp"
    "src/ModLoader.cpp"
    "src/ModLoader.h"
    "src/Common/Dump.cpp"
    "src/Common/Dump.h"
    "src/Common/Logger.cpp"
    "src/Common/Logger.h"
    "src/Common/Settings.h"
    "src/Game/TextureFormats.h"
    "src/Hooks/TableStubs.asm"
    "src/Hooks/ScriptInjectionStubs.asm"
 
    "src/Hooks/TableHooks.cpp"
    "src/Hooks/TextureHooks.cpp"
    "src/Hooks/Hooks.h"
    "src/Hooks/LuaDebugHooks.cpp"
    "src/Game/Functions.h"
    "src/Game/Types.h"
    "src/Game/Functions.cpp"
    "src/Lua/LuaCommandQueue.cpp"
    "src/Lua/LuaCommandQueue.h"
    "src/Hooks/ScriptInjectionHooks.cpp"
    "src/Game/Globals.h"
    "src/Game/Globals.cpp"
    "src/Hooks/ScriptUpdateHooks.cpp"
    "src/API/api.cpp"
    "src/API/api.h"
    "src/API/LunarTear.h"
    "src/Lua/CoreBindings.h"
    "src/Lua/CoreBindings.cpp"

 "src/Hooks/VFSHooks.cpp" "include/tether/tether.h" "src/VFS/ArchivePatcher.cpp" "src/VFS/ArchivePatcher.h"   "src/Lua/CallbackQueue.cpp" "src/Init.cpp" "src/Game/d3d11.h" "src/Hooks/FPSUnlockHooks.cpp"  "src/Common/Patch.h" "src/Common/Patch.cpp"  "src/Game/d3d11.cpp" "src/Game/dinput8.h" "src/Hooks/EnumDevicesHooks.cpp" "src/Common/Backup.h")

add_executable(LunarTearLauncher "src/Launcher.cpp")


target_include_directories(LunarTearLoader PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(LunarTearLoader PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)


target_link_libraries(LunarTearLoader PRIVATE minhook::minhook unofficial::inih::inireader libreplicant 
                                              Crc32c::crc32c nlohmann_json::nlohmann_json)

target_link_libraries(LunarTearLauncher PRIVATE ) 