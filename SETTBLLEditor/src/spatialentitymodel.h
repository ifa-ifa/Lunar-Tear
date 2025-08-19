#pragma once

#include <QAbstractTableModel>
#include <vector>
#include <replicant/stbl.h>

class SpatialEntityModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit SpatialEntityModel(QObject* parent = nullptr);

    void setEntities(std::shared_ptr<const std::vector<StblSpatialEntity>> entities);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

private:
    std::weak_ptr<const std::vector<StblSpatialEntity>> m_entities;
};