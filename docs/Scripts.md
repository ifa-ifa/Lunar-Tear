The game has a script for every phase (map) plus 3 more: \_\_root__, \_\_libnier__ and \_\_game__. To do Script modding youl want to have a good look at these. You can extract them by enabling Script Dumping in config. Then decompile them using a lua 5.0.2 decompiler. I use this one: https://github.com/loosepolygon/luadec-5.0 but for some scripts it doesn't work, in which case i use this one: `https://sourceforge.net/projects/unluac/`. The `batchDecompile.py` in this repository will batch decompile + handle weird Japanese encoding issues.

## Injection

You can mark lua scripts to be injected alongside the original by placing scripts in the `inject/[point]/` folder. Right now the only injection points supported is `post_phase` and `post_lib` The loader will look for scripts with the same name as the original being loaded. Scripts named `_any.lua` will always be loaded.

With script injection you can do things like monkey patching:
```
pickupitem_original = pickupitem
pickupitem = function()
	_DebugPrint("Do whatever you want here")
	pickupitem_original()
end
```
`_DebugPrint` is the games built in logging, you can view this by enabling lua logs in the LunarTear config. LunarTear provides a custom `_LTDebug` binding that will log without having to enable lua logs.

By injecting a script like that you can run custom logic whenever you pick up an item in game. A useful hook target is `load` or `start`. 

If you want a list of all bindings available, goto address `141306710` in Ghidra. Note that some of these functions had their implementation stripped out for the release build (Like `_DebugPrint`, i had to write a custom implemetation and hook to get that working)

## Plugin integration

Using a C/C++ plguin you can register custom bindings and call script functions from any thread. See the plugin section in the docs for more info.


## Script loading 

This is more of a legacy thing, I don't reccomend it becuase its very hard, incompatible with other mods and takes a long time to correct the broken decompiler output. But if you want to load an entirly custom script just name it what you want and put it in the root directory of your mod. I really don't reccomend this, injection is a far superior method.
