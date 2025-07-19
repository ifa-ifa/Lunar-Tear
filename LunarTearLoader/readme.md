# Lunar Tear Loader


When the game tries to load a file, the hook looks for a file with the same name in `gamedir/LunarTear`. If it finds your loose file, it loads your version into memory and tells the game to use that instead of the original. Note that currently there is no deallocation. There is however, a cache which makes sure each file is only loaded once. This isn't a big deal for tables, scripts, and mods with only a small amount of textures. If you are loose loading 15MB of files, consider that a constant +15MB memory usage, even when those files arnt in use.

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
 
There will also be a launcher in case you dont want to use special k. Just place it in the same directory and launch from that exe

## Future Improvements

-   DLL Injection built in
-   Custom amount of mipmaps
-   Deallocation, at least for textures


## Modding

For modding instructions see `modderinstructions.md`
