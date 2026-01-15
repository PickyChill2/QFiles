#pragma once

#include <QMainWindow>
#include <QTabWidget>
#include <QFileSystemModel>
#include <QLineEdit>
#include <QToolBar>
#include <QAction>
#include <QSettings>
#include <QComboBox>
#include <QToolButton>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include "contextmenu.h"
#include "fileoperations.h"
#include "searchwidget.h"
#include "folderviewsettings.h"
#include "favoritesmenu.h"
#include "adminlauncher.h"
#include "archiver.h"
#include "notificationmanager.h"

class FileSystemTab;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    bool isShowingHiddenFiles() const { return showHiddenFiles; }
    void toggleHiddenFiles();
    void startRename(const QModelIndex &index);
    void startCreateNewFolder();
    QString getCurrentPath() const;
    int loadViewModeForPath(const QString &path);
    void loadSortSettingsForPath(const QString &path, int &sortColumn, Qt::SortOrder &sortOrder);
    void saveSortSettingsForPath(const QString &path, int sortColumn, Qt::SortOrder sortOrder);
    bool isExecutableFile(const QString &filePath) const;
    Archiver* getArchiver() const { return m_archiver; }
    NotificationManager* getNotificationManager() const { return m_notificationManager; }

    FileSystemTab* getCurrentTab() const;

public slots:
    void navigateTo(const QString &path);
    void navigateBack();
    void navigateForward();
    void navigateUp();
    void showHomePage();
    void refresh();
    void changeViewMode();
    void createNewFolder();
    void renameSelectedItem();
    void newTab();
    void closeCurrentTab();
    void deleteSelectedItems();
    void copySelectedItems();
    void cutSelectedItems();
    void pasteFiles();
    void softRefresh();
    void showSearchWidget();
    void showContextMenu(const QPoint &pos);
    void closeRecycleBin();
    void openInNewTab(const QString &path);

    // Sorting slots
    void onSortTypeChanged(int index);
    void toggleSortOrder();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void moveEvent(QMoveEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void onTabCurrentChanged(int index);
    void onTabCloseRequested(int index);
    void onTabContextMenuRequested(const QPoint &pos);
    void onTabPathChanged(const QString &path);
    void onItemDoubleClicked(const QModelIndex &index);
    void onMiddleClickOnFolder(const QString &path);
    void onQuickAccessFolderClicked(const QString &path);
    void onPathEdited();
    void onFileOperationCompleted();
    void delayedRefresh();
    void onSearchRequested(const QString &text);
    void onSearchClosed();
    void onSearchResultSelected(const QString &path);
    void onNavigateToContainingFolder(const QString &folderPath);
    void undoDelete();
    void setupUndoShortcut();
    void onFavoriteMiddleClicked(const QString &path);
    void onNavigateToFile(const QString &filePath);
    void onSortChanged(int column, Qt::SortOrder order);

    // Notification slots
    void onNotificationRequested(const QString &title, const QString &message, int type, bool autoClose, int timeout);
    void onProgressNotificationRequested(const QString &title, const QString &message, int progress, bool autoClose);

private:
    void setupUI();
    void setupConnections();
    void setupShortcuts();
    void setupSearchWidget();
    void setupNotificationManager();
    void updateCurrentTabInfo();
    void updateNavigationButtons();
    void updateViewModeForCurrentPath();
    void updateSortControls();
    void saveViewModeForPath(const QString &path, int mode);
    void applyViewMode(int mode);
    void updateQuickAccessVisibility();
    void updatePathHistory(const QString &path);
    bool isDrivePath(const QString &path);

    FileSystemTab* getTab(int index) const;
    QStringList getSelectedFiles() const;

    QPushButton *addTabButton;
    void setupAddTabButton();
    void updateAddTabButtonPosition();
    bool eventFilter(QObject *watched, QEvent *event) override;

    QTabWidget *tabWidget;
    QFileSystemModel *model;
    QLineEdit *pathEdit;
    QToolBar *buttonToolBar;
    QToolBar *pathToolBar;
    ContextMenu *contextMenu;
    FileOperations *fileOperations;
    SearchWidget *searchWidget;
    FolderViewSettings *folderViewSettings;
    AdminLauncher *adminLauncher;
    Archiver *m_archiver;
    NotificationManager *m_notificationManager;

    QAction *backAction;
    QAction *forwardAction;
    QAction *upAction;
    QAction *homeAction;
    QAction *refreshAction;
    QAction *viewModeAction;
    QAction *newFolderAction;
    QAction *renameAction;
    QAction *newTabAction;
    QAction *closeTabAction;
    QAction *showHiddenAction;

    // Sorting controls
    QComboBox *sortTypeComboBox;
    QToolButton *sortOrderButton;

    QMap<QString, int> folderViewModes;
    bool showHiddenFiles = false;
    int currentViewMode = 0;

    FavoritesAction *favoritesAction;
};