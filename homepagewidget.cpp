#include "homepagewidget.h"
#include "styles.h"
#include "strings.h"
#include <QFrame>
#include <QFileIconProvider>
#include <QMessageBox>
#include <QProcess>
#include <windows.h>
#include <shlobj.h>

HomePageWidget::HomePageWidget(QWidget *parent)
    : QWidget(parent)
    , mainLayout(new QVBoxLayout(this))
    , quickFoldersWidget(new QWidget(this))
    , quickFoldersLabel(new QLabel("Quick Folders", this))
    , quickFoldersContainer(new QHBoxLayout())
    , networkDriveWidget(new NetworkDriveWidget(this))
    , recycleBinButton(nullptr)
    , recycleBinContextMenu(new QMenu(this))
{
    setupUI();
    populateQuickFolders();

    // Подключаем сигналы сетевых дисков
    connect(networkDriveWidget, &NetworkDriveWidget::driveConnected, this, &HomePageWidget::onNetworkDriveConnected);
}

void HomePageWidget::setupUI()
{
    // Используем константы из Styles
    mainLayout->setContentsMargins(Styles::HomePageLayoutMargins, Styles::HomePageLayoutMargins,
                                  Styles::HomePageLayoutMargins, Styles::HomePageLayoutMargins);
    mainLayout->setSpacing(Styles::HomePageLayoutSpacing);

    // Используем стили из Styles
    setStyleSheet(Styles::Labels::HomePage);

    // Quick folders section - используем константы из Styles
    QVBoxLayout *foldersLayout = new QVBoxLayout(quickFoldersWidget);
    foldersLayout->setContentsMargins(Styles::HomePageLayoutMargins, Styles::HomePageLayoutMargins,
                                     Styles::HomePageLayoutMargins, Styles::HomePageLayoutMargins);
    foldersLayout->setSpacing(Styles::HomePageLayoutSpacing);

    foldersLayout->addWidget(quickFoldersLabel);

    quickFoldersContainer->setContentsMargins(0, 0, 0, 0);
    quickFoldersContainer->setSpacing(8);
    foldersLayout->addLayout(quickFoldersContainer);

    // Добавляем сетевые диски под quick folders - слева
    QHBoxLayout *networkDrivesLayout = new QHBoxLayout();
    networkDrivesLayout->setContentsMargins(0, Styles::HomePageLayoutMargins, 0, 0);
    networkDrivesLayout->setSpacing(5);
    networkDrivesLayout->addWidget(networkDriveWidget);
    networkDrivesLayout->addStretch();
    foldersLayout->addLayout(networkDrivesLayout);

    quickFoldersWidget->setLayout(foldersLayout);

    // Main layout
    mainLayout->addWidget(quickFoldersWidget);

    // Используем константу из Styles
    setFixedHeight(Styles::HomePageWidgetHeight);
}

void HomePageWidget::populateQuickFolders()
{
    struct QuickFolder {
        QString name;
        QString path;
        QString emoji;
        bool isRecycleBin;
    };

    QList<QuickFolder> folders = {
        {Strings::Pictures, QStandardPaths::writableLocation(QStandardPaths::PicturesLocation), Strings::EmojiPictures, false},
        {Strings::Documents, QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), Strings::EmojiDocuments, false},
        {Strings::Downloads, QStandardPaths::writableLocation(QStandardPaths::DownloadLocation), Strings::EmojiDownloads, false},
        {Strings::Desktop, QStandardPaths::writableLocation(QStandardPaths::DesktopLocation), Strings::EmojiDesktop, false},
        {Strings::Music, QStandardPaths::writableLocation(QStandardPaths::MusicLocation), Strings::EmojiMusic, false},
        {Strings::Videos, QStandardPaths::writableLocation(QStandardPaths::MoviesLocation), Strings::EmojiVideos, false},
        {Strings::RecycleBinTitle, "", Strings::EmojiRecycleBin, true}
    };

    for (const auto &folder : folders) {
        if (folder.isRecycleBin) {
            // Создаем кнопку корзины
            recycleBinButton = new QPushButton(this);
            recycleBinButton->setText(QString("%1\n%2").arg(folder.emoji).arg(folder.name));
            // Используем константы из Styles
            recycleBinButton->setMinimumSize(Styles::HomePageButtonWidth, Styles::HomePageButtonHeight);
            recycleBinButton->setMaximumSize(Styles::HomePageButtonWidth, Styles::HomePageButtonHeight);
            // Используем стили из Styles
            recycleBinButton->setStyleSheet(Styles::Buttons::QuickAccess);

            recycleBinButton->setProperty("isRecycleBin", true);
            connect(recycleBinButton, &QPushButton::clicked, this, &HomePageWidget::onFolderClicked);

            // Включаем контекстное меню для корзины
            recycleBinButton->setContextMenuPolicy(Qt::CustomContextMenu);
            connect(recycleBinButton, &QPushButton::customContextMenuRequested,
                    this, &HomePageWidget::onRecycleBinContextMenuRequested);

            // Устанавливаем фильтр событий для среднего клика
            recycleBinButton->installEventFilter(this);

            quickFoldersContainer->addWidget(recycleBinButton);
        }
        else if (!folder.path.isEmpty() && QDir(folder.path).exists()) {
            QPushButton *folderButton = new QPushButton(this);
            folderButton->setText(QString("%1\n%2").arg(folder.emoji).arg(folder.name));
            // Используем константы из Styles
            folderButton->setMinimumSize(Styles::HomePageButtonWidth, Styles::HomePageButtonHeight);
            folderButton->setMaximumSize(Styles::HomePageButtonWidth, Styles::HomePageButtonHeight);
            // Используем стили из Styles
            folderButton->setStyleSheet(Styles::Buttons::QuickAccess);

            folderButton->setProperty("folderPath", folder.path);
            folderButton->setProperty("isRecycleBin", false);
            connect(folderButton, &QPushButton::clicked, this, &HomePageWidget::onFolderClicked);

            // Устанавливаем фильтр событий для среднего клика
            folderButton->installEventFilter(this);

            quickFoldersContainer->addWidget(folderButton);
        }
    }
    quickFoldersContainer->addStretch();

    // Настраиваем контекстное меню для корзины
    QAction *openAction = recycleBinContextMenu->addAction(Strings::OpenRecycleBin);
    QAction *emptyAction = recycleBinContextMenu->addAction(Strings::EmptyRecycleBin);

    connect(openAction, &QAction::triggered, this, &HomePageWidget::openRecycleBin);
    connect(emptyAction, &QAction::triggered, this, &HomePageWidget::emptyRecycleBin);
}

bool HomePageWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

        if (mouseEvent->button() == Qt::MiddleButton) {
            QPushButton *button = qobject_cast<QPushButton*>(obj);
            if (button) {
                bool isRecycleBin = button->property("isRecycleBin").toBool();
                if (isRecycleBin) {
                    // Для корзины
                    emit middleClickOnFolder(Strings::RecycleBinPath);
                } else {
                    // Для обычных папок
                    QString path = button->property("folderPath").toString();
                    if (!path.isEmpty()) {
                        emit middleClickOnFolder(path);
                    }
                }
                return true; // Событие обработано
            }
        }
    }

    return QWidget::eventFilter(obj, event);
}

void HomePageWidget::onFolderClicked()
{
    QPushButton *button = qobject_cast<QPushButton*>(sender());
    if (button) {
        bool isRecycleBin = button->property("isRecycleBin").toBool();
        if (isRecycleBin) {
            openRecycleBin();
        } else {
            QString path = button->property("folderPath").toString();
            emit folderClicked(path);
        }
    }
}

void HomePageWidget::onRecycleBinContextMenuRequested(const QPoint &pos)
{
    // Показываем контекстное меню для корзины
    if (recycleBinButton) {
        recycleBinContextMenu->exec(recycleBinButton->mapToGlobal(pos));
    }
}

void HomePageWidget::openRecycleBin()
{
    // Открываем корзину внутри приложения
    emit folderClicked(Strings::RecycleBinPath);
}

void HomePageWidget::emptyRecycleBin()
{
    // Подтверждение очистки корзины
    int result = QMessageBox::question(this,
                                      Strings::EmptyRecycleBinTitle,
                                      Strings::EmptyRecycleBinMessage,
                                      QMessageBox::Yes | QMessageBox::No,
                                      QMessageBox::No);

    if (result == QMessageBox::Yes) {
        // Используем Windows API для очистки корзины
        // Определяем константы, если они не определены
        #ifndef SHERB_NOCONFIRMATION
        #define SHERB_NOCONFIRMATION 0x00000001
        #endif
        #ifndef SHERB_NOPROGRESSUI
        #define SHERB_NOPROGRESSUI 0x00000002
        #endif
        #ifndef SHERB_NOSOUND
        #define SHERB_NOSOUND 0x00000004
        #endif

        SHEmptyRecycleBinW(nullptr, nullptr, SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI | SHERB_NOSOUND);

        QMessageBox::information(this,
                               Strings::EmptyRecycleBinTitle,
                               Strings::EmptyRecycleBinSuccess);
    }
}

QString HomePageWidget::getRecycleBinPath()
{
    // Получаем путь к корзине с помощью Windows API
    wchar_t recycleBinPath[MAX_PATH];

    if (SHGetFolderPathW(nullptr, CSIDL_BITBUCKET, nullptr, 0, recycleBinPath) == S_OK) {
        return QString::fromWCharArray(recycleBinPath);
    }

    // Альтернативный способ через GUID корзины
    return "::{645FF040-5081-101B-9F08-00AA002F954E}";
}

void HomePageWidget::onNetworkDriveConnected(const QString &path)
{
    // Переходим на подключенный диск
    emit folderClicked(path);
}