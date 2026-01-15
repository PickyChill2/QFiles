#pragma once

#include <QWidget>
//#include <QFileSystemModel>
#include <QListView>
#include <QTreeView>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include "thumbnailview.h"
#include "homepagewidget.h"
#include "customfilesystemmodel.h"

#ifdef Q_OS_WIN
#include "recyclebinwidget.h"
#endif

class MainWindow;
class ListModeDelegate;
class TreeSizeDelegate;

class FileSystemTab : public QWidget
{
    Q_OBJECT

public:
    explicit FileSystemTab(QWidget *parent = nullptr);
    ~FileSystemTab();

    void navigateTo(const QString &path);
    void refresh();
    void setViewMode(int mode, bool saveSetting = true); // Добавлен параметр для сохранения
    int getViewMode() const { return currentViewMode; }
    QString getCurrentPath() const;
    QString getTabName() const;

    QAbstractItemView* getCurrentView() const;
    QStringList getSelectedFiles() const;
    CustomFileSystemModel* getFileSystemModel() const { return model; }
    MainWindow* getMainWindow() const;

    // Navigation methods
    bool canGoBack() const;
    bool canGoForward() const;
    void goBack();
    void goForward();

    // Recycle bin methods
    void closeRecycleBin();
    bool isRecycleBinActive() const;

    // Rename methods
    void startRename();

    // Sorting methods
    void setSorting(int column, Qt::SortOrder order);
    int getSortColumn() const { return currentSortColumn; }
    Qt::SortOrder getSortOrder() const { return currentSortOrder; }

signals:
    void viewModeChanged(int mode); // Добавлен сигнал для изменения режима просмотра
    void pathChanged(const QString &path);
    void itemDoubleClicked(const QModelIndex &index);
    void contextMenuRequested(const QPoint &pos);
    void middleClickOnFolder(const QString &path);
    void navigationStateChanged();
    void sortChanged(int column, Qt::SortOrder order);

public slots:
    void showHomePage();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void onDirectoryLoaded(const QString &path);
    void onItemDoubleClicked(const QModelIndex &index);
    void onHomePageFolderClicked(const QString &path);
    void onMiddleClickOnHomePageFolder(const QString &path);
    void showContextMenu(const QPoint &pos);
    void onHeaderClicked(int logicalIndex);
    void updateDiskSizeInfo();

private:
    void setupUI();
    void setupConnections();
    void applyViewMode(int mode);
    void applySorting();
    bool isDrivePath(const QString &path);
    void addToHistory(const QString &path);
    void loadSavedViewMode();

    CustomFileSystemModel *model;
    QListView *listView;
    QTreeView *treeView;
    ThumbnailView *thumbnailView;
    HomePageWidget *homePageWidget;
    ListModeDelegate *listModeDelegate;
    TreeSizeDelegate *treeSizeDelegate;

    // Navigation history
    QStringList backHistory;
    QStringList forwardHistory;
    QString currentPath;

    int currentViewMode = 0;
    int currentSortColumn = 0;
    Qt::SortOrder currentSortOrder = Qt::AscendingOrder;
    bool initialLoadDone = false;
    bool isRenaming = false;
    QTimer *diskInfoTimer;
    bool isLoadingSavedViewMode = false; // Флаг для предотвращения повторного сохранения при загрузке
};