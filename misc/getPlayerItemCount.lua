--[[
  Cheat Engine Lua Script - get player item count
]]

-- Hardcoded values from the request
local moduleName = "NieR Replicant ver.1.22474487139.exe"
local saveDataOffset = 0x4374a20
local globalFlagOffset = 0x435af40
local itemIdToFind = 33

-- Get the base address of the game's module
local moduleBase = getAddress(moduleName)

if moduleBase == 0 then
  print("Error: Game process not found. Please make sure the game is running and attached in Cheat Engine.")
  return
end


local saveDataPtr = moduleBase + saveDataOffset


local globalFlagAddr = moduleBase + globalFlagOffset


local isKainePlayer = readInteger(saveDataPtr + 0xE30)
local globalFlag = readByte(globalFlagAddr)

local itemCount = 0


if (isKainePlayer == 1) and (globalFlag == 0) then

  print("Condition met: Kaine is the player and global flag is off.")

  if itemIdToFind < 0x300 then

    itemCount = readByte(saveDataPtr + 0xE44 + itemIdToFind)

  end

else



  if itemIdToFind < 0x300 then

    itemCount = readByte(saveDataPtr + 0xC0 + itemIdToFind)

  end
end

local message = string.format("Item count for ID 0x%X is: %d", itemIdToFind, itemCount)
print(message)