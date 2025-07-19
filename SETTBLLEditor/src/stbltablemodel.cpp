#include "stbltablemodel.h"
#include <QApplication>
#include <QMessageBox>
#include <QColor>
#include <QPalette>
#include <cmath>

StblTableModel::StblTableModel(QObject* parent)
    : QAbstractTableModel(parent)
{}

std::shared_ptr<StblTable> StblTableModel::getTable() const {
    return m_table.lock();
}

void StblTableModel::setTable(std::shared_ptr<StblTable> table, std::optional<Schema> schema) {
    beginResetModel();
    m_table = table;
    m_schema = schema;
    endResetModel();
}




int StblTableModel::rowCount(const QModelIndex&) const {
    if (auto table = m_table.lock()) {
        return table->getRowCount();
    }
    return 0;
}

int StblTableModel::columnCount(const QModelIndex&) const
{
    if (auto table = m_table.lock()) {
        return table->getFieldCount();
      
    }
     return 0;
}

QVariant StblTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    if (orientation == Qt::Horizontal) {
        if (m_schema) {
            if (auto col = m_schema->getColumn(section)) {
                return col->name;
            }
        }
        return QString("Field %1").arg(section);
    }
    else {
        return QString("%1").arg(section);
    }
}

Qt::ItemFlags StblTableModel::flags(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }
    return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}

QVariant StblTableModel::data(const QModelIndex& index, int role) const
{

    auto table = m_table.lock();
    if (!table || !index.isValid()) {
        return QVariant();
    }

    if (role == Qt::ToolTipRole && m_schema) {
        if (auto col = m_schema->getColumn(index.column())) {
            if (!col->description.isEmpty()) {
                return QString("<b>%1</b> (%2)<hr>%3")
                    .arg(col->name)
                    .arg(this->data(index, Qt::DisplayRole).toString())
                    .arg(col->description);
            }
        }
    }

    const auto& field = table->getRow(index.row())[index.column()];


    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        return std::visit(
            [](auto&& arg) -> QVariant {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, std::monostate>) {
                    return QVariant("[DEFAULT]");
                }
                else if constexpr (std::is_same_v<T, int32_t>) {
                    return QVariant(arg);
                }
                else if constexpr (std::is_same_v<T, float>) {
                    return QVariant(arg);
                }
                else if constexpr (std::is_same_v<T, std::string>) {
                    return QVariant(QString::fromStdString(arg));
                }
            }
            , field.getData());
    }

    if (role == Qt::BackgroundRole) {
        const QPalette& palette = QApplication::palette();
        bool isDarkMode = palette.color(QPalette::WindowText).lightness() > palette.color(QPalette::Base).lightness();

        if (field.getType() != StblFieldType::DEFAULT) {
            if (isDarkMode) {
                switch (field.getType()) {
                case StblFieldType::INT:    return QColor(45, 45, 70);
                case StblFieldType::FLOAT:  return QColor(35, 55, 50);
                case StblFieldType::STRING: return QColor(65, 45, 60);
                default: break;
                }
            }
            else {  
                switch (field.getType()) {
                case StblFieldType::INT:    return QColor(230, 235, 255);
                case StblFieldType::FLOAT:  return QColor(230, 245, 233);
                case StblFieldType::STRING: return QColor(250, 230, 245);
                default: break;
                }
            }
        }
        else {
            if (index.row() % 2 != 0) {
                if (isDarkMode) {
                    return palette.color(QPalette::Base).lighter(115);
                }
                else {
                    return palette.color(QPalette::Base).darker(105);
                }
            }
        }
    }

    return QVariant();
}

bool StblTableModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    auto table = m_table.lock();
    if (!table || !index.isValid() || role != Qt::EditRole) {
        return false;
    }

    auto& field = table->getRow(index.row())[index.column()];
    QString strValue = value.toString().trimmed();
    bool conversionOk = true;


    switch (field.getType()) { 
    case StblFieldType::INT: {
        int intVal = strValue.toInt(&conversionOk);
        if (conversionOk) field.setInt(static_cast<int32_t>(intVal));
        break;
    }
    case StblFieldType::FLOAT: {
        float floatVal = strValue.toFloat(&conversionOk);
        if (conversionOk) field.setFloat(floatVal);
        break;
    }
    case StblFieldType::STRING:
    case StblFieldType::DEFAULT: { // Treat editing a default cell as creating a string
        field.setString(strValue.toStdString());
        break;
    }
    default:
        conversionOk = false;
        break;
    }


    if (conversionOk) {
        emit dataChanged(index, index, { Qt::DisplayRole, Qt::EditRole, Qt::BackgroundRole });
        return true;
    }
    else {
        QMessageBox::warning(
            QApplication::activeWindow(),
            "Invalid Value",
            QString("The value '%1' is not a valid for the cell's current type (%2).\n\n"
                "The edit has been rejected. To change the cell's type, "
                "right-click it and use the context menu.")
            .arg(strValue)
            .arg(field.getType() == StblFieldType::INT ? "Integer" : "Float")
        );
        return false;
    }
}