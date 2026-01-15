#pragma once

#include <QObject>
#include <QThread>
#include <QStringList>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QFile>
#include <QTemporaryFile>
#include <QTextStream>

#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#endif

class PasteWorker : public QObject
{
    Q_OBJECT

public:
    explicit PasteWorker(QObject *parent = nullptr);

    enum OperationType {
        Copy,
        Move
    };

    struct PasteOperation {
        QString sourcePath;
        QString destinationPath;
        OperationType type;
        bool skip = false;
        bool replace = false;
        bool rename = false;
    };

#ifdef Q_OS_WIN
    static bool tryAdminOperations(const QList<PasteOperation> &operations, QStringList &errors);
#endif

public slots:
    void startOperations(const QList<PasteOperation> &operations);

    signals:
        void progressChanged(int value, const QString &currentFile);
    void operationCompleted(bool success, const QString &message);
    void errorOccurred(const QString &error);
    void adminRightsRequired(const QList<PasteOperation> &operations, const QStringList &errorFiles);

private:
    bool copyRecursive(const QString &src, const QString &dest);
    bool moveRecursive(const QString &src, const QString &dest);
    QString generateUniqueName(const QString &dir, const QString &fileName);
    bool hasWriteAccess(const QString &path);
};