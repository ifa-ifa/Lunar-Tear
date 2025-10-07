 # Lunar Tear

Tool repository for Nier Replicant

- libreplicant - File type utility
- UnsealesVerses - .arc patcher and repacker (command line)
- SETTBLLEditor - STBL editor (GUI)
- Lunar Tear Loader - Script tool / modding api / loose loader

For modding instructions see the [Wiki](https://github.com/ifa-ifa/Lunar-Tear/wiki)

For installation and usage of the loader see the [Nexus page](https://www.nexusmods.com/nierreplicant/mods/87)

## Build

Set the flags in the main cmakelists.txt to determine which subprojects you want to build. Make sure to clear cache after changing them as cmake wont invalidate it for you. 

If building the loader you will need to install vcpkg and set the vcpkg root environment variable.

If building the editor, you will need Qt 6. I reccomend you install it yourself and then add its path to CMakePresets.json in the cacheVariables dictionary like this: "CMAKE_PREFIX_PATH": "C:/path/to/Qt/6.x.x/msvcxxxx_64". Alternatively, you can have vcpkg manage it by adding "qt6-base" to the dependencies in vcpkg.json. This will download and build Qt from source, which can take a very long time and a lot of storage.

## Credits

"Kaine" by yretenai (https://github.com/yretenai/kaine) and binary templates by WoefulWolf (https://github.com/WoefulWolf/replicant_templates/tree/main) were very helpful when making this

iffyfuistatement for helping me test the mod and providing test files.
