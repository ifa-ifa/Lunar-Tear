# Lunar Tear Loader

The loader currently works by hooking a function i call "LoadMapResources" (+415150). 
The limitations of this are that it only loads map resources (snow/script/). There may be more that this loader misses, but for now this method is simple because it allows us to reliably access both the buffer and filename in one hook. Unfortunately it relys on getting the filename from the stack rather than a register.

For tables, we hook right before the call to the function that initialises its handle. 
For lua files, we hook right before the call to a wrapper function which calls luaL_loadbuffer and lua_pcall on the buffer.
The game passes bytecode here, but loadbuffer accepts both source and bytecode. 



I have not implemented any deallocation yet. 
Becuase then i would have to make another hook to listen for the games deallocator. 
So loose data will leak. This is countered by using a cache so each resource gets
leaked only once. Since the game uses an arena allocator, we dont have to worry about the
game trying to free the cache.


TODO: Documentation for texture hooking


Future improvments:

- Pattern scanning so it doesn't break (as much) on game update. Low priority,  I don't think this game will ever get an update again.
- More reliable hook that works for all resources, this may miss some
- Proper deallocation
