--[[
  Cheat Engine Lua Script - set player item count
]]


local moduleName = "NieR Replicant ver.1.22474487139.exe"
local saveDataOffset = 0x4374a20


local itemIdToSet = 0x01
local itemCountToSet = 99

-- Get the base address of the game's module
local moduleBase = getAddress(moduleName)

if moduleBase == 0 then
  print("Error: Game process not found. Please make sure the game is running and attached in Cheat Engine.")
  return
end


local saveDataPtr = moduleBase + saveDataOffset




if itemIdToSet < 0x300 then

  local isKainePlayer = readInteger(saveDataPtr + 0xE30)

  if isKainePlayer == 1 then

    local targetAddress = saveDataPtr + 0xE44 + itemIdToSet
    writeByte(targetAddress, itemCountToSet)
    print(string.format("Kaine is player. Wrote count %d to item 0x%X in Kaine's inventory.", itemCountToSet, itemIdToSet))
  else

    local targetAddress = saveDataPtr + 0xC0 + itemIdToSet
    writeByte(targetAddress, itemCountToSet)
    print(string.format("Wrote count %d to item 0x%X in standard inventory.", itemCountToSet, itemIdToSet))
  end
else

  print(string.format("Error: Item ID 0x%X is invalid (must be less than 0x300).", itemIdToSet))
end