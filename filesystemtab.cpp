#include "filesystemtab.h"
#include "contextmenu.h"
#include "mainwindow.h"
#include "strings.h"
#include "iconsizes.h"
#include "listmodedelegate.h"
#include "treesizedelegate.h"
#include "disksizeutils.h"
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QDebug>
#include <QFileInfo>
#include <QUrl>
#include <QMimeData>
#include <QDesktopServices>
#include <QHeaderView>
#include <QTimer>
#include <windows.h>
#include <shlobj.h>
#include <QSettings>

#include "homepagewidget.h"
#include "recyclebinwidget.h"

FileSystemTab::FileSystemTab(QWidget *parent)
    : QWidget(parent)
    , model(new CustomFileSystemModel(this))
    , listView(new QListView(this))
    , treeView(new QTreeView(this))
    , thumbnailView(new ThumbnailView(this))
    , homePageWidget(new HomePageWidget(this))
    , listModeDelegate(nullptr)
    , treeSizeDelegate(nullptr)
    , diskInfoTimer(new QTimer(this))
    , isLoadingSavedViewMode(false)
{
    setupUI();
    setupConnections();

    // Настраиваем модель файловой системы
    model->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::AllDirs);
    model->setRootPath("");
    model->setReadOnly(false);

    // Устанавливаем размеры иконок
    listView->setIconSize(IconSizes::ListView::Default);
    treeView->setIconSize(IconSizes::TreeView::Default);
    thumbnailView->setIconSize(IconSizes::ThumbnailView::Default);

    // Создаем и устанавливаем кастомные делегаты
    listModeDelegate = new ListModeDelegate(this);
    treeSizeDelegate = new TreeSizeDelegate(this);

    listView->setItemDelegate(listModeDelegate);
    treeView->setItemDelegateForColumn(1, treeSizeDelegate);

    // Включаем редактирование
    listView->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed | QAbstractItemView::SelectedClicked);
    treeView->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed | QAbstractItemView::SelectedClicked);
    thumbnailView->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed | QAbstractItemView::SelectedClicked);

    listView->setModel(model);
    treeView->setModel(model);
    thumbnailView->setModel(model);

    // Настраиваем колонки дерева
    treeView->setHeaderHidden(false);
    treeView->setUniformRowHeights(true);

    // Устанавливаем растяжение колонок
    treeView->header()->setStretchLastSection(false);
    treeView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    treeView->header()->setSectionResizeMode(1, QHeaderView::Interactive);
    treeView->header()->setSectionResizeMode(2, QHeaderView::Interactive);
    treeView->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);

    // Устанавливаем начальные размеры колонок
    treeView->header()->resizeSection(1, 120);
    treeView->header()->resizeSection(2, 150);
    treeView->header()->resizeSection(3, 120);

    treeView->hide();
    thumbnailView->hide();

    // Устанавливаем фильтры событий на viewport'ы
    listView->viewport()->installEventFilter(this);
    treeView->viewport()->installEventFilter(this);
    thumbnailView->viewport()->installEventFilter(this);

    // Настраиваем таймер для периодического обновления информации о размерах дисков
    diskInfoTimer->setInterval(30000);
    connect(diskInfoTimer, &QTimer::timeout, this, &FileSystemTab::updateDiskSizeInfo);
    diskInfoTimer->start();

    showHomePage();
}

FileSystemTab::~FileSystemTab()
{
    if (diskInfoTimer && diskInfoTimer->isActive()) {
        diskInfoTimer->stop();
    }
    delete listModeDelegate;
    delete treeSizeDelegate;
}

bool FileSystemTab::isRecycleBinActive() const
{
#ifdef Q_OS_WIN
    RecycleBinWidget *recycleBin = findChild<RecycleBinWidget*>();
    return recycleBin && recycleBin->isVisible();
#else
    return false;
#endif
}

void FileSystemTab::startRename()
{
    QAbstractItemView *view = getCurrentView();
    if (!view) return;

    QModelIndexList selectedIndexes = view->selectionModel()->selectedIndexes();
    if (selectedIndexes.isEmpty() || isRenaming) return;

    isRenaming = true;

    QModelIndex index = selectedIndexes.first();
    if (index.isValid()) {
        view->edit(index);
    }

    QTimer::singleShot(100, this, [this]() {
        isRenaming = false;
    });
}

void FileSystemTab::setSorting(int column, Qt::SortOrder order)
{
    currentSortColumn = column;
    currentSortOrder = order;
    applySorting();
    emit sortChanged(column, order);
}

void FileSystemTab::applySorting()
{
    model->sort(currentSortColumn, currentSortOrder);
    treeView->header()->setSortIndicator(currentSortColumn, currentSortOrder);
    treeView->header()->setSortIndicatorShown(true);
}

void FileSystemTab::updateDiskSizeInfo()
{
    if (treeView->isVisible()) {
        treeView->viewport()->update();
    }
    if (listView->isVisible()) {
        listView->viewport()->update();
    }
    if (thumbnailView->isVisible()) {
        thumbnailView->viewport()->update();
    }
}

void FileSystemTab::loadSavedViewMode()
{
    QSettings settings;
    QString currentPath = getCurrentPath();
    qDebug() << "Current path in loadSavedViewMode:" << currentPath;

    if (currentPath == Strings::HomePage) {
        // Для домашней страницы всегда используем режим дерева
        currentViewMode = 1; // 1 - это дерево
        applyViewMode(currentViewMode);
    }
}

void FileSystemTab::setViewMode(int mode, bool saveSetting)
{
    if (mode < 0 || mode > 2) {
        return;
    }

    if (currentViewMode != mode) {
        currentViewMode = mode;
        applyViewMode(mode);

        // Испускаем сигнал об изменении режима просмотра
        emit viewModeChanged(mode);

        // Сохраняем настройки, если не идет загрузка сохраненного режима
        if (saveSetting) {
            QSettings settings;
            QString currentPath = getCurrentPath();
            qDebug() << "Saving view mode for current path:" << currentPath;

            // Для домашней страницы сохраняем под специальным ключом
            if (currentPath == Strings::HomePage) {
                QString key = QString("viewMode/%1").arg(Strings::HomePage);
                qDebug() << "Saving view mode with key:" << key << "mode:" << mode;
                settings.setValue(key, mode);
            }
        }
    }
}

bool FileSystemTab::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

        if (mouseEvent->button() == Qt::MiddleButton) {
            QAbstractItemView *view = nullptr;

            if (obj == listView->viewport()) {
                view = listView;
            } else if (obj == treeView->viewport()) {
                view = treeView;
            } else if (obj == thumbnailView->viewport()) {
                view = thumbnailView;
            }

            if (view) {
                QModelIndex index = view->indexAt(mouseEvent->pos());
                if (index.isValid() && model->isDir(index)) {
                    QString path = model->filePath(index);
                    qDebug() << "Middle click on folder:" << path;
                    emit middleClickOnFolder(path);
                    return true;
                }
            }
        }
    }

    return QWidget::eventFilter(obj, event);
}

void FileSystemTab::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void FileSystemTab::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void FileSystemTab::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        QList<QUrl> urls = event->mimeData()->urls();
        QString destinationPath = getCurrentPath();

        if (destinationPath == Strings::HomePage) {
            destinationPath = QDir::homePath();
        }

        for (const QUrl &url : urls) {
            QString filePath = url.toLocalFile();
            QFileInfo fileInfo(filePath);
            QString destination = destinationPath + "/" + fileInfo.fileName();

            if (QFile::exists(filePath)) {
                if (event->modifiers() & Qt::ControlModifier) {
                    QFile::copy(filePath, destination);
                } else {
                    QFile::rename(filePath, destination);
                }
            }
        }

        event->acceptProposedAction();
        refresh();
    } else {
        event->ignore();
    }
}

void FileSystemTab::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    listView->setViewMode(QListView::ListMode);
    listView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    listView->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed | QAbstractItemView::SelectedClicked);
    treeView->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed | QAbstractItemView::SelectedClicked);
    thumbnailView->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed | QAbstractItemView::SelectedClicked);

    // Настраиваем стили для viewport'ов - убираем темную подложку
    QString viewportStyle = "QListView::item:!selected, QTreeView::item:!selected { background: transparent; }";
    listView->viewport()->setStyleSheet(viewportStyle);
    treeView->viewport()->setStyleSheet(viewportStyle);
    thumbnailView->viewport()->setStyleSheet(viewportStyle);

    // Включаем поддержку drag & drop
    listView->setDragEnabled(true);
    listView->setAcceptDrops(true);
    listView->setDropIndicatorShown(true);
    listView->setDragDropMode(QAbstractItemView::DragDrop);

    treeView->setDragEnabled(true);
    treeView->setAcceptDrops(true);
    treeView->setDropIndicatorShown(true);
    treeView->setDragDropMode(QAbstractItemView::DragDrop);

    thumbnailView->setDragEnabled(true);
    thumbnailView->setAcceptDrops(true);
    thumbnailView->setDropIndicatorShown(true);
    thumbnailView->setDragDropMode(QAbstractItemView::DragDrop);

    // Включаем сортировку для treeView
    treeView->setSortingEnabled(true);
    setAcceptDrops(true);

    // Добавляем домашнюю страницу
    layout->addWidget(homePageWidget);

    // Добавляем основные представления
    layout->addWidget(listView);
    layout->addWidget(treeView);
    layout->addWidget(thumbnailView);

    homePageWidget->hide();
}

void FileSystemTab::setupConnections()
{
    connect(model, &QFileSystemModel::directoryLoaded, this, &FileSystemTab::onDirectoryLoaded);
    connect(model, &QFileSystemModel::directoryLoaded, thumbnailView, &ThumbnailView::onDirectoryLoaded);

    connect(listView, &QListView::doubleClicked, this, &FileSystemTab::onItemDoubleClicked);
    connect(treeView, &QTreeView::doubleClicked, this, &FileSystemTab::onItemDoubleClicked);
    connect(thumbnailView, &QListView::doubleClicked, this, &FileSystemTab::onItemDoubleClicked);

    connect(listView, &QListView::customContextMenuRequested, this, &FileSystemTab::showContextMenu);
    connect(treeView, &QTreeView::customContextMenuRequested, this, &FileSystemTab::showContextMenu);
    connect(thumbnailView, &QListView::customContextMenuRequested, this, &FileSystemTab::showContextMenu);

    connect(treeView->header(), &QHeaderView::sectionClicked, this, &FileSystemTab::onHeaderClicked);

    listView->setContextMenuPolicy(Qt::CustomContextMenu);
    treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    thumbnailView->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(homePageWidget, &HomePageWidget::folderClicked, this, &FileSystemTab::onHomePageFolderClicked);
    connect(homePageWidget, &HomePageWidget::middleClickOnFolder, this, &FileSystemTab::onMiddleClickOnHomePageFolder);
}

void FileSystemTab::onHeaderClicked(int logicalIndex)
{
    if (logicalIndex == currentSortColumn) {
        currentSortOrder = (currentSortOrder == Qt::AscendingOrder) ? Qt::DescendingOrder : Qt::AscendingOrder;
    } else {
        currentSortColumn = logicalIndex;
        currentSortOrder = Qt::AscendingOrder;
    }

    applySorting();

    MainWindow* mainWindow = getMainWindow();
    if (mainWindow) {
        mainWindow->saveSortSettingsForPath(getCurrentPath(), currentSortColumn, currentSortOrder);
    }

    emit sortChanged(currentSortColumn, currentSortOrder);
}

void FileSystemTab::showHomePage()
{
    closeRecycleBin();

    model->setRootPath("");
    listView->setRootIndex(QModelIndex());
    treeView->setRootIndex(QModelIndex());
    thumbnailView->setRootIndex(QModelIndex());
    homePageWidget->show();

    // Загружаем сохраненный режим просмотра для домашней страницы
    loadSavedViewMode();

    // Применяем режим просмотра
    applyViewMode(currentViewMode);

    currentPath = Strings::HomePage;
    backHistory.clear();
    forwardHistory.clear();
    emit pathChanged(Strings::HomePage);
    emit navigationStateChanged();
}

void FileSystemTab::onDirectoryLoaded(const QString &path)
{
    if (!path.isEmpty()) {
        emit pathChanged(QDir::toNativeSeparators(path));

        MainWindow* mainWindow = getMainWindow();
        if (mainWindow) {
            int sortColumn;
            Qt::SortOrder sortOrder;
            mainWindow->loadSortSettingsForPath(path, sortColumn, sortOrder);
            setSorting(sortColumn, sortOrder);
        }
    }
}

void FileSystemTab::onItemDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) return;

    QString path = model->filePath(index);

    if (model->isDir(index)) {
        navigateTo(path);
    } else {
        emit itemDoubleClicked(index);
    }
}

void FileSystemTab::onHomePageFolderClicked(const QString &path)
{
    if (path == Strings::RecycleBinPath) {
#ifdef Q_OS_WIN
        RecycleBinWidget *recycleWidget = new RecycleBinWidget(this);

        connect(recycleWidget, &RecycleBinWidget::folderDoubleClicked, this, &FileSystemTab::navigateTo);

        homePageWidget->hide();
        listView->hide();
        treeView->hide();
        thumbnailView->hide();

        QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(this->layout());
        if (layout) {
            QLayoutItem *child;
            while ((child = layout->takeAt(0)) != nullptr) {
                if (child->widget() && child->widget() != homePageWidget &&
                    child->widget() != listView && child->widget() != treeView &&
                    child->widget() != thumbnailView) {
                    child->widget()->hide();
                    child->widget()->deleteLater();
                }
                delete child;
            }

            layout->addWidget(homePageWidget);
            layout->addWidget(listView);
            layout->addWidget(treeView);
            layout->addWidget(thumbnailView);
            layout->insertWidget(0, recycleWidget);
        }

        recycleWidget->show();

        MainWindow* mainWindow = getMainWindow();
        if (mainWindow) {
            int viewMode = mainWindow->loadViewModeForPath(Strings::RecycleBinTitle);
            recycleWidget->setViewMode(viewMode);
        } else {
            recycleWidget->setViewMode(currentViewMode);
        }

        currentPath = Strings::RecycleBinTitle;
        emit pathChanged(Strings::RecycleBinTitle);
        addToHistory(Strings::RecycleBinTitle);
#else
        QMessageBox::information(this, Strings::Info,
                               "Корзина доступна только в Windows");
#endif
    } else {
        navigateTo(path);
    }
}

void FileSystemTab::onMiddleClickOnHomePageFolder(const QString &path)
{
    qDebug() << "Middle click on home page folder:" << path;
    emit middleClickOnFolder(path);
}

void FileSystemTab::showContextMenu(const QPoint &pos)
{
    emit contextMenuRequested(pos);
}

void FileSystemTab::addToHistory(const QString &path)
{
    if (path != currentPath) {
        if (!currentPath.isEmpty()) {
            backHistory.append(currentPath);
        }

        forwardHistory.clear();
        currentPath = path;

        emit navigationStateChanged();
    }
}

void FileSystemTab::navigateTo(const QString &path)
{
    closeRecycleBin();

    QString normalizedPath = QDir::cleanPath(path);
    QModelIndex index = model->index(normalizedPath);

    if (index.isValid()) {
        listView->setRootIndex(index);
        treeView->setRootIndex(index);
        thumbnailView->setRootIndex(index);
        model->setRootPath(normalizedPath);

        homePageWidget->hide();

        // Загружаем сохраненный режим просмотра для этого пути
        loadSavedViewMode();

        // Применяем режим просмотра
        applyViewMode(currentViewMode);

        addToHistory(QDir::toNativeSeparators(normalizedPath));
        emit pathChanged(QDir::toNativeSeparators(normalizedPath));
        currentPath = normalizedPath;
    } else {
        qDebug() << "Invalid index for path:" << normalizedPath;
        QDir dir(normalizedPath);
        if (dir.exists()) {
            listView->setRootIndex(model->index(normalizedPath));
            treeView->setRootIndex(model->index(normalizedPath));
            thumbnailView->setRootIndex(model->index(normalizedPath));
            model->setRootPath(normalizedPath);

            homePageWidget->hide();

            // Загружаем сохраненный режим просмотра для этого пути
            loadSavedViewMode();

            // Применяем режим просмотра
            applyViewMode(currentViewMode);

            addToHistory(QDir::toNativeSeparators(normalizedPath));
            emit pathChanged(QDir::toNativeSeparators(normalizedPath));
            currentPath = normalizedPath;
        }
    }
}

void FileSystemTab::refresh()
{
#ifdef Q_OS_WIN
    RecycleBinWidget *recycleBin = findChild<RecycleBinWidget*>();
    if (recycleBin && recycleBin->isVisible()) {
        recycleBin->refresh();
        return;
    }
#endif

    QString currentPath = model->rootPath();
    if (currentPath.isEmpty()) {
        showHomePage();
    } else {
        model->setRootPath("");
        model->setRootPath(currentPath);
    }
}

QString FileSystemTab::getCurrentPath() const
{
    if (isRecycleBinActive()) {
        return Strings::RecycleBinTitle;
    }

    QString path = model->rootPath();
    return path.isEmpty() ? Strings::HomePage : path;
}

QString FileSystemTab::getTabName() const
{
    if (isRecycleBinActive()) {
        return Strings::RecycleBinTitle;
    }

    QString path = model->rootPath();
    if (path.isEmpty()) return Strings::HomePage;

    QDir dir(path);
    return dir.dirName().isEmpty() ? path : dir.dirName();
}

QAbstractItemView* FileSystemTab::getCurrentView() const
{
    if (isRecycleBinActive()) {
        return nullptr;
    }

    switch (currentViewMode) {
        case 0: return listView;
        case 1: return treeView;
        case 2: return thumbnailView;
        default: return listView;
    }
}

QStringList FileSystemTab::getSelectedFiles() const
{
    if (isRecycleBinActive()) {
        return QStringList();
    }

    QAbstractItemView *view = getCurrentView();
    if (!view) return QStringList();

    QModelIndexList selectedIndexes = view->selectionModel()->selectedIndexes();
    QStringList files;
    for (const QModelIndex &index : selectedIndexes) {
        QString path = model->filePath(index);
        files << QDir::cleanPath(path);
    }
    return files;
}

MainWindow* FileSystemTab::getMainWindow() const
{
    return qobject_cast<MainWindow*>(window());
}

bool FileSystemTab::canGoBack() const
{
    return !backHistory.isEmpty();
}

bool FileSystemTab::canGoForward() const
{
    return !forwardHistory.isEmpty();
}

void FileSystemTab::goBack()
{
    if (canGoBack()) {
        closeRecycleBin();

        forwardHistory.prepend(currentPath);
        QString previousPath = backHistory.takeLast();
        currentPath = previousPath;

        if (previousPath == Strings::HomePage) {
            showHomePage();
        } else {
            QModelIndex index = model->index(previousPath);
            if (index.isValid()) {
                listView->setRootIndex(index);
                treeView->setRootIndex(index);
                thumbnailView->setRootIndex(index);
                model->setRootPath(previousPath);
                homePageWidget->hide();
                applyViewMode(currentViewMode);
                emit pathChanged(QDir::toNativeSeparators(previousPath));
            }
        }

        emit navigationStateChanged();
    }
}

void FileSystemTab::goForward()
{
    if (canGoForward()) {
        closeRecycleBin();

        backHistory.append(currentPath);
        QString nextPath = forwardHistory.takeFirst();
        currentPath = nextPath;

        if (nextPath == Strings::HomePage) {
            showHomePage();
        } else {
            QModelIndex index = model->index(nextPath);
            if (index.isValid()) {
                listView->setRootIndex(index);
                treeView->setRootIndex(index);
                thumbnailView->setRootIndex(index);
                model->setRootPath(nextPath);
                homePageWidget->hide();
                applyViewMode(currentViewMode);
                emit pathChanged(QDir::toNativeSeparators(nextPath));
            }
        }

        emit navigationStateChanged();
    }
}

void FileSystemTab::applyViewMode(int mode)
{
#ifdef Q_OS_WIN
    RecycleBinWidget *recycleBin = findChild<RecycleBinWidget*>();
    if (recycleBin && recycleBin->isVisible()) {
        recycleBin->setViewMode(mode);
        return;
    }
#endif

    listView->hide();
    treeView->hide();
    thumbnailView->hide();

    switch (mode) {
    case 0:
        listView->setIconSize(IconSizes::ListView::Default);
        listView->show();
        break;
    case 1:
        treeView->setIconSize(IconSizes::TreeView::Default);
        treeView->show();
        break;
    case 2:
        thumbnailView->setIconSize(IconSizes::ThumbnailView::Default);
        thumbnailView->show();
        break;
    }
}

bool FileSystemTab::isDrivePath(const QString &path)
{
    QDir dir(path);
    return dir.isRoot();
}

void FileSystemTab::closeRecycleBin()
{
#ifdef Q_OS_WIN
    RecycleBinWidget *recycleWidget = findChild<RecycleBinWidget*>();
    if (recycleWidget) {
        recycleWidget->hide();
        recycleWidget->deleteLater();

        homePageWidget->show();
        listView->hide();
        treeView->hide();
        thumbnailView->hide();
        applyViewMode(currentViewMode);
    }
#endif
}