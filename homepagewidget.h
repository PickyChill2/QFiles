#pragma once

#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QStorageInfo>
#include <QStandardPaths>
#include <QMenu> // Добавлено
#include <QContextMenuEvent> // Добавлено
#include "networkdrivewidget.h"

class HomePageWidget : public QWidget
{
    Q_OBJECT

public:
    explicit HomePageWidget(QWidget *parent = nullptr);

    signals:
        void folderClicked(const QString &path);
    void middleClickOnFolder(const QString &path); // Добавлено

private slots:
    void onFolderClicked();
    void onNetworkDriveConnected(const QString &path);
    void onRecycleBinContextMenuRequested(const QPoint &pos); // Добавлено
    void openRecycleBin(); // Добавлено
    void emptyRecycleBin(); // Добавлено

private:
    void setupUI();
    void populateQuickFolders();
    QString getRecycleBinPath(); // Добавлено
    bool eventFilter(QObject *obj, QEvent *event) override; // Добавлено

    QVBoxLayout *mainLayout;
    QWidget *quickFoldersWidget;
    QLabel *quickFoldersLabel;
    QHBoxLayout *quickFoldersContainer;
    NetworkDriveWidget *networkDriveWidget;
    QPushButton *recycleBinButton; // Добавлено
    QMenu *recycleBinContextMenu; // Добавлено
};