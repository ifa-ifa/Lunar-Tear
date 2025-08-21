 # Lunar Tear

Tool repository for Nier Replicant

- UnsealesVerses - .arc patcher and repacker
- SETTBLLEditor - STBL editor
- Lunar Tear Loader - Script tool / modding api / loose loader

For modding instructions see `docs/`

## Install Lunar Tear Loader

#### With SpecialK 

Place LunarTearLoader.dll in the game directory. You can use SpecialK to automatically load the dll by adding:

```
[Import.LunarTear]
Architecture=x64
Role=ThirdParty
When=PlugIn
Filename=LunarTearLoader.dll 
```	

To the end of the Special K config file (e.g dxgi.ini or d3d11.ini). Change the filepath to point to your LunarTearLoader if you havnt put it in the same directory.

#### Without Special K

Place both LunarTearLoader.dll and LunarTearLauncher.exe into the game directory. Load with that exe whenever you want the loader to activate.

## Install Mods

Install mods by dropping the folder with the files in `LunarTear/mods/`. For conflicting files, the alphabeticaly last file is loaded. If a mod requires a plugin, you will have to enable plugins in LunarTear/LunarTear.ini


## Build

Set the flags in the main cmakelists.txt to determine which subprojects you want to build. Make sure to clear cache after changing them as cmake wont invalidate it for you. 

If building the loader you will need to install vcpkg and set the vcpkg root environment variable.

If building the editor, you will need Qt 6. I reccomend you install it yourself and then add its path to CMakePresets.json in the cacheVariables dictionary like this: "CMAKE_PREFIX_PATH": "C:/path/to/Qt/6.x.x/msvcxxxx_64". Alternatively, you can have vcpkg manage it by adding "qt6-base" to the dependencies in vcpkg.json. This will download and build Qt from source, which can take a very long time and a lot of storage.

## Credits

"Kaine" by yretenai (https://github.com/yretenai/kaine) and binary templates by WoefulWolf (https://github.com/WoefulWolf/replicant_templates/tree/main) were very helpful when making this

iffyfuistatement for helping me test the mod and providing test files.