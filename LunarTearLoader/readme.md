# Lunar Tear Loader


When the game tries to load a file, the hook looks for a file with the same name in `gamedir/LunarTear/mods/[modname]`. If it finds your loose file, it loads your version into memory and tells the game to use that instead of the original.

By default there is no deallocation. There is however, a cache which makes sure each file is only loaded once. This isn't a big deal for tables, scripts, and mods with only a small amount of textures. If you are loose loading 15MB of files, consider that a constant +15MB memory usage, even when those files arnt in use. This is becuase the game uses an arena allocator which would have been time consuming to reverse engineer. For larger mods, there is an expermental texture unload feature that tries to unload the texture a few seconds later, hoping that the texture is already on GPU. I havn't noticed any issues with it but i am still marking it as experimental. If you get a lot of crashes it may be worth turning off.

For conflicting files, the alphabeticaly last file is loaded and a warning is logged.

## Usage

You need a DLL injector. there are manual tools out there but you can use special k to do it automatically:

```
[Import.LunarTear]
Architecture=x64
Role=ThirdParty
When=PlugIn
Filename=path\to\LunarTearLoader.dll
```	

Change the filepath to point to your LunarTearLoader. Place that at the end of the Special K config file (e.g dxgi.ini or d3d11.ini)
 



## Modding

For modding instructions see `modderinstructions.md`
