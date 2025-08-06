the games stores a GameDifficulty singleton at 0x2b62af8. This stores a pointer to an array of floats used for difficulty stats lookup. To my knowledge, the data is constant and loaded from a file.

The game also stores a difficulty mode int at +11fdfa8. 0 = easy, 1 = medium, 2 = hard.

Look at this example:
```
float getHealthDifficultyFactor(snow::GameDifficult::Singleton *singleton)  // +0x65fb50

{
  bool bVar1;
  longlong lVar2;
  snow::GameDifficult *gameDifficult;
  float fVar3;
  
  if (singleton->pGameDifficult == 0x0) {
    return 1.0;
  }
  bVar1 = CheckRouteEActive(&endingsData);
  if ((bVar1) && (PlayerSaveData_144374a20.map_string[0] == 'X')) {
    gameDifficult = singleton->pGameDifficult;
    fVar3 = gameDifficult->field84_0x150_(1.0);
  }
  else {
    fVar3 = 1.0;
    gameDifficult = singleton->pGameDifficult;
  }
  if (DifficultyMode == hard) {
    return fVar3 * gameDifficult->MaxHpFactorHard_(0.7);
  }
  lVar2 = 0;
  if (DifficultyMode == medium) {
    lVar2 = 0x70;
  }
  return fVar3 * *(&gameDifficult->field0_0x0_(1.5) + lVar2);
}
```

This function determines what multiplier should be used on max HP. The decompilers slacking here, so heres the equivilent psuedocode:
```
if (!singleton->gameDifficult) {
    return 1
}

if (endingEActive and inDLCMap) {
    mul = 1
}
else { 
    mul = 1 // these wont always be the same
}

if (hard) {
    return 0.7 * mul
}

if (medium) {
    return 1 * mul
}

if (easy) {
    return 1.5 * mul
}
```