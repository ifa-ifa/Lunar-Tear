-- Run in Cheat Engines Lua Engine

local targetId = 0x98ef2c
local entryCount = 100000




local moduleName = "NieR Replicant ver.1.22474487139.exe"
local baseOffset = 0x27e0590

local ENTRY_SIZE = 0x10
local ID_OFFSET = 0x0
local STR_OFFSET = 0x8

local moduleBase = getAddressSafe(moduleName)
if not moduleBase then
    print("ERROR: Module not found: " .. moduleName)
    return
end

local baseAddress = moduleBase + baseOffset
local found = false


for i = 0, entryCount - 1 do
    local entryAddr = baseAddress + (i * ENTRY_SIZE)
    local id = readInteger(entryAddr + ID_OFFSET)
    if id == targetId then

        local strPtr = readPointer(entryAddr + STR_OFFSET)

        if strPtr and strPtr > 0x10000 then
            local s = readString(strPtr)
            print(string.format("ID: %d | %s", id, s))
            found = true
            break
        end
    end
end

if not found then
    print("Failed to find string for ID: " .. targetId)
end
