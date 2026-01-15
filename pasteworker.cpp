#include "pasteworker.h"
#include <QMessageBox>
#include <QApplication>

#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#include <sddl.h>
#include <winnt.h>
#include <accctrl.h>
#include <aclapi.h>
#endif

PasteWorker::PasteWorker(QObject *parent)
    : QObject(parent)
{
}

void PasteWorker::startOperations(const QList<PasteOperation> &operations)
{
    if (operations.isEmpty()) {
        emit operationCompleted(false, "Нет операций для выполнения");
        return;
    }

    int total = operations.size();
    int successCount = 0;
    int failCount = 0;
    QStringList errors;
    QList<PasteOperation> operationsToRetry;
    QStringList failedFiles;

    for (int i = 0; i < total; ++i) {
        const PasteOperation &op = operations[i];

        if (op.skip) {
            emit progressChanged((i * 100) / total, QString("Пропуск: %1").arg(QFileInfo(op.sourcePath).fileName()));
            continue;
        }

        QString currentFile = QFileInfo(op.sourcePath).fileName();
        emit progressChanged((i * 100) / total,
                           QString("%1: %2").arg(op.type == Copy ? "Копирование" : "Перемещение").arg(currentFile));

        bool success = false;
        QString errorReason;

        // Выполняем операцию
        if (op.type == Copy) {
            qDebug() << "Copy operation:" << op.sourcePath << "->" << op.destinationPath;
            success = copyRecursive(op.sourcePath, op.destinationPath);
            if (!success) {
                errorReason = "копирование не удалось";
                qDebug() << "Copy failed for:" << op.sourcePath << "->" << op.destinationPath;
            }
        } else { // Move
            qDebug() << "Move operation:" << op.sourcePath << "->" << op.destinationPath;
            success = moveRecursive(op.sourcePath, op.destinationPath);
            if (!success) {
                errorReason = "перемещение не удалось";
                qDebug() << "Move failed for:" << op.sourcePath << "->" << op.destinationPath;
            }
        }

        if (success) {
            successCount++;
        } else {
            failCount++;
            errors.append(QString("%1: %2").arg(currentFile).arg(errorReason));
            operationsToRetry.append(op);
            failedFiles.append(currentFile);
        }
    }

    // Если есть неудачные операции, пытаемся выполнить их с правами администратора
    if (!operationsToRetry.isEmpty()) {
#ifdef Q_OS_WIN
        // Отправляем сигнал для запроса прав администратора
        emit adminRightsRequired(operationsToRetry, failedFiles);
#else
        // На других ОС просто завершаем с ошибками
        QString errorMessage = QString("Успешно выполнено: %1, Не удалось: %2\n%3")
                               .arg(successCount)
                               .arg(failCount)
                               .arg(errors.join("\n"));
        emit operationCompleted(false, errorMessage);
#endif
        return;
    }

    emit progressChanged(100, "Завершено");

    if (failCount > 0) {
        QString errorMessage = QString("Успешно выполнено: %1, Не удалось: %2\n%3")
                               .arg(successCount)
                               .arg(failCount)
                               .arg(errors.join("\n"));
        emit operationCompleted(false, errorMessage);
    } else {
        emit operationCompleted(true, QString("Успешно выполнено: %1 операций").arg(successCount));
    }
}

bool PasteWorker::copyRecursive(const QString &src, const QString &dest)
{
    qDebug() << "copyRecursive:" << src << "->" << dest;

    QFileInfo srcInfo(src);
    if (!srcInfo.exists()) {
        qDebug() << "Source does not exist:" << src;
        return false;
    }

    if (srcInfo.isDir()) {
        qDebug() << "Source is directory, copying recursively";
        QDir destDir(dest);
        if (!destDir.mkpath(".")) {
            qDebug() << "Failed to create destination directory:" << dest;
            return false;
        }

        QDir srcDir(src);
        QStringList files = srcDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
        qDebug() << "Directory contains" << files.size() << "items";

        for (const QString &file : files) {
            QString srcFilePath = src + "/" + file;
            QString destFilePath = dest + "/" + file;
            if (!copyRecursive(srcFilePath, destFilePath)) {
                qDebug() << "Failed to copy sub-item:" << file;
                return false;
            }
        }
        return true;
    } else {
        qDebug() << "Source is file, copying directly";

        // Если целевой файл существует, пытаемся удалить его сначала
        if (QFile::exists(dest)) {
            qDebug() << "Destination file exists, attempting to remove:" << dest;
            if (!QFile::remove(dest)) {
                qDebug() << "Failed to remove existing destination file:" << dest;
                return false;
            }
        }

        bool result = QFile::copy(src, dest);
        qDebug() << "File copy result:" << result;
        return result;
    }
}

bool PasteWorker::moveRecursive(const QString &src, const QString &dest)
{
    qDebug() << "moveRecursive:" << src << "->" << dest;

    // Сначала копируем
    if (copyRecursive(src, dest)) {
        qDebug() << "Copy successful, now removing source";

        // Затем удаляем исходник
        QFileInfo srcInfo(src);
        if (srcInfo.isDir()) {
            QDir srcDir(src);
            bool removeResult = srcDir.removeRecursively();
            qDebug() << "Directory remove result:" << removeResult;
            return removeResult;
        } else {
            bool removeResult = QFile::remove(src);
            qDebug() << "File remove result:" << removeResult;
            return removeResult;
        }
    }

    qDebug() << "Copy failed, move operation aborted";
    return false;
}

QString PasteWorker::generateUniqueName(const QString &dir, const QString &fileName)
{
    QFileInfo fileInfo(fileName);
    QString baseName = fileInfo.completeBaseName();
    QString extension = fileInfo.suffix();
    QString newName = fileName;

    int counter = 1;
    while (QFile::exists(dir + "/" + newName)) {
        if (extension.isEmpty()) {
            newName = QString("%1 (%2)").arg(baseName).arg(counter);
        } else {
            newName = QString("%1 (%2).%3").arg(baseName).arg(counter).arg(extension);
        }
        counter++;
    }

    return dir + "/" + newName;
}


bool PasteWorker::hasWriteAccess(const QString &path)
{
    QFileInfo info(path);
    QString checkPath;

    if (info.exists() && info.isFile()) {
        checkPath = path;
    } else {
        checkPath = info.absolutePath();
    }

    QString testFilePath = checkPath + "/qfiles_test_write_access.tmp";

    QFile testFile(testFilePath);
    if (testFile.open(QIODevice::WriteOnly)) {
        testFile.write("test");
        testFile.close();
        testFile.remove();
        qDebug() << "Write access GRANTED for:" << path;
        return true;
    } else {
        qDebug() << "Write access DENIED for:" << path << "Error:" << testFile.errorString();
        return false;
    }
}

#ifdef Q_OS_WIN
bool PasteWorker::tryAdminOperations(const QList<PasteOperation> &operations, QStringList &errors)
{
    if (operations.isEmpty()) {
        return true;
    }

    qDebug() << "Attempting admin operations for" << operations.size() << "files";

    // Создаем временный пакетный файл для всех операций
    QTemporaryFile batchFile;
    if (!batchFile.open()) {
        qDebug() << "Failed to create temporary batch file";
        errors.append("Не удалось создать временный файл для операций");
        return false;
    }

    QTextStream stream(&batchFile);
    // В Qt6 setCodec заменен на setEncoding
    stream.setEncoding(QStringConverter::Utf8);

    // Добавляем команды для каждой операции
    for (const PasteOperation &op : operations) {
        QFileInfo sourceInfo(op.sourcePath);

        if (op.type == Copy) {
            if (sourceInfo.isDir()) {
                stream << "xcopy \"" << QDir::toNativeSeparators(op.sourcePath) << "\" \""
                       << QDir::toNativeSeparators(op.destinationPath) << "\" /E /I /H /Y\n";
            } else {
                stream << "copy \"" << QDir::toNativeSeparators(op.sourcePath) << "\" \""
                       << QDir::toNativeSeparators(op.destinationPath) << "\" /Y\n";
            }
        } else { // Move
            if (sourceInfo.isDir()) {
                // Для папок: копируем, затем удаляем
                stream << "xcopy \"" << QDir::toNativeSeparators(op.sourcePath) << "\" \""
                       << QDir::toNativeSeparators(op.destinationPath) << "\" /E /I /H /Y\n";
                stream << "rmdir /S /Q \"" << QDir::toNativeSeparators(op.sourcePath) << "\"\n";
            } else {
                stream << "move \"" << QDir::toNativeSeparators(op.sourcePath) << "\" \""
                       << QDir::toNativeSeparators(op.destinationPath) << "\" /Y\n";
            }
        }
    }

    batchFile.close();
    qDebug() << "Batch file created:" << batchFile.fileName();

    // Запускаем пакетный файл с правами администратора
    SHELLEXECUTEINFO shExecInfo;
    ZeroMemory(&shExecInfo, sizeof(shExecInfo));
    shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
    shExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
    shExecInfo.lpVerb = L"runas";
    shExecInfo.lpFile = L"cmd.exe";

    QString params = QString("/c \"%1\"").arg(batchFile.fileName());
    std::wstring wideParams = params.toStdWString();
    shExecInfo.lpParameters = wideParams.c_str();
    shExecInfo.nShow = SW_HIDE;

    if (ShellExecuteEx(&shExecInfo)) {
        WaitForSingleObject(shExecInfo.hProcess, INFINITE);

        DWORD exitCode;
        if (GetExitCodeProcess(shExecInfo.hProcess, &exitCode)) {
            CloseHandle(shExecInfo.hProcess);
            qDebug() << "Admin batch process exit code:" << exitCode;
            return (exitCode == 0);
        }
        CloseHandle(shExecInfo.hProcess);
    } else {
        qDebug() << "ShellExecuteEx failed, error:" << GetLastError();
    }

    return false;
}
#endif