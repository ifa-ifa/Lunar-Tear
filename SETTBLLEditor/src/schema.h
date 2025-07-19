#pragma once

#include <QString>
#include <QVector>
#include <optional>
#include <settbll.h> 

struct SchemaColumn {
    QString name;
    QString description;
    StblFieldType type;
};

struct Schema {
    QString tableName;
    QVector<SchemaColumn> columns;

    std::optional<SchemaColumn> getColumn(int index) const {
        if (index >= 0 && index < columns.size()) {
            return columns[index];
        }
        return std::nullopt;
    }
};