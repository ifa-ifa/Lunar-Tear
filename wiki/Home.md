This wiki is a guide on how to make mods using Lunar Tear. I may forget to update it sometimes so if you need help either create an issue or ask me on discord (@iiiba)

## Extracting Assets

Chances are you will want to start by extracting the files. To do that, download Unsealed Verses from the Releases page (https://github.com/ifa-ifa/Lunar-Tear/releases). then, run `UnsealedVerses.exe unarchive C:\Program Files (x86)\Steam\steamapps\common\NieR Replicant ver.1.22474487139\data\info.arc" "C:\Users\[User]\Desktop\extracted"`. Swap out the first path for your own path to the games assets, and the second path for where you want to extract the assets to. If you only want the character models and weapons, add `--chara-only` to the end. That will save you about ~20GB).

To find out what to do with these, see the index below:

## Index  

Models & Textures - https://github.com/WoefulWolf/Replicant2Blender
Textures only - [Loose Textures](Loose-Textures.md), [Custom Archives](Custom-Archives.md)  
Scripting - [Scripts](Scripts.md), [Game Lua API](Game-Lua-API.md), [LT Lua API](LT-Lua-API.md)  
Plugins - [Plugins](Plugins.md)  
Tables - [Tables](Tables.md)  
Any other assets - [Custom Archives](Custom-Archives.md)  
Custom Weapons - [Custom Weapons](Custom-Weapons.md)  

## Preparing a mod

Create your mod as a folder. By default the mod name is the folder name. Mod names, wherever determined by folder name or manifest, must be unique. Duplicates will not be loaded. If multiple mods contain the same assets, the file from the alphabetically last mod will be favoured.

## Config

If you create a config.ini file in your mods root directory you can access it using the C and Lua apis.

## Manifest (Optional)

 create a `manifest.json` to decouple the folder name form the mod name, which can make mod load lookups more consistent:

`LunarTear/mods/[mod]/manifest.json`:
```
{
	"Name" : "MyMod"
}
```

This is entirely optional and likely unnecessary for most cases. But it is useful as it makes api functions like `_LTIsModLoaded()` behave consistently. Even more optionally, define more fields like:

```
{
	"name" : "MyMod",
	"author": "ifaifa",
	"version": "1.0.0"
	"description:" "Mod desc here",
	"image" : "image.png"
}
```

## Licensing

Lunar Tear Loader is licensed under GPL-2. If you distribute a plugin that uses the Lunar Tear Plugin API, the plugin must be open source and licensed under GPL-2 or compatible license. Other mods, even ones that use Lua bindings exposed by Lunar Tear, do not have this requirement.

