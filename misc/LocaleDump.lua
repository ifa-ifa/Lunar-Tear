-- Run in Cheat Engines Lua Engine

local moduleName = "NieR Replicant ver.1.22474487139.exe"
local baseOffset = 0x27e0590
local outputFileName = os.getenv("USERPROFILE") .. "\\Desktop\\NierReplicant_StringDump.txt"

local ENTRY_SIZE = 0x10
local ID_OFFSET = 0x0
local STR_OFFSET = 0x8

local primaryCountOffset = 0xF070

function escapeString(s)
    if not s then return "" end
    s = s:gsub("\\", "\\\\")
    s = s:gsub("\n", "\\n")
    s = s:gsub("\r", "\\r")
    return s
end

local moduleBase = getAddressSafe(moduleName)
if not moduleBase then
    print("ERROR: Module not found: " .. moduleName)
    return
end

local baseAddress = moduleBase + baseOffset

local primaryCountAddress = baseAddress + primaryCountOffset
local stringCount = readInteger(primaryCountAddress)

if not stringCount or stringCount <= 0 then
    print("ERROR: Could not read a valid string count from memory.")
    return
end

local outFile, err = io.open(outputFileName, "wb")
if not outFile then
    print("ERROR: Could not open output file for writing: " .. tostring(err))
    return
end
outFile:write("\xEF\xBB\xBF") -- UTF-8 Byte Order Mark
local stringsFound = 0


for i = 0, stringCount - 1 do
    local entryAddr = baseAddress + (i * ENTRY_SIZE)
    local id = readInteger(entryAddr + ID_OFFSET)
    local strPtr = readPointer(entryAddr + STR_OFFSET)

    if id ~= -1 and strPtr and strPtr > 0x10000 then
        local rawString = readString(strPtr, 4096, false)
        if rawString and #rawString > 0 then
            local processedString = escapeString(rawString)
            outFile:write(string.format("%d %s\n", id, processedString))
            stringsFound = stringsFound + 1
        end
    end
end


outFile:close()

print("Output file location: " .. outputFileName)