#include "archiver.h"
#include <QDebug>
#include <QTimer>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include "colors.h"

ArchiverWorker::ArchiverWorker(QObject *parent)
    : QObject(parent)
    , m_isRunning(false)
{
}

void ArchiverWorker::createArchive(const QString &sourcePath, const QString &archivePath, const QString &sevenZipPath)
{
    QMutexLocker locker(&m_mutex);
    if (m_isRunning) {
        emit errorOccurred("Операция уже выполняется");
        return;
    }

    m_isRunning = true;
    m_sevenZipPath = !sevenZipPath.isEmpty() ? sevenZipPath : get7zPath();

    if (m_sevenZipPath.isEmpty()) {
        emit errorOccurred("7-Zip не найден. Пожалуйста, укажите путь к 7z.exe в настройках.");
        m_isRunning = false;
        return;
    }

    emit operationStarted("Создание архива: " + QFileInfo(sourcePath).fileName());

    QProcess process;
    QStringList arguments;
    arguments << "a" << "-tzip" << archivePath << sourcePath;

    qDebug() << "Creating archive with 7z:" << m_sevenZipPath << arguments;

    process.start(m_sevenZipPath, arguments);

    if (!process.waitForStarted(5000)) {
        emit operationFinished("Создание архива", false, "Не удалось запустить процесс архивации");
        m_isRunning = false;
        return;
    }

    // Отправляем начальный прогресс
    emit progressChanged(QFileInfo(sourcePath).fileName(), 0);

    QByteArray output;
    while (process.waitForReadyRead(100)) {
        output += process.readAll();
        // Простая симуляция прогресса - в реальности можно парсить вывод 7z
        static int progress = 0;
        progress = qMin(progress + 10, 90);
        emit progressChanged(QFileInfo(sourcePath).fileName(), progress);
        QThread::msleep(100);
    }

    if (!process.waitForFinished(30000)) {
        emit operationFinished("Создание архива", false, "Процесс архивации занял слишком много времени");
        m_isRunning = false;
        return;
    }

    bool success = (process.exitCode() == 0);
    QString message = success ? "Архив успешно создан" : process.readAllStandardError();

    emit progressChanged(QFileInfo(sourcePath).fileName(), 100);
    emit operationFinished("Создание архива", success, message);

    m_isRunning = false;
}

void ArchiverWorker::extractArchive(const QString &archivePath, const QString &extractPath, const QString &sevenZipPath)
{
    QMutexLocker locker(&m_mutex);
    if (m_isRunning) {
        emit errorOccurred("Операция уже выполняется");
        return;
    }

    m_isRunning = true;
    m_sevenZipPath = !sevenZipPath.isEmpty() ? sevenZipPath : get7zPath();

    if (m_sevenZipPath.isEmpty()) {
        emit errorOccurred("7-Zip не найден. Пожалуйста, укажите путь к 7z.exe в настройках.");
        m_isRunning = false;
        return;
    }

    emit operationStarted("Извлечение архива: " + QFileInfo(archivePath).fileName());

    QProcess process;
    QStringList arguments;
    arguments << "x" << archivePath << "-o" + extractPath << "-y";

    qDebug() << "Extracting archive with 7z:" << m_sevenZipPath << arguments;

    process.start(m_sevenZipPath, arguments);

    if (!process.waitForStarted(5000)) {
        emit operationFinished("Извлечение архива", false, "Не удалось запустить процесс извлечения");
        m_isRunning = false;
        return;
    }

    emit progressChanged(QFileInfo(archivePath).fileName(), 0);

    QByteArray output;
    while (process.waitForReadyRead(100)) {
        output += process.readAll();
        static int progress = 0;
        progress = qMin(progress + 10, 90);
        emit progressChanged(QFileInfo(archivePath).fileName(), progress);
        QThread::msleep(100);
    }

    if (!process.waitForFinished(30000)) {
        emit operationFinished("Извлечение архива", false, "Процесс извлечения занял слишком много времени");
        m_isRunning = false;
        return;
    }

    bool success = (process.exitCode() == 0);
    QString message = success ? "Архив успешно извлечен" : process.readAllStandardError();

    emit progressChanged(QFileInfo(archivePath).fileName(), 100);
    emit operationFinished("Извлечение архива", success, message);

    m_isRunning = false;
}

QString ArchiverWorker::get7zPath()
{
    QSettings settings;
    QString savedPath = settings.value("7z_path").toString();

    if (!savedPath.isEmpty() && QFile::exists(savedPath)) {
        return savedPath;
    }

    QStringList possiblePaths = {
        "C:\\Program Files\\7-Zip\\7z.exe",
        "C:\\Program Files (x86)\\7-Zip\\7z.exe",
        "R:\\Prog\\7-Zip\\7z.exe",
    };

    for (const QString &path : possiblePaths) {
        if (QFile::exists(path)) {
            settings.setValue("7z_path", path);
            return path;
        }
    }

    QProcess process;
    process.start("where", QStringList() << "7z");
    if (process.waitForFinished(1000) && process.exitCode() == 0) {
        QString path = QString::fromLocal8Bit(process.readAllStandardOutput()).trimmed();
        if (!path.isEmpty() && QFile::exists(path)) {
            settings.setValue("7z_path", path);
            return path;
        }
    }

    return QString();
}

Archiver::Archiver(QObject *parent)
    : QObject(parent)
    , m_isRunning(false)
{
    m_worker = new ArchiverWorker();
    m_workerThread = new QThread();

    m_worker->moveToThread(m_workerThread);

    connect(m_worker, &ArchiverWorker::progressChanged,
            this, &Archiver::onWorkerProgressChanged);
    connect(m_worker, &ArchiverWorker::operationStarted,
            this, &Archiver::onWorkerOperationStarted);
    connect(m_worker, &ArchiverWorker::operationFinished,
            this, &Archiver::onWorkerOperationFinished);
    connect(m_worker, &ArchiverWorker::errorOccurred,
            this, &Archiver::onWorkerErrorOccurred);

    m_workerThread->start();
}

Archiver::~Archiver()
{
    if (m_workerThread->isRunning()) {
        m_workerThread->quit();
        m_workerThread->wait();
    }
    delete m_worker;
    delete m_workerThread;
}

void Archiver::createArchive(const QString &sourcePath, const QString &archivePath)
{
    QMetaObject::invokeMethod(m_worker, "createArchive",
                              Q_ARG(QString, sourcePath),
                              Q_ARG(QString, archivePath),
                              Q_ARG(QString, QString()));
}

void Archiver::extractArchive(const QString &archivePath, const QString &extractPath)
{
    QMetaObject::invokeMethod(m_worker, "extractArchive",
                              Q_ARG(QString, archivePath),
                              Q_ARG(QString, extractPath),
                              Q_ARG(QString, QString()));
}

bool Archiver::isRunning() const
{
    return m_isRunning;
}

QString Archiver::currentOperation() const
{
    return m_currentOperation;
}

void Archiver::onWorkerProgressChanged(const QString &fileName, int percent)
{
    emit progressChanged(fileName, percent);
}

void Archiver::onWorkerOperationStarted(const QString &operation)
{
    m_isRunning = true;
    m_currentOperation = operation;
    emit operationStarted(operation);
}

void Archiver::onWorkerOperationFinished(const QString &operation, bool success, const QString &message)
{
    m_isRunning = false;
    emit operationFinished(operation, success, message);
}

void Archiver::onWorkerErrorOccurred(const QString &error)
{
    emit errorOccurred(error);
}