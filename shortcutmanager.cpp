#include "shortcutmanager.h"
#include "mainwindow.h"

ShortcutManager::ShortcutManager(MainWindow *mainWindow, QObject *parent)
    : QObject(parent)
    , mainWindow(mainWindow)
{
}

void ShortcutManager::setupShortcuts()
{
    // Очищаем существующие шорткаты
    qDeleteAll(shortcuts);
    shortcuts.clear();

    // Создаем шорткаты с использованием setKey() вместо конструктора с параметрами
    shortcuts["NewTab"] = new QShortcut(mainWindow);
    shortcuts["NewTab"]->setKey(QKeySequence(Qt::CTRL | Qt::Key_T));

    shortcuts["CloseTab"] = new QShortcut(mainWindow);
    shortcuts["CloseTab"]->setKey(QKeySequence(Qt::CTRL | Qt::Key_W));

    shortcuts["Rename"] = new QShortcut(mainWindow);
    shortcuts["Rename"]->setKey(QKeySequence(Qt::Key_F2));

    shortcuts["NewFolder"] = new QShortcut(mainWindow);
    shortcuts["NewFolder"]->setKey(QKeySequence(Qt::CTRL | Qt::Key_N));

    shortcuts["Delete"] = new QShortcut(mainWindow);
    shortcuts["Delete"]->setKey(QKeySequence::Delete);

    shortcuts["Copy"] = new QShortcut(mainWindow);
    shortcuts["Copy"]->setKey(QKeySequence::Copy);

    shortcuts["Cut"] = new QShortcut(mainWindow);
    shortcuts["Cut"]->setKey(QKeySequence::Cut);

    shortcuts["Paste"] = new QShortcut(mainWindow);
    shortcuts["Paste"]->setKey(QKeySequence::Paste);

    shortcuts["Back"] = new QShortcut(mainWindow);
    shortcuts["Back"]->setKey(QKeySequence::Back);

    shortcuts["Forward"] = new QShortcut(mainWindow);
    shortcuts["Forward"]->setKey(QKeySequence::Forward);

    shortcuts["ToggleHidden"] = new QShortcut(mainWindow);
    shortcuts["ToggleHidden"]->setKey(QKeySequence(Qt::CTRL | Qt::Key_H));

    shortcuts["Search"] = new QShortcut(mainWindow);
    shortcuts["Search"]->setKey(QKeySequence(Qt::CTRL | Qt::Key_F));

    shortcuts["Undo"] = new QShortcut(mainWindow);
    shortcuts["Undo"]->setKey(QKeySequence(Qt::CTRL | Qt::Key_Z));

    shortcuts["AltUndo"] = new QShortcut(mainWindow);
    shortcuts["AltUndo"]->setKey(QKeySequence(Qt::ALT | Qt::Key_Backspace));

    // Подключаем сигналы
    connect(shortcuts["NewTab"], &QShortcut::activated, this, &ShortcutManager::newTab);
    connect(shortcuts["CloseTab"], &QShortcut::activated, this, &ShortcutManager::closeCurrentTab);
    connect(shortcuts["Rename"], &QShortcut::activated, this, &ShortcutManager::renameSelectedItem);
    connect(shortcuts["NewFolder"], &QShortcut::activated, this, &ShortcutManager::createNewFolder);
    connect(shortcuts["Delete"], &QShortcut::activated, this, &ShortcutManager::deleteSelectedItems);
    connect(shortcuts["Copy"], &QShortcut::activated, this, &ShortcutManager::copySelectedItems);
    connect(shortcuts["Cut"], &QShortcut::activated, this, &ShortcutManager::cutSelectedItems);
    connect(shortcuts["Paste"], &QShortcut::activated, this, &ShortcutManager::pasteFiles);
    connect(shortcuts["Back"], &QShortcut::activated, this, &ShortcutManager::navigateBack);
    connect(shortcuts["Forward"], &QShortcut::activated, this, &ShortcutManager::navigateForward);
    connect(shortcuts["ToggleHidden"], &QShortcut::activated, this, &ShortcutManager::toggleHiddenFiles);
    connect(shortcuts["Search"], &QShortcut::activated, this, &ShortcutManager::showSearchWidget);
    connect(shortcuts["Undo"], &QShortcut::activated, this, &ShortcutManager::undoDelete);
    connect(shortcuts["AltUndo"], &QShortcut::activated, this, &ShortcutManager::undoDelete);
}

// Реализации слотов, которые делегируют вызовы в MainWindow
void ShortcutManager::newTab() { mainWindow->newTab(); }
void ShortcutManager::closeCurrentTab() { mainWindow->closeCurrentTab(); }
void ShortcutManager::renameSelectedItem() { mainWindow->renameSelectedItem(); }
void ShortcutManager::createNewFolder() { mainWindow->createNewFolder(); }
void ShortcutManager::deleteSelectedItems() { mainWindow->deleteSelectedItems(); }
void ShortcutManager::copySelectedItems() { mainWindow->copySelectedItems(); }
void ShortcutManager::cutSelectedItems() { mainWindow->cutSelectedItems(); }
void ShortcutManager::pasteFiles() { mainWindow->pasteFiles(); }
void ShortcutManager::navigateBack() { mainWindow->navigateBack(); }
void ShortcutManager::navigateForward() { mainWindow->navigateForward(); }
void ShortcutManager::toggleHiddenFiles() { mainWindow->toggleHiddenFiles(); }
void ShortcutManager::showSearchWidget() { mainWindow->showSearchWidget(); }
void ShortcutManager::undoDelete() { mainWindow->undoDelete(); }