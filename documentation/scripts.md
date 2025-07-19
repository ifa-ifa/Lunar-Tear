The game has a single PhaseScriptManager at +48923c0. This contains a ScriptManager, which containts a pointer to one (or more?) LuaState objects. Creating our own state doesn't seem to be easy to due to global state / memory managment issues. Its better to just use an existing state and make sure you clean the stack.

The games binding table can be found at +b9f6e0.

The game uses a interesting lua implementation. It is based off of 5.0.2, proven by a license statement in the executable. The precompiled bytecode has 6 byte instruction size, which is interesting as I have never heard of anything other than 4. It's for this reason i don't recommend precompiling any loose scripts. `luaL_loadbuffer` takes either bytecode or source so we just pass the source to that.

Here are some of the lua API functions I found. Its really easy to find these by comparing against the source code.

- lua_open = 0x3e84d0 
- luaL_loadbuffer = 0x3d8290
- lua_dump = 0x3d7630
- lua_close = 0x3e8460
- lua_tolstring = 0x3d95c0
- lua_settop = 0x3d7280
- lua_gettop = 0x3d68c0
