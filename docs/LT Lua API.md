```
LT_LOG_INFO = 0
LT_LOG_VERBOSE = 1
LT_LOG_WARNING = 2
LT_LOG_ERROR= 3
LT_LOG_LUA = 4
_LTLog(str modName, int logLevel, str message)
```

Log using Lunar Tear

```
str _LTConfigGetString(str modName, str sectionName, str keyName, str defaultValue)
int _LTConfigGetInt(str modName, str sectionName, str keyName, int defaultValue)
real _LTConfigGetReal(str modName, str sectionName, str keyName, real defaultValue)
bool _LTConfigGetBool(str modName, str sectionName, str keyName, bool defaultValue)
```

Access the config file at `LunarTear/mods/[modname]/config.ini`

```
bool _LTIsPluginActive(str modName)
bool _LTIsModActive(str modName)
```

Checks that there is a mod loaded with that name, or if there is a plugin loaded for that mod. Try to check `isPluginActive` before calling any of your custom bindings to avoid a runtime error. Note that if there is no manifest then the name is the name is the folder name, which may be altered by users

```
str _LTGetModDirectory(str modName)
```

Use this instead of crafting the path yourself, as the user may change mod folders for some reason.

