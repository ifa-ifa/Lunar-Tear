#pragma once

#include <QAbstractTableModel>
#include <settbll.h>
#include "schema.h" 
#include <optional> 

class StblTable;

class StblTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit StblTableModel(QObject* parent = nullptr);

    void setTable(std::shared_ptr<StblTable> table, std::optional<Schema> schema = std::nullopt);
    std::shared_ptr<StblTable> getTable() const;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

private:
    std::weak_ptr<StblTable> m_table;
    std::optional<Schema> m_schema;
};