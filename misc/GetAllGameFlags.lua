-- Run in cheat engine lua engine

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
local totalFlags = 2048
local bitsPerChunk = 64
local bytesPerChunk = 8
local totalChunks = totalFlags // bitsPerChunk

local baseAddress = moduleBase + playerSaveDataOffset

local function getFlagBitstring(offset)
    local bitfieldAddress = baseAddress + bitfieldOffset + offset
    local result = ""

    for chunk = 0, totalChunks - 1 do
        local addr = bitfieldAddress + (chunk * bytesPerChunk)
        local qword = readQword(addr)
        if qword == nil then
            print(string.format("Failed to read memory at 0x%X", addr))
            result = result .. string.rep("?", bitsPerChunk)
        else
            for bit = 0, bitsPerChunk - 1 do
                local isSet = (qword >> bit) & 1
                result = result .. tostring(isSet)
            end
        end
    end

    return result
end


local normalBitstring = getFlagBitstring(normalOffset)
local snowBitstring = getFlagBitstring(snowOffset)

print("==== Normal Flags Bitstring ====")
print(normalBitstring)

print("\n==== Snow Flags Bitstring ====")
print(snowBitstring)
