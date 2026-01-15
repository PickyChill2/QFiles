// mainwindow.cpp
#include "mainwindow.h"
#include "contextmenu.h"
#include "thumbnailview.h"
#include "quickaccesswidget.h"
#include "filesystemtab.h"
#include "strings.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>
#include <QMessageBox>
#include <QFileDialog>
#include <QSettings>
#include <QCloseEvent>
#include <QFileInfo>
#include <QStorageInfo>
#include <QDebug>
#include <QKeySequence>
#include <QShortcut>
#include <QKeyEvent>
#include <QTabBar>
#include <QMouseEvent>
#include <QMenu>
#include <QDesktopServices>
#include <QTimer>
#include <QInputDialog>
#include <QFrame>
#include <QProgressBar>
#include <QPushButton>
#include <QLabel>
#include <QGraphicsOpacityEffect>
#include <qpropertyanimation.h>
#include <QDateTime>

#include "colors.h"
#include "recyclebinwidget.h"
#include "notificationmanager.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , tabWidget(new QTabWidget(this))
    , model(new QFileSystemModel(this))
    , pathEdit(new QLineEdit(this))
    , buttonToolBar(new QToolBar(this))
    , pathToolBar(new QToolBar(this))
    , contextMenu(new ContextMenu(this))
    , fileOperations(new FileOperations(this))
    , searchWidget(new SearchWidget(this))
    , folderViewSettings(new FolderViewSettings(this))
    , m_archiver(new Archiver(this))
    , m_notificationManager(nullptr)
    , favoritesAction(nullptr)
    , sortTypeComboBox(new QComboBox(this))
    , sortOrderButton(new QToolButton(this))
{
    setupUI();
    setupAddTabButton();
    setupConnections();
    setupShortcuts();
    setupSearchWidget();
    setupUndoShortcut();
    setupNotificationManager();

    QSettings settings;
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());

    showHiddenFiles = settings.value("ShowHiddenFiles", false).toBool();
    showHiddenAction->setChecked(showHiddenFiles);

    newTab();

    QTimer::singleShot(100, this, &MainWindow::updateAddTabButtonPosition);
}

MainWindow::~MainWindow()
{
    if (m_notificationManager) {
        m_notificationManager->closeAllNotifications();
        delete m_notificationManager;
    }

    // Удаляем кнопку "+"
    if (addTabButton) {
        delete addTabButton;
    }
}

void MainWindow::setupUI()
{
    setWindowTitle(Strings::AppName);
    resize(1000, 700);

    // Создаем центральный виджет и основной вертикальный layout
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Создаем первый тулбар для кнопок
    buttonToolBar = new QToolBar(this);
    buttonToolBar->setObjectName("buttonToolBar");
    buttonToolBar->setIconSize(QSize(16, 16));

    // Создаем действия, если они еще не созданы
    backAction = buttonToolBar->addAction(Strings::Back);
    forwardAction = buttonToolBar->addAction(Strings::Forward);
    upAction = buttonToolBar->addAction(Strings::Up);
    buttonToolBar->addSeparator();
    homeAction = buttonToolBar->addAction(Strings::Home);

    favoritesAction = new FavoritesAction(this);
    buttonToolBar->addAction(favoritesAction);
    buttonToolBar->addSeparator();

    refreshAction = buttonToolBar->addAction(Strings::Refresh);
    viewModeAction = buttonToolBar->addAction(Strings::ListView);
    newFolderAction = buttonToolBar->addAction(Strings::NewFolder);
    renameAction = buttonToolBar->addAction(Strings::Rename);
    buttonToolBar->addSeparator();

    // Добавляем элементы сортировки
    buttonToolBar->addWidget(new QLabel("Сортировка:", this));

    // Настраиваем комбобокс для типа сортировки
    sortTypeComboBox->addItem("По имени", 0);
    sortTypeComboBox->addItem("По размеру", 1);
    sortTypeComboBox->addItem("По типу", 2);
    sortTypeComboBox->addItem("По дате", 3);
    sortTypeComboBox->setMaximumWidth(120);
    buttonToolBar->addWidget(sortTypeComboBox);

    // Настраиваем кнопку порядка сортировки
    sortOrderButton->setText("↑");
    sortOrderButton->setToolTip("Порядок сортировки");
    sortOrderButton->setMaximumWidth(30);
    buttonToolBar->addWidget(sortOrderButton);

    buttonToolBar->addSeparator();
    newTabAction = buttonToolBar->addAction(Strings::MakeNewTab);
    closeTabAction = buttonToolBar->addAction(Strings::CloseTab);
    buttonToolBar->addSeparator();
    showHiddenAction = buttonToolBar->addAction(Strings::Hidden);
    showHiddenAction->setCheckable(true);

    // Создаем второй тулбар для пути
    pathToolBar = new QToolBar(this);
    pathToolBar->setObjectName("pathToolBar");

    QLabel *pathLabel = new QLabel(Strings::Path + ":", this);
    pathToolBar->addWidget(pathLabel);
    pathToolBar->addWidget(pathEdit);

    // Настраиваем QTabWidget для отображения кнопок закрытия
    tabWidget->setTabsClosable(true);
    tabWidget->setMovable(true);

    // Настраиваем стиль для кнопок закрытия с символом ☠️
    tabWidget->setStyleSheet(
        "QTabBar::close-button {"
        "    subcontrol-position: right;"
        "    margin: 2px;"
        "    padding: 2px;"
        "}"
        "QTabBar::close-button:hover {"
        "    background-color: #a7000e;"
        "    border-radius: 2px;"
        "}"
    );

    auto createStyledSeparator = [this]() {
        QFrame *separator = new QFrame(this);
        separator->setFixedHeight(3);
        separator->setStyleSheet(
            QString(
                "QFrame {"
                "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
                "        stop:0 %1, stop:0.5 %2, stop:1 %1);"
                "    margin: 1px 2px;"
                "}"
            ).arg(Colors::DarkTertiary, Colors::DarkQuaternary)
        );
        return separator;
    };

    // Добавляем оба тулбара в основной layout
    mainLayout->addWidget(buttonToolBar);
    mainLayout->addWidget(createStyledSeparator());
    mainLayout->addWidget(pathToolBar);
    mainLayout->addWidget(createStyledSeparator());
    mainLayout->addWidget(tabWidget);
    setCentralWidget(centralWidget);

    updateNavigationButtons();
    updateSortControls();
}

void MainWindow::setupAddTabButton()
{
    // Создаем кнопку "+" и делаем ее дочерним элементом QTabWidget
    addTabButton = new QPushButton("+", tabWidget);

    // Используем ту же высоту, что и у вкладок (берем из tabBar)
    int tabBarHeight = tabWidget->tabBar()->height();
    if (tabBarHeight > 0) {
        addTabButton->setFixedSize(25, tabBarHeight - 2); // Немного меньше для лучшего вида
    } else {
        addTabButton->setFixedSize(25, 20); // Значение по умолчанию
    }

    // Удаляем старый стиль и используем минимальный стиль, чтобы кнопка выглядела как вкладка
    addTabButton->setStyleSheet("");

    // Используем палитру вкладок для кнопки
    QPalette tabPalette = tabWidget->tabBar()->palette();
    QPalette buttonPalette = addTabButton->palette();

    // Копируем основные цвета из палитры вкладок
    buttonPalette.setColor(QPalette::Button, tabPalette.color(QPalette::Button));
    buttonPalette.setColor(QPalette::ButtonText, tabPalette.color(QPalette::ButtonText));
    buttonPalette.setColor(QPalette::Highlight, tabPalette.color(QPalette::Highlight));

    addTabButton->setPalette(buttonPalette);
    addTabButton->setAutoFillBackground(true);

    // Устанавливаем базовые свойства для имитации вкладки
    addTabButton->setFlat(true); // Делаем кнопку плоской как вкладка
    addTabButton->setFocusPolicy(Qt::NoFocus);

    // Устанавливаем подсказку
    addTabButton->setToolTip("Новая вкладка");

    // Подключаем сигнал
    connect(addTabButton, &QPushButton::clicked, this, &MainWindow::newTab);

    // Устанавливаем event filter для отслеживания изменений геометрии
    tabWidget->tabBar()->installEventFilter(this);

    // Изначально позиционируем кнопку
    updateAddTabButtonPosition();
}

void MainWindow::updateAddTabButtonPosition()
{
    if (!addTabButton) return;

    QTabBar *tabBar = tabWidget->tabBar();

    // Ждем, пока вкладки будут готовы к отображению
    if (!tabBar->isVisible()) {
        QTimer::singleShot(50, this, &MainWindow::updateAddTabButtonPosition);
        return;
    }

    // Обновляем размер кнопки в соответствии с текущей высотой вкладок
    int tabBarHeight = tabBar->height();
    if (tabBarHeight > 0) {
        addTabButton->setFixedHeight(tabBarHeight - 2);
    }

    // Вычисляем общую ширину всех вкладок
    int totalTabsWidth = 0;
    for (int i = 0; i < tabBar->count(); ++i) {
        totalTabsWidth += tabBar->tabRect(i).width();
    }

    // Получаем ширину области вкладок
    int tabBarWidth = tabBar->width();

    // Если вкладки не помещаются, QTabWidget добавляет кнопку прокрутки
    // Мы должны это учесть
    if (totalTabsWidth > tabBarWidth && tabBarWidth > 0) {
        // Вкладки прокручиваются, размещаем кнопку в правой части
        addTabButton->move(tabBarWidth - addTabButton->width() + 25,
                          (tabBar->height() - addTabButton->height()) / 2);
    } else {
        // Вкладки помещаются, размещаем после последней
        int lastTabIndex = tabBar->count() - 1;

        if (lastTabIndex >= 0) {
            QRect lastTabRect = tabBar->tabRect(lastTabIndex);
            int x = lastTabRect.right() + 25;
            int y = (tabBar->height() - addTabButton->height()) / 2;

            // Проверяем, чтобы кнопка не выходила за границы
            if (x + addTabButton->width() > tabBarWidth - 10) {
                x = tabBarWidth - addTabButton->width() + 25;
            }

            addTabButton->move(x, y);
        } else {
            // Нет вкладок
            addTabButton->move(5, (tabBar->height() - addTabButton->height()) / 2);
        }
    }

    addTabButton->show();
    addTabButton->raise();
}

void MainWindow::openInNewTab(const QString &path)
{
    if (path.isEmpty()) return;

    // Создаем новую вкладку через существующий в коде механизм
    newTab();

    // Получаем последнюю созданную вкладку
    FileSystemTab *tab = getTab(tabWidget->currentIndex());
    if (tab) {
        tab->navigateTo(path);
    }

    // Активируем окно
    this->show();
    this->raise();
    this->activateWindow();
    if (this->windowState() & Qt::WindowMinimized) {
        this->setWindowState(this->windowState() & ~Qt::WindowMinimized);
    }
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);

    // После отображения окна синхронизируем стиль кнопки со вкладками
    if (addTabButton) {
        QPalette tabPalette = tabWidget->tabBar()->palette();
        QPalette buttonPalette = addTabButton->palette();
        buttonPalette.setColor(QPalette::Button, tabPalette.color(QPalette::Button));
        buttonPalette.setColor(QPalette::ButtonText, tabPalette.color(QPalette::ButtonText));
        buttonPalette.setColor(QPalette::Highlight, tabPalette.color(QPalette::Highlight));
        addTabButton->setPalette(buttonPalette);
    }
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    // Отслеживаем изменения геометрии панели вкладок
    if (watched == tabWidget->tabBar()) {
        if (event->type() == QEvent::Resize ||
            event->type() == QEvent::Move ||
            event->type() == QEvent::LayoutRequest ||
            event->type() == QEvent::StyleChange) { // Добавляем отслеживание изменения стиля
            updateAddTabButtonPosition();

            // При изменении стиля обновляем палитру кнопки
            if (event->type() == QEvent::StyleChange && addTabButton) {
                QPalette tabPalette = tabWidget->tabBar()->palette();
                QPalette buttonPalette = addTabButton->palette();
                buttonPalette.setColor(QPalette::Button, tabPalette.color(QPalette::Button));
                buttonPalette.setColor(QPalette::ButtonText, tabPalette.color(QPalette::ButtonText));
                addTabButton->setPalette(buttonPalette);
            }
            }

        // Также обрабатываем изменение палитры
        if (event->type() == QEvent::PaletteChange && addTabButton) {
            QPalette tabPalette = tabWidget->tabBar()->palette();
            QPalette buttonPalette = addTabButton->palette();
            buttonPalette.setColor(QPalette::Button, tabPalette.color(QPalette::Button));
            buttonPalette.setColor(QPalette::ButtonText, tabPalette.color(QPalette::ButtonText));
            addTabButton->setPalette(buttonPalette);
        }
    }

    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::setupNotificationManager()
{
    m_notificationManager = new NotificationManager(this);
}

void MainWindow::setupConnections()
{
    connect(tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabCurrentChanged);
    connect(tabWidget, &QTabWidget::tabCloseRequested, this, &MainWindow::onTabCloseRequested);
    connect(tabWidget->tabBar(), &QTabBar::customContextMenuRequested, this, &MainWindow::onTabContextMenuRequested);

    connect(backAction, &QAction::triggered, this, &MainWindow::navigateBack);
    connect(forwardAction, &QAction::triggered, this, &MainWindow::navigateForward);
    connect(upAction, &QAction::triggered, this, &MainWindow::navigateUp);
    connect(homeAction, &QAction::triggered, this, &MainWindow::showHomePage);

    // Исправлено: подключение сигнала избранного
    if (favoritesAction) {
        connect(favoritesAction, &FavoritesAction::favoriteClicked, this, &MainWindow::navigateTo);
        connect(favoritesAction, &FavoritesAction::favoriteMiddleClicked, this, &MainWindow::onFavoriteMiddleClicked);
    }

    connect(refreshAction, &QAction::triggered, this, &MainWindow::refresh);
    connect(viewModeAction, &QAction::triggered, this, &MainWindow::changeViewMode);
    connect(newFolderAction, &QAction::triggered, this, &MainWindow::createNewFolder);
    connect(renameAction, &QAction::triggered, this, &MainWindow::renameSelectedItem);

    // Подключаем элементы сортировки
    connect(sortTypeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onSortTypeChanged);
    connect(sortOrderButton, &QToolButton::clicked, this, &MainWindow::toggleSortOrder);

    connect(newTabAction, &QAction::triggered, this, &MainWindow::newTab);
    connect(closeTabAction, &QAction::triggered, this, &MainWindow::closeCurrentTab);
    connect(showHiddenAction, &QAction::toggled, this, &MainWindow::toggleHiddenFiles);
    connect(pathEdit, &QLineEdit::returnPressed, this, &MainWindow::onPathEdited);

    connect(searchWidget, &SearchWidget::navigateToContainingFolder, this, &MainWindow::onNavigateToContainingFolder);
    connect(searchWidget, &SearchWidget::navigateToFile, this, &MainWindow::onNavigateToFile);

    connect(fileOperations, &FileOperations::operationCompleted, this, &MainWindow::onFileOperationCompleted);
    connect(fileOperations, &FileOperations::requestRefresh, this, &MainWindow::refresh);

    // Подключаем сигналы уведомлений от FileOperations
    connect(fileOperations, &FileOperations::notificationRequested,
            this, &MainWindow::onNotificationRequested);
    connect(fileOperations, &FileOperations::progressNotificationRequested,
        this, [this](const QString &operationId, const QString &title, const QString &message, int progress) {
            if (m_notificationManager) {
                m_notificationManager->showProgressNotification(operationId, title, message, progress);
            }
        });

connect(fileOperations, &FileOperations::finishProgressNotification,
        this, [this](const QString &operationId, const QString &title, const QString &message, int type) {
            if (m_notificationManager) {
                m_notificationManager->finishProgressNotification(operationId, title, message,
                    static_cast<NotificationWidget::Type>(type));
            }
        });

// Подключаем сигналы архиватора через новую систему
connect(m_archiver, &Archiver::progressChanged,
        this, [this](const QString &fileName, int percent) {
            // Используем фиксированный ID для архивации
            if (m_notificationManager) {
                m_notificationManager->updateProgressNotification("archive_operation", fileName, percent);
            }
        });

    connect(m_archiver, &Archiver::operationStarted,
            this, [this](const QString &operation) {
                if (m_notificationManager) {
                    m_notificationManager->showProgressNotification("archive_operation",
                        "Архивация", operation, 0);
                }
            });

    connect(m_archiver, &Archiver::operationFinished,
            this, [this](const QString &operation, bool success, const QString &message) {
                if (m_notificationManager) {
                    NotificationWidget::Type type = success ? NotificationWidget::Success : NotificationWidget::Error;
                    // Используем ту же operationId, что и при старте
                    m_notificationManager->finishProgressNotification("archive_operation",
                        success ? "Архивация завершена" : "Ошибка архивации",
                        message, type);
                }
            });

connect(m_archiver, &Archiver::errorOccurred,
        this, [this](const QString &error) {
            if (m_notificationManager) {
                m_notificationManager->finishProgressNotification("archive_operation", "Ошибка архивации", error,
                    NotificationWidget::Error);
            }
        });
}

void MainWindow::setupShortcuts()
{
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_T), this, SLOT(newTab()));
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_W), this, SLOT(closeCurrentTab()));
    new QShortcut(QKeySequence(Qt::Key_F2), this, SLOT(renameSelectedItem()));
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_N), this, SLOT(createNewFolder()));
    new QShortcut(QKeySequence::Delete, this, SLOT(deleteSelectedItems()));
    new QShortcut(QKeySequence::Copy, this, SLOT(copySelectedItems()));
    new QShortcut(QKeySequence::Cut, this, SLOT(cutSelectedItems()));
    new QShortcut(QKeySequence::Paste, this, SLOT(pasteFiles()));
    new QShortcut(QKeySequence::Back, this, SLOT(navigateBack()));
    new QShortcut(QKeySequence::Forward, this, SLOT(navigateForward()));
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_H), this, SLOT(toggleHiddenFiles()));
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_F), this, SLOT(showSearchWidget()));

    // Добавляем горячие клавиши для сортировки
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_1), this, SLOT(sortByName()));
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_2), this, SLOT(sortBySize()));
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_3), this, SLOT(sortByDate()));
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_4), this, SLOT(sortByType()));
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_R), this, SLOT(toggleSortOrder()));
}

void MainWindow::setupUndoShortcut()
{
    QShortcut *undoShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Z), this);
    connect(undoShortcut, &QShortcut::activated, this, &MainWindow::undoDelete);

    QShortcut *altUndoShortcut = new QShortcut(QKeySequence(Qt::ALT | Qt::Key_Backspace), this);
    connect(altUndoShortcut, &QShortcut::activated, this, &MainWindow::undoDelete);
}

void MainWindow::setupSearchWidget()
{
    searchWidget->hide();
    connect(searchWidget, &SearchWidget::searchRequested, this, &MainWindow::onSearchRequested);
    connect(searchWidget, &SearchWidget::closed, this, &MainWindow::onSearchClosed);
    connect(searchWidget, &SearchWidget::resultSelected, this, &MainWindow::onSearchResultSelected);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    QMainWindow::keyPressEvent(event);
}

void MainWindow::toggleHiddenFiles()
{
    showHiddenFiles = showHiddenAction->isChecked();

    QSettings settings;
    settings.setValue("ShowHiddenFiles", showHiddenFiles);

    for (int i = 0; i < tabWidget->count(); ++i) {
        FileSystemTab *tab = getTab(i);
        if (tab) {
            QFileSystemModel *model = tab->getFileSystemModel();
            if (model) {
                if (showHiddenFiles) {
                    model->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Hidden);
                } else {
                    model->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::AllDirs);
                }
                tab->refresh();
            }
        }
    }
}

void MainWindow::saveViewModeForPath(const QString &path, int mode)
{
    folderViewSettings->saveViewMode(path, mode);
    qDebug() << "Saved view mode for" << path << ":" << mode;
}

int MainWindow::loadViewModeForPath(const QString &path)
{
    return folderViewSettings->loadViewMode(path);
}

void MainWindow::updateViewModeForCurrentPath()
{
    FileSystemTab *currentTab = getCurrentTab();
    if (currentTab) {
        QString currentPath = currentTab->getCurrentPath();

        // Для домашней страницы не загружаем сохраненный режим
        if (currentPath == Strings::HomePage) {
            return; // Домашняя страница всегда в режиме дерева
        }

        int savedMode = loadViewModeForPath(currentPath);

        // Применяем режим только если он изменился
        if (currentTab->getViewMode() != savedMode) {
            currentTab->setViewMode(savedMode, false); // Не сохраняем при загрузке
            currentViewMode = savedMode;

            // Обновляем текст кнопки
            switch (currentViewMode) {
                case 0: viewModeAction->setText(Strings::ListView); break;
                case 1: viewModeAction->setText(Strings::TreeView); break;
                case 2: viewModeAction->setText(Strings::ThumbnailView); break;
            }

            qDebug() << "Applied view mode" << savedMode << "for path:" << currentPath;
        }
    }
}

void MainWindow::newTab()
{
    FileSystemTab *newTab = new FileSystemTab(this);
    int index = tabWidget->addTab(newTab, Strings::HomePage);
    tabWidget->setCurrentIndex(index);

    connect(newTab, &FileSystemTab::pathChanged, this, &MainWindow::onTabPathChanged);
    connect(newTab, &FileSystemTab::itemDoubleClicked, this, &MainWindow::onItemDoubleClicked);
    connect(newTab, &FileSystemTab::contextMenuRequested, this, &MainWindow::showContextMenu);
    connect(newTab, &FileSystemTab::middleClickOnFolder, this, &MainWindow::onMiddleClickOnFolder);
    connect(newTab, &FileSystemTab::navigationStateChanged, this, &MainWindow::updateNavigationButtons);
    connect(newTab, &FileSystemTab::sortChanged, this, &MainWindow::onSortChanged);

    // Подключаем сигнал изменения режима просмотра
    connect(newTab, &FileSystemTab::viewModeChanged, this, [this, newTab](int mode) {
        // Сохраняем режим для текущего пути вкладки
        QString currentPath = newTab->getCurrentPath();
        saveViewModeForPath(currentPath, mode);

        // Обновляем кнопку в главном окне
        currentViewMode = mode;
        switch (currentViewMode) {
            case 0: viewModeAction->setText(Strings::ListView); break;
            case 1: viewModeAction->setText(Strings::TreeView); break;
            case 2: viewModeAction->setText(Strings::ThumbnailView); break;
        }
    });

    contextMenu->setParentTab(newTab);
    contextMenu->setFileOperations(fileOperations);

    connect(contextMenu, &ContextMenu::navigateRequested, newTab, &FileSystemTab::navigateTo);
    connect(contextMenu, &ContextMenu::refreshRequested, newTab, &FileSystemTab::refresh);

    updateCurrentTabInfo();
    updateNavigationButtons();
    updateViewModeForCurrentPath();
    updateSortControls();
    updateAddTabButtonPosition();

    if (newTab->getFileSystemModel()) {
        if (showHiddenFiles) {
            newTab->getFileSystemModel()->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Hidden);
        } else {
            newTab->getFileSystemModel()->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::AllDirs);
        }
    }

    QTimer::singleShot(100, this, &MainWindow::updateAddTabButtonPosition);
}

void MainWindow::closeCurrentTab()
{
    if (tabWidget->count() > 1) {
        int currentIndex = tabWidget->currentIndex();
        tabWidget->removeTab(currentIndex);
        updateAddTabButtonPosition();
    } else {
        close();
    }
}

void MainWindow::loadSortSettingsForPath(const QString &path, int &sortColumn, Qt::SortOrder &sortOrder)
{
    folderViewSettings->loadSortSettings(path, sortColumn, sortOrder);
}

void MainWindow::saveSortSettingsForPath(const QString &path, int sortColumn, Qt::SortOrder sortOrder)
{
    folderViewSettings->saveSortSettings(path, sortColumn, sortOrder);
}

void MainWindow::onSortTypeChanged(int index)
{
    FileSystemTab *currentTab = getCurrentTab();
    if (!currentTab) return;

    int sortColumn = sortTypeComboBox->itemData(index).toInt();
    Qt::SortOrder sortOrder = currentTab->getSortOrder();

    currentTab->setSorting(sortColumn, sortOrder);
    saveSortSettingsForPath(currentTab->getCurrentPath(), sortColumn, sortOrder);
    updateSortControls();
}

void MainWindow::toggleSortOrder()
{
    FileSystemTab *currentTab = getCurrentTab();
    if (!currentTab) return;

    Qt::SortOrder newOrder = (currentTab->getSortOrder() == Qt::AscendingOrder)
        ? Qt::DescendingOrder : Qt::AscendingOrder;

    int sortColumn = currentTab->getSortColumn();
    currentTab->setSorting(sortColumn, newOrder);
    saveSortSettingsForPath(currentTab->getCurrentPath(), sortColumn, newOrder);
    updateSortControls();
}

void MainWindow::updateSortControls()
{
    FileSystemTab *currentTab = getCurrentTab();
    if (!currentTab) return;

    int sortColumn = currentTab->getSortColumn();
    Qt::SortOrder sortOrder = currentTab->getSortOrder();

    // Устанавливаем текущий тип сортировки в комбобокс
    int index = sortTypeComboBox->findData(sortColumn);
    if (index >= 0) {
        sortTypeComboBox->setCurrentIndex(index);
    }

    // Обновляем иконку порядка сортировки
    sortOrderButton->setText(sortOrder == Qt::AscendingOrder ? "↑" : "↓");
}

void MainWindow::onSortChanged(int column, Qt::SortOrder order)
{
    Q_UNUSED(column)
    Q_UNUSED(order)
    updateSortControls();
}


void MainWindow::onTabCurrentChanged(int index)
{
    if (index >= 0) {
        QWidget *currentWidget = tabWidget->widget(index);
        RecycleBinWidget *recycleBin = qobject_cast<RecycleBinWidget*>(currentWidget);

        if (recycleBin) {
            // Если переключились на корзину, обновляем навигационные кнопки соответствующим образом
            backAction->setEnabled(false);
            forwardAction->setEnabled(false);
            upAction->setEnabled(false);
            pathEdit->setText("Корзина");
            qDebug() << "Switched to recycle bin tab";
        } else {
            updateCurrentTabInfo();
            updateNavigationButtons();
            updateViewModeForCurrentPath();
            updateSortControls(); // Обновляем элементы сортировки
        }
    }

    // Обновляем позицию кнопки "+" при смене вкладки
    updateAddTabButtonPosition();
}

void MainWindow::onTabCloseRequested(int index)
{
    if (tabWidget->count() > 1) {
        tabWidget->removeTab(index);
        updateAddTabButtonPosition();
    } else {
        close();
    }
}

void MainWindow::onTabContextMenuRequested(const QPoint &pos)
{
    QMenu menu;
    QAction *closeAction = menu.addAction("Закрыть вкладку");

    QAction *selectedAction = menu.exec(tabWidget->tabBar()->mapToGlobal(pos));
    if (selectedAction == closeAction) {
        int tabIndex = tabWidget->tabBar()->tabAt(pos);
        if (tabIndex >= 0) {
            onTabCloseRequested(tabIndex);
        }
    }
}

void MainWindow::onTabPathChanged(const QString &path)
{
    FileSystemTab *tab = qobject_cast<FileSystemTab*>(sender());
    if (tab) {
        int tabIndex = tabWidget->indexOf(tab);
        if (tabIndex >= 0) {
            QString tabName = path;
            if (path == Strings::HomePage) {
                tabName = Strings::HomePage;
            } else {
                QDir dir(path);
                tabName = dir.dirName().isEmpty() ? path : dir.dirName();
                if (tabName.length() > 15) {
                    tabName = tabName.left(12) + "...";
                }
            }
            tabWidget->setTabText(tabIndex, tabName);
            tabWidget->setTabToolTip(tabIndex, path);

            if (tabWidget->currentWidget() == tab) {
                pathEdit->setText(path);
            }
        }
    }

    QTimer::singleShot(100, this, &MainWindow::updateViewModeForCurrentPath);
    updateSortControls(); // Обновляем элементы сортировки при смене пути
}

void MainWindow::onMiddleClickOnFolder(const QString &path)
{
    newTab();
    FileSystemTab *newTabWidget = getCurrentTab();
    if (newTabWidget) {
        newTabWidget->navigateTo(path);
        qDebug() << "Middle click: opened folder in new tab:" << path;
    }
}

void MainWindow::onQuickAccessFolderClicked(const QString &path)
{
    navigateTo(path);
}

QString MainWindow::getCurrentPath() const
{
    FileSystemTab *currentTab = getCurrentTab();
    if (currentTab) {
        QString path = currentTab->getCurrentPath();
        return (path == Strings::HomePage) ? QDir::homePath() : path;
    }
    return QDir::homePath();
}

FileSystemTab* MainWindow::getCurrentTab() const
{
    return qobject_cast<FileSystemTab*>(tabWidget->currentWidget());
}

FileSystemTab* MainWindow::getTab(int index) const
{
    return qobject_cast<FileSystemTab*>(tabWidget->widget(index));
}

void MainWindow::updateCurrentTabInfo()
{
    FileSystemTab *currentTab = getCurrentTab();
    if (currentTab) {
        QString currentPath = currentTab->getCurrentPath();
        pathEdit->setText(currentPath);

        // Устанавливаем свойство для поиска
        setProperty("currentPath", currentPath);

        currentViewMode = currentTab->getViewMode();

        switch (currentViewMode) {
            case 0: viewModeAction->setText(Strings::ListView); break;
            case 1: viewModeAction->setText(Strings::TreeView); break;
            case 2: viewModeAction->setText(Strings::ThumbnailView); break;
        }

        contextMenu->setParentTab(currentTab);
    }
}

void MainWindow::updateNavigationButtons()
{
    FileSystemTab *currentTab = getCurrentTab();
    if (currentTab) {
        backAction->setEnabled(currentTab->canGoBack());
        forwardAction->setEnabled(currentTab->canGoForward());
    } else {
        backAction->setEnabled(false);
        forwardAction->setEnabled(false);
    }
}

void MainWindow::navigateTo(const QString &path)
{
    // Закрываем корзину при любой навигации
    closeRecycleBin();

    FileSystemTab *currentTab = getCurrentTab();
    if (currentTab) {
        currentTab->navigateTo(path);
        updateNavigationButtons();
    }
}


void MainWindow::navigateBack()
{
    // Закрываем корзину при навигации назад
    closeRecycleBin();

    FileSystemTab *currentTab = getCurrentTab();
    if (currentTab && currentTab->canGoBack()) {
        currentTab->goBack();
        updateNavigationButtons();
    }
}

void MainWindow::navigateForward()
{
    // Закрываем корзину при навигации вперед
    closeRecycleBin();

    FileSystemTab *currentTab = getCurrentTab();
    if (currentTab && currentTab->canGoForward()) {
        currentTab->goForward();
        updateNavigationButtons();
    }
}

void MainWindow::navigateUp()
{
    // Закрываем корзину при навигации вверх
    closeRecycleBin();

    FileSystemTab *currentTab = getCurrentTab();
    if (!currentTab) return;

    QString currentPath = currentTab->getCurrentPath();
    if (currentPath == Strings::HomePage || isDrivePath(currentPath)) {
        currentTab->showHomePage();
    } else {
        QDir currentDir(currentPath);
        if (currentDir.cdUp()) {
            currentTab->navigateTo(currentDir.absolutePath());
        }
    }
    updateNavigationButtons();
}

void MainWindow::closeRecycleBin()
{
    // Ищем вкладку с корзиной и закрываем ее
    for (int i = 0; i < tabWidget->count(); ++i) {
        QWidget *widget = tabWidget->widget(i);
        RecycleBinWidget *recycleBin = qobject_cast<RecycleBinWidget*>(widget);
        if (recycleBin) {
            tabWidget->removeTab(i);
            recycleBin->deleteLater();
            qDebug() << "Recycle bin tab closed";
            break;
        }
    }
}

void MainWindow::showHomePage()
{
    // Закрываем корзину при переходе на домашнюю страницу
    closeRecycleBin();

    FileSystemTab *currentTab = getCurrentTab();
    if (currentTab) {
        currentTab->showHomePage();
        updateNavigationButtons();
    }
}

void MainWindow::refresh()
{
    FileSystemTab *currentTab = getCurrentTab();
    if (currentTab) {
        currentTab->refresh();
    }
}

void MainWindow::changeViewMode()
{
    currentViewMode = (currentViewMode + 1) % 3;
    FileSystemTab *currentTab = getCurrentTab();
    if (currentTab) {
        QString currentPath = currentTab->getCurrentPath();

        // Сохраняем настройки только если это не домашняя страница
        if (currentPath != Strings::HomePage) {
            saveViewModeForPath(currentPath, currentViewMode);
        }

        currentTab->setViewMode(currentViewMode);
        updateCurrentTabInfo();
    }
}

void MainWindow::onPathEdited()
{
    QString path = QDir::fromNativeSeparators(pathEdit->text());

    if (path.isEmpty() || path == Strings::HomePage) {
        showHomePage();
    } else if (QDir(path).exists()) {
        navigateTo(path);
    } else {
        if (m_notificationManager) {
            m_notificationManager->showNotification("Ошибка", Strings::DirectoryError,
                NotificationWidget::Error, true, 3000);
        } else {
            QMessageBox::warning(this, Strings::Error, Strings::DirectoryError);
        }

        FileSystemTab *currentTab = getCurrentTab();
        if (currentTab) {
            pathEdit->setText(currentTab->getCurrentPath());
        }
    }
}

QStringList MainWindow::getSelectedFiles() const
{
    FileSystemTab *currentTab = getCurrentTab();
    return currentTab ? currentTab->getSelectedFiles() : QStringList();
}

void MainWindow::deleteSelectedItems()
{
    QStringList selectedFiles = getSelectedFiles();
    if (!selectedFiles.isEmpty()) {
        fileOperations->deleteFiles(selectedFiles);
    }
}

void MainWindow::cutSelectedItems()
{
    QStringList selectedFiles = getSelectedFiles();
    if (!selectedFiles.isEmpty()) {
        fileOperations->cutFiles(selectedFiles);
    }
}

void MainWindow::copySelectedItems()
{
    QStringList selectedFiles = getSelectedFiles();
    if (!selectedFiles.isEmpty()) {
        fileOperations->copyFiles(selectedFiles);
    }
}

void MainWindow::pasteFiles()
{
    FileSystemTab *currentTab = getCurrentTab();
    if (currentTab) {
        QString currentPath = currentTab->getCurrentPath();
        if (currentPath == Strings::HomePage) {
            currentPath = QDir::homePath();
        }
        fileOperations->pasteFiles(currentPath);
    }
}

void MainWindow::undoDelete()
{
    if (fileOperations && fileOperations->canUndo()) {
        fileOperations->undoDelete();
    }
}

void MainWindow::onItemDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) return;

    FileSystemTab *currentTab = getCurrentTab();
    if (!currentTab) return;

    QFileSystemModel *tabModel = qobject_cast<QFileSystemModel*>(currentTab->getCurrentView()->model());
    if (tabModel && !tabModel->isDir(index)) {
        QString path = tabModel->filePath(index);

        // Проверяем, является ли файл исполняемым
        bool isExecutable = isExecutableFile(path);

        if (isExecutable) {
#ifdef Q_OS_WIN
            // Проверяем, требуется ли запуск от имени администратора
            bool requireElevation = false;

            // Метод 1: Проверяем реестр
            HKEY hKey = nullptr;
            QString keyPath = QString("Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers");
            QString valueName = QDir::toNativeSeparators(path);

            LSTATUS result = RegOpenKeyExW(HKEY_CURRENT_USER,
                                          keyPath.toStdWString().c_str(),
                                          0,
                                          KEY_READ,
                                          &hKey);

            if (result == ERROR_SUCCESS && hKey != nullptr) {
                wchar_t buffer[256] = {0};
                DWORD bufferSize = sizeof(buffer);
                DWORD type = 0;

                result = RegQueryValueExW(hKey,
                                         valueName.toStdWString().c_str(),
                                         nullptr,
                                         &type,
                                         reinterpret_cast<BYTE*>(buffer),
                                         &bufferSize);

                RegCloseKey(hKey);

                if (result == ERROR_SUCCESS && type == REG_SZ) {
                    QString value = QString::fromWCharArray(buffer);
                    requireElevation = value.contains("RUNASADMIN", Qt::CaseInsensitive);
                }
            }

            // Метод 2: Проверяем манифест файла
            if (!requireElevation) {
                HMODULE hModule = LoadLibraryExW(path.toStdWString().c_str(),
                                                 nullptr,
                                                 LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE);

                if (hModule != nullptr) {
                    HRSRC hRes = FindResourceW(hModule, MAKEINTRESOURCE(1), RT_MANIFEST);

                    if (hRes != nullptr) {
                        HGLOBAL hResData = LoadResource(hModule, hRes);
                        if (hResData != nullptr) {
                            DWORD size = SizeofResource(hModule, hRes);
                            LPVOID data = LockResource(hResData);

                            if (data != nullptr && size > 0) {
                                QString manifest = QString::fromUtf16(static_cast<const char16_t*>(data), size / 2);
                                if (manifest.contains("requireAdministrator", Qt::CaseInsensitive) ||
                                    manifest.contains("level=\"requireAdministrator\"", Qt::CaseInsensitive)) {
                                    requireElevation = true;
                                }
                            }
                        }
                    }

                    FreeLibrary(hModule);
                }
            }

            // Если требуется elevation, запускаем с повышенными правами
            if (requireElevation) {
                SHELLEXECUTEINFO sei = { sizeof(sei) };
                sei.lpVerb = L"runas";
                sei.lpFile = path.toStdWString().c_str();
                sei.nShow = SW_NORMAL;

                if (!ShellExecuteEx(&sei)) {
                    DWORD error = GetLastError();
                    if (error == ERROR_CANCELLED) {
                        if (m_notificationManager) {
                            m_notificationManager->showNotification("Отменено",
                                "Запуск от имени администратора отменен пользователем.",
                                NotificationWidget::Info, true, 3000);
                        } else {
                            QMessageBox::information(this, "Отменено",
                                "Запуск от имени администратора отменен пользователем.");
                        }
                    } else {
                        if (m_notificationManager) {
                            m_notificationManager->showNotification("Ошибка",
                                QString("Не удалось запустить программу с правами администратора.\nКод ошибки: %1").arg(error),
                                NotificationWidget::Error, true, 5000);
                        } else {
                            QMessageBox::warning(this, "Ошибка",
                                QString("Не удалось запустить программу с правами администратора.\n"
                                       "Код ошибки: %1").arg(error));
                        }
                    }
                }
                return;
            }
#endif
        }

        // Обычный запуск для неисполняемых файлов или без требований администратора
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    }
}

bool MainWindow::isExecutableFile(const QString &filePath) const
{
    QFileInfo info(filePath);
    QString extension = info.suffix().toLower();
    QStringList executableExtensions = {"exe", "com", "bat", "cmd", "msi", "ps1", "scr", "vbs", "js", "wsf"};

    return executableExtensions.contains(extension);
}

void MainWindow::showContextMenu(const QPoint &pos)
{
    FileSystemTab *currentTab = getCurrentTab();
    if (!currentTab) {
        qDebug() << "ShowContextMenu: No current tab";
        return;
    }

    QAbstractItemView *view = currentTab->getCurrentView();
    if (!view) {
        qDebug() << "ShowContextMenu: No current view";
        return;
    }

    contextMenu->setParentTab(currentTab);

    QModelIndexList selectedIndexes = view->selectionModel()->selectedIndexes();
    QString currentPath = currentTab->getCurrentPath();

    qDebug() << "ShowContextMenu: Selected items:" << selectedIndexes.size()
             << "Current path:" << currentPath;

    contextMenu->setSelection(selectedIndexes, currentPath);
    contextMenu->popup(view->viewport()->mapToGlobal(pos));
}

void MainWindow::onFileOperationCompleted()
{
    QTimer::singleShot(500, this, &MainWindow::delayedRefresh);
}

void MainWindow::delayedRefresh()
{
    softRefresh();
}

void MainWindow::softRefresh()
{
    FileSystemTab *currentTab = getCurrentTab();
    if (currentTab) {
        currentTab->refresh();
    }
}

void MainWindow::createNewFolder()
{
    startCreateNewFolder();
}

void MainWindow::renameSelectedItem()
{
    FileSystemTab *currentTab = getCurrentTab();
    if (!currentTab) return;

    QAbstractItemView *view = currentTab->getCurrentView();
    if (!view) return;

    QModelIndexList selectedIndexes = view->selectionModel()->selectedIndexes();
    if (selectedIndexes.size() == 1) {
        QModelIndex index = selectedIndexes.first();

        // Проверяем, что элемент существует
        QString filePath = index.data(QFileSystemModel::FilePathRole).toString();
        if (!QFileInfo::exists(filePath)) {
            qDebug() << "File does not exist for renaming:" << filePath;
            return;
        }

        // Запрашиваем новое имя через диалог
        QFileInfo fileInfo(filePath);
        bool ok;
        QString newName = QInputDialog::getText(this,
                                              "Переименование",
                                              "Введите новое имя:",
                                              QLineEdit::Normal,
                                              fileInfo.fileName(),
                                              &ok);

        if (ok && !newName.isEmpty() && newName != fileInfo.fileName()) {
            // Выполняем переименование через FileOperations
            fileOperations->renameFile(filePath, newName);
        }
    } else {
        qDebug() << "Rename: No or multiple items selected";
    }
}

void MainWindow::startRename(const QModelIndex &index)
{
    FileSystemTab *currentTab = getCurrentTab();
    if (!currentTab) {
        qDebug() << "startRename: No current tab";
        return;
    }

    QAbstractItemView *view = currentTab->getCurrentView();
    if (!view) {
        qDebug() << "startRename: No current view";
        return;
    }

    // Проверим существование файла
    QString filePath = index.data(QFileSystemModel::FilePathRole).toString();
    if (!QFileInfo::exists(filePath)) {
        qDebug() << "startRename: File does not exist:" << filePath;
        return;
    }

    // Очистим выделение и выберем нужный элемент
    view->selectionModel()->clearSelection();
    view->selectionModel()->select(index, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
    view->setCurrentIndex(index);

    // Даем фокус представлению
    view->setFocus();

    // Запускаем редактирование через метод вкладки
    QTimer::singleShot(50, currentTab, [currentTab]() {
        currentTab->startRename();
    });
}

void MainWindow::startCreateNewFolder()
{
    FileSystemTab *currentTab = getCurrentTab();
    if (!currentTab) return;

    QString currentPath = currentTab->getCurrentPath();
    if (currentPath == Strings::HomePage) {
        currentPath = QDir::homePath();
    }

    if (currentPath.isEmpty()) {
        if (m_notificationManager) {
            m_notificationManager->showNotification("Информация", Strings::NavigateError,
                NotificationWidget::Info, true, 3000);
        } else {
            QMessageBox::information(this, Strings::Info, Strings::NavigateError);
        }
        return;
    }

    QDir dir(currentPath);
    currentPath = dir.absolutePath();

    qDebug() << "Creating folder in:" << currentPath;

    QString newFolderPath;
    int counter = 1;
    do {
        newFolderPath = currentPath + "/New Folder";
        if (counter > 1) {
            newFolderPath += " (" + QString::number(counter) + ")";
        }
        counter++;
    } while (QDir(newFolderPath).exists());

    if (QDir().mkpath(newFolderPath)) {
        qDebug() << "Folder created:" << newFolderPath;

        // Обновляем представление и запускаем переименование
        refresh();

        QTimer::singleShot(500, [this, newFolderPath]() {
            FileSystemTab *currentTab = getCurrentTab();
            if (!currentTab) return;

            QAbstractItemView *view = currentTab->getCurrentView();
            if (!view) return;

            QFileSystemModel *model = qobject_cast<QFileSystemModel*>(view->model());
            if (!model) return;

            // Ищем индекс созданной папки
            QModelIndex newIndex = model->index(newFolderPath);
            if (newIndex.isValid()) {
                startRename(newIndex);
            } else {
                qDebug() << "Failed to find index for new folder:" << newFolderPath;
            }
        });
    } else {
        if (m_notificationManager) {
            m_notificationManager->showNotification("Ошибка", Strings::FolderError,
                NotificationWidget::Error, true, 5000);
        } else {
            QMessageBox::warning(this, Strings::Error, Strings::FolderError);
        }
        qDebug() << "Failed to create folder:" << newFolderPath;
    }
}

bool MainWindow::isDrivePath(const QString &path)
{
    QDir dir(path);
    return dir.isRoot();
}

void MainWindow::applyViewMode(int mode)
{
    currentViewMode = mode;
    FileSystemTab *currentTab = getCurrentTab();
    if (currentTab) {
        currentTab->setViewMode(mode);
    }
}

void MainWindow::updateQuickAccessVisibility()
{
}

void MainWindow::updatePathHistory(const QString &path)
{
    Q_UNUSED(path)
}

void MainWindow::showSearchWidget()
{
    FileSystemTab *currentTab = getCurrentTab();
    if (!currentTab) return;

    QPoint pos = currentTab->mapToGlobal(QPoint(10, 10));
    searchWidget->showAtPosition(pos);
}

void MainWindow::onSearchRequested(const QString &text)
{
    Q_UNUSED(text)
}

void MainWindow::onSearchClosed()
{
    searchWidget->clearSearch();
}

void MainWindow::onSearchResultSelected(const QString &path)
{
    QFileInfo info(path);
    if (info.exists()) {
        if (info.isDir()) {
            navigateTo(path);
        } else {
            QDesktopServices::openUrl(QUrl::fromLocalFile(path));
        }
    }
}

void MainWindow::onNavigateToContainingFolder(const QString &folderPath)
{
    navigateTo(folderPath);
    searchWidget->hide();
}

void MainWindow::onNavigateToFile(const QString &filePath)
{
    QFileInfo info(filePath);
    QString folderPath = info.path();

    // Переходим в папку, содержащую файл
    navigateTo(folderPath);

    // Откладываем выделение файла до тех пор, пока представление не обновится
    QTimer::singleShot(200, this, [this, filePath]() {
        FileSystemTab *currentTab = getCurrentTab();
        if (!currentTab) return;

        QAbstractItemView *view = currentTab->getCurrentView();
        if (!view) return;

        QFileSystemModel *model = currentTab->getFileSystemModel();
        if (!model) return;

        QModelIndex index = model->index(filePath);
        if (index.isValid()) {
            // Выделяем файл
            view->setCurrentIndex(index);
            view->scrollTo(index, QAbstractItemView::PositionAtCenter);

            // Убедимся, что вид имеет фокус
            view->setFocus();
        }
    });

    searchWidget->hide();
}

void MainWindow::onFavoriteMiddleClicked(const QString &path)
{
    // Создаем новую вкладку и переходим по пути
    newTab();
    FileSystemTab *newTabWidget = getCurrentTab();
    if (newTabWidget) {
        newTabWidget->navigateTo(path);
        qDebug() << "Middle click on favorite: opened in new tab:" << path;
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QSettings settings;
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.setValue("ShowHiddenFiles", showHiddenFiles);

    // Очищаем настройки для несуществующих папок
    folderViewSettings->cleanupNonExistentFolders();

    QMainWindow::closeEvent(event);
}

// Notification slots
void MainWindow::onNotificationRequested(const QString &title, const QString &message, int type, bool autoClose, int timeout)
{
    if (m_notificationManager) {
        m_notificationManager->showNotification(title, message,
            static_cast<NotificationWidget::Type>(type), autoClose, timeout);
    }
}

void MainWindow::onProgressNotificationRequested(const QString &title, const QString &message, int progress, bool autoClose)
{
    if (m_notificationManager) {
        QString operationId = QString("%1_%2").arg(title).arg(QDateTime::currentMSecsSinceEpoch());
        m_notificationManager->showProgressNotification(operationId, title, message, progress);
    }
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    // Уведомления сами обновят свои позиции через NotificationManager
}

void MainWindow::moveEvent(QMoveEvent *event)
{
    QMainWindow::moveEvent(event);
    // Уведомления сами обновят свои позиции через NotificationManager
}