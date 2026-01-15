#pragma once

#include <QObject>
#include <QProcess>
#include <QThread>
#include <QMutex>
#include <QSettings>
#include <QFileInfo>

class ArchiverWorker : public QObject
{
    Q_OBJECT

public:
    explicit ArchiverWorker(QObject *parent = nullptr);

public slots:
    void createArchive(const QString &sourcePath, const QString &archivePath, const QString &sevenZipPath);
    void extractArchive(const QString &archivePath, const QString &extractPath, const QString &sevenZipPath);

    signals:
        void progressChanged(const QString &fileName, int percent);
    void operationStarted(const QString &operation);
    void operationFinished(const QString &operation, bool success, const QString &message);
    void errorOccurred(const QString &error);

private:
    QString m_sevenZipPath;
    QMutex m_mutex;
    bool m_isRunning;

    QString get7zPath();
};

class Archiver : public QObject
{
    Q_OBJECT

public:
    explicit Archiver(QObject *parent = nullptr);
    ~Archiver();

    void createArchive(const QString &sourcePath, const QString &archivePath);
    void extractArchive(const QString &archivePath, const QString &extractPath);

    bool isRunning() const;
    QString currentOperation() const;

    signals:
        void progressChanged(const QString &fileName, int percent);
    void operationStarted(const QString &operation);
    void operationFinished(const QString &operation, bool success, const QString &message);
    void errorOccurred(const QString &error);

private slots:
    void onWorkerProgressChanged(const QString &fileName, int percent);
    void onWorkerOperationStarted(const QString &operation);
    void onWorkerOperationFinished(const QString &operation, bool success, const QString &message);
    void onWorkerErrorOccurred(const QString &error);

private:
    ArchiverWorker *m_worker;
    QThread *m_workerThread;
    QString m_currentOperation;
    bool m_isRunning;
};