#include "fileoperations.h"
#include "strings.h"
#include "replacefiledialog.h"
#include "pasteworker.h"
#include "colors.h"
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QTimeZone>
#include <QProcess>
#include <QApplication>
#include <QInputDialog>

#ifdef Q_OS_WIN
#include <shellapi.h>
#include <windows.h>
#include <sddl.h>
#include <winnt.h>
#include <accctrl.h>
#include <aclapi.h>
#endif


FileOperations::FileOperations(QObject *parent)
    : QObject(parent)
{
}

QString FileOperations::generateOperationId(const QString &operationName)
{
    return QString("%1_%2").arg(operationName).arg(QDateTime::currentMSecsSinceEpoch());
}

// Функция для получения стиля диалоговых окон
QString FileOperations::getDialogStyle() const
{
    return
        "QDialog {"
        "    background: " + Colors::DarkPrimary + ";"
        "    border: 1px solid " + Colors::DarkTertiary + ";"
        "    border-radius: " + Colors::RadiusMedium + ";"
        "    color: " + Colors::TextLight + ";"
        "}"
        "QLabel {"
        "    color: " + Colors::TextLight + ";"
        "    background: transparent;"
        "}"
        "QPushButton {"
        "    background: " + Colors::DarkTertiary + ";"
        "    color: " + Colors::TextLight + ";"
        "    border: 1px solid " + Colors::TextDarkGray + ";"
        "    border-radius: " + Colors::RadiusSmall + ";"
        "    padding: 8px 16px;"
        "    font-weight: bold;"
        "    min-width: 80px;"
        "}"
        "QPushButton:hover {"
        "    background: " + Colors::DarkQuaternary + ";"
        "    border: 1px solid " + Colors::TextBlueLight + ";"
        "}"
        "QPushButton:pressed {"
        "    background: " + Colors::ButtonPressedStart + ";"
        "}"
        "QPushButton:disabled {"
        "    background: " + Colors::DarkTertiary + ";"
        "    color: " + Colors::TextMuted + ";"
        "}"
        "QMessageBox {"
        "    background: " + Colors::DarkPrimary + ";"
        "    border: 1px solid " + Colors::DarkTertiary + ";"
        "    border-radius: " + Colors::RadiusMedium + ";"
        "}"
        "QInputDialog {"
        "    background: " + Colors::DarkPrimary + ";"
        "    border: 1px solid " + Colors::DarkTertiary + ";"
        "    border-radius: " + Colors::RadiusMedium + ";"
        "}"
        "QLineEdit {"
        "    background: " + Colors::DarkTertiary + ";"
        "    border: 1px solid " + Colors::TextDarkGray + ";"
        "    border-radius: " + Colors::RadiusSmall + ";"
        "    padding: 8px;"
        "    color: " + Colors::TextLight + ";"
        "    selection-background-color: " + Colors::TextBlue + ";"
        "}";
}

// Функция для стилизации любого диалога
void FileOperations::styleDialog(QDialog *dialog) const
{
    if (dialog) {
        dialog->setStyleSheet(getDialogStyle());
    }
}

// Функция для создания стилизованного QMessageBox
QMessageBox* FileOperations::createStyledMessageBox(QWidget *parent, const QString &title, const QString &text, QMessageBox::Icon icon) const
{
    QMessageBox *msgBox = new QMessageBox(parent);
    msgBox->setWindowTitle(title);
    msgBox->setText(text);
    msgBox->setIcon(icon);
    styleDialog(msgBox);

    // Стилизуем кнопки
    QList<QPushButton*> buttons = msgBox->findChildren<QPushButton*>();
    for (QPushButton *button : buttons) {
        button->setStyleSheet(getDialogStyle());
    }

    return msgBox;
}

void FileOperations::deleteFiles(const QStringList &files)
{
    if (files.isEmpty()) return;

    // Генерируем ID операции
    QString operationId = generateOperationId("delete");

    // Показываем начальное уведомление
    emit progressNotificationRequested(operationId, "Удаление файлов",
                                      QString("Подготовка к удалению %1 файлов...").arg(files.size()),
                                      0);

    // Используем стилизованное сообщение с правильными кнопками
    QMessageBox msgBox;
    msgBox.setWindowTitle(Strings::DeleteTitle);
    msgBox.setText("Переместить выбранные объекты в корзину?");
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);

    // Применяем стиль
    styleDialog(&msgBox);

    // Стилизуем кнопки
    QList<QPushButton*> buttons = msgBox.findChildren<QPushButton*>();
    for (QPushButton *button : buttons) {
        button->setStyleSheet(getDialogStyle());
    }

    int result = msgBox.exec();

    if (result == QMessageBox::Yes) {
        qDebug() << "User confirmed deletion, moving files to recycle bin...";

        bool allSuccess = true;
        QStringList errors;

        int totalFiles = files.size();
        int processedFiles = 0;

        for (const QString &filePath : files) {
            processedFiles++;
            int progress = (processedFiles * 100) / totalFiles;

            // Обновляем уведомление
            QFileInfo fileInfo(filePath);
            emit progressNotificationRequested(operationId, "Удаление файлов",
                                             QString("Удаление: %1").arg(fileInfo.fileName()),
                                             progress);

            qDebug() << "Processing file for deletion:" << filePath;

            // Проверяем существование файла
            if (!QFile::exists(filePath)) {
                qDebug() << "File does not exist, skipping:" << filePath;
                errors.append(filePath + " (не существует)");
                allSuccess = false;
                continue;
            }

            #ifdef Q_OS_WIN
            // Используем Windows API для перемещения в корзину
            if (moveToRecycleBin(filePath)) {
                // Сохраняем информацию для возможной отмены
                DeletedFile deletedFile;
                deletedFile.originalPath = filePath;
                deletedFile.deletionTime = QDateTime::currentDateTimeUtc();
                deletedFilesStack.push(deletedFile);
                emit undoStateChanged();

                qDebug() << "Successfully moved to recycle bin:" << filePath;
            } else {
                allSuccess = false;
                errors.append(filePath);
                qDebug() << "Failed to move to recycle bin:" << filePath;

                // Попробуем альтернативный метод удаления
                qDebug() << "Trying alternative deletion method...";
                if (alternativeDelete(filePath)) {
                    qDebug() << "Alternative deletion successful for:" << filePath;
                    allSuccess = true; // Восстанавливаем флаг успеха для этого файла
                    errors.removeAll(filePath); // Убираем из ошибок
                } else {
                    qDebug() << "Alternative deletion also failed for:" << filePath;
                }
            }
            #else
            // На других ОС используем стандартное удаление
            QFileInfo info(filePath);
            if (info.isDir()) {
                QDir dir(filePath);
                if (!dir.removeRecursively()) {
                    allSuccess = false;
                    errors.append(filePath);
                }
            } else {
                if (!QFile::remove(filePath)) {
                    allSuccess = false;
                    errors.append(filePath);
                }
            }
            #endif
        }

        // Показываем финальное уведомление
        if (!allSuccess && !errors.isEmpty()) {
            QString errorMessage = "Не удалось переместить в корзину:\n" + errors.join("\n");
            qDebug() << errorMessage;

            emit finishProgressNotification(operationId, "Удаление файлов",
                                           QString("Не удалось удалить %1 из %2 файлов").arg(errors.size()).arg(totalFiles),
                                           2); // Type 2 = Error
        } else {
            qDebug() << "All files successfully processed";

            // Показываем финальное уведомление
            QString message = QString("Успешно удалено %1 файлов").arg(totalFiles - errors.size());
            if (!errors.isEmpty()) {
                message += QString(" (%1 не удалось удалить)").arg(errors.size());
                emit finishProgressNotification(operationId, "Удаление файлов", message,
                                               1); // Type 1 = Warning
            } else {
                emit finishProgressNotification(operationId, "Удаление файлов", message,
                                               0); // Type 0 = Info
            }

            emit operationCompleted();
            emit requestRefresh();
        }
    } else {
        qDebug() << "User canceled deletion";
        // Закрываем прогресс-уведомление
        emit finishProgressNotification(operationId, "Удаление файлов",
                                       "Операция удаления отменена пользователем",
                                       0); // Type 0 = Info
    }
}

#ifdef Q_OS_WIN
bool FileOperations::moveToRecycleBin(const QString &filePath)
{
    qDebug() << "Attempting to move to recycle bin:" << filePath;

    // Проверяем существование файла
    if (!QFile::exists(filePath)) {
        qDebug() << "File does not exist:" << filePath;
        return false;
    }

    // Конвертируем путь в формат Windows
    QString nativePath = QDir::toNativeSeparators(filePath);

    // Подготовка пути для Windows API - должен быть двойной null-terminated
    std::wstring widePath = nativePath.toStdWString();
    widePath += L'\0'; // Добавляем второй null terminator

    SHFILEOPSTRUCTW fileOp;
    ZeroMemory(&fileOp, sizeof(fileOp));

    fileOp.wFunc = FO_DELETE;
    fileOp.pFrom = widePath.c_str();
    fileOp.fFlags = FOF_ALLOWUNDO | FOF_NO_UI;

    qDebug() << "Calling SHFileOperationW with path:" << nativePath;

    int result = SHFileOperationW(&fileOp);
    bool success = (result == 0) && (!fileOp.fAnyOperationsAborted);

    qDebug() << "SHFileOperationW result:" << result
             << "Operations aborted:" << fileOp.fAnyOperationsAborted
             << "Success:" << success;

    if (result != 0) {
        qDebug() << "SHFileOperationW error code:" << result;
        switch (result) {
            case 2: qDebug() << "Error: File not found"; break;
            case 3: qDebug() << "Error: Path not found"; break;
            case 5: qDebug() << "Error: Access denied"; break;
            case 12: qDebug() << "Error: Invalid access"; break;
            case 15: qDebug() << "Error: Invalid drive"; break;
            case 16: qDebug() << "Error: Current directory cannot be removed"; break;
            case 32: qDebug() << "Error: Sharing violation"; break;
            case 33: qDebug() << "Error: Lock violation"; break;
            case 117: qDebug() << "Error: Invalid path"; break;
            default: qDebug() << "Error: Unknown error"; break;
        }
    }

    return success;
}

#endif

// Альтернативный метод удаления через IFileOperation (более современный)
bool FileOperations::alternativeDelete(const QString &filePath)
{
    qDebug() << "Trying alternative delete method for:" << filePath;

    // Проверяем существование файла
    if (!QFile::exists(filePath)) {
        qDebug() << "File does not exist in alternative method:" << filePath;
        return false;
    }

    // Пытаемся использовать обычное удаление как запасной вариант
    QFileInfo info(filePath);
    bool success = false;

    if (info.isDir()) {
        QDir dir(filePath);
        success = dir.removeRecursively();
        qDebug() << "Alternative directory deletion result:" << success;
    } else {
        success = QFile::remove(filePath);
        qDebug() << "Alternative file deletion result:" << success;
    }

    return success;
}

bool FileOperations::hasWriteAccess(const QString &path)
{
    QFileInfo info(path);
    QString checkPath;

    if (info.exists() && info.isFile()) {
        // Для существующего файла проверяем возможность записи
        checkPath = path;
        qDebug() << "Checking write access for file:" << checkPath;
    } else {
        // Для директории или несуществующего файла проверяем родительскую директорию
        checkPath = info.absolutePath();
        qDebug() << "Checking write access for directory:" << checkPath;
    }

    // Пытаемся создать временный файл для проверки прав
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

bool FileOperations::requestAdminPrivileges(const QString &operationDescription)
{
    QMessageBox msgBox;
    msgBox.setWindowTitle("Требуются права администратора");
    msgBox.setText(QString("Для операции \"%1\" требуются права администратора.\nХотите выполнить операцию с повышенными правами?")
                  .arg(operationDescription));
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);

    styleDialog(&msgBox);

    return msgBox.exec() == QMessageBox::Yes;
}

#ifdef Q_OS_WIN
bool FileOperations::tryAdminOperation(const QString &operation, const QString &source, const QString &destination)
{
    // Этот метод больше не используется для массовых операций
    // Оставлен для обратной совместимости
    qDebug() << "tryAdminOperation called for single operation (legacy)";

    QString command;
    QFileInfo sourceInfo(source);
    QFileInfo destInfo(destination);

    if (operation == "copy") {
        if (sourceInfo.isDir()) {
            command = QString("xcopy \"%1\" \"%2\" /E /I /H /Y").arg(source, destination);
        } else {
            command = QString("copy \"%1\" \"%2\" /Y").arg(source, destination);
        }
    } else if (operation == "move") {
        command = QString("move \"%1\" \"%2\" /Y").arg(source, destination);
    } else {
        qDebug() << "Unknown operation type:" << operation;
        return false;
    }

    qDebug() << "Admin command:" << command;

    // Запускаем процесс с повышенными правами
    SHELLEXECUTEINFO shExecInfo;
    ZeroMemory(&shExecInfo, sizeof(shExecInfo));
    shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
    shExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
    shExecInfo.lpVerb = L"runas";
    shExecInfo.lpFile = L"cmd.exe";

    QString params = QString("/c \"%1\"").arg(command);
    std::wstring wideParams = params.toStdWString();
    shExecInfo.lpParameters = wideParams.c_str();
    shExecInfo.nShow = SW_HIDE;

    if (ShellExecuteEx(&shExecInfo)) {
        WaitForSingleObject(shExecInfo.hProcess, INFINITE);

        DWORD exitCode;
        if (GetExitCodeProcess(shExecInfo.hProcess, &exitCode)) {
            CloseHandle(shExecInfo.hProcess);
            qDebug() << "Admin process exit code:" << exitCode;
            return (exitCode == 0);
        }
        CloseHandle(shExecInfo.hProcess);
    } else {
        qDebug() << "ShellExecuteEx failed, error:" << GetLastError();
    }

    return false;
}
#endif

#ifdef Q_OS_WIN
bool FileOperations::restoreTreeFromRecycleBin(const QString &origRootPath, const QDateTime &delTime)
{
    qDebug() << "=== Starting restore process ===";
    qDebug() << "Target:" << origRootPath;
    qDebug() << "Deletion time:" << delTime.toString("yyyy-MM-dd hh:mm:ss.zzz");

    // Получаем SID текущего пользователя
    HANDLE hToken = NULL;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        qDebug() << "Failed to open process token";
        return false;
    }

    DWORD dwSize = 0;
    GetTokenInformation(hToken, TokenUser, NULL, 0, &dwSize);
    if (dwSize == 0) {
        CloseHandle(hToken);
        return false;
    }

    PTOKEN_USER ptu = (PTOKEN_USER)HeapAlloc(GetProcessHeap(), 0, dwSize);
    if (!ptu) {
        CloseHandle(hToken);
        return false;
    }

    if (!GetTokenInformation(hToken, TokenUser, ptu, dwSize, &dwSize)) {
        HeapFree(GetProcessHeap(), 0, ptu);
        CloseHandle(hToken);
        return false;
    }

    LPWSTR sidString = NULL;
    if (!ConvertSidToStringSidW(ptu->User.Sid, &sidString)) {
        HeapFree(GetProcessHeap(), 0, ptu);
        CloseHandle(hToken);
        return false;
    }

    QString userSid = QString::fromWCharArray(sidString);
    LocalFree(sidString);
    HeapFree(GetProcessHeap(), 0, ptu);
    CloseHandle(hToken);

    // Определяем диск из оригинального пути
    QString drive = origRootPath.left(2); // e.g., "C:"
    QString recyclePath = drive + "/$RECYCLE.BIN/" + userSid + "/";

    // Проверяем существование директории корзины
    QDir recycleDir(recyclePath);
    if (!recycleDir.exists()) {
        qDebug() << "Recycle bin directory does not exist:" << recyclePath;

        // Попробуем альтернативный путь
        recyclePath = drive + "/Recycler/" + userSid + "/";
        recycleDir.setPath(recyclePath);
        if (!recycleDir.exists()) {
            qDebug() << "Alternative recycle bin path also does not exist:" << recyclePath;
            return false;
        }
    }

    qDebug() << "Using recycle bin path:" << recyclePath;
    qDebug() << "User SID:" << userSid;

    // Получаем список всех $I* файлов
    QStringList infFiles = recycleDir.entryList(QStringList() << "$I*", QDir::Files | QDir::Hidden);
    qDebug() << "Found" << infFiles.size() << "$I files in recycle bin";

    struct RestoreItem {
        QString infPath;
        QString dataPath;
        QString origPath;
        bool isFolder;
        int depth;
    };

    QList<RestoreItem> toRestore;
    QDateTime delUtc = delTime; // Already UTC

    // Нормализуем целевой путь для сравнения
    QString normalizedTargetPath = QDir::cleanPath(origRootPath);

    for (const QString &infName : infFiles) {
        QString infPath = recyclePath + infName;
        QFile inf(infPath);
        if (!inf.open(QIODevice::ReadOnly)) {
            qDebug() << "Failed to open $I file:" << infPath;
            continue;
        }

        QByteArray data = inf.readAll();
        inf.close();

        if (data.size() < 20) {
            qDebug() << "$I file too small:" << infPath << "size:" << data.size();
            continue;
        }

        const char* ptr = data.constData();
        qint64 offset = 0;

        // Skip possible BOM
        if (ptr[0] == (char)0xFF && ptr[1] == (char)0xFE) {
            ptr += 2;
            offset += 2;
        }

        qint64 versionVal = *(qint64*)ptr;
        int version = 0;

        if (versionVal == 0x0000000000000001LL) {
            version = 1;
            offset = 8;
        } else if (versionVal == 0x0000000000000002LL) {
            version = 2;
            offset = 8;
        } else {
            qDebug() << "Unknown $I file version:" << versionVal << "for file:" << infName;
            continue;
        }

        // Original size
        qint64 origSize = *(qint64*)(ptr + offset);
        Q_UNUSED(origSize);
        offset += 8;

        // FILETIME
        FILETIME ft = *(FILETIME*)(ptr + offset);
        offset += 8;

        // Convert FILETIME to QDateTime (UTC) - ДОБАВЛЯЕМ ЭТОТ БЛОК КОДА
        ULARGE_INTEGER uli;
        uli.HighPart = ft.dwHighDateTime;
        uli.LowPart = ft.dwLowDateTime;
        qint64 ticks = uli.QuadPart;
        qint64 secs = (ticks / 10000000) - 11644473600LL;
        QDateTime infTime = QDateTime::fromSecsSinceEpoch(secs, QTimeZone::UTC);

        qint32 pathLen = 0;
        if (version == 2) {
            pathLen = *(qint32*)(ptr + offset);
            offset += 4;
        }

        // Original path
        const wchar_t* pathPtr = (wchar_t*)(ptr + offset);
        QString infOrigPath;
        if (version == 2) {
            infOrigPath = QString::fromWCharArray(pathPtr, pathLen - 1); // Exclude null terminator
        } else {
            infOrigPath = QString::fromWCharArray(pathPtr);
        }

        // Нормализуем путь из корзины для сравнения
        QString normalizedInfPath = QDir::cleanPath(infOrigPath);

        // Check match - используем более гибкое сравнение
        bool pathMatch = (normalizedInfPath.compare(normalizedTargetPath, Qt::CaseInsensitive) == 0) ||
                         normalizedInfPath.startsWith(normalizedTargetPath + "/", Qt::CaseInsensitive) ||
                         normalizedInfPath.startsWith(normalizedTargetPath + "\\", Qt::CaseInsensitive);

        // Увеличиваем временной допуск до 30 секунд
        bool timeMatch = qAbs(infTime.toMSecsSinceEpoch() - delUtc.toMSecsSinceEpoch()) < 30000;

        qDebug() << "Checking $I file:" << infName;
        qDebug() << "  Original path from recycle bin:" << infOrigPath << "(normalized:" << normalizedInfPath << ")";
        qDebug() << "  Our target path:" << origRootPath << "(normalized:" << normalizedTargetPath << ")";
        qDebug() << "  Path match:" << pathMatch;
        qDebug() << "  Time from recycle bin:" << infTime.toString("yyyy-MM-dd hh:mm:ss.zzz");
        qDebug() << "  Our deletion time:" << delUtc.toString("yyyy-MM-dd hh:mm:ss.zzz");
        qDebug() << "  Time diff:" << qAbs(infTime.toMSecsSinceEpoch() - delUtc.toMSecsSinceEpoch()) << "ms";
        qDebug() << "  Time match:" << timeMatch;

        if (pathMatch && timeMatch) {
            QString dataName = "$R" + infName.mid(2);
            QString dataPath = recyclePath + dataName;

            // Проверяем, существует ли соответствующий $R файл
            if (QFile::exists(dataPath)) {
                bool isFolder = QFileInfo(dataPath).isDir();
                int depth = normalizedInfPath.count('/') + normalizedInfPath.count('\\');

                RestoreItem item{infPath, dataPath, infOrigPath, isFolder, depth};
                toRestore.append(item);
                qDebug() << "  -> ADDED TO RESTORE LIST";
            } else {
                qDebug() << "  -> SKIPPED: $R file does not exist:" << dataPath;
            }
        } else {
            qDebug() << "  -> SKIPPED: No match (path:" << pathMatch << "time:" << timeMatch << ")";
        }
    }

    qDebug() << "Total items to restore:" << toRestore.size();

    if (toRestore.isEmpty()) {
        qDebug() << "No matching items found for restoration";
        return false;
    }

    // Sort: by depth ascending, then folders before files
    std::sort(toRestore.begin(), toRestore.end(), [](const RestoreItem &a, const RestoreItem &b) {
        if (a.depth != b.depth) return a.depth < b.depth;
        return a.isFolder > b.isFolder; // true (1) > false (0)
    });

    // Restore each item
    bool success = true;
    for (const RestoreItem &item : toRestore) {
        qDebug() << "Restoring:" << item.origPath << "(isFolder:" << item.isFolder << ")";

        // Создаем директорию назначения если нужно
        QFileInfo destInfo(item.origPath);
        QDir destDir(destInfo.absolutePath());
        if (!destDir.exists()) {
            qDebug() << "Creating destination directory:" << destInfo.absolutePath();
            if (!destDir.mkpath(".")) {
                qDebug() << "Failed to create destination directory:" << destInfo.absolutePath();
                success = false;
                continue;
            }
        }

        // Проверяем, не существует ли уже файл/папка
        if (QFile::exists(item.origPath)) {
            qDebug() << "Destination already exists, attempting to remove:" << item.origPath;
            if (item.isFolder) {
                QDir d(item.origPath);
                QStringList entries = d.entryList(QDir::NoDotAndDotDot | QDir::AllEntries);
                if (entries.isEmpty()) {
                    if (!d.rmdir(item.origPath)) {
                        qDebug() << "Failed to remove existing empty directory:" << item.origPath;
                        success = false;
                        continue;
                    }
                } else {
                    qDebug() << "Cannot remove existing directory - not empty:" << item.origPath;
                    success = false;
                    continue;
                }
            } else {
                if (!QFile::remove(item.origPath)) {
                    qDebug() << "Failed to remove existing file:" << item.origPath;
                    success = false;
                    continue;
                }
            }
        }

        // Перемещаем файл/папку обратно
        std::wstring wData = item.dataPath.toStdWString();
        std::wstring wOrig = item.origPath.toStdWString();

        qDebug() << "Moving from:" << item.dataPath << "to:" << item.origPath;

        if (!MoveFileW(wData.c_str(), wOrig.c_str())) {
            DWORD error = GetLastError();
            qDebug() << "MoveFileW failed with error:" << error << "for" << item.dataPath << "->" << item.origPath;
            success = false;
        } else {
            qDebug() << "Successfully restored:" << item.origPath;
            // Удаляем $I файл только если восстановление успешно
            if (!QFile::remove(item.infPath)) {
                qDebug() << "Failed to remove $I file:" << item.infPath;
            } else {
                qDebug() << "Removed $I file:" << item.infPath;
            }
        }
    }

    qDebug() << "Restore process completed. Overall success:" << success;
    return success;
}
#endif

void FileOperations::undoDelete()
{
    if (deletedFilesStack.isEmpty()) {
        QString operationId = generateOperationId("undo");
        emit finishProgressNotification(operationId, "Отмена удаления",
                                       "Нет операций для отмены",
                                       0); // Type 0 = Info
        return;
    }

    #ifdef Q_OS_WIN
    // Генерируем ID операции
    QString operationId = generateOperationId("undo");

    // Показываем начальное уведомление
    emit progressNotificationRequested(operationId, "Восстановление файлов",
                                      QString("Подготовка к восстановлению %1 файлов...").arg(deletedFilesStack.size()),
                                      0);

    // Сохраняем стек для отладки
    QStack<DeletedFile> toRestore = deletedFilesStack;
    deletedFilesStack.clear();
    emit undoStateChanged();

    qDebug() << "=== Starting UNDO operation ===";
    qDebug() << "Attempting to undo deletion of" << toRestore.size() << "items";

    bool allSuccess = true;
    QStringList errors;

    int totalItems = toRestore.size();
    int processedItems = 0;

    // Восстанавливаем в обратном порядке (последний удаленный первым)
    while (!toRestore.isEmpty()) {
        processedItems++;
        int progress = (processedItems * 100) / totalItems;

        DeletedFile df = toRestore.pop();

        // Обновляем уведомление
        QFileInfo fileInfo(df.originalPath);
        emit progressNotificationRequested(operationId, "Восстановление файлов",
                                          QString("Восстановление: %1").arg(fileInfo.fileName()),
                                          progress);

        qDebug() << "Processing undo for:" << df.originalPath;

        if (!restoreTreeFromRecycleBin(df.originalPath, df.deletionTime)) {
            allSuccess = false;
            errors.append(df.originalPath);
            qDebug() << "Failed to restore:" << df.originalPath;
        } else {
            qDebug() << "Successfully restored:" << df.originalPath;
        }
    }

    if (!allSuccess) {
        QString errorMsg = "Не удалось восстановить: " + errors.join(", ");
        qDebug() << errorMsg;

        emit finishProgressNotification(operationId, "Восстановление файлов", errorMsg,
                                       2); // Type 2 = Error
    } else {
        qDebug() << "All items successfully restored";

        // Показываем финальное уведомление
        QString message = QString("Успешно восстановлено %1 файлов").arg(totalItems - errors.size());
        if (!errors.isEmpty()) {
            message += QString(" (%1 не удалось восстановить)").arg(errors.size());
            emit finishProgressNotification(operationId, "Восстановление файлов", message,
                                           1); // Type 1 = Warning
        } else {
            emit finishProgressNotification(operationId, "Восстановление файлов", message,
                                           0); // Type 0 = Info
        }

        emit operationCompleted();
    }

    #else
    QString operationId = generateOperationId("undo");
    emit finishProgressNotification(operationId, "Ошибка",
                                   "Отмена удаления поддерживается только в Windows",
                                   2); // Type 2 = Error
    #endif
}

bool FileOperations::canUndo() const
{
    return !deletedFilesStack.isEmpty();
}

void FileOperations::copyFiles(const QStringList &files)
{
    filesToPaste = files;
    isCutOperation = false;
    qDebug() << "Files copied to clipboard:" << files;

    // Показываем уведомление
    emit notificationRequested("Копирование",
                               QString("Скопировано %1 файлов в буфер обмена").arg(files.size()),
                               0,  // Type 0 = Info
                               true, 3000);  // Автозакрытие через 3 секунды
}

void FileOperations::cutFiles(const QStringList &files)
{
    filesToPaste = files;
    isCutOperation = true;
    qDebug() << "Files cut to clipboard:" << files;

    // Показываем уведомление
    emit notificationRequested("Вырезание",
                               QString("Вырезано %1 файлов в буфер обмена").arg(files.size()),
                               0,  // Type 0 = Info
                               true, 3000);  // Автозакрытие через 3 секунды
}

void FileOperations::pasteFiles(const QString &destinationDir)
{
    if (filesToPaste.isEmpty()) {
        qDebug() << "No files to paste";
        QString operationId = generateOperationId("paste");
        emit finishProgressNotification(operationId, "Вставка",
                                       "Нет файлов для вставки",
                                       0);
        return;
    }

    // Генерируем ID операции
    QString operationId = generateOperationId(isCutOperation ? "move" : "copy");

    // Показываем начальное уведомление
    QString operationName = isCutOperation ? "Перемещение файлов" : "Копирование файлов";
    emit progressNotificationRequested(operationId, operationName,
                                      QString("Подготовка к %1 %2 файлов...").arg(isCutOperation ? "перемещению" : "копированию").arg(filesToPaste.size()),
                                      0);

    qDebug() << "=== STARTING PASTE OPERATION ===";
    qDebug() << "Pasting" << filesToPaste.size() << "files to:" << destinationDir;
    qDebug() << "Operation type:" << (isCutOperation ? "MOVE" : "COPY");

    // Шаг 1: Проверяем все конфликты
    QList<PasteWorker::PasteOperation> operations;
    ReplaceFileDialog::Result lastDecision = ReplaceFileDialog::Replace;
    bool applyToAll = false;
    bool hasConflict = false;

    for (const QString &srcPath : filesToPaste) {
        QFileInfo srcInfo(srcPath);
        QString destPath = destinationDir + "/" + srcInfo.fileName();

        PasteWorker::PasteOperation op;
        op.sourcePath = srcPath;
        op.destinationPath = destPath;
        op.type = isCutOperation ? PasteWorker::Move : PasteWorker::Copy;

        // Проверяем существование целевого файла
        if (QFile::exists(destPath) && !applyToAll) {
            qDebug() << "Target file exists, showing replace dialog";
            hasConflict = true;

            ReplaceFileDialog *dialog = new ReplaceFileDialog(srcInfo.fileName(), nullptr);
            styleDialog(dialog);

            if (dialog->exec() == QDialog::Accepted) {
                lastDecision = dialog->getResult();
                applyToAll = dialog->shouldApplyToAll();
                qDebug() << "User decision:" << lastDecision << "apply to all:" << applyToAll;

                switch (lastDecision) {
                case ReplaceFileDialog::Skip:
                    op.skip = true;
                    break;

                case ReplaceFileDialog::Copy:
                    op.destinationPath = generateUniqueName(destinationDir, srcInfo.fileName());
                    break;

                case ReplaceFileDialog::Replace:
                    op.replace = true;
                    break;

                case ReplaceFileDialog::Cancel:
                    // Отменяем всю операцию
                    operations.clear();
                    break;
                }
            } else {
                qDebug() << "User canceled replace dialog";
                operations.clear();
                dialog->deleteLater();
                break;
            }
            dialog->deleteLater();
        } else if (QFile::exists(destPath) && applyToAll) {
            // Применяем предыдущее решение ко всем
            if (lastDecision == ReplaceFileDialog::Copy) {
                op.destinationPath = generateUniqueName(destinationDir, srcInfo.fileName());
            } else if (lastDecision == ReplaceFileDialog::Skip) {
                op.skip = true;
            } else if (lastDecision == ReplaceFileDialog::Replace) {
                op.replace = true;
            }
        }

        // Если операция не была отменена, добавляем ее в список
        if (lastDecision != ReplaceFileDialog::Cancel) {
            operations.append(op);
        } else {
            break;
        }
    }

    if (operations.isEmpty() || (hasConflict && lastDecision == ReplaceFileDialog::Cancel)) {
        emit finishProgressNotification(operationId, operationName,
                                       "Операция отменена пользователем",
                                       0);
        return;
    }

    // Шаг 2: Выполняем операции в отдельном потоке
    QThread *thread = new QThread();
    PasteWorker *worker = new PasteWorker();
    worker->moveToThread(thread);

    // Подключаем сигналы и слоты
    connect(thread, &QThread::started, [worker, operations]() {
        worker->startOperations(operations);
    });

    connect(worker, &PasteWorker::progressChanged, this,
            [this, operationId, operationName](int value, const QString &currentFile) {
        emit progressNotificationRequested(operationId, operationName,
                                          currentFile, value);
    });

    connect(worker, &PasteWorker::adminRightsRequired, this,
            [this, operationId, operationName, thread, worker, destinationDir]
            (const QList<PasteWorker::PasteOperation> &operations, const QStringList &/*errorFiles*/) {

        // Запрашиваем права администратора один раз для всех файлов
        QString operationDescription = QString("%1 %2 файлов")
            .arg(operations.first().type == PasteWorker::Copy ? "Копирование" : "Перемещение")
            .arg(operations.size());

        // Показываем диалог запроса прав администратора
        QMessageBox msgBox;
        msgBox.setWindowTitle("Требуются права администратора");
        msgBox.setText(QString("Для операции %1 %2 файлов требуются права администратора.\n"
                              "Хотите выполнить операцию с повышенными правами?")
                      .arg(operations.first().type == PasteWorker::Copy ? "копирования" : "перемещения")
                      .arg(operations.size()));
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);

        styleDialog(&msgBox);

        if (msgBox.exec() == QMessageBox::Yes) {
            // Пользователь согласился - выполняем операции с правами администратора
            qDebug() << "User granted admin rights for" << operations.size() << "files";

            // Обновляем прогресс
            emit progressNotificationRequested(operationId, operationName,
                                             QString("Выполнение с правами администратора..."), 50);

            // Выполняем операции с правами администратора
            QStringList errors;
#ifdef Q_OS_WIN
            // Используем статический метод для выполнения операций с правами администратора
            bool adminSuccess = PasteWorker::tryAdminOperations(operations, errors);

            if (adminSuccess) {
                qDebug() << "Admin operations successful";

                emit progressNotificationRequested(operationId, operationName,
                                                 "Операции завершены успешно", 100);

                emit finishProgressNotification(operationId, operationName,
                                               QString("Успешно выполнено %1 операций с правами администратора")
                                               .arg(operations.size()), 0);

                emit operationCompleted();
                emit requestRefresh();

                // Очищаем операции cut после успешного выполнения
                if (isCutOperation) {
                    filesToPaste.clear();
                }
            } else {
                qDebug() << "Admin operations failed";
                QString errorMessage = QString("Не удалось выполнить %1 операций даже с правами администратора.\n%2")
                                       .arg(operations.size())
                                       .arg(errors.join("\n"));

                emit finishProgressNotification(operationId, operationName, errorMessage, 2);
            }
#endif
        } else {
            // Пользователь отказался
            qDebug() << "User denied admin rights";
            QString errorMessage = QString("Операция отменена. Не удалось выполнить %1 файлов без прав администратора.")
                                   .arg(operations.size());

            emit finishProgressNotification(operationId, operationName, errorMessage, 1);
        }

        // Очищаем память
        thread->quit();
        thread->wait();
        worker->deleteLater();
        thread->deleteLater();
    });

    connect(worker, &PasteWorker::operationCompleted, this,
            [this, operationId, operationName, thread, worker, operations](bool success, const QString &message) {

        if (success) {
            emit operationCompleted();
            emit requestRefresh();
            qDebug() << "Paste operation completed successfully";

            emit finishProgressNotification(operationId, operationName, message, 0);
        } else {
            qDebug() << "Paste operation completed with errors:" << message;
            emit finishProgressNotification(operationId, operationName, message, 2);
        }

        // Очищаем операции cut после успешного выполнения
        if (isCutOperation) {
            filesToPaste.clear();
        }

        // Очищаем память
        thread->quit();
        thread->wait();
        worker->deleteLater();
        thread->deleteLater();
    });

    connect(worker, &PasteWorker::errorOccurred, this,
            [this, operationId, operationName, thread, worker](const QString &error) {
        qDebug() << "Paste worker error:" << error;

        QString errorMessage = "Ошибка при выполнении операции: " + error;
        emit finishProgressNotification(operationId, operationName, errorMessage, 2);

        thread->quit();
        thread->wait();
        worker->deleteLater();
        thread->deleteLater();
    });

    // Запускаем поток
    thread->start();

    qDebug() << "=== PASTE OPERATION STARTED IN SEPARATE THREAD ===";
}

// Улучшенная функция copyRecursive с детальной отладкой
bool FileOperations::copyRecursive(const QString &src, const QString &dest)
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

// Улучшенная функция moveRecursive с детальной отладкой
bool FileOperations::moveRecursive(const QString &src, const QString &dest)
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

void FileOperations::renameFile(const QString &oldPath, const QString &newName)
{
    // Генерируем ID операции
    QString operationId = generateOperationId("rename");

    // Показываем начальное уведомление
    QFileInfo oldInfo(oldPath);
    emit progressNotificationRequested(operationId, "Переименование",
                                      QString("Переименование %1...").arg(oldInfo.fileName()),
                                      0);

    // Нормализуем пути
    QString normalizedOldPath = QDir::cleanPath(oldPath);
    QString newPath = oldInfo.absolutePath() + "/" + newName;
    QString normalizedNewPath = QDir::cleanPath(newPath);

    qDebug() << "Renaming:" << normalizedOldPath << "->" << normalizedNewPath;

    // Проверяем, не переименовываем ли в то же самое имя
    if (normalizedOldPath.compare(normalizedNewPath, Qt::CaseInsensitive) == 0) {
        qDebug() << "Same name, no rename needed";
        emit finishProgressNotification(operationId, "Переименование",
                                       QString("Файл '%1' уже имеет такое имя").arg(oldInfo.fileName()),
                                       0); // Type 0 = Info
        emit operationCompleted();
        return;
    }

    // Проверяем существование целевого файла
    if (QFile::exists(normalizedNewPath)) {
        QString errorMsg = Strings::RenameExistsError.arg(newName);
        qDebug() << errorMsg;

        emit finishProgressNotification(operationId, "Переименование", errorMsg,
                                       2); // Type 2 = Error
        return;
    }

    // Создаем родительскую директорию если нужно
    QDir parentDir(oldInfo.absolutePath());
    if (!parentDir.exists()) {
        if (!parentDir.mkpath(".")) {
            QString errorMsg = "Не удается создать родительскую директорию: " + parentDir.absolutePath();
            qDebug() << errorMsg;

            emit finishProgressNotification(operationId, "Переименование", errorMsg,
                                           2); // Type 2 = Error
            return;
        }
    }

    // Пытаемся переименовать
    bool success = false;

    // Для директорий используем QDir::rename, для файлов - QFile::rename
    if (oldInfo.isDir()) {
        QDir dir;
        success = dir.rename(normalizedOldPath, normalizedNewPath);
        qDebug() << "Directory rename result:" << success;
    } else {
        QFile file(normalizedOldPath);
        success = file.rename(normalizedNewPath);
        qDebug() << "File rename result:" << success;

        // Если стандартное переименование не удалось, пробуем альтернативный метод
        if (!success) {
            qDebug() << "Standard rename failed, trying copy+remove method...";
            if (QFile::copy(normalizedOldPath, normalizedNewPath)) {
                if (QFile::remove(normalizedOldPath)) {
                    success = true;
                    qDebug() << "Copy+remove rename successful";
                } else {
                    // Удаляем скопированный файл если не удалось удалить оригинал
                    QFile::remove(normalizedNewPath);
                    qDebug() << "Copy+remove failed: couldn't remove original";
                }
            }
        }
    }

    if (!success) {
        #ifdef Q_OS_WIN
        // Проверяем права доступа
        if (!hasWriteAccess(oldInfo.absolutePath())) {
            QString operationDesc = QString("Переименование %1 %2").arg(oldInfo.isDir() ? "папки" : "файла").arg(oldInfo.fileName());
            if (requestAdminPrivileges(operationDesc)) {
                if (tryAdminOperation("move", normalizedOldPath, normalizedNewPath)) {
                    qDebug() << "Admin rename successful";

                    emit finishProgressNotification(operationId, "Переименование",
                                                   QString("Файл '%1' успешно переименован в '%2'").arg(oldInfo.fileName(), newName),
                                                   0); // Type 0 = Info

                    emit operationCompleted();
                    emit requestRefresh();
                    return;
                }
            } else {
                emit finishProgressNotification(operationId, "Переименование",
                                               "Операция переименования отменена",
                                               0); // Type 0 = Info
                return;
            }
        }
        #endif

        QString errorMsg = Strings::RenameError.arg(oldInfo.fileName());
        qDebug() << errorMsg << "Error: Operation failed";

        emit finishProgressNotification(operationId, "Переименование", errorMsg,
                                       2); // Type 2 = Error
    } else {
        qDebug() << "Rename successful";

        emit finishProgressNotification(operationId, "Переименование",
                                       QString("Файл '%1' успешно переименован в '%2'").arg(oldInfo.fileName(), newName),
                                       0); // Type 0 = Info

        // Даем файловой системе время на обновление
        QThread::msleep(50);

        // Испускаем сигналы для обновления UI
        emit operationCompleted();
        emit requestRefresh();

        // Дополнительное обновление через короткое время
        QTimer::singleShot(100, this, [this]() {
            emit requestRefresh();
        });
    }
}

bool FileOperations::hasFilesToPaste() const
{
    return !filesToPaste.isEmpty();
}

void FileOperations::clearOperation()
{
    filesToPaste.clear();
    isCutOperation = false;
    qDebug() << "File operations cleared";
}

QString FileOperations::generateUniqueName(const QString &dir, const QString &fileName)
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