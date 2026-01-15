#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QFile>
#include <QDir>
#include <QMessageBox>
#include <QClipboard>
#include <QApplication>
#include <QFileInfo>
#include <QStack>
#include <QProcess>
#include <QDateTime>
#include <QTimer>
#include <QThread>
#include "pasteworker.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#endif

struct DeletedFile {
    QString originalPath;
    QDateTime deletionTime;
};

class FileOperations : public QObject
{
    Q_OBJECT

public:
    explicit FileOperations(QObject *parent = nullptr);

    void deleteFiles(const QStringList &files);
    void copyFiles(const QStringList &files);
    void cutFiles(const QStringList &files);
    void pasteFiles(const QString &destinationDir);
    void renameFile(const QString &oldPath, const QString &newName);
    void undoDelete();

    bool hasFilesToPaste() const;
    bool canUndo() const;
    void clearOperation();

signals:
    void operationCompleted();
    void errorOccurred(const QString &message);
    void requestRefresh();
    void undoStateChanged();

    // Новые сигналы для уведомлений
    void notificationRequested(const QString &title, const QString &message,
                               int type, bool autoClose = true, int timeoutMs = 5000);
    void progressNotificationRequested(const QString &operationId, const QString &title,
                                       const QString &message, int progress = 0);
    void finishProgressNotification(const QString &operationId, const QString &title,
                                    const QString &message, int type);

private:
    bool copyRecursive(const QString &src, const QString &dest);
    bool moveRecursive(const QString &src, const QString &dest);
    QString generateUniqueName(const QString &dir, const QString &fileName);

    QString generateOperationId(const QString &operationName);

    QString getDialogStyle() const;
    void styleDialog(QDialog *dialog) const;
    QMessageBox* createStyledMessageBox(QWidget *parent, const QString &title, const QString &text, QMessageBox::Icon icon) const;

#ifdef Q_OS_WIN
    bool moveToRecycleBin(const QString &filePath);
    bool alternativeDelete(const QString &filePath);
    bool restoreTreeFromRecycleBin(const QString &originalPath, const QDateTime &deletionTime);
    bool hasWriteAccess(const QString &path);
    bool requestAdminPrivileges(const QString &operationDescription);
    bool tryAdminOperation(const QString &operation, const QString &source, const QString &destination);
#endif

    QStringList filesToPaste;
    bool isCutOperation = false;
    QStack<DeletedFile> deletedFilesStack;
};