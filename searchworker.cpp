#include "searchworker.h"

SearchWorker::SearchWorker(QObject *parent) : QObject(parent) {}

void SearchWorker::search(const QString &text, const QString &startPath, bool searchInAllDrives,
                         bool caseSensitive, bool searchInNamesOnly)
{
    QStringList results;
    Qt::CaseSensitivity sensitivity = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
    QElapsedTimer timer;
    timer.start();

    qDebug() << "=== STARTING SEARCH ===";
    qDebug() << "Search text:" << text;
    qDebug() << "Start path:" << startPath;
    qDebug() << "Search in all drives:" << searchInAllDrives;
    qDebug() << "Case sensitive:" << caseSensitive;
    qDebug() << "Search in names only:" << searchInNamesOnly;

    try {
        if (searchInAllDrives) {
            // Поиск по всем дискам
            QFileInfoList drives = QDir::drives();
            qDebug() << "Available drives:" << drives;

            for (const QFileInfo &drive : drives) {
                if (QThread::currentThread()->isInterruptionRequested()) break;

                QString drivePath = drive.absoluteFilePath();
                qDebug() << "Searching in drive:" << drivePath;

                // Пропускаем системные и сетевые диски если нужно
                if (drivePath.startsWith("A:") || drivePath.startsWith("B:")) {
                    continue;
                }

                QDirIterator it(drivePath,
                               QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System,
                               QDirIterator::Subdirectories);

                int filesChecked = 0;
                while (it.hasNext()) {
                    if (QThread::currentThread()->isInterruptionRequested()) break;

                    it.next();
                    filesChecked++;

                    // Проверяем таймаут каждые 100 файлов
                    if (filesChecked % 100 == 0) {
                        if (timer.elapsed() > 30000) {
                            qDebug() << "Search timeout after 30 seconds";
                            emit searchFinished(results, true);
                            return;
                        }
                    }

                    QFileInfo fileInfo = it.fileInfo();
                    QString fileName = fileInfo.fileName();

                    // Проверяем соответствие поисковому запросу
                    if (fileName.contains(text, sensitivity)) {
                        QString resultPath = fileInfo.absoluteFilePath();
                        results.append(resultPath);
                        emit progressUpdate(results.size());

                        qDebug() << "Found:" << resultPath;

                        // Ограничиваем количество результатов
                        if (results.size() >= 500) {
                            qDebug() << "Reached maximum results limit (500)";
                            break;
                        }
                    }
                }

                if (results.size() >= 500) break;
            }
        } else {
            // Поиск в указанной папке и ее подпапках
            QString searchPath = startPath;

            // Если путь пустой или не существует, используем домашнюю директорию
            if (searchPath.isEmpty() || !QDir(searchPath).exists()) {
                searchPath = QDir::homePath();
                qDebug() << "Invalid start path, using home directory:" << searchPath;
            }

            qDebug() << "Searching in folder:" << searchPath;

            // Исключаем системные папки из поиска
            QStringList excludedPaths;
            excludedPaths << "/AppData" << "/$Recycle.Bin" << "/Recovery" << "/System Volume Information";

            QDirIterator it(searchPath,
                           QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System,
                           QDirIterator::Subdirectories);

            int filesChecked = 0;
            while (it.hasNext()) {
                if (QThread::currentThread()->isInterruptionRequested()) break;

                it.next();
                filesChecked++;

                // Проверяем прерывание каждые 50 файлов
                if (filesChecked % 50 == 0) {
                    if (QThread::currentThread()->isInterruptionRequested()) break;
                }

                QFileInfo fileInfo = it.fileInfo();
                QString filePath = fileInfo.absoluteFilePath();
                QString fileName = fileInfo.fileName();

                // Пропускаем системные папки
                bool skip = false;
                for (const QString &excludedPath : excludedPaths) {
                    if (filePath.contains(excludedPath, Qt::CaseInsensitive)) {
                        skip = true;
                        break;
                    }
                }

                if (skip) {
                    continue;
                }

                // Проверяем, что файл находится в пределах стартовой папки
                // (защита от символических ссылок и junction points)
                if (!filePath.startsWith(searchPath, Qt::CaseInsensitive)) {
                    qDebug() << "Skipping file outside search path:" << filePath;
                    continue;
                }

                if (fileName.contains(text, sensitivity)) {
                    QString resultPath = fileInfo.absoluteFilePath();
                    results.append(resultPath);
                    emit progressUpdate(results.size());

                    qDebug() << "Found:" << resultPath;

                    if (results.size() >= 500) {
                        qDebug() << "Reached maximum results limit (500)";
                        break;
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        qDebug() << "Search error:" << e.what();
    } catch (...) {
        qDebug() << "Unknown search error occurred";
    }

    qDebug() << "=== SEARCH COMPLETED ===";
    qDebug() << "Total results:" << results.size();
    qDebug() << "Search duration:" << timer.elapsed() << "ms";

    if (!QThread::currentThread()->isInterruptionRequested()) {
        emit searchFinished(results, false);
    }
}