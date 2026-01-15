#include "searchwidget.h"
#include "searchworker.h"
#include "styles.h"
#include <QKeyEvent>
#include <QApplication>
#include <QDebug>
#include <QFocusEvent>
#include <QTimer>
#include <QShowEvent>
#include <QScrollBar>
#include <QDir>
#include <QPainter>

SearchWidget::SearchWidget(QWidget *parent)
    : QWidget(parent)
    , searchEdit(new QLineEdit(this))
    , searchButton(new QPushButton("Поиск", this))
    , closeButton(new QPushButton("❌", this))
    , caseSensitiveCheck(new QCheckBox("Учет регистра", this))
    , namesOnlyCheck(new QCheckBox("Только имена", this))
    , searchScopeCombo(new QComboBox(this))
    , resultsList(new QListWidget(this))
    , progressBar(new QProgressBar(this))
    , statusLabel(new QLabel(this))
    , loadingIndicator(new QLabel(this))
    , contextMenu(new QMenu(this))
    , searchWorker(nullptr)
    , searchThread(nullptr)
    , isSearching(false)
{
    setupUI();
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);

    // Устанавливаем минимальные размеры для предотвращения слишком маленького окна
    setMinimumWidth(500);
    setMinimumHeight(200);
}

SearchWidget::~SearchWidget()
{
    stopSearch();
}

void SearchWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(8);

    // Первая строка: поле поиска и кнопки
    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchLayout->setSpacing(6);

    searchEdit->setPlaceholderText("Введите текст для поиска...");
    searchEdit->setMinimumWidth(350);
    searchEdit->setMinimumHeight(30);

    searchButton->setFixedSize(80, 30);
    closeButton->setFixedSize(30, 30);
    closeButton->setObjectName("closeButton");

    searchLayout->addWidget(searchEdit);
    searchLayout->addWidget(searchButton);
    searchLayout->addWidget(closeButton);

    // Вторая строка: опции поиска
    QHBoxLayout *optionsLayout = new QHBoxLayout();
    optionsLayout->setSpacing(15);

    searchScopeCombo->addItem("Текущая папка");
    searchScopeCombo->addItem("Весь диск");
    searchScopeCombo->setMinimumWidth(120);

    optionsLayout->addWidget(caseSensitiveCheck);
    optionsLayout->addWidget(namesOnlyCheck);
    optionsLayout->addWidget(searchScopeCombo);
    optionsLayout->addStretch();

    // Строка состояния и индикатор
    QHBoxLayout *statusLayout = new QHBoxLayout();
    progressBar->setVisible(false);
    progressBar->setTextVisible(false);
    progressBar->setMinimumHeight(4);
    progressBar->setMaximumHeight(4);

    // Настраиваем индикатор загрузки (но мы используем progressBar как indeterminate)
    loadingIndicator->setVisible(false);
    loadingIndicator->setFixedSize(16, 16);
    loadingIndicator->setStyleSheet(
        "background: none;"
        "border: 2px solid #0078d4;"
        "border-radius: 50%;"
        "border-top: 2px solid transparent;"
    );

    statusLabel->setVisible(false);
    statusLabel->setStyleSheet("color: #cccccc; font-size: 11px;");
    statusLabel->setMinimumHeight(16);

    statusLayout->addWidget(loadingIndicator);
    statusLayout->addWidget(statusLabel);
    statusLayout->addStretch();
    statusLayout->addWidget(progressBar);

    // Список результатов
    resultsList->setVisible(false);
    resultsList->setMinimumHeight(120);
    resultsList->setMaximumHeight(400);
    resultsList->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    resultsList->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    resultsList->setContextMenuPolicy(Qt::CustomContextMenu);

    // Устанавливаем минимальную ширину для списка результатов
    resultsList->setMinimumWidth(450);

    mainLayout->addLayout(searchLayout);
    mainLayout->addLayout(optionsLayout);
    mainLayout->addLayout(statusLayout);
    mainLayout->addWidget(resultsList);

    // Подключаем сигналы
    connect(searchButton, &QPushButton::clicked, this, &SearchWidget::onSearchClicked);
    connect(closeButton, &QPushButton::clicked, this, &SearchWidget::onCloseClicked);
    connect(searchEdit, &QLineEdit::returnPressed, this, &SearchWidget::onSearchClicked);
    connect(resultsList, &QListWidget::itemClicked, this, &SearchWidget::onResultClicked);
    connect(resultsList, &QListWidget::customContextMenuRequested, this, &SearchWidget::showResultsContextMenu);

    // Настраиваем контекстное меню
    QAction *showInFolderAction = contextMenu->addAction("📁 Перейти к расположению");
    connect(showInFolderAction, &QAction::triggered, this, &SearchWidget::onShowInContainingFolder);

    // Устанавливаем стили из styles.h
    setStyleSheet(Styles::SearchWidgetStyle);
}

void SearchWidget::showAtPosition(const QPoint &pos)
{
    // Корректируем позицию, чтобы окно не выходило за экран
    QScreen *screen = QApplication::screenAt(pos);
    if (screen) {
        QRect screenGeometry = screen->availableGeometry();
        QPoint adjustedPos = pos;

        if (pos.x() + width() > screenGeometry.right()) {
            adjustedPos.setX(screenGeometry.right() - width() - 10);
        }
        if (pos.y() + height() > screenGeometry.bottom()) {
            adjustedPos.setY(screenGeometry.bottom() - height() - 10);
        }

        move(adjustedPos);
    } else {
        move(pos);
    }

    show();
    activateWindow();
    raise();

    QTimer::singleShot(50, this, [this]() {
        searchEdit->setFocus();
        searchEdit->selectAll();
    });
}

QString SearchWidget::getSearchText() const
{
    return searchEdit->text();
}

bool SearchWidget::searchInCurrentFolder() const
{
    return searchScopeCombo->currentIndex() == 0;
}

bool SearchWidget::searchInAllDrives() const
{
    return searchScopeCombo->currentIndex() == 1;
}

bool SearchWidget::caseSensitive() const
{
    return caseSensitiveCheck->isChecked();
}

bool SearchWidget::searchInNamesOnly() const
{
    return namesOnlyCheck->isChecked();
}

void SearchWidget::clearSearch()
{
    stopSearch();
    searchEdit->clear();
    resultsList->clear();
    resultsList->setVisible(false);
    statusLabel->setVisible(false);
    progressBar->setVisible(false);
    loadingIndicator->setVisible(false);
    adjustSize();
}

void SearchWidget::startSearch()
{
    if (isSearching) return;

    QString searchText = searchEdit->text().trimmed();
    if (searchText.isEmpty()) return;

    stopSearch(); // Останавливаем предыдущий поиск

    isSearching = true;
    resultsList->clear();
    resultsList->setVisible(false);

    // Показываем индикаторы
    progressBar->setVisible(true);
    progressBar->setRange(0, 0);  // Set to indeterminate mode for cycling progress
    loadingIndicator->setVisible(true);  // Optional, but can keep for visual
    statusLabel->setVisible(true);
    statusLabel->setText("Поиск...");
    searchButton->setEnabled(false);

    // Вычисляем startPath перед запуском потока
    QString startPath = "";
    if (searchInCurrentFolder()) {
        // Получаем текущий путь более надежным способом
        QWidget *mainWindow = qApp->activeWindow();
        if (mainWindow) {
            QVariant currentPath = mainWindow->property("currentPath");
            if (currentPath.isValid()) {
                startPath = currentPath.toString();
                qDebug() << "Got current path from main window property:" << startPath;
            }
        }

        // Если не удалось получить путь, используем домашнюю директорию
        if (startPath.isEmpty()) {
            startPath = QDir::homePath();
            qDebug() << "Using home directory as fallback:" << startPath;
        }

        qDebug() << "Using search path:" << startPath;
    }

    // Создаем worker и thread для поиска
    searchWorker = new SearchWorker();
    searchThread = new QThread();

    searchWorker->moveToThread(searchThread);

    // Подключаем сигналы
    connect(searchWorker, &SearchWorker::searchFinished, this, &SearchWidget::onSearchFinished);
    connect(searchWorker, &SearchWorker::progressUpdate, this, &SearchWidget::onProgressUpdate);
    connect(searchThread, &QThread::finished, searchWorker, &QObject::deleteLater);

    // Подключаем сигнал для запуска поиска в потоке (fix for hanging)
    connect(this, &SearchWidget::startSearchSignal, searchWorker, &SearchWorker::search, Qt::QueuedConnection);

    // Запускаем поток
    searchThread->start();

    // Эмитируем сигнал для запуска поиска в потоке worker'а
    emit startSearchSignal(searchText, startPath, searchInAllDrives(), caseSensitive(), searchInNamesOnly());
}

void SearchWidget::stopSearch()
{
    if (searchThread && searchThread->isRunning()) {
        searchThread->requestInterruption();
        searchThread->quit();
        searchThread->wait(1000);
        if (searchThread->isRunning()) {
            searchThread->terminate();
            searchThread->wait();
        }
        delete searchThread;
        searchThread = nullptr;
    }

    isSearching = false;
    searchButton->setEnabled(true);
    progressBar->setVisible(false);
    progressBar->setRange(0, 100);  // Reset to determinate mode
    loadingIndicator->setVisible(false);
}

void SearchWidget::onSearchFinished(const QStringList &results, bool timeout)
{
    isSearching = false;
    searchButton->setEnabled(true);
    progressBar->setVisible(false);
    progressBar->setRange(0, 100);  // Reset range
    loadingIndicator->setVisible(false);

    if (timeout) {
        statusLabel->setText("Поиск прерван по таймауту (30 сек)");
        resultsList->setVisible(false);
    } else if (results.isEmpty()) {
        statusLabel->setText("Ничего не найдено");
        resultsList->setVisible(false);
    } else {
        statusLabel->setText(QString("Найдено: %1 файлов").arg(results.size()));

        // Добавляем результаты в список
        for (const QString &result : results) {
            QFileInfo fileInfo(result);
            QString displayText = fileInfo.fileName();
            if (fileInfo.isDir()) {
                displayText = "📁 " + displayText;
            } else {
                displayText = "📄 " + displayText;
            }

            QListWidgetItem *item = new QListWidgetItem(displayText);
            item->setData(Qt::UserRole, result);
            item->setToolTip(result); // Показываем полный путь при наведении
            resultsList->addItem(item);
        }

        resultsList->setVisible(true);
        updateResultsHeight();
    }

    adjustSize();

    // Очищаем thread
    if (searchThread) {
        searchThread->quit();
        searchThread->wait();
        delete searchThread;
        searchThread = nullptr;
    }
    searchWorker = nullptr;
}

void SearchWidget::onProgressUpdate(int count)
{
    statusLabel->setText(QString("Найдено: %1 файлов...").arg(count));
}

void SearchWidget::updateResultsHeight()
{
    int itemHeight = resultsList->sizeHintForRow(0);
    int visibleItems = qMin(resultsList->count(), 8); // Максимум 8 элементов видно сразу
    int totalHeight = itemHeight * visibleItems + resultsList->frameWidth() * 2;

    // Учитываем скроллбар
    if (resultsList->verticalScrollBar()->isVisible()) {
        totalHeight += resultsList->verticalScrollBar()->width();
    }

    resultsList->setMinimumHeight(qMax(120, totalHeight));
    resultsList->setMaximumHeight(qMin(400, totalHeight));

    // Обновляем размер всего виджета
    adjustSize();
}

void SearchWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        stopSearch();
        hide();
        emit closed();
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

void SearchWidget::hideEvent(QHideEvent *event)
{
    stopSearch();
    clearSearch();
    emit closed();
    QWidget::hideEvent(event);
}

void SearchWidget::showEvent(QShowEvent *event)
{
    activateWindow();
    raise();
    QWidget::showEvent(event);
}

bool SearchWidget::event(QEvent *event)
{
    if (event->type() == QEvent::WindowActivate || event->type() == QEvent::FocusIn) {
        activateWindow();
        raise();
    }
    return QWidget::event(event);
}

void SearchWidget::onSearchClicked()
{
    QString searchText = searchEdit->text().trimmed();
    if (!searchText.isEmpty()) {
        startSearch();
    }
}

void SearchWidget::onCloseClicked()
{
    stopSearch();
    hide();
    emit closed();
}

void SearchWidget::onResultClicked(QListWidgetItem *item)
{
    if (item) {
        QString path = item->data(Qt::UserRole).toString();
        emit resultSelected(path);
        hide();
    }
}

void SearchWidget::showResultsContextMenu(const QPoint &pos)
{
    QListWidgetItem *item = resultsList->itemAt(pos);
    if (item) {
        currentRightClickedPath = item->data(Qt::UserRole).toString();
        contextMenu->exec(resultsList->mapToGlobal(pos));
    }
}

void SearchWidget::onShowInContainingFolder()
{
    if (!currentRightClickedPath.isEmpty()) {
        // Испускаем новый сигнал с путем к файлу
        emit navigateToFile(currentRightClickedPath);
        hide();
    }
}