#pragma once

#include <QMenu>
#include <QFileSystemModel>
#include <QFileInfo>
#include <QClipboard>
#include <QApplication>
#include <QMessageBox>
#include <QInputDialog>
#include <QProcess>
#include <QTimer>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include "fileoperations.h"

// Forward declaration
class FileSystemTab;

class ContextMenu : public QMenu
{
    Q_OBJECT

public:
    explicit ContextMenu(QWidget *parent = nullptr);
    void setSelection(const QModelIndexList &indexes, const QString &currentPath);
    void setParentTab(FileSystemTab *parentTab);
    void setFileOperations(FileOperations *fileOps);

signals:
    void refreshRequested();
    void navigateRequested(const QString &path);
    void fileOperationCompleted();

private slots:
    void open();
    void openWith();
    void cut();
    void copy();
    void paste();
    void rename();
    void deleteFile();
    void newFolder();
    void properties();
    void showInExplorer();
    void toggleHiddenFiles();
    void createTextFile();
    void createDocFile();
    void editScriptFile();
    void runPowerShell();
    void runPowerShellAsAdmin();
    void createNewFile(const QString &extension);
    void extractHere();
    void setupStyle();
    QString getDialogStyle() const;
    void styleDialog(QDialog *dialog) const;
    QMessageBox* createStyledMessageBox(QWidget *parent, const QString &title, const QString &text, QMessageBox::Icon icon) const;
    void createShortcut();
    void createArchive();

private:
    void setupActions();
    bool isSingleFileSelected() const;
    bool isSingleFolderSelected() const;
    bool isArchiveFile(const QString& filePath) const;
    bool isScriptFile(const QString& filePath) const;
    void show7zPathDialog();
    QStringList getSelectedFilePaths() const;

    // Методы для работы с ассоциациями программ
    void showOpenWithMenu(const QString &filePath);
    void launchWithProgram(const QString &filePath, const QString &programPath);
    void chooseProgramForExtension(const QString &extension);
    void showAssociationsDialog();
    void suggestProgramForSimilarExtensions(const QString &extension, const QString &programPath);
    QStringList getSimilarExtensions(const QString &extension);
    QStringList getProgramsForExtension(const QString &extension);
    void addProgramForExtension(const QString &extension, const QString &programPath);
    void removeProgramForExtension(const QString &extension, const QString &programPath);
    void saveAssociations();
    void loadAssociations();
    QString getJsonFilePath() const;
    QIcon getProgramIcon(const QString &programPath) const;

    QModelIndexList selectedIndexes;
    QString currentDirectory;
    QAction *openAction;
    QAction *openWithAction;
    QAction *cutAction;
    QAction *copyAction;
    QAction *pasteAction;
    QAction *renameAction;
    QAction *deleteAction;
    QAction *newFolderAction;
    QAction *propertiesAction;
    QAction *showInExplorerAction;
    QAction *showHiddenAction;
    QAction *undoDeleteAction;
    QMenu *createMenu;
    QAction *createAction;
    QAction *createTextFileAction;
    QAction *createDocFileAction;
    QAction *editScriptFileAction;
    QAction *runPowerShellAction;
    QAction *runPowerShellAsAdminAction;
    QAction *extractHereAction;
    QAction *createShortcutAction;
    QAction *createArchiveAction;
    QString get7zPath();

    // Хранение ассоциаций программ
    QJsonObject associations;

    FileOperations *fileOperations = nullptr;
    FileSystemTab *parentTab = nullptr;

};