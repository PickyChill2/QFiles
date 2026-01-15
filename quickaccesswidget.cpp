#include "quickaccesswidget.h"
#include "strings.h"
#include "styles.h"
#include <QDir>

QuickAccessWidget::QuickAccessWidget(QWidget *parent)
    : QWidget(parent)
    , mainLayout(new QHBoxLayout(this))
{
    setupUI();
    populateQuickFolders();
}

void QuickAccessWidget::setupUI()
{
    mainLayout->setContentsMargins(Styles::QuickAccessLayoutMargins, Styles::QuickAccessLayoutMargins,
                                  Styles::QuickAccessLayoutMargins, Styles::QuickAccessLayoutMargins);
    mainLayout->setSpacing(Styles::QuickAccessLayoutSpacing);
    setStyleSheet(Styles::Buttons::QuickAccess); // Используем стили из Styles
}

void QuickAccessWidget::populateQuickFolders()
{
    struct QuickFolder {
        QString name;
        QString path;
        QString emoji;
    };

    QList<QuickFolder> folders = {
        {Strings::Pictures, QStandardPaths::writableLocation(QStandardPaths::PicturesLocation), Strings::EmojiPictures},
        {Strings::Documents, QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), Strings::EmojiDocuments},
        {Strings::Downloads, QStandardPaths::writableLocation(QStandardPaths::DownloadLocation), Strings::EmojiDownloads},
        {Strings::Desktop, QStandardPaths::writableLocation(QStandardPaths::DesktopLocation), Strings::EmojiDesktop},
        {Strings::Music, QStandardPaths::writableLocation(QStandardPaths::MusicLocation), Strings::EmojiMusic},
        {Strings::Videos, QStandardPaths::writableLocation(QStandardPaths::MoviesLocation), Strings::EmojiVideos},
        {Strings::RecycleBinTitle, Strings::RecycleBinPath, Strings::EmojiRecycleBin}  // Добавлено
    };

    for (const auto &folder : folders) {
        if ((!folder.path.isEmpty() && QDir(folder.path).exists()) || folder.path == Strings::RecycleBinPath) {
            QPushButton *folderButton = new QPushButton(this);
            folderButton->setText(QString("%1\n%2").arg(folder.emoji).arg(folder.name));
            folderButton->setMinimumSize(Styles::QuickAccessButtonWidth, Styles::QuickAccessButtonHeight); // Используем константы
            folderButton->setMaximumSize(Styles::QuickAccessButtonWidth, Styles::QuickAccessButtonHeight); // Используем константы
            folderButton->setProperty("folderPath", folder.path);
            connect(folderButton, &QPushButton::clicked, this, &QuickAccessWidget::onFolderClicked);
            mainLayout->addWidget(folderButton);
        }
    }
    mainLayout->addStretch();
}

void QuickAccessWidget::onFolderClicked()
{
    QPushButton *button = qobject_cast<QPushButton*>(sender());
    if (button) {
        QString path = button->property("folderPath").toString();
        emit folderClicked(path);
    }
}