#pragma once

#include <QDateTime>
#include <QWidget>
#include <QTableView>
#include <QListView>
#include <QStandardItemModel>
#include <QVBoxLayout>
#include <QDebug>
#include <QMessageBox>
#include <QDesktopServices>
#include <QMenu>
#include <QAction>
#include <QFileIconProvider>
#include <QIcon>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

struct RecycleBinItem {
    QString originalPath;
    QDateTime deletionTime;
    qint64 fileSize;
    bool isValid = false;
};

class RecycleBinWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RecycleBinWidget(QWidget *parent = nullptr);
    void refresh();
    bool isEmpty() const;
    void setViewMode(int mode);

    signals:
        void folderDoubleClicked(const QString &path);

private slots:
    void onItemDoubleClicked(const QModelIndex &index);
    void showContextMenu(const QPoint &pos);
    void restoreSelectedItems();
    void deleteSelectedItemsPermanently();

private:
    void populateRecycleBin();
    void setupListView();
    void setupTableView();
    void setupThumbnailView();
    void generateThumbnails(); // Добавлено
    QIcon getThumbnailIcon(const QString &filePath, bool isDir); // Добавлено

#ifdef Q_OS_WIN
    QString getCurrentUserSid();
    RecycleBinItem parseIFile(const QString &iFilePath);
#endif

    int scanRecycleBinFolder(const QString &folderPath, const QString &driveLetter);

    QTableView *tableView;
    QListView *listView;
    QListView *thumbnailView;
    QStandardItemModel *model;
    QMenu *contextMenu;
    QAction *restoreAction;
    QAction *deletePermanentlyAction;
    int currentViewMode = 0;
};