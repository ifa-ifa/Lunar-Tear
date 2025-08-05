#include "mainwindow.h"
#include "stbltablemodel.h"
#include "spatialentitymodel.h"

#include <QtWidgets>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    m_leftPanel = new QWidget(this);
    m_tableListWidget = new QListWidget(this);
    m_hideEmptyColumnsCheckbox = new QCheckBox(tr("Hide Empty Columns"), this);
    m_hideEmptyColumnsCheckbox->setChecked(true);

    m_tableView = new QTableView(this);
    m_tableModel = new StblTableModel(this);
    m_spatialEntityModel = new SpatialEntityModel(this);

    m_tableView->setModel(m_tableModel);
    m_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_tableView->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    m_tableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_tableView, &QTableView::customContextMenuRequested, this, &MainWindow::showContextMenu);

    connect(m_tableListWidget, &QListWidget::currentRowChanged, this, &MainWindow::onTableSelected);
    connect(m_hideEmptyColumnsCheckbox, &QCheckBox::toggled, this, &MainWindow::updateColumnVisibility);

    QVBoxLayout* leftLayout = new QVBoxLayout();
    leftLayout->addWidget(m_tableListWidget);
    leftLayout->addWidget(m_hideEmptyColumnsCheckbox);
    leftLayout->addStretch();
    m_leftPanel->setLayout(leftLayout);

    QSplitter* splitter = new QSplitter(Qt::Horizontal, this); // For the window resizer
    splitter->addWidget(m_leftPanel);
    splitter->addWidget(m_tableView);
    splitter->setSizes({ 250, 1030 });

    setCentralWidget(splitter);

    m_openAction = new QAction(tr("&Open..."), this);
    m_openAction->setShortcut(QKeySequence::Open);
    connect(m_openAction, &QAction::triggered, this, &MainWindow::openFile);
    m_saveAction = new QAction(tr("&Save As..."), this);
    m_saveAction->setShortcut(QKeySequence::SaveAs);
    connect(m_saveAction, &QAction::triggered, this, &MainWindow::saveFile);
    m_exitAction = new QAction(tr("E&xit"), this);
    m_exitAction->setShortcut(QKeySequence::Quit);
    connect(m_exitAction, &QAction::triggered, this, &QWidget::close);

    QMenu* fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(m_openAction);
    fileMenu->addAction(m_saveAction);
    fileMenu->addSeparator();
    fileMenu->addAction(m_exitAction);

    updateWindowTitle();
    resize(1280, 768);
}

MainWindow::~MainWindow() {}


void MainWindow::showContextMenu(const QPoint& pos)
{
    if (m_tableView->model() != m_tableModel) {
        return;
    }

    QModelIndexList selectedIndexes = m_tableView->selectionModel()->selectedIndexes();
    if (selectedIndexes.isEmpty()) {
        return;
    }

    std::optional<StblFieldType> commonType;
    bool isFirst = true;
    for (const QModelIndex& index : selectedIndexes) {
        const auto& field = m_tableModel->getTable()->getRow(index.row())[index.column()];
        if (isFirst) {
            commonType = field.getType(); 
            isFirst = false;
        }
        else if (commonType.has_value() && commonType.value() != field.getType()) {
            commonType.reset(); // Mixed types, so no common type
            break;
        }
    }

    QMenu contextMenu(tr("Context menu"), this);
    QActionGroup* typeGroup = new QActionGroup(&contextMenu);
    typeGroup->setExclusive(true);

    auto addTypeAction = [&](const QString& text, StblFieldType type) {
        QAction* action = contextMenu.addAction(text);
        action->setCheckable(true);
        typeGroup->addAction(action);
        if (commonType.has_value() && commonType.value() == type) {
            action->setChecked(true);
        }
        connect(action, &QAction::triggered, [this, type]() {
            convertSelectedCells(type);
            });
        };

    addTypeAction("Integer", StblFieldType::INT);
    addTypeAction("Float", StblFieldType::FLOAT);
    addTypeAction("String", StblFieldType::STRING);
    contextMenu.addSeparator();
    addTypeAction("Default (Empty)", StblFieldType::DEFAULT);

    contextMenu.exec(m_tableView->viewport()->mapToGlobal(pos));
}

void MainWindow::convertSelectedCells(StblFieldType newType)
{
    if (!m_tableModel || !m_tableModel->getTable()) return;

    QModelIndexList selectedIndexes = m_tableView->selectionModel()->selectedIndexes();
    if (selectedIndexes.isEmpty()) return;

    QStringList failedConversions;

    for (const QModelIndex& index : selectedIndexes) {
        if (!index.isValid()) continue;

        auto& field = m_tableModel->getTable()->getRow(index.row())[index.column()];
        QString currentValue = m_tableModel->data(index, Qt::EditRole).toString();
        bool conversionOk = true;

        switch (newType) {
        case StblFieldType::INT: {
            int intVal = currentValue.toInt(&conversionOk);
            if (conversionOk) field.setInt(static_cast<int32_t>(intVal));
            break;
        }
        case StblFieldType::FLOAT: {
            float floatVal = currentValue.toFloat(&conversionOk);
            if (conversionOk) field.setFloat(floatVal); 
            break;
        }
        case StblFieldType::STRING:
            field.setString(currentValue.toStdString()); 
            conversionOk = true;
            break;
        case StblFieldType::DEFAULT:
            field.setDefault(); 
            conversionOk = true;
            break;
        default:
            conversionOk = false;
            break;
        }


        if (conversionOk) {
            emit m_tableModel->dataChanged(index, index, { Qt::DisplayRole, Qt::EditRole, Qt::BackgroundRole });
        }
        else {
            failedConversions.append(QString("Row %1, Col %2: Failed to convert '%3'.")
                .arg(index.row()).arg(index.column()).arg(currentValue));
        }
    }

    if (!failedConversions.isEmpty()) {
        QMessageBox::warning(this, "Conversion Errors",
            "Some cells could not be converted and were left unchanged:\n\n" + failedConversions.join("\n"));
    }
}


void MainWindow::updateWindowTitle(const QString& currentFile)
{
    QString title = "Lunar Tear - STBL Editor";
    if (!currentFile.isEmpty()) {
        title += " - " + QFileInfo(currentFile).fileName();
    }
    setWindowTitle(title);
}

void MainWindow::openFile()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("Open STBL File"), "", tr("STBL Files (*.settbll *.stbl);;All Files (*)"));
    if (filePath.isEmpty()) { return; }

    m_tableModel->setTable(nullptr, std::nullopt);
    m_spatialEntityModel->setEntities(nullptr);
    m_tableView->setModel(nullptr); 


    m_stblFile = std::make_shared<StblFile>();
    if (m_stblFile->loadFromFile(filePath.toStdString())) {
        m_currentFilePath = filePath;
        updateWindowTitle(m_currentFilePath);
        m_tableListWidget->clear();
        m_loadedSchemas.clear();
        loadSchemasForFile(m_currentFilePath);

        if (!m_stblFile->getSpatialEntities().empty()) {
            QListWidgetItem* spatialItem = new QListWidgetItem(tr("Spatial Entities"));
            spatialItem->setForeground(QColor(150, 220, 255));
            m_tableListWidget->addItem(spatialItem);
        }
        for (const auto& table : m_stblFile->getTables()) {
            m_tableListWidget->addItem(QString::fromStdString(table.getName()));
        }
        if (m_tableListWidget->count() > 0) {
            m_tableListWidget->setCurrentRow(0);
        }
    }
    else {
        QMessageBox::critical(this, tr("Error"), tr("Failed to load the STBL file."));
        updateWindowTitle();
    }
}

void MainWindow::loadSchemasForFile(const QString& basePath) {
    QDir schemaDir(QCoreApplication::applicationDirPath() + "/schemas");
    if (!schemaDir.exists()) {
        qDebug() << "Schema directory not found:" << schemaDir.path();
        return;
    }

    for (const auto& table : m_stblFile->getTables()) {
        QString tableName = QString::fromStdString(table.getName());
        QString schemaPath = schemaDir.filePath(tableName + ".schema.json");
        QFile schemaFile(schemaPath);

        if (!schemaFile.exists()) continue;

        if (!schemaFile.open(QIODevice::ReadOnly)) {
            qWarning() << "Could not open schema file:" << schemaPath;
            continue;
        }

        QByteArray schemaData = schemaFile.readAll();
        QJsonDocument doc(QJsonDocument::fromJson(schemaData));
        QJsonObject root = doc.object();

        Schema schema;
        schema.tableName = root["tableName"].toString();

        QJsonArray columns = root["columns"].toArray();
        for (const QJsonValue& val : columns) {
            QJsonObject colObj = val.toObject();
            SchemaColumn col;
            col.name = colObj["name"].toString();
            col.description = colObj["description"].toString();

            QString typeStr = colObj["type"].toString().toLower();
            if (typeStr == "int") col.type = StblFieldType::INT;
            else if (typeStr == "float") col.type = StblFieldType::FLOAT;
            else if (typeStr == "string") col.type = StblFieldType::STRING;
            else col.type = StblFieldType::DEFAULT;

            schema.columns.append(col);
        }
        m_loadedSchemas[tableName] = schema;
        qDebug() << "Loaded schema for table:" << tableName;
    }
}

void MainWindow::saveFile()
{
    if (m_currentFilePath.isEmpty()) { return; }
    QString filePath = QFileDialog::getSaveFileName(this, tr("Save STBL File As..."), m_currentFilePath, tr("STBL Files (*.settbll *.stbl);;All Files (*)"));
    if (filePath.isEmpty()) { 
        return; 
    }
    if (m_stblFile->saveToFile(filePath.toStdString())) {
        QMessageBox::information(this, tr("Success"), tr("File saved successfully"));
        m_currentFilePath = filePath;
        updateWindowTitle(m_currentFilePath);
    }
    else {
        QMessageBox::critical(this, tr("Error"), tr("Failed to save the STBL file."));
    }
}

void MainWindow::onTableSelected(int currentRow)
{
    if (currentRow < 0 || !m_stblFile) {
        return;
    }

    bool hasSpatialTable = !m_stblFile->getSpatialEntities().empty();
    bool isSpatialTableSelected = hasSpatialTable && currentRow == 0;

    if (isSpatialTableSelected) {

        auto entities_sp = std::shared_ptr<std::vector<StblSpatialEntity>>(m_stblFile, &m_stblFile->getSpatialEntities());
        m_spatialEntityModel->setEntities(entities_sp);
        m_tableView->setModel(m_spatialEntityModel);
    }
    else {
        int tableIndex = hasSpatialTable ? currentRow - 1 : currentRow;
        if (tableIndex >= 0 && tableIndex < m_stblFile->getTables().size()) {
            auto table_sp = std::shared_ptr<StblTable>(m_stblFile, &m_stblFile->getTables()[tableIndex]);     

            QString tableName = QString::fromStdString(table_sp->getName());
            std::optional<Schema> schema;
            if (m_loadedSchemas.contains(tableName)) {
                schema = m_loadedSchemas[tableName];
            }

            m_tableModel->setTable(table_sp, schema); 
            m_tableView->setModel(m_tableModel);
        }
    } 
    updateColumnVisibility();
}

void MainWindow::updateColumnVisibility()
{
    if (m_tableView->model() != m_tableModel) {
        if (m_spatialEntityModel->columnCount() > 0) {
            for (int i = 0; i < m_spatialEntityModel->columnCount(); ++i) {
                m_tableView->setColumnHidden(i, false);
            }
        }
        return;
    }
    bool hide = m_hideEmptyColumnsCheckbox->isChecked();
    if (!hide) {
        for (int i = 0; i < m_tableModel->columnCount(); ++i) {
            m_tableView->setColumnHidden(i, false);
        }
        return;
    }
    for (int col = 0; col < m_tableModel->columnCount(); ++col) {
        bool isColumnEmpty = true;
        for (int row = 0; row < m_tableModel->rowCount(); ++row) {
            const auto& field = m_tableModel->getTable()->getRow(row)[col];
            if (field.getType() != StblFieldType::DEFAULT) {
                isColumnEmpty = false;
                break;
            }
        }
        m_tableView->setColumnHidden(col, isColumnEmpty);
    }
}