#pragma once

#include <QShortcut>
#include <QObject>
#include <QKeySequence>
#include <QShortcut>
#include <QMap>

class MainWindow;

class ShortcutManager : public QObject
{
    Q_OBJECT

public:
    explicit ShortcutManager(MainWindow *mainWindow, QObject *parent = nullptr);
    void setupShortcuts();

private slots:
    void newTab();
    void closeCurrentTab();
    void renameSelectedItem();
    void createNewFolder();
    void deleteSelectedItems();
    void copySelectedItems();
    void cutSelectedItems();
    void pasteFiles();
    void navigateBack();
    void navigateForward();
    void toggleHiddenFiles();
    void showSearchWidget();
    void undoDelete();

private:
    MainWindow *mainWindow;
    QMap<QString, QShortcut*> shortcuts;
};