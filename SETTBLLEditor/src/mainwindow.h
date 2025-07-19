#pragma once

#include <QMainWindow>
#include <settbll.h>
#include <optional>
#include "schema.h"
#include <QMap>

class QListWidget;
class QTableView;
class QAction;
class QCheckBox;
class StblTableModel;
class SpatialEntityModel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void openFile();
    void saveFile();
    void onTableSelected(int currentRow);
    void updateColumnVisibility();
    void showContextMenu(const QPoint& pos); 

private:
    void updateWindowTitle(const QString& currentFile = "");
    void convertSelectedCells(StblFieldType newType); 
    void loadSchemasForFile(const QString& basePath);
    QMap<QString, Schema> m_loadedSchemas;

    std::shared_ptr<StblFile> m_stblFile;
    QString m_currentFilePath;

    QWidget* m_leftPanel;
    QListWidget* m_tableListWidget;
    QTableView* m_tableView;
    QCheckBox* m_hideEmptyColumnsCheckbox;

    StblTableModel* m_tableModel;
    SpatialEntityModel* m_spatialEntityModel;

    QAction* m_openAction;
    QAction* m_saveAction;
    QAction* m_exitAction;
};