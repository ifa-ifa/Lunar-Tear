

+0x11fdf84. 8 bit Flag of what endings have been seen

- Ending A seen
- Ending B seen
- Ending C seen
- Ending D seen (Clears after E)
- unknown
- Ending E seen
- unknown
- unknown


Example usage:
```
bool CheckRouteEActive(EndingsData *endingData)  // +0x3b3c00
{
  return (endingData->endingsBitflags & 8) != 0;
}
```



Note that ending C is skippable, ending D will wipe save data and begin route E regardless of wherever C is completed. After completing ending E it will revert back to before ending d is
completed (with a special flag) and clearing the flag. Therefore checking that D is completed is a valid way of checking that route E is active

Completing ending E will revert back to before the completion of ending c but with a special flag
