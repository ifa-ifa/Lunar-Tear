--[[
  Run in Cheat Engine's Lua Engine
]]


local config = {
  module_name = "NieR Replicant ver.1.22474487139.exe",
  target_object_ptr = 0x172C76DD030, 
  show_rtti_tree = true
}



local image_base = getAddress(config.module_name)

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


function inspectObjectRtti(object_ptr)
  print("=====================================================")
  print("             RTTI Inspector for Object")
  print("=====================================================")
  print(string.format("Target Object Pointer: 0x%X", object_ptr))

  local vtable = readPointer(object_ptr)
  if not vtable or vtable == 0 then
    print("Error: Could not read vtable from object pointer.")
    return
  end

  print(string.format("VTable Address: 0x%X", vtable))

  if config.show_rtti_tree then
    local rtti_tree, err = getRttiTree(vtable)
    if rtti_tree then
      print("RTTI Hierarchy:")
      printRttiTree(rtti_tree, "  ")
    else
      print("RTTI Hierarchy: " .. (err or "Unknown error"))
    end
  end

  print("=====================================================")
end


inspectObjectRtti(config.target_object_ptr)
