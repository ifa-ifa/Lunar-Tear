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

local flagIndex = 712          -- which flag to modify
local applyToNormal = true     -- true to modify normal flag bitfield
local applyToSnow = false      -- true to modify snow flag bitfield
local setFlagOn = false         -- true to set the bit, false to clear the bit

local function modifyFlag(offset, index, setOn)
    local chunkIndex = index // bitsPerChunk
    local bitIndex = index % bitsPerChunk
    local addr = baseAddress + bitfieldOffset + offset + (chunkIndex * bytesPerChunk)

    local qword = readQword(addr)
    if qword == nil then
        print(string.format("Failed to read memory at 0x%X", addr))
        return false
    end

    local newQword
    if setOn then
        -- Set the bit
        newQword = qword | (1 << bitIndex)
    else
        -- Clear the bit
        newQword = qword & (~(1 << bitIndex))
    end

    local success = writeQword(addr, newQword)

    if not success then
        print(string.format("Failed to write memory at 0x%X", addr))
        return false
    end

    return true
end

if applyToNormal then
    local normalSuccess = modifyFlag(normalOffset, flagIndex, setFlagOn)
    print("Normal Flag modification:", normalSuccess and "SUCCESS" or "FAILED")
end

if applyToSnow then
    local snowSuccess = modifyFlag(snowOffset, flagIndex, setFlagOn)
    print("Snow Flag modification:", snowSuccess and "SUCCESS" or "FAILED")
end