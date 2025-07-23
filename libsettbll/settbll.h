#pragma once

#include <string>
#include <vector>
#include <variant>
#include <optional> 
#include <cstdint>

// Types in this file are intermediary and not necessarily applicable to the raw binary data


class StblFile;

struct Vector4 {
    float x = 0.0f, y = 0.0f, z = 0.0f, w = 0.0f;
};

struct StblSpatialEntity {
    Vector4 position;
    Vector4 volume_data;
    std::string name;
    int32_t unknown_params[4]{};
    std::vector<uint8_t> padding;
};

using StblFieldData = std::variant<std::monostate, int32_t, float, std::string>;

enum class StblFieldType {
    DEFAULT = 0,
    INT = 1,
    FLOAT = 2,
    STRING = 3,
    UNKNOWN_4 = 4,
    UNKNOWN_5 = 5
};

struct StblField {
public:

    StblFieldType getType() const { return m_type; }
    const StblFieldData& getData() const { return m_data; }

    std::optional<int32_t> getInt() const {
        return (m_type == StblFieldType::INT) ? std::optional(std::get<int32_t>(m_data)) : std::nullopt;
    }
    std::optional<float> getFloat() const {
        return (m_type == StblFieldType::FLOAT) ? std::optional(std::get<float>(m_data)) : std::nullopt;
    }
    std::optional<std::string> getString() const {
        return (m_type == StblFieldType::STRING) ? std::optional(std::get<std::string>(m_data)) : std::nullopt;
    }

    void setInt(int32_t value) {
        m_data = value;
        m_type = StblFieldType::INT;
    }

    void setFloat(float value) {
        m_data = value;
        m_type = StblFieldType::FLOAT;
    }

    void setString(const std::string& value) {
        m_data = value;
        m_type = StblFieldType::STRING;
    }

    void setString(std::string&& value) { 
        m_data = std::move(value);
        m_type = StblFieldType::STRING;
    }

    void setDefault() {
        m_data = std::monostate{};
        m_type = StblFieldType::DEFAULT;
    }

private:
    StblFieldType m_type = StblFieldType::DEFAULT;
    StblFieldData m_data = std::monostate{};
};






using StblRow = std::vector<StblField>;

class StblTable {
public:
    const std::string& getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }

    size_t getRowCount() const { return m_rows.size(); }
    size_t getFieldCount() const { return m_rows.empty() ? 0 : m_rows[0].size(); }

    StblRow& getRow(size_t rowIndex) { return m_rows.at(rowIndex); }
    const StblRow& getRow(size_t rowIndex) const { return m_rows.at(rowIndex); }

    void addRow(const StblRow& row);
    void removeRow(size_t rowIndex);

private:
    friend class StblFile;
    std::string m_name;
    std::vector<StblRow> m_rows;
    std::vector<uint8_t> m_unknown_data;
};

class StblFile {
public:

    static size_t InferSize(const char* buffer);

    bool loadFromFile(const std::string& filepath);
    bool loadFromMemory(const char* buffer, size_t size);
    bool saveToFile(const std::string& filepath);

    std::vector<StblTable>& getTables() { return m_tables; }
    const std::vector<StblTable>& getTables() const { return m_tables; }

    std::vector<StblSpatialEntity>& getSpatialEntities() { return m_spatial_entities; }
    const std::vector<StblSpatialEntity>& getSpatialEntities() const { return m_spatial_entities; }

    int getVersion() const { return m_version; }
    void setVersion(int version) { m_version = version; }

private:
    int m_version = 0;
    std::vector<uint8_t> m_header_unknown_data;
    std::vector<StblSpatialEntity> m_spatial_entities;
    std::vector<StblTable> m_tables;
};