#include "spatialentitymodel.h"
#include <QString>

SpatialEntityModel::SpatialEntityModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

void SpatialEntityModel::setEntities(std::shared_ptr<const std::vector<StblSpatialEntity>> entities) {
    beginResetModel();
    m_entities = entities;
    endResetModel();
}

int SpatialEntityModel::rowCount(const QModelIndex&) const
{
    if (auto entities = m_entities.lock()) {
        return entities->size();
    }
    return 0;
}

int SpatialEntityModel::columnCount(const QModelIndex&) const
{
    // Show the unknown params for completeness
    return 4;
}

QVariant SpatialEntityModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal) {
        return QVariant();
    }
    switch (section) {
    case 0: return "Name";
    case 1: return "Position (X, Y, Z, W)";
    case 2: return "Volume Data (X, Y, Z, W)";
    case 3: return "Unknown Params";
    default: return QVariant();
    }
}

Qt::ItemFlags SpatialEntityModel::flags(const QModelIndex& index) const
{
    // This table is read-only. I may add editing later if needed.
    return QAbstractTableModel::flags(index);
}

QVariant SpatialEntityModel::data(const QModelIndex& index, int role) const
{


    auto entities = m_entities.lock();
    if (!entities || !index.isValid() || role != Qt::DisplayRole) {
        return QVariant();
    }

    const auto& entity = (*entities)[index.row()];

    switch (index.column()) {
    case 0:
        return QString::fromStdString(entity.name);
    case 1:
        return QString("(%1, %2, %3, %4)")
            .arg(entity.position.x, 0, 'f', 2)
            .arg(entity.position.y, 0, 'f', 2)
            .arg(entity.position.z, 0, 'f', 2)
            .arg(entity.position.w, 0, 'f', 2);
    case 2:
        return QString("(%1, %2, %3, %4)")
            .arg(entity.volume_data.x, 0, 'f', 2)
            .arg(entity.volume_data.y, 0, 'f', 2)
            .arg(entity.volume_data.z, 0, 'f', 2)
            .arg(entity.volume_data.w, 0, 'f', 2);
    case 3:
        return QString("(%1, %2, %3, %4)")
            .arg(entity.unknown_params[0])
            .arg(entity.unknown_params[1])
            .arg(entity.unknown_params[2])
            .arg(entity.unknown_params[3]);
    }
    return QVariant();
}