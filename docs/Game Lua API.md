(Largely incomplete)


```
int _IsGameFlag(int flagId)
int _IsSnowGameFlag(int flagId)

_GameFlagOn(int flagId)
_GameFlagOff(int flagId)
_SnowGameFlagOn(int flagId)
_SnowGameFlagOff(int flagId)
```

Get/Set the state of a specific game flag, used by the game to mark game events and progress. Snow flags are seperate flags added for the remaster. Returns 0 on flag off, 1 on flag on. Do not evaluate this function like this: `if _IsGameFlag(5) then` this will always evaluate to true. Do this :`if _IsGameFlag(5) == 1 then` instead.


```
str _GetNextPhase()
```

Retrieves the name of the current phase. (not a typo) (blame Cavia).


```
str _GetRealNextPhase()
```

Retrieves the name of the next phase.

```
_AddActorRangeCallback(int primaryActor, int secondaryActor, float distance, string funcName, ?, ?)
```

calls the callback with arg `1` if they enter range, calls with `2` if they leave

```
_ChangeMap(string phaseName, int phaseInPoint)
```
Loads the map and spawns the character at a point (defined in the phase table)
```
_ChangePhase(string phaseName, float x, float y, float z, float rot)
```
Untested. Presumably spawns the player at the given coordinates.