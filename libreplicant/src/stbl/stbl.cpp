#include "replicant/stbl.h"
#include <fstream>
#include <vector>
#include <set>
#include <map>
#include <iostream>
#include <cstring>

// These structs match the binary file format exactly

namespace {

#pragma pack(push, 1)
    struct STBL_FileHeader {
        char magic_bytes[4];
        int32_t version;
        uint8_t padding_0[4];
        int32_t spatialEntityCount;
        int32_t header_size;
        uint8_t padding_1[4];
        int32_t table_descriptor_offset;
        int32_t table_count;
        uint8_t unknown[48];
    };
    struct STBL_SpatialEntity {
        Vector4 position;
        Vector4 volume_data;
        char name[32];
        int32_t unknown_params[4];
        uint8_t padding[16];
    };
    union STBL_FieldData {
        float as_float;
        int32_t as_int;
        int32_t offset_to_string;
        uint8_t rawdata[4];
    };
    struct STBL_Field {
        uint32_t field_type;
        STBL_FieldData data;
    };
    struct STBL_TableDescriptor {
        char tableName[32];
        int32_t rowCount;
        int32_t rowSizeInFields;
        int32_t dataOffset;
        uint8_t unknown[20];
    };
#pragma pack(pop)

    std::string stringFromFixed(const char* buffer, size_t max_len) {
        size_t len = strnlen(buffer, max_len);
        return std::string(buffer, len);
    }
}

void StblTable::addRow(const StblRow& row) {
    if (!m_rows.empty() && row.size() != getFieldCount()) {
        return;
    }
    m_rows.push_back(row);
}

void StblTable::removeRow(size_t rowIndex) {
    if (rowIndex < m_rows.size()) {
        m_rows.erase(m_rows.begin() + rowIndex);
    }
}

bool StblFile::loadFromFile(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file) { return false; }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size)) { return false; }
    return loadFromMemory(buffer.data(), buffer.size());
}

bool StblFile::loadFromMemory(const char* buffer, size_t size) {
    m_tables.clear();
    m_spatial_entities.clear();
    m_version = 0;
    if (!buffer || size < sizeof(STBL_FileHeader)) { return false; }
    const auto* header = reinterpret_cast<const STBL_FileHeader*>(buffer);
    if (strncmp(header->magic_bytes, "STBL", 4) != 0) { return false; }
    m_version = header->version;
    m_header_unknown_data.assign(std::begin(header->unknown), std::end(header->unknown));

    const auto* raw_entities = reinterpret_cast<const STBL_SpatialEntity*>(buffer + header->header_size);
    for (int32_t i = 0; i < header->spatialEntityCount; ++i) {

        const auto& raw_entity = raw_entities[i];
        StblSpatialEntity entity;
        entity.position = raw_entity.position;
        entity.volume_data = raw_entity.volume_data;
        entity.name = stringFromFixed(raw_entity.name, 32);
        std::copy(std::begin(raw_entity.unknown_params), std::end(raw_entity.unknown_params), std::begin(entity.unknown_params));
        entity.padding.assign(std::begin(raw_entity.padding), std::end(raw_entity.padding));
        m_spatial_entities.push_back(entity);
    }

    const auto* raw_descriptors = reinterpret_cast<const STBL_TableDescriptor*>(buffer + header->table_descriptor_offset);
    for (int32_t i = 0; i < header->table_count; ++i) {
        const auto& raw_desc = raw_descriptors[i];
        StblTable table;
        table.setName(stringFromFixed(raw_desc.tableName, 32));
        table.m_unknown_data.assign(std::begin(raw_desc.unknown), std::end(raw_desc.unknown));
        const auto* raw_fields_base = reinterpret_cast<const STBL_Field*>(buffer + raw_desc.dataOffset);
        for (int32_t r = 0; r < raw_desc.rowCount; ++r) {
            StblRow row;
            for (int32_t f = 0; f < raw_desc.rowSizeInFields; ++f) {
                const auto& raw_field = raw_fields_base[r * raw_desc.rowSizeInFields + f];
                StblField field;

                auto field_type_from_file = static_cast<StblFieldType>(raw_field.field_type);

                switch (field_type_from_file) {
                case StblFieldType::INT:
                    field.setInt(raw_field.data.as_int);
                    break;
                case StblFieldType::FLOAT:
                    field.setFloat(raw_field.data.as_float);
                    break;
                case StblFieldType::STRING:
                {
                    int32_t offset = raw_field.data.offset_to_string;
                    if (offset > 0 && offset < size) {
                        size_t max_len = size - offset;
                        size_t actual_len = strnlen(buffer + offset, max_len);
                        field.setString(std::string(buffer + offset, actual_len)); 
                    }
                    else {
                        field.setString(""); 
                    }
                }
                break;
                case StblFieldType::DEFAULT:
                default:
                    field.setDefault(); 
                    break;
                }
                row.push_back(field);
            }
            table.addRow(row);
        }
        m_tables.push_back(table);
    }
    return true;
}

bool StblFile::saveToFile(const std::string& filepath) {
    std::ofstream file(filepath, std::ios::binary);
    if (!file) { return false; }

    // Using a single string pool for the whole file in theory shouldn't be problematic, but i have no idea why the original game ones didnt do this
    std::set<std::string> string_set;
    for (const auto& table : m_tables) {
        for (const auto& row : table.m_rows) {
            for (const auto& field : row) {
                if (std::holds_alternative<std::string>(field.getData())) {
                    string_set.insert(std::get<std::string>(field.getData()));
                }
            }
        }
    }
    std::vector<std::string> string_pool(string_set.begin(), string_set.end());

    uint32_t current_offset = sizeof(STBL_FileHeader);

    const uint32_t spatial_block_offset = current_offset;
    current_offset += m_spatial_entities.size() * sizeof(STBL_SpatialEntity);

    const uint32_t table_descriptors_offset = current_offset;
    current_offset += m_tables.size() * sizeof(STBL_TableDescriptor);

    std::vector<uint32_t> table_data_offsets;
    for (const auto& table : m_tables) {
        table_data_offsets.push_back(current_offset);
        current_offset += table.getRowCount() * table.getFieldCount() * sizeof(STBL_Field);
    }

    const uint32_t string_pool_offset = current_offset;
    std::map<std::string, uint32_t> string_to_offset_map;
    uint32_t string_data_size = 0;
    for (const auto& str : string_pool) {
        string_to_offset_map[str] = string_pool_offset + string_data_size;
        string_data_size += str.length() + 1;
    }

    std::vector<char> buffer(current_offset + string_data_size, 0);
    auto* header = reinterpret_cast<STBL_FileHeader*>(buffer.data());
    memcpy(header->magic_bytes, "STBL", 4);
    header->version = m_version;
    header->spatialEntityCount = m_spatial_entities.size();
    header->header_size = sizeof(STBL_FileHeader);
    header->table_descriptor_offset = table_descriptors_offset;
    header->table_count = m_tables.size();
    if (m_header_unknown_data.size() == 48) {
        memcpy(header->unknown, m_header_unknown_data.data(), 48);
    }

    auto* raw_entities = reinterpret_cast<STBL_SpatialEntity*>(buffer.data() + spatial_block_offset);
    for (size_t i = 0; i < m_spatial_entities.size(); ++i) {
        raw_entities[i].position = m_spatial_entities[i].position;
        raw_entities[i].volume_data = m_spatial_entities[i].volume_data;

        strncpy_s(raw_entities[i].name, sizeof(raw_entities[i].name), m_spatial_entities[i].name.c_str(), _TRUNCATE);

        std::copy(std::begin(m_spatial_entities[i].unknown_params), std::end(m_spatial_entities[i].unknown_params), std::begin(raw_entities[i].unknown_params));
        if (m_spatial_entities[i].padding.size() == 16) {
            memcpy(raw_entities[i].padding, m_spatial_entities[i].padding.data(), 16);
        }
    }

    auto* raw_descriptors = reinterpret_cast<STBL_TableDescriptor*>(buffer.data() + table_descriptors_offset);
    for (size_t i = 0; i < m_tables.size(); ++i) {
        strncpy_s(raw_descriptors[i].tableName, sizeof(raw_descriptors[i].tableName), m_tables[i].getName().c_str(), _TRUNCATE);

        raw_descriptors[i].rowCount = m_tables[i].getRowCount();
        raw_descriptors[i].rowSizeInFields = m_tables[i].getFieldCount();
        raw_descriptors[i].dataOffset = table_data_offsets[i];
        if (m_tables[i].m_unknown_data.size() == 20) {
            memcpy(raw_descriptors[i].unknown, m_tables[i].m_unknown_data.data(), 20);
        }
    }

    for (size_t i = 0; i < m_tables.size(); ++i) {
        auto* raw_fields = reinterpret_cast<STBL_Field*>(buffer.data() + table_data_offsets[i]);
        const auto& table = m_tables[i];
        for (size_t r = 0; r < table.getRowCount(); ++r) {
            for (size_t f = 0; f < table.getFieldCount(); ++f) {
                auto& raw_field = raw_fields[r * table.getFieldCount() + f];
                const auto& field = table.getRow(r)[f];
                raw_field.field_type = static_cast<uint32_t>(field.getType());

                std::visit([&](auto&& arg) {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, std::monostate>) {
                        raw_field.data.as_int = 0;
                    }
                    else if constexpr (std::is_same_v<T, int32_t>) {
                        raw_field.data.as_int = arg;
                    }
                    else if constexpr (std::is_same_v<T, float>) {
                        raw_field.data.as_float = arg;
                    }
                    else if constexpr (std::is_same_v<T, std::string>) {
                        if (string_to_offset_map.count(arg)) {
                            raw_field.data.offset_to_string = string_to_offset_map.at(arg);
                        }
                        else {
                            raw_field.data.offset_to_string = 0;
                        }
                    }
                    }, field.getData());
            }
        }
    }

    char* string_ptr = buffer.data() + string_pool_offset;
    for (const auto& str : string_pool) {
        memcpy(string_ptr, str.c_str(), str.length() + 1);
        string_ptr += str.length() + 1;
    }

    file.write(buffer.data(), buffer.size());
    return true;
}

size_t StblFile::InferSize(const char* buffer, size_t maxSize) {
    if (!buffer || maxSize < sizeof(STBL_FileHeader)) {
        return 0;
    }

    const auto* header = reinterpret_cast<const STBL_FileHeader*>(buffer);
    if (strncmp(header->magic_bytes, "STBL", 4) != 0) {
        return 0; 
    }

    size_t max_extent = sizeof(STBL_FileHeader);

    if (header->spatialEntityCount > 0) {
        if (header->header_size > maxSize) return 0;

        size_t spatial_block_end = (size_t)header->header_size + (header->spatialEntityCount * sizeof(STBL_SpatialEntity));
        if (spatial_block_end > maxSize) {
            return 0; 
        }
        max_extent = std::max(max_extent, spatial_block_end);
    }

    if (header->table_count > 0) {
        if (header->table_descriptor_offset > maxSize) return 0;

        size_t descriptor_block_end = (size_t)header->table_descriptor_offset + (header->table_count * sizeof(STBL_TableDescriptor));
        if (descriptor_block_end > maxSize) {
            return 0;
        }
        max_extent = std::max(max_extent, descriptor_block_end);
    }
    else {
        return max_extent;
    }

    const auto* descriptors = reinterpret_cast<const STBL_TableDescriptor*>(buffer + header->table_descriptor_offset);
    for (int32_t i = 0; i < header->table_count; ++i) {
        const auto& desc = descriptors[i];

        if (desc.dataOffset > maxSize) return 0;
        size_t table_data_end = desc.dataOffset + (desc.rowCount * desc.rowSizeInFields * sizeof(STBL_Field));
        if (table_data_end > maxSize) {
            return 0; 
        }
        max_extent = std::max(max_extent, table_data_end);

        const auto* fields_base = reinterpret_cast<const STBL_Field*>(buffer + desc.dataOffset);
        for (int32_t r = 0; r < desc.rowCount; ++r) {
            for (int32_t f = 0; f < desc.rowSizeInFields; ++f) {
                const auto& field = fields_base[r * desc.rowSizeInFields + f];

                if (static_cast<StblFieldType>(field.field_type) == StblFieldType::STRING) {
                    int32_t offset = field.data.offset_to_string;

                    if (offset > 0 && (size_t)offset < maxSize) {
                        size_t max_possible_len = maxSize - offset;
                        size_t actual_len = strnlen(buffer + offset, max_possible_len);

                        if (actual_len == max_possible_len) {
                            return 0; 
                        }

                        size_t string_end = offset + actual_len + 1;
                        max_extent = std::max(max_extent, string_end);
                    }
                    else if (offset != 0) {
                        return 0; 
                    }
                }
            }
        }
    }

    return max_extent;
}


static_assert(sizeof(STBL_FileHeader) == 80, "STBL_FileHeader size mismatch");
static_assert(sizeof(STBL_SpatialEntity) == 96, "STBL_SpatialEntity size mismatch");
static_assert(sizeof(STBL_TableDescriptor) == 64, "STBL_TableDescriptor size mismatch");
static_assert(sizeof(STBL_Field) == 8, "STBL_Field size mismatch");