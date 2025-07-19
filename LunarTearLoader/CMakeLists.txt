project ("LunarTearLoader" LANGUAGES C CXX ASM_NASM)

find_package(minhook REQUIRED CONFIG)

add_library (LunarTearLoader SHARED "src/LunarTearLoader.cpp" "src/stubs.asm" "src/tex.h" "src/Logger.h" "src/Logger.cpp")

target_link_libraries(LunarTearLoader PRIVATE minhook::minhook)

#add_executable(LunarTearLauncher "src/launcher.cpp")
