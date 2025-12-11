cmake_minimum_required(VERSION 3.21)

project(LunarTear)

add_link_options("/PDBALTPATH:%_PDB%")

if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported)
    if(ipo_supported)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


# Clear cache for these option changed to take effect
option(BUILD_libreplicant "Build libreplicant" ON)
option(BUILD_UNSEALED_VERSES "Build Unsealed Verses" ON)
option(BUILD_SETTBLLEDITOR "Build SETTBLLEditor" ON)
option(BUILD_LunarTearLoader "Build LunarTearLoader" ON)

if(BUILD_libreplicant)
    add_subdirectory(libreplicant)
endif()

if(BUILD_UNSEALED_VERSES)
    if(NOT TARGET libreplicant)
        message(FATAL_ERROR "BUILD_UNSEALED_VERSES is ON but its dependency BUILD_LIBREPLICANT is OFF.")
    endif()
    add_subdirectory(UnsealedVerses)
endif()

if(BUILD_SETTBLLEDITOR)
    if(NOT TARGET libreplicant)
        message(FATAL_ERROR "BUILD_SETTBLLEDITOR is ON but its dependency BUILD_libreplicant is OFF.")
    endif()
    add_subdirectory(SETTBLLEditor)
endif()

if(BUILD_LunarTearLoader)
    if(NOT TARGET libreplicant)
        message(FATAL_ERROR "BUILD_LunarTearLoader is ON but its dependency BUILD_libreplicant is OFF.")
    endif()
    add_subdirectory(LunarTearLoader)
endif()