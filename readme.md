# Lunar Tear

### Lunar Tear Loader 
- See the [Nexus page](https://www.nexusmods.com/nierreplicant/mods/87)
### UnsealedVerses 
- Command line tool for Nier Replicant modding
- Extract files from .arc archives
- Repack files into .arc archives
- More commands for manipulating assets (run `UnsealedVerses.exe -h` for more info)
### libreplicant 
- C++ library for reading and writing Nier Replicant file formats

---

For modding instructions see the [Wiki](https://github.com/ifa-ifa/Lunar-Tear/wiki)

## Build

- vcpkg required, set VCPKG_ROOT environment variable 
- Set flags in the top level cmakelists.txt for the subprojects you want to build. Clear CMake cache after altering flags.
- SETTBLL Editor requires Qt 6. Manually install it and add it to cacheVariables (CMakePresets.json): `"CMAKE_PREFIX_PATH": "C:/path/to/Qt/6.x.x/msvcxxxx_64"`. Alternatively, let vcpkg manage it - add `"qt6-base"` to vcpkg.json. But that will build from source, taking a ton of time and storage
