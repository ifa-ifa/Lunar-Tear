## Base lua

The game only loads the `math` base lua library. Anything else must be manually recreated. Lunar Tear does some for you:

```
_LTLua_type
_LTLua_xpcall
_LTLua_tostring
_LTLua_loadstring
_LTLua_table_getn
```

While the game only loads math, the other libraries can be found in the executable. If you really need anything else, you can find the c side function using a decompiler and register a custom binding in a plugin, or if you don't know how to do that just ask me and i can probably find it and add it.

## Scripts

The game has a script for every phase (map) plus 3 more: \_\_root__, \_\_libnier__ and \_\_game__. You can extract them by enabling Script Dumping in config. Then decompile them using a lua 5.0.2 decompiler. I use this one: https://github.com/loosepolygon/luadec-5.0 but for some scripts it doesn't work, in which case i use this one: `https://sourceforge.net/projects/unluac/`. The `batchDecompile.py` in this repository will batch decompile + handle weird Japanese encoding issues.

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
`_DebugPrint` is the games built in logging, you can view this by enabling lua logs in the LunarTear config. 

You can read `Game Lua API.md` to see documentation on (some) builtin game functions. `LT Lua API.md` documents core bindings exposed by Lunar Tear

By injecting a script like that you can run custom logic whenever you pick up an item in game. A useful hook target is `load` or `start`. 

If you want a list of all bindings available, goto address `141306710` in Ghidra. Note that some of these functions had their implementation stripped out for the release build (Like `_DebugPrint`, i had to write a custom implemetation and hook to get that working)

## Plugin integration

Using a C/C++ plguin you can register custom bindings and call script functions from any thread. See the plugin section in the docs for more info.
