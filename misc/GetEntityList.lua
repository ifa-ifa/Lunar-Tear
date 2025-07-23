--[[
  Nier Replicant - Entity List Reader

  Run in Cheat Engines lua engine. change the "show_rtti_tree" to see actor inheritance structure.
]]

-- ========================== CONFIGURATION ==========================

local config = {
  show_rtti_tree = false,
  module_name = "NieR Replicant ver.1.22474487139.exe",
  manager_address = "NieR Replicant ver.1.22474487139.exe+26FA120", 
}

-- ===================================================================

-- =========================== OFFSETS ===============================

local offset_pool_base = 0x10
local pool_capacity = 4096

local offset_actor_vtable = 0x0
local offset_actor_type_id = 0x1D8
local offset_pParamSet = 0x14350 

local offset_paramset_vtable = 0x0
local offset_paramset_entity_id = 0x20 
local offset_paramset_health = 0x2EC  

local ACTOR_TYPE_INFO_TABLE_RVA = 0x1101bc0 
local ACTOR_TYPE_INFO_TABLE_END_RVA = 0x110ac40 
local ACTOR_TYPE_INFO_SIZE = 0x80 

local ACTOR_TYPE_INFO_NAME_OFFSET = 0x00     
local ACTOR_TYPE_INFO_ID_OFFSET = 0x40        
local ACTOR_TYPE_INFO_ASSET_PATH_OFFSET = 0x48 

-- ===================================================================

-- ========================= GLOBAL CACHE ============================
local image_base = getAddress(config.module_name)
local cached_actor_type_names = {}  
local cached_actor_asset_paths = {}
-- ===================================================================

-- ======================== RTTI PARSING LOGIC =======================

function buildTreeFromList(flat_descriptor_list, cursor)
  if not cursor or not flat_descriptor_list or cursor.i > #flat_descriptor_list then
    return nil, 0
  end

  local current_desc_addr = flat_descriptor_list[cursor.i]
  if not current_desc_addr or current_desc_addr == 0 then return nil, 1 end

  local type_desc_rva = readInteger(current_desc_addr + 0x00)
  local num_contained_bases = readInteger(current_desc_addr + 0x04)

  local type_desc_addr = image_base + type_desc_rva

  local node = {
    name = readString(type_desc_addr + 0x10, 255, false) or "???",
    bases = {}
  }

  cursor.i = cursor.i + 1
  local nodes_processed_in_branch = 1

  while nodes_processed_in_branch < num_contained_bases do
    local child_node, consumed_by_child = buildTreeFromList(flat_descriptor_list, cursor)
    if child_node then
      table.insert(node.bases, child_node)
    end
    if not consumed_by_child or consumed_by_child == 0 then break end
    nodes_processed_in_branch = nodes_processed_in_branch + consumed_by_child
  end

  return node, nodes_processed_in_branch
end

function getRttiTree(vtable_address)
  local success, result = pcall(function()
    if not vtable_address or vtable_address == 0 then return nil, "Invalid VTable Ptr" end
    local rtti_col_ptr = readPointer(vtable_address - 8)
    if not rtti_col_ptr or rtti_col_ptr == 0 then return nil, "Invalid RTTI Locator Ptr" end
    local hierarchy_desc_rva = readInteger(rtti_col_ptr + 0x10)
    if not hierarchy_desc_rva or hierarchy_desc_rva == 0 then return nil, "Invalid Hierarchy RVA" end

    local hierarchy_desc_addr = image_base + hierarchy_desc_rva
    local num_base_classes = readInteger(hierarchy_desc_addr + 0x08)
    local base_class_array_rva = readInteger(hierarchy_desc_addr + 0x0C)
    if not base_class_array_rva or base_class_array_rva == 0 then return nil, "No Base Class Array" end

    local base_class_array_addr = image_base + base_class_array_rva
    local flat_descriptor_list = {}
    for i = 0, num_base_classes - 1 do
        local desc_rva = readInteger(base_class_array_addr + i * 4)
        if desc_rva and desc_rva ~= 0 then
            table.insert(flat_descriptor_list, image_base + desc_rva)
        end
    end

    if #flat_descriptor_list == 0 then return nil, "Empty Descriptor List" end

    local cursor = { i = 1 }
    local tree, consumed = buildTreeFromList(flat_descriptor_list, cursor)
    return tree
  end)

  if success then return result else return nil, "Critical Parse Failure: " .. (result or "Unknown error") end
end

function printRttiTree(rtti_tree, prefix, is_last)
  if type(rtti_tree) ~= "table" then return end
  prefix = prefix or ""
  is_last = is_last == nil and true or is_last
  print(prefix .. (is_last and "└─ " or "├─ ") .. rtti_tree.name)
  local child_prefix = prefix .. (is_last and "    " or "│   ")
  if rtti_tree.bases and #rtti_tree.bases > 0 then
    for i, base_node in ipairs(rtti_tree.bases) do
      printRttiTree(base_node, child_prefix, i == #rtti_tree.bases)
    end
  end
end

-- ======================== GAME DATA PARSING ========================

function buildActorTypeInfoCache()
  local table_start_addr = image_base + ACTOR_TYPE_INFO_TABLE_RVA
  local table_end_addr = image_base + ACTOR_TYPE_INFO_TABLE_END_RVA

  local current_entry_addr = table_start_addr
  local entries_found = 0

  while current_entry_addr < table_end_addr do
    local entity_name_raw = readString(current_entry_addr + ACTOR_TYPE_INFO_NAME_OFFSET, 64, false)
    local entity_id = readInteger(current_entry_addr + ACTOR_TYPE_INFO_ID_OFFSET)
    local asset_path_ptr = readPointer(current_entry_addr + ACTOR_TYPE_INFO_ASSET_PATH_OFFSET)

    if entity_name_raw and entity_name_raw ~= "" and entity_id then
      local entity_name = entity_name_raw:gsub("%z", ""):gsub("%c", "")
      local asset_path = "N/A"
      if asset_path_ptr and asset_path_ptr > 0x10000 then
        asset_path = readString(asset_path_ptr, 255, false) or "N/A"
        asset_path = asset_path:gsub("%z", ""):gsub("%c", "")
      end

      cached_actor_type_names[entity_id] = entity_name
      cached_actor_asset_paths[entity_id] = asset_path
      entries_found = entries_found + 1
    end

    current_entry_addr = current_entry_addr + ACTOR_TYPE_INFO_SIZE
  end
end

-- ======================== MAIN SCRIPT LOGIC ========================

function readEntityList()
  if not image_base or image_base == 0 then
    print("Error: Could not get module base address for '"..config.module_name.."'. Is it spelled correctly?")
    return
  end

  if next(cached_actor_type_names) == nil then 
    buildActorTypeInfoCache()
  end

  local pManagerData = getAddress(config.manager_address)
  if not pManagerData then
    print("Error: Could not resolve CHandleManager address. Is the game running and the address correct?")
    return
  end

  local pool_base = pManagerData + offset_pool_base
  print("=====================================================")
  print("           Nier Replicant Entity Scanner           ")
  print(string.format(" CHandleManager at: 0x%X", pManagerData))
  print("=====================================================")

  for i = pool_capacity - 1, 0, -1 do
    local pCActor = readPointer(pool_base + (i * 8))

    if pCActor and pCActor > 0x10000 then

      local actor_type_id = readInteger(pCActor + offset_actor_type_id) or -1
      local entity_name = cached_actor_type_names[actor_type_id] or "Unknown (ID: "..actor_type_id..")"
      local asset_path = cached_actor_asset_paths[actor_type_id] or "N/A"

      local current_hp = "N/A"
      local paramset_id = "N/A"
      local pCParamSet = readPointer(pCActor + offset_pParamSet)

      if pCParamSet and pCParamSet > 0x10000 then
        current_hp = readInteger(pCParamSet + offset_paramset_health) or "N/A"
        paramset_id = readInteger(pCParamSet + offset_paramset_entity_id) or "N/A"
      end

      print("-----------------------------------------------------")
      print(string.format("Index[%d] Type: %-12s (ID: %d)", i, entity_name, actor_type_id))
      -- print(string.format("  Asset Path: %s", asset_path))
      print(string.format("  Current HP: %s | CParamSet EntityID: %s", current_hp, paramset_id))
      print(string.format("  pCActor: 0x%X | pCParamSet: 0x%X", pCActor, pCParamSet or 0))

      if config.show_rtti_tree then
        local vtable_actor = readPointer(pCActor + offset_actor_vtable)
        local actor_tree, actor_err = getRttiTree(vtable_actor)
        if actor_tree then
          print("  RTTI Hierarchy (cActor):")
          printRttiTree(actor_tree, "    ")
        else
          print("  RTTI Hierarchy (cActor): " .. (actor_err or "N/A"))
        end

        if pCParamSet and pCParamSet > 0x10000 then
          local vtable_paramset = readPointer(pCParamSet + offset_paramset_vtable)
          local param_tree, param_err = getRttiTree(vtable_paramset)
          if param_tree then
            print("  RTTI Hierarchy (CParamSet):")
            printRttiTree(param_tree, "    ")
          else
            print("  RTTI Hierarchy (CParamSet): " .. (param_err or "N/A"))
          end
        end
      end
    end
    collectgarbage("step") 
  end
  print("-----------------------------------------------------")
  print("Scan Complete.")
  collectgarbage("collect")
end

readEntityList()