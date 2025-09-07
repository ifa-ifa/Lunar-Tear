This wiki is a guide on how to make mods using Lunar Tear. I may forget to update it sometimes so if you need help either create an issue or ask me on discord (@iiiba)

## Preparing a mod


Create your mod as a folder. To see where the differnet types of file should go, see the corresponding pages on this wiki. By default the mod name is the folder name. Mod names, wherever determined by folder name or manifest, must be unique. Duplicates will not be loaded. If multiple mods try loose load the same texture or table, the file from the alphebetically last mod will be loaded.


## Manifest

OPTIONAL - create a `manifest.json` to decouple the folder name form the mod name, which can make mod load lookups and complex interactions more consistent:

`LunarTear/mods/[mod]/manifest.json`:
```
{
	"Name" : "MyMod"
}
```

Stick to alphanumeric characters (a-z, A-Z, 0-9), hyphens (-) and underscores (_) for the name.


This is entirely optional and likely unnecessary for most cases. But it is useful as it makes api functions like `_LTIsModLoaded()` behave consisently. Even more optionally, define more fields like:

```
{
	"name" : "MyMod",
	"author": "ifaifa",
	"version": "1.0.0"
	"description:" "Mod desc here",
	"image" : "image.png"
}
```

You can use whatever characters you like for these fields. They are not read by the loader, they are for users convenience.

## Config

If you create a config.ini file in your mods root directory you can access it using the C and Lua apis.











If you need any help, there are example mods posted on the nexus page. Or message me or tag me in the Nier modding discord


