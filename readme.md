# Lunar Tear

Loose File loader for Nier Replicant. Can load textures, lua scripts, and game tables. 

This repository also includes some mod development tools, documentation for the games internal types and functionality, and some misc scripts.

For modding instructions see `modderinstructions.md`

## Install and use

Place LunarTearLoader.dll in the game directory. You can use SpecialK to automatically load the dll by adding:

```
[Import.LunarTear]
Architecture=x64
Role=ThirdParty
When=PlugIn
Filename=path\to\LunarTearLoader.dll
```	

To the end of the Special K config file (e.g dxgi.ini or d3d11.ini). Change the filepath to point to your LunarTearLoader.

If you don't want to use specialk you can use any injection tool / launcher you want. There is LunarTearLauncher.exe which works by placing it in the same directory and usinf that to launch, but if you are using specialK then its unnecessary.

Install mods by dropping the folder with the files in `LunarTear/mods/`. If there is a conflict, the alphabetically first mod will be favoured and the unfavoured mods will have none of their files loaded.

## Build

Set the flags in the main cmakelists.txt to determine which subprojects you want to build. Make sure to clear cache after changing them as cmake wont invalidate it for you. 

If building the loader you will need to install vcpkg and set the vcpkg root environment variable.

If building the editor, you will need Qt 6. The recommended way is to install it yourself and then add its path to CMakePresets.json in the cacheVariables dictionary, like this: "CMAKE_PREFIX_PATH": "C:/path/to/Qt/6.x.x/msvcxxxx_64". Alternatively, you can have vcpkg manage Qt by adding "qt6-base" to the dependencies in vcpkg.json. Be aware that this will download and build Qt from source, which can take a very long time.

## Credits

"Kaine" by yretenai (https://github.com/yretenai/kaine) and binary templates by WoefulWolf (https://github.com/WoefulWolf/replicant_templates/tree/main) were very helpful when making this mod

