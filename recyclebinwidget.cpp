#include "recyclebinwidget.h"
#include "strings.h"
#include <QLabel>
#include <QHeaderView>
#include <QDirIterator>
#include <QMessageBox>
#include <QDesktopServices>
#include <QStorageInfo>
#include <QMenu>
#include <QAction>
#include <QIcon>
#include <QFileIconProvider>
#include <QImageReader>
#include <QPainter>
#include <QApplication>
#include <QThread>
#include <QFuture>
#include <QtConcurrent>

#ifdef Q_OS_WIN
#include <windows.h>
#include <sddl.h>
#endif

RecycleBinWidget::RecycleBinWidget(QWidget *parent)
    : QWidget(parent)
    , tableView(new QTableView(this))
    , listView(new QListView(this))
    , thumbnailView(new QListView(this))
    , model(new QStandardItemModel(this))
    , contextMenu(new QMenu(this))
{
    // Настройка основного layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Добавляем заголовок
    QLabel *titleLabel = new QLabel("🗑️ " + Strings::RecycleBinTitle, this);
    titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; margin: 10px;");
    mainLayout->addWidget(titleLabel);

    // Настройка представлений
    setupTableView();
    setupListView();
    setupThumbnailView();

    // Добавляем все представления в layout
    mainLayout->addWidget(tableView);
    mainLayout->addWidget(listView);
    mainLayout->addWidget(thumbnailView);

    // Изначально показываем табличное представление
    tableView->show();
    listView->hide();
    thumbnailView->hide();

    // Настройка контекстного меню
    tableView->setContextMenuPolicy(Qt::CustomContextMenu);
    listView->setContextMenuPolicy(Qt::CustomContextMenu);
    thumbnailView->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(tableView, &QTableView::customContextMenuRequested, this, &RecycleBinWidget::showContextMenu);
    connect(listView, &QListView::customContextMenuRequested, this, &RecycleBinWidget::showContextMenu);
    connect(thumbnailView, &QListView::customContextMenuRequested, this, &RecycleBinWidget::showContextMenu);

    // Создание действий контекстного меню
    restoreAction = contextMenu->addAction("📤 Восстановить", this, &RecycleBinWidget::restoreSelectedItems);
    deletePermanentlyAction = contextMenu->addAction("💀 Удалить навсегда", this, &RecycleBinWidget::deleteSelectedItemsPermanently);

    // Заполнение данными
    populateRecycleBin();
}

void RecycleBinWidget::setupTableView()
{
    // Настройка таблицы
    tableView->setModel(model);
    tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableView->setSortingEnabled(true);
    tableView->setAlternatingRowColors(true);

    // Настройка колонок
    model->setHorizontalHeaderLabels({
        "Имя", "Исходное расположение", "Дата удаления", "Размер", "Тип"
    });

    // Настройка внешнего вида таблицы
    tableView->horizontalHeader()->setStretchLastSection(true);
    tableView->verticalHeader()->setVisible(false);
    tableView->setShowGrid(false);

    // Увеличиваем колонку с именем в 3 раза
    tableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    tableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Interactive);
    tableView->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    tableView->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    tableView->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);

    // Подключение двойного клика
    connect(tableView, &QTableView::doubleClicked, this, &RecycleBinWidget::onItemDoubleClicked);
}

void RecycleBinWidget::setupListView()
{
    // Настройка списка
    listView->setModel(model);
    listView->setViewMode(QListView::ListMode);
    listView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    listView->setWrapping(true);
    listView->setResizeMode(QListView::Adjust);
    listView->setIconSize(QSize(32, 32));

    // Подключение двойного клика
    connect(listView, &QListView::doubleClicked, this, &RecycleBinWidget::onItemDoubleClicked);
}

void RecycleBinWidget::setupThumbnailView()
{
    // Настройка вида миниатюр
    thumbnailView->setModel(model);
    thumbnailView->setViewMode(QListView::IconMode);
    thumbnailView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    thumbnailView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    thumbnailView->setResizeMode(QListView::Adjust);
    thumbnailView->setGridSize(QSize(150, 150));
    thumbnailView->setIconSize(QSize(96, 96));
    thumbnailView->setSpacing(15);
    thumbnailView->setWrapping(true);
    thumbnailView->setMovement(QListView::Static);

    // Подключение двойного клика
    connect(thumbnailView, &QListView::doubleClicked, this, &RecycleBinWidget::onItemDoubleClicked);
}

void RecycleBinWidget::setViewMode(int mode)
{
    // Скрываем все представления
    tableView->hide();
    listView->hide();
    thumbnailView->hide();

    // Показываем нужное представление
    switch (mode) {
    case 0: // Таблица
        tableView->show();
        currentViewMode = 0;
        break;
    case 1: // Список
        listView->show();
        currentViewMode = 1;
        break;
    case 2: // Миниатюры
        thumbnailView->show();
        currentViewMode = 2;
        // Генерируем миниатюры при переключении в этот режим
        QTimer::singleShot(100, this, &RecycleBinWidget::generateThumbnails);
        break;
    default:
        tableView->show();
        currentViewMode = 0;
        break;
    }
}

QIcon RecycleBinWidget::getThumbnailIcon(const QString &filePath, bool isDir)
{
    if (isDir) {
        // Для папок используем стандартную иконку
        QFileIconProvider provider;
        return provider.icon(QFileIconProvider::Folder);
    }

    // Для файлов пытаемся загрузить миниатюру
    QImageReader reader(filePath);
    if (reader.canRead()) {
        QImage image = reader.read();
        if (!image.isNull()) {
            // Масштабируем изображение до размера миниатюры
            image = image.scaled(96, 96, Qt::KeepAspectRatio, Qt::SmoothTransformation);

            // Создаем изображение с прозрачным фоном
            QImage result(96, 96, QImage::Format_ARGB32);
            result.fill(Qt::transparent);

            // Рисуем изображение по центру
            QPainter painter(&result);
            int x = (96 - image.width()) / 2;
            int y = (96 - image.height()) / 2;
            painter.drawImage(x, y, image);
            painter.end();

            return QIcon(QPixmap::fromImage(result));
        }
    }

    // Если не удалось загрузить миниатюру, используем стандартную иконку для файла
    QFileIconProvider provider;
    return provider.icon(QFileIconProvider::File);
}

void RecycleBinWidget::generateThumbnails()
{
    if (currentViewMode != 2) return; // Только для режима миниатюр

    // Генерируем миниатюры в отдельном потоке
    QtConcurrent::run([this]() {
        for (int row = 0; row < model->rowCount(); ++row) {
            QStandardItem *item = model->item(row, 0);
            if (!item) continue;

            QString filePath = item->data(Qt::UserRole).toString();
            bool isDir = item->data(Qt::UserRole + 2).toBool();

            QIcon thumbnail = getThumbnailIcon(filePath, isDir);

            // Обновляем иконку в основном потоке
            QMetaObject::invokeMethod(this, [this, row, thumbnail]() {
                QStandardItem *item = model->item(row, 0);
                if (item) {
                    item->setIcon(thumbnail);
                }
            }, Qt::QueuedConnection);

            // Небольшая задержка для снижения нагрузки
            QThread::msleep(10);
        }
    });
}

#ifdef Q_OS_WIN
QString RecycleBinWidget::getCurrentUserSid()
{
    HANDLE hToken = NULL;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        return QString();
    }

    DWORD dwSize = 0;
    GetTokenInformation(hToken, TokenUser, NULL, 0, &dwSize);
    if (dwSize == 0) {
        CloseHandle(hToken);
        return QString();
    }

    PTOKEN_USER ptu = (PTOKEN_USER)HeapAlloc(GetProcessHeap(), 0, dwSize);
    if (!ptu) {
        CloseHandle(hToken);
        return QString();
    }

    if (!GetTokenInformation(hToken, TokenUser, ptu, dwSize, &dwSize)) {
        HeapFree(GetProcessHeap(), 0, ptu);
        CloseHandle(hToken);
        return QString();
    }

    LPWSTR sidString = NULL;
    if (!ConvertSidToStringSidW(ptu->User.Sid, &sidString)) {
        HeapFree(GetProcessHeap(), 0, ptu);
        CloseHandle(hToken);
        return QString();
    }

    QString userSid = QString::fromWCharArray(sidString);
    LocalFree(sidString);
    HeapFree(GetProcessHeap(), 0, ptu);
    CloseHandle(hToken);

    return userSid;
}

RecycleBinItem RecycleBinWidget::parseIFile(const QString &iFilePath)
{
    RecycleBinItem item;

    QFile file(iFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return item;
    }

    QByteArray data = file.readAll();
    file.close();

    if (data.size() < 20) {
        return item;
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
        return item;
    }

    // File size
    item.fileSize = *(qint64*)(ptr + offset);
    offset += 8;

    // FILETIME
    FILETIME ft = *(FILETIME*)(ptr + offset);
    offset += 8;

    qint32 pathLen = 0;
    if (version == 2) {
        pathLen = *(qint32*)(ptr + offset);
        offset += 4;
    }

    // Original path
    const wchar_t* pathPtr = (wchar_t*)(ptr + offset);
    if (version == 2) {
        item.originalPath = QString::fromWCharArray(pathPtr, pathLen - 1); // Exclude null terminator
    } else {
        item.originalPath = QString::fromWCharArray(pathPtr);
    }

    // Convert FILETIME to QDateTime (UTC)
    ULARGE_INTEGER uli;
    uli.HighPart = ft.dwHighDateTime;
    uli.LowPart = ft.dwLowDateTime;
    qint64 ticks = uli.QuadPart;
    qint64 secs = (ticks / 10000000) - 11644473600LL;
    item.deletionTime = QDateTime::fromSecsSinceEpoch(secs, Qt::UTC);

    item.isValid = true;
    return item;
}
#endif

int RecycleBinWidget::scanRecycleBinFolder(const QString &folderPath, const QString &driveLetter)
{
    QDir dir(folderPath);
    if (!dir.exists()) {
        return 0;
    }

    // Устанавливаем фильтры для показа всех файлов
    dir.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
    dir.setSorting(QDir::Time | QDir::Reversed); // Сортировка по дате изменения

    QStringList entries = dir.entryList();
    int itemsFound = 0;

    qDebug() << "Scanning folder:" << folderPath << "Found entries:" << entries.size();

    for (const QString &entry : entries) {
        // Пропускаем все файлы, которые НЕ начинаются с "$R"
        if (!entry.startsWith("$R")) {
            continue;
        }

        // Пропускаем служебные файлы
        if (entry.endsWith(".ini", Qt::CaseInsensitive) ||
            entry == "desktop.ini" || entry == "Thumbs.db") {
            continue;
        }

        QString entryPath = folderPath + "\\" + entry;
        QFileInfo fileInfo(entryPath);

        // Пропускаем слишком маленькие файлы (вероятно, метаданные)
        if (fileInfo.isFile() && fileInfo.size() < 10) {
            continue;
        }

        // Ищем соответствующий $I файл для получения информации
        QString iFileName = "$I" + entry.mid(2); // Заменяем $R на $I
        QString iFilePath = folderPath + "\\" + iFileName;

        RecycleBinItem binInfo;
        bool hasValidInfo = false;

        #ifdef Q_OS_WIN
        if (QFile::exists(iFilePath)) {
            binInfo = parseIFile(iFilePath);
            hasValidInfo = binInfo.isValid;
        }
        #endif

        // Создаем элемент для модели
        QString displayName;
        QString originalPath;
        QString deletionDate;
        QString sizeStr;
        QString type;
        QIcon icon;

        if (hasValidInfo) {
            // Используем информацию из $I файла
            QFileInfo origInfo(binInfo.originalPath);
            displayName = origInfo.fileName();
            originalPath = binInfo.originalPath;
            deletionDate = binInfo.deletionTime.toString("dd.MM.yyyy HH:mm:ss");

            // Форматируем размер
            qint64 size = binInfo.fileSize;
            if (size < 1024) {
                sizeStr = QString::number(size) + " Б";
            } else if (size < 1024 * 1024) {
                sizeStr = QString::number(size / 1024.0, 'f', 1) + " КБ";
            } else if (size < 1024 * 1024 * 1024) {
                sizeStr = QString::number(size / (1024.0 * 1024.0), 'f', 1) + " МБ";
            } else {
                sizeStr = QString::number(size / (1024.0 * 1024.0 * 1024.0), 'f', 1) + " ГБ";
            }

            type = origInfo.isDir() ? "Папка" : "Файл";

            // Получаем иконку
            QFileIconProvider iconProvider;
            icon = iconProvider.icon(origInfo);

            if (!origInfo.isDir()) {
                QString ext = origInfo.suffix().toLower();
                if (!ext.isEmpty()) {
                    type = ext.toUpper() + " файл";
                }
            }
        } else {
            // Используем базовую информацию из файловой системы
            displayName = entry.mid(2); // Убираем префикс $R
            originalPath = driveLetter + ":\\";
            deletionDate = fileInfo.lastModified().toString("dd.MM.yyyy HH:mm:ss");

            // Форматируем размер
            qint64 size = fileInfo.size();
            if (size < 1024) {
                sizeStr = QString::number(size) + " Б";
            } else if (size < 1024 * 1024) {
                sizeStr = QString::number(size / 1024.0, 'f', 1) + " КБ";
            } else if (size < 1024 * 1024 * 1024) {
                sizeStr = QString::number(size / (1024.0 * 1024.0), 'f', 1) + " МБ";
            } else {
                sizeStr = QString::number(size / (1024.0 * 1024.0 * 1024.0), 'f', 1) + " ГБ";
            }

            type = fileInfo.isDir() ? "Папка" : "Файл";

            // Получаем иконку
            QFileIconProvider iconProvider;
            icon = iconProvider.icon(fileInfo);

            if (fileInfo.isFile()) {
                QString ext = fileInfo.suffix().toLower();
                if (!ext.isEmpty()) {
                    type = ext.toUpper() + " файл";
                }
            }
        }

        // Создаем элементы для разных представлений
        QList<QStandardItem*> rowItems;

        // Для табличного представления
        QStandardItem *nameItem = new QStandardItem(icon, displayName);
        QStandardItem *pathItem = new QStandardItem(originalPath);
        QStandardItem *dateItem = new QStandardItem(deletionDate);
        QStandardItem *sizeItem = new QStandardItem(sizeStr);
        QStandardItem *typeItem = new QStandardItem(type);

        rowItems << nameItem << pathItem << dateItem << sizeItem << typeItem;

        // Сохраняем путь к файлу в пользовательской роли
        for (QStandardItem *rowItem : rowItems) {
            rowItem->setData(entryPath, Qt::UserRole); // путь в корзине
            rowItem->setData(originalPath, Qt::UserRole + 1); // оригинальный путь
            rowItem->setData(fileInfo.isDir(), Qt::UserRole + 2); // является ли элемент папкой
            if (hasValidInfo) {
                rowItem->setData(binInfo.deletionTime, Qt::UserRole + 3); // время удаления
            }
        }

        model->appendRow(rowItems);
        itemsFound++;

        qDebug() << "Added $R item:" << displayName << "Path:" << entryPath << "Size:" << sizeStr << "IsDir:" << fileInfo.isDir();
    }

    return itemsFound;
}

void RecycleBinWidget::populateRecycleBin()
{
    #ifndef Q_OS_WIN
    QMessageBox::information(this, "Корзина", "Просмотр корзины поддерживается только в Windows");
    return;
    #endif

    // Очищаем модель перед заполнением
    model->removeRows(0, model->rowCount());

    #ifdef Q_OS_WIN
    // Получаем SID текущего пользователя
    QString userSid = getCurrentUserSid();
    if (userSid.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Не удалось получить SID текущего пользователя");
        return;
    }

    // Получаем список всех дисков
    QList<QStorageInfo> drives = QStorageInfo::mountedVolumes();

    int totalItems = 0;

    // Для каждого диска пытаемся найти корзину
    for (const QStorageInfo &drive : drives) {
        if (drive.isValid() && drive.isReady()) {
            QString rootPath = drive.rootPath();
            if (rootPath.length() >= 2 && rootPath[1] == ':') {
                QString driveLetter = rootPath.left(1);

                // Пробуем несколько возможных путей к корзине
                QStringList possiblePaths;
                possiblePaths << driveLetter + ":\\$Recycle.Bin"
                             << driveLetter + ":\\Recycler"
                             << driveLetter + ":\\Recycled";

                for (const QString &recyclePath : possiblePaths) {
                    QDir recycleDir(recyclePath);
                    if (recycleDir.exists()) {
                        qDebug() << "Found recycle bin at:" << recyclePath;

                        // Ищем все подпапки (каждая соответствует пользователю)
                        QStringList userDirs = recycleDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);

                        for (const QString &userDir : userDirs) {
                            if (userDir == userSid || userDir.startsWith("S-1-5-")) {
                                QString userPath = recyclePath + "\\" + userDir;
                                totalItems += scanRecycleBinFolder(userPath, driveLetter);
                            }
                        }

                        // Также проверяем корень корзины
                        totalItems += scanRecycleBinFolder(recyclePath, driveLetter);
                    }
                }
            }
        }
    }

    if (totalItems == 0) {
        QMessageBox::information(this, "Корзина",
                               "Корзина пуста или не удалось получить доступ к файлам.\n\n"
                               "Возможные причины:\n"
                               "1. Корзина действительно пуста\n"
                               "2. Нет прав доступа к папке корзины\n"
                               "3. Файлы в корзине имеют системные атрибуты");
    } else {
        qDebug() << "Loaded" << totalItems << "items from recycle bin";

        // Устанавливаем ширину колонки имени после заполнения данных
        tableView->setColumnWidth(0, 300); // Увеличиваем колонку имени
        tableView->setColumnWidth(1, 150); // Оригинальный путь
        tableView->setColumnWidth(2, 200); // Дата удаления
        tableView->setColumnWidth(3, 100); // Размер
        tableView->setColumnWidth(4, 100); // Тип

        // Генерируем миниатюры, если активен соответствующий режим
        if (currentViewMode == 2) {
            QTimer::singleShot(100, this, &RecycleBinWidget::generateThumbnails);
        }
    }
    #endif
}

void RecycleBinWidget::onItemDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) return;

    // Получаем путь к файлу из данных модели
    QString filePath = index.data(Qt::UserRole).toString();
    bool isDir = index.data(Qt::UserRole + 2).toBool();

    if (!filePath.isEmpty()) {
        if (isDir) {
            // Для папок - эмитируем сигнал для открытия внутри приложения
            qDebug() << "Emitting folderDoubleClicked for path:" << filePath;
            emit folderDoubleClicked(filePath);
        } else {
            // Для файлов открываем в ассоциированной программе
            QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
        }
    }
}

void RecycleBinWidget::showContextMenu(const QPoint &pos)
{
    QAbstractItemView *currentView = nullptr;

    // Определяем активное представление
    if (tableView->isVisible()) {
        currentView = tableView;
    } else if (listView->isVisible()) {
        currentView = listView;
    } else if (thumbnailView->isVisible()) {
        currentView = thumbnailView;
    }

    if (currentView) {
        QModelIndex index = currentView->indexAt(pos);
        if (index.isValid()) {
            contextMenu->exec(currentView->viewport()->mapToGlobal(pos));
        }
    }
}

void RecycleBinWidget::restoreSelectedItems()
{
    QAbstractItemView *currentView = nullptr;

    // Определяем активное представление
    if (tableView->isVisible()) {
        currentView = tableView;
    } else if (listView->isVisible()) {
        currentView = listView;
    } else if (thumbnailView->isVisible()) {
        currentView = thumbnailView;
    }

    if (!currentView) return;

    QModelIndexList selectedIndexes = currentView->selectionModel()->selectedIndexes();
    if (selectedIndexes.isEmpty()) {
        QMessageBox::information(this, "Восстановление", "Выберите элементы для восстановления.");
        return;
    }

    int successCount = 0;
    int failCount = 0;

    for (const QModelIndex &index : selectedIndexes) {
        if (!index.isValid()) continue;

        // Получаем данные из модели
        QString binPath = index.data(Qt::UserRole).toString();
        QString originalPath = index.data(Qt::UserRole + 1).toString();
        bool isDir = index.data(Qt::UserRole + 2).toBool();

        if (binPath.isEmpty() || originalPath.isEmpty()) {
            failCount++;
            continue;
        }

        qDebug() << "Восстановление:" << binPath << "->" << originalPath;

        // Проверяем, существует ли оригинальный путь
        QFileInfo origInfo(originalPath);
        if (origInfo.exists()) {
            // Если файл/папка уже существует, предлагаем переименовать
            QString newName;
            int counter = 1;
            QString baseName = origInfo.baseName();
            QString suffix = origInfo.completeSuffix();
            QString path = origInfo.path();

            do {
                if (suffix.isEmpty()) {
                    newName = QString("%1 (%2)").arg(baseName).arg(counter);
                } else {
                    newName = QString("%1 (%2).%3").arg(baseName).arg(counter).arg(suffix);
                }
                counter++;
            } while (QFile::exists(path + "/" + newName));

            originalPath = path + "/" + newName;
        }

        // Восстанавливаем файл/папку
        bool success = false;
        if (isDir) {
            QDir dir;
            success = dir.rename(binPath, originalPath);
        } else {
            success = QFile::rename(binPath, originalPath);
        }

        if (success) {
            // Удаляем соответствующий $I файл
            QString iFilePath = binPath;
            iFilePath.replace("$R", "$I");
            if (QFile::exists(iFilePath)) {
                QFile::remove(iFilePath);
            }

            // Удаляем строку из модели
            model->removeRow(index.row());
            successCount++;

            qDebug() << "Успешно восстановлено:" << originalPath;
        } else {
            failCount++;
            qDebug() << "Ошибка восстановления:" << binPath;
        }
    }

    // Показываем результат
    if (failCount > 0) {
        QMessageBox::warning(this, "Восстановление",
                           QString("Успешно восстановлено: %1\nНе удалось восстановить: %2")
                           .arg(successCount).arg(failCount));
    } else if (successCount > 0) {
        QMessageBox::information(this, "Восстановление",
                               QString("Успешно восстановлено: %1 элементов").arg(successCount));
    }
}

void RecycleBinWidget::deleteSelectedItemsPermanently()
{
    QAbstractItemView *currentView = nullptr;

    // Определяем активное представление
    if (tableView->isVisible()) {
        currentView = tableView;
    } else if (listView->isVisible()) {
        currentView = listView;
    } else if (thumbnailView->isVisible()) {
        currentView = thumbnailView;
    }

    if (!currentView) return;

    QModelIndexList selectedIndexes = currentView->selectionModel()->selectedIndexes();
    if (selectedIndexes.isEmpty()) {
        QMessageBox::information(this, "Удаление", "Выберите элементы для удаления.");
        return;
    }

    int result = QMessageBox::question(this, "Удаление навсегда",
                                     "Вы уверены, что хотите удалить выбранные элементы навсегда?",
                                     QMessageBox::Yes | QMessageBox::No);

    if (result != QMessageBox::Yes) {
        return;
    }

    int successCount = 0;
    int failCount = 0;

    // Удаляем в обратном порядке, чтобы индексы не сдвигались
    for (int i = selectedIndexes.size() - 1; i >= 0; i--) {
        QModelIndex index = selectedIndexes[i];
        if (!index.isValid()) continue;

        QString binPath = index.data(Qt::UserRole).toString();
        bool isDir = index.data(Qt::UserRole + 2).toBool();

        if (binPath.isEmpty()) {
            failCount++;
            continue;
        }

        bool success = false;
        if (isDir) {
            QDir dir(binPath);
            success = dir.removeRecursively();
        } else {
            success = QFile::remove(binPath);
        }

        if (success) {
            // Удаляем соответствующий $I файл
            QString iFilePath = binPath;
            iFilePath.replace("$R", "$I");
            if (QFile::exists(iFilePath)) {
                QFile::remove(iFilePath);
            }

            model->removeRow(index.row());
            successCount++;
        } else {
            failCount++;
        }
    }

    // Показываем результат
    if (failCount > 0) {
        QMessageBox::warning(this, "Удаление",
                           QString("Успешно удалено: %1\nНе удалось удалить: %2")
                           .arg(successCount).arg(failCount));
    } else if (successCount > 0) {
        QMessageBox::information(this, "Удаление",
                               QString("Успешно удалено: %1 элементов").arg(successCount));
    }
}

void RecycleBinWidget::refresh()
{
    populateRecycleBin();
}

bool RecycleBinWidget::isEmpty() const
{
    return model->rowCount() == 0;
}