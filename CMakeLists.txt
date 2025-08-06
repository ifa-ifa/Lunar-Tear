cmake_minimum_required(VERSION 3.21)

project(LunarTear)

if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported)
    if(ipo_supported)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

option(BUILD_LIBSETTBLL "Build the libsettbll library" ON)
option(BUILD_SETTBLLEDITOR "Build the SETTBLLEditor application" ON)
option(BUILD_LunarTearLoader "Build the LunarTearLoader library" ON)

if(BUILD_LIBSETTBLL)
    add_subdirectory(libsettbll)
endif()

if(BUILD_SETTBLLEDITOR)
    if(NOT TARGET libsettbll)
        message(FATAL_ERROR "BUILD_SETTBLLEDITOR is ON but its dependency BUILD_LIBSETTBLL is OFF.")
    endif()
    add_subdirectory(SETTBLLEditor)
endif()

if(BUILD_LunarTearLoader)
    if(NOT TARGET libsettbll)
        message(FATAL_ERROR "BUILD_LunarTearLoader is ON but its dependency BUILD_LIBSETTBLL is OFF.")
    endif()
    add_subdirectory(LunarTearLoader)
endif()