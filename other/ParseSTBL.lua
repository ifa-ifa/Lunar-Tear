--[[
  Intended for use in Cheat Engine. This script reads an STBL data file directly from a Controller object.
--]]

-- LocalNPC Controller offset: 0x4a23b80
-- GlobalNPC Controller offset: 0x48921f0

-- =============================================================================
-- Configuration
-- =============================================================================
local config = {
    executableName = "NieR Replicant ver.1.22474487139.exe",
    controllerOffset = 0x4a23b80
}

-- =============================================================================
-- Type Definitions & Constants
-- =================================================a============================
local STBL_FieldType = {
    [0] = "DEFAULT",
    [1] = "INT",
    [2] = "FLOAT",
    [3] = "STRING_OFFSET",
    [4] = "UNKNOWN",
    [5] = "ERROR"
}

-- =============================================================================
-- Memory Reading Helpers
-- =============================================================================

function readCString(address)
    if address == 0 or address == nil then return "" end
    return readString(address, 512, false) or ""
end

function readFixedString(address, len)
    if address == 0 or address == nil then return "" end
    local s = readString(address, len, false) or ""
    local zeroPos = s:find('\0')
    return zeroPos and s:sub(1, zeroPos - 1) or s
end

function readVector4(address)
    return {
        x = readFloat(address + 0),
        y = readFloat(address + 4),
        z = readFloat(address + 8),
        w = readFloat(address + 12)
    }
end

-- =============================================================================
-- STBL Parsing Functions
-- =============================================================================

function parseHeader(headerAddr)
    return {
        address = headerAddr,
        magic = readFixedString(headerAddr + 0, 4),
        version = readInteger(headerAddr + 4),
        spatialEntityCount = readInteger(headerAddr + 12),
        headerSize = readInteger(headerAddr + 16),
        tableDescriptorOffset = readInteger(headerAddr + 24),
        tableCount = readInteger(headerAddr + 28)
    }
end

function parseSpatialEntities(entitiesAddr, count)
    local entities = {}
    local entitySize = 96
    for i = 0, count - 1 do
        local currentAddr = entitiesAddr + (i * entitySize)
        local pos = readVector4(currentAddr + 0)
        local vol = readVector4(currentAddr + 16)
        table.insert(entities, {
            index = i,
            name = readFixedString(currentAddr + 32, 32),
            position = string.format("(%.2f, %.2f, %.2f, %.2f)", pos.x, pos.y, pos.z, pos.w),
            volume = string.format("(%.2f, %.2f, %.2f, %.2f)", vol.x, vol.y, vol.z, vol.w)
        })
    end
    return entities
end

function parseTableDescriptors(descriptorsAddr, count)
    local descriptors = {}
    local descriptorSize = 64
    for i = 0, count - 1 do
        local currentAddr = descriptorsAddr + (i * descriptorSize)
        table.insert(descriptors, {
            tableName = readFixedString(currentAddr + 0, 32),
            rowCount = readInteger(currentAddr + 32),
            rowSizeInFields = readInteger(currentAddr + 36),
            dataOffset = readInteger(currentAddr + 40)
        })
    end
    return descriptors
end

function getFieldValue(headerAddr, fieldType, dataAddr)
    if fieldType == 1 then -- FIELD_TYPE_INT
        return tostring(readInteger(dataAddr))
    elseif fieldType == 2 then -- FIELD_TYPE_FLOAT
        return string.format("%.4f", readFloat(dataAddr))
    elseif fieldType == 3 then -- FIELD_TYPE_STRING_OFFSET
        local offset = readInteger(dataAddr)
        if offset > 0 then
            return '"' .. readCString(headerAddr + offset) .. '"'
        else
            return '""'
        end
    elseif fieldType == 4 then -- FIELD_TYPE_UNKNOWN
        return string.format("UNK(0x%X)", readInteger(dataAddr))
    elseif fieldType == 5 then -- FIELD_TYPE_ERROR
        return "ERROR"
    else
        return "INVALID_TYPE"
    end
end

function printTableInColumns(headers, rows)
    if #rows == 0 then
        print("(Table contains no non-default values)")
        return
    end

    local maxWidths = {}
    for i, header in ipairs(headers) do
        maxWidths[i] = #header
    end

    for _, row in ipairs(rows) do
        for i, value in ipairs(row) do
            maxWidths[i] = math.max(maxWidths[i], #tostring(value))
        end
    end

    local headerLine, separatorLine = "", ""
    for i, header in ipairs(headers) do
        headerLine = headerLine .. string.format("%-" .. (maxWidths[i] + 2) .. "s", header)
        separatorLine = separatorLine .. string.rep("-", maxWidths[i]) .. "  "
    end
    print(headerLine)
    print(separatorLine)

    for _, row in ipairs(rows) do
        local rowLine = ""
        for i, value in ipairs(row) do
            rowLine = rowLine .. string.format("%-" .. (maxWidths[i] + 2) .. "s", value)
        end
        print(rowLine)
    end
end


function main()
    if getOpenedProcessID() == 0 then print("Error: No process attached.") return end
    local baseAddr = getAddress(config.executableName)
    if baseAddr == 0 then print("Error: Module '" .. config.executableName .. "' not found.") return end

    local controllerAddr = baseAddr + config.controllerOffset
    print(string.format("Attempting to parse STBL from Controller at: 0x%X\n", controllerAddr))

    local headerAddr = readQword(controllerAddr)
    if headerAddr == 0 then print("Error: Controller points to NULL. Is the area loaded?") return end

    local header = parseHeader(headerAddr)
    print("--- STBL File Header ---")
    print(string.format("Magic: %s, Version: %d, Header Address: 0x%X", header.magic, header.version, header.address))
    print(string.format("Spatial Entities: %d, Tables: %d\n", header.spatialEntityCount, header.tableCount))

    print("--- 1. Spatial Entity Block ---")
    local spatialEntities = parseSpatialEntities(headerAddr + header.headerSize, header.spatialEntityCount)
    printTableInColumns(
        {"Index", "Name", "Position (X, Y, Z, W)", "Volume Data (X, Y, Z, W)"},
        (function()
            local rows = {}
            for _, e in ipairs(spatialEntities) do table.insert(rows, {e.index, e.name, e.position, e.volume}) end
            return rows
        end)()
    )
    print("")

    local tableDescriptors = parseTableDescriptors(headerAddr + header.tableDescriptorOffset, header.tableCount)

    for i, desc in ipairs(tableDescriptors) do
        print(string.format("--- %d. Table: %s ---", i + 1, desc.tableName))
        print(string.format("Rows: %d, Fields/Row: %d, Data Offset: 0x%X", desc.rowCount, desc.rowSizeInFields, desc.dataOffset))

        local dataAddr = headerAddr + desc.dataOffset
        local rowSizeInBytes = desc.rowSizeInFields * 8

        local parsedRows, activeColumns = {}, {}

        for r = 0, desc.rowCount - 1 do
            local rowAddr = dataAddr + (r * rowSizeInBytes)
            local hasData = false
            local currentRow = { originalIndex = r }

            for f = 0, desc.rowSizeInFields - 1 do
                local fieldAddr = rowAddr + (f * 8)
                local fieldTypeVal = readInteger(fieldAddr)

                if fieldTypeVal ~= 0 then -- Not FIELD_TYPE_DEFAULT
                    hasData = true
                    local fieldValue = getFieldValue(headerAddr, fieldTypeVal, fieldAddr + 4)
                    currentRow[f] = fieldValue
                    activeColumns[f] = true
                end
            end
            if hasData then
                table.insert(parsedRows, currentRow)
            end
        end

        local sortedColumns = {}
        for k, _ in pairs(activeColumns) do table.insert(sortedColumns, k) end
        table.sort(sortedColumns)

        local finalHeaders = {"Row"}
        for _, colIndex in ipairs(sortedColumns) do table.insert(finalHeaders, "F" .. colIndex) end

        local finalRows = {}
        for _, pRow in ipairs(parsedRows) do
            local newRow = {pRow.originalIndex}
            for _, colIndex in ipairs(sortedColumns) do
                table.insert(newRow, pRow[colIndex] or "")
            end
            table.insert(finalRows, newRow)
        end

        printTableInColumns(finalHeaders, finalRows)
        print("")
    end
    print("--- Parsing Complete ---")
end

main()