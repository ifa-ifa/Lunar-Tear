-- Run in cheat engine lua engine


local moduleName = "NieR Replicant ver.1.22474487139.exe"
local baseOffset = 0x27e0590          -- offset to locale table
local entryCount = 1000000              -- Number of entries to scan
local targetString = "..."     -- String to search for

local ENTRY_SIZE = 0x10               -- 16 bytes per entry
local STR_OFFSET = 0x8                -- offset to char* str
local ID_OFFSET = 0x0                 -- offset to int id

local moduleBase = getAddressSafe(moduleName)
if not moduleBase then
    print("ERROR: Module not found: " .. moduleName)
    return
end

local baseAddress = moduleBase + baseOffset


for i = 0, entryCount - 1 do
    local entryAddr = baseAddress + (i * ENTRY_SIZE)

    local strPtr = readPointer(entryAddr + STR_OFFSET)
    if strPtr and strPtr > 0x10000 then
        local s = readString(strPtr)
        if s == targetString then
            local id = readInteger(entryAddr + ID_OFFSET)
            print(string.format("Found at index %d (Entry Addr: 0x%X)", i, entryAddr))
            print(string.format("ID: %d | String: %s", id, s))
            break
        end
    end
end
