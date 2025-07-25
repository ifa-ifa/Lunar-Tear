﻿set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


project ("LunarTearLoader" LANGUAGES C CXX ASM_NASM)

find_package(minhook REQUIRED CONFIG)
find_package(unofficial-inih REQUIRED CONFIG)



add_library(LunarTearLoader SHARED
    "src/DLLMain.cpp"
    "src/ModLoader.cpp"
    "src/ModLoader.h"
    "src/Common/Dump.cpp"
    "src/Common/Dump.h"
    "src/Common/Logger.cpp"
    "src/Common/Logger.h"
    "src/Common/Settings.h"
    "src/Formats/TextureFormats.h"
    "src/Hooks/TableStubs.asm"
    "src/Hooks/ScriptStubs.asm"
    "src/Hooks/ScriptHooks.cpp"
    "src/Hooks/TableHooks.cpp"
    "src/Hooks/TextureHooks.cpp"
    "src/Hooks/Hooks.h"
  )

add_executable(LunarTearLauncher "src/Launcher.cpp")

target_include_directories(LunarTearLoader PRIVATE ${PROJECT_SOURCE_DIR}/src)

target_link_libraries(LunarTearLoader PRIVATE minhook::minhook unofficial::inih::inireader libsettbll)
