-- Run in Cheat Engine Lua Engine

local moduleName = "NieR Replicant ver.1.22474487139.exe"
local moduleBase = getAddressSafe(moduleName)

if moduleBase == nil then
    print("Failed to find module base for " .. moduleName)
    return
end


local playerSaveDataOffset = 0x4374a20
local bitfieldOffset = 0x4EC
local normalOffset = 0x14
local snowOffset = 0x744
local bitsPerChunk = 64
local bytesPerChunk = 8

local baseAddress = moduleBase + playerSaveDataOffset

-- Change this to the flag index you want to check
local flagIndex = 727


local function isFlagSet(offset, index)
    local chunkIndex = index // bitsPerChunk
    local bitIndex = index % bitsPerChunk
    local addr = baseAddress + bitfieldOffset + offset + (chunkIndex * bytesPerChunk)

    local qword = readQword(addr)
    if qword == nil then
        print(string.format("Failed to read memory at 0x%X", addr))
        return nil
    end

    local isSet = (qword >> bitIndex) & 1
    return isSet == 1
end


local normalFlag = isFlagSet(normalOffset, flagIndex)
local snowFlag = isFlagSet(snowOffset, flagIndex)

print("Flag Index:", flagIndex)

if normalFlag == nil then
    print("Normal Flag: Could not read")
else
    print("Normal Flag:", normalFlag and "SET" or "NOT SET")
end

if snowFlag == nil then
    print("Snow Flag: Could not read")
else
    print("Snow Flag:", snowFlag and "SET" or "NOT SET")
end

