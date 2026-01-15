#include "favoritesmenu.h"
#include "mainwindow.h"
#include "strings.h"
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QDir>
#include <QMouseEvent>
#include <QDebug>
#include <QLabel>
#include <QStandardPaths>
#include "colors.h"

// Реализация ManageFavoritesDialog (без изменений)
ManageFavoritesDialog::ManageFavoritesDialog(const QMap<QString, QString> &favorites, QWidget *parent)
    : QDialog(parent), favorites(favorites), selectedFavorite("")
{
    setWindowTitle(Strings::ManageFavorites);
    setModal(true);
    setMinimumSize(400, 300);

    // Устанавливаем стиль для диалога в темной теме
    setStyleSheet(
        "QDialog {"
        "    background: " + Colors::GradientDark + ";"
        "    border-radius: " + Colors::RadiusLarge + ";"
        "    color: " + Colors::TextLight + ";"
        "}"
        "QLabel {"
        "    color: " + Colors::TextLight + ";"
        "    font-size: 14px;"
        "    padding: 10px;"
        "}"
        "QListWidget {"
        "    background: " + Colors::DarkTertiary + ";"
        "    border: 1px solid " + Colors::TextDarkGray + ";"
        "    border-radius: " + Colors::RadiusMedium + ";"
        "    padding: 5px;"
        "    font-size: 13px;"
        "    outline: none;"
        "    color: " + Colors::TextLight + ";"
        "}"
        "QListWidget::item {"
        "    padding: 8px;"
        "    border-bottom: 1px solid " + Colors::TextDarkGray + ";"
        "    background: " + Colors::DarkTertiary + ";"
        "}"
        "QListWidget::item:hover {"
        "    background: " + Colors::DarkQuaternary + ";"
        "}"
        "QListWidget::item:selected {"
        "    background: " + Colors::TextBlue + ";"
        "    color: " + Colors::TextWhite + ";"
        "    border-radius: " + Colors::RadiusSmall + ";"
        "}"
        "QPushButton {"
        "    background: " + Colors::GradientGreen + ";"
        "    color: " + Colors::TextWhite + ";"
        "    border: none;"
        "    padding: 8px 16px;"
        "    border-radius: " + Colors::RadiusSmall + ";"
        "    font-weight: bold;"
        "    min-width: 80px;"
        "}"
        "QPushButton:hover {"
        "    background: " + Colors::GradientGreenHover + ";"
        "}"
        "QPushButton:pressed {"
        "    background: " + Colors::GradientGreenPressed + ";"
        "}"
        "QPushButton:disabled {"
        "    background: " + Colors::DarkTertiary + ";"
        "    color: " + Colors::TextMuted + ";"
        "}"
    );

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    QLabel *label = new QLabel(Strings::SelectFavoriteToRemove, this);
    mainLayout->addWidget(label);

    listWidget = new QListWidget(this);
    for (auto it = favorites.begin(); it != favorites.end(); ++it) {
        listWidget->addItem(it.key() + " -> " + it.value());
    }
    mainLayout->addWidget(listWidget);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    removeButton = new QPushButton("Удалить выбранное", this);
    removeButton->setEnabled(false);
    QPushButton *closeButton = new QPushButton("Закрыть", this);

    buttonLayout->addWidget(removeButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);

    mainLayout->addLayout(buttonLayout);

    connect(listWidget, &QListWidget::itemSelectionChanged, this, &ManageFavoritesDialog::onItemSelectionChanged);
    connect(removeButton, &QPushButton::clicked, this, &ManageFavoritesDialog::onRemoveClicked);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
}

void ManageFavoritesDialog::onItemSelectionChanged()
{
    QList<QListWidgetItem*> selectedItems = listWidget->selectedItems();
    removeButton->setEnabled(!selectedItems.isEmpty());
}

void ManageFavoritesDialog::onRemoveClicked()
{
    QList<QListWidgetItem*> selectedItems = listWidget->selectedItems();
    if (selectedItems.isEmpty()) return;

    QListWidgetItem *item = selectedItems.first();
    QString itemText = item->text();
    QString name = itemText.split(" -> ").first();

    selectedFavorite = name;

    // Удаляем из списка
    delete listWidget->takeItem(listWidget->row(item));

    // Если список пуст, отключаем кнопку удаления
    if (listWidget->count() == 0) {
        removeButton->setEnabled(false);
    }
}

// Упрощенная реализация FavoritesMenu без кастомной отрисовки
FavoritesMenu::FavoritesMenu(const QString &title, QWidget *parent)
    : QMenu(title, parent)
{
    setupStyle();
}

void FavoritesMenu::setupStyle()
{
    // Устанавливаем стиль с скруглениями для темной темы
    setStyleSheet(
        "QMenu {"
        "    background: " + Colors::DarkPrimary + ";"
        "    border: 1px solid " + Colors::DarkTertiary + ";"
        "    border-radius: " + Colors::RadiusMedium + ";"
        "    padding: 4px;"
        "    color: " + Colors::TextLight + ";"
        "}"
        "QMenu::item {"
        "    background: transparent;"
        "    padding: 6px 12px;"
        "    border-radius: " + Colors::RadiusSmall + ";"
        "    margin: 1px 0px;"
        "}"
        "QMenu::item:selected {"
        "    background: " + Colors::TextBlue + ";"
        "    color: " + Colors::TextWhite + ";"
        "}"
        "QMenu::item:disabled {"
        "    color: " + Colors::TextDarkGray + ";"
        "}"
        "QMenu::separator {"
        "    height: 1px;"
        "    background: " + Colors::DarkTertiary + ";"
        "    margin: 4px 8px;"
        "}"
    );
}

void FavoritesMenu::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton) {
        QAction *action = activeAction();
        if (action) {
            QString path = action->data().toString();
            if (!path.isEmpty()) {
                // Проверяем, что это не управляющее действие (добавить/управление)
                QString actionText = action->text();
                if (!actionText.contains("➕") && !actionText.contains("⚙️")) {
                    emit favoriteMiddleClicked(path);
                    event->accept();
                    return;
                }
            }
        }
    }

    QMenu::mouseReleaseEvent(event);
}

// Реализация FavoritesAction
FavoritesAction::FavoritesAction(QWidget *parent)
    : QWidgetAction(parent)
    , favoritesMenu(new FavoritesMenu("Избранное"))
    , settings("YourCompany", "YourApp")
    , mainWindow(qobject_cast<QWidget*>(parent))
{
    setText(Strings::Favorites);

    QWidget *widget = new QWidget();
    QHBoxLayout *layout = new QHBoxLayout(widget);
    layout->setContentsMargins(2, 2, 2, 2);

    QPushButton *favoritesButton = new QPushButton(Strings::Favorites);
    favoritesButton->setMenu(favoritesMenu);

    // Стиль для кнопки избранного в темной теме
    favoritesButton->setStyleSheet(
        "QPushButton {"
        "    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
        "        stop: 0 " + Colors::DarkTertiary + ", stop: 1 " + Colors::DarkPrimary + ");"
        "    color: " + Colors::TextLight + ";"
        "    border: 1px solid " + Colors::TextDarkGray + ";"
        "    padding: 6px 12px;"
        "    border-radius: " + Colors::RadiusSmall + ";"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
        "        stop: 0 " + Colors::DarkQuaternary + ", stop: 1 " + Colors::ButtonPressedStart + ");"
        "    border: 1px solid " + Colors::TextBlueLight + ";"
        "}"
        "QPushButton:pressed {"
        "    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
        "        stop: 0 " + Colors::ButtonPressedStart + ", stop: 1 " + Colors::ButtonPressedEnd + ");"
        "}"
        "QPushButton::menu-indicator {"
        "    image: none;"
        "    subcontrol-origin: padding;"
        "    subcontrol-position: center right;"
        "    width: 10px;"
        "}"
    );

    layout->addWidget(favoritesButton);
    setDefaultWidget(widget);

    loadFavorites();
    updateMenu();

    connect(favoritesMenu, &FavoritesMenu::aboutToShow, this, &FavoritesAction::updateMenu);
    connect(favoritesMenu, &FavoritesMenu::favoriteMiddleClicked, this, &FavoritesAction::favoriteMiddleClicked);
}

FavoritesAction::~FavoritesAction()
{
    delete favoritesMenu;
}

void FavoritesAction::updateFavorites()
{
    loadFavorites();
    updateMenu();
}

void FavoritesAction::updateMenu()
{
    favoritesMenu->clear();

    // Добавляем пользовательские избранные папки
    for (auto it = favorites.begin(); it != favorites.end(); ++it) {
        QAction *action = favoritesMenu->addAction("⭐ " + it.key());
        action->setData(it.value());
    }

    if (!favorites.isEmpty()) {
        favoritesMenu->addSeparator();
    }

    // Добавляем действия управления
    QAction *addAction = favoritesMenu->addAction("➕ " + Strings::AddToFavorites);
    QAction *manageAction = favoritesMenu->addAction("⚙️ " + Strings::ManageFavorites);

    // Подключаем сигналы для кликов
    for (QAction *action : favoritesMenu->actions()) {
        if (action != addAction && action != manageAction) {
            connect(action, &QAction::triggered, this, &FavoritesAction::onFavoriteTriggered);
        }
    }

    connect(addAction, &QAction::triggered, this, &FavoritesAction::addCurrentFolderToFavorites);
    connect(manageAction, &QAction::triggered, this, &FavoritesAction::manageFavorites);
}

void FavoritesAction::onFavoriteTriggered()
{
    QAction *action = qobject_cast<QAction*>(sender());
    if (action) {
        QString path = action->data().toString();
        if (!path.isEmpty()) {
            emit favoriteClicked(path);
        }
    }
}

void FavoritesAction::addCurrentFolderToFavorites()
{
    QWidget *parent = getMainWindow();
    if (!parent) parent = mainWindow;

    // Устанавливаем темную тему для диалоговых окон
    QString darkStyle =
        "QDialog {"
        "    background: " + Colors::DarkPrimary + ";"
        "    color: " + Colors::TextLight + ";"
        "}"
        "QLabel {"
        "    color: " + Colors::TextLight + ";"
        "}"
        "QLineEdit {"
        "    background: " + Colors::DarkTertiary + ";"
        "    border: 1px solid " + Colors::TextDarkGray + ";"
        "    border-radius: " + Colors::RadiusExtraSmall + ";"
        "    padding: 5px;"
        "    color: " + Colors::TextLight + ";"
        "    selection-background-color: " + Colors::TextBlue + ";"
        "}"
        "QPushButton {"
        "    background: " + Colors::DarkTertiary + ";"
        "    color: " + Colors::TextLight + ";"
        "    border: 1px solid " + Colors::TextDarkGray + ";"
        "    border-radius: " + Colors::RadiusExtraSmall + ";"
        "    padding: 5px 10px;"
        "}"
        "QPushButton:hover {"
        "    background: " + Colors::DarkQuaternary + ";"
        "    border: 1px solid " + Colors::TextBlueLight + ";"
        "}"
    ;

    QString currentPath = getCurrentPath();
    if (currentPath.isEmpty()) {
        QMessageBox msgBox(parent);
        msgBox.setStyleSheet(darkStyle);
        msgBox.setWindowTitle(Strings::Info);
        msgBox.setText(Strings::NavigateError);
        msgBox.setIcon(QMessageBox::Information);
        msgBox.exec();
        return;
    }

    QFileInfo info(currentPath);
    QString folderName = info.fileName();
    if (folderName.isEmpty()) {
        folderName = QDir(currentPath).dirName();
    }

    QInputDialog dialog(parent);
    dialog.setStyleSheet(darkStyle);
    dialog.setWindowTitle(Strings::AddToFavorites);
    dialog.setLabelText(Strings::EnterFavoriteName);
    dialog.setTextValue(folderName);
    dialog.setOkButtonText("Добавить");
    dialog.setCancelButtonText("Отмена");

    bool ok = dialog.exec() == QDialog::Accepted;
    QString name = dialog.textValue();

    if (ok && !name.isEmpty()) {
        if (favorites.contains(name)) {
            QMessageBox msgBox(parent);
            msgBox.setStyleSheet(darkStyle);
            msgBox.setWindowTitle(Strings::Warning);
            msgBox.setText(Strings::FavoriteExists);
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.exec();
            return;
        }

        favorites.insert(name, currentPath);
        saveFavorites();
        updateMenu();
    }
}

void FavoritesAction::manageFavorites()
{
    QWidget *parent = getMainWindow();
    if (!parent) parent = mainWindow;

    if (favorites.isEmpty()) {
        QMessageBox msgBox(parent);
        msgBox.setStyleSheet(
            "QMessageBox {"
            "    background: " + Colors::DarkPrimary + ";"
            "    color: " + Colors::TextLight + ";"
            "}"
            "QLabel {"
            "    color: " + Colors::TextLight + ";"
            "}"
            "QPushButton {"
            "    background: " + Colors::DarkTertiary + ";"
            "    color: " + Colors::TextLight + ";"
            "    border: 1px solid " + Colors::TextDarkGray + ";"
            "    border-radius: " + Colors::RadiusExtraSmall + ";"
            "    padding: 5px 10px;"
            "}"
        );
        msgBox.setWindowTitle(Strings::Info);
        msgBox.setText(Strings::NoFavorites);
        msgBox.setIcon(QMessageBox::Information);
        msgBox.exec();
        return;
    }

    // Используем кастомный диалог вместо QInputDialog
    ManageFavoritesDialog dialog(favorites, parent);
    if (dialog.exec() == QDialog::Accepted) {
        QString nameToRemove = dialog.getSelectedFavorite();
        if (!nameToRemove.isEmpty()) {
            favorites.remove(nameToRemove);
            saveFavorites();
            updateMenu();

            QMessageBox msgBox(parent);
            msgBox.setStyleSheet(
                "QMessageBox {"
                "    background: " + Colors::DarkPrimary + ";"
                "    color: " + Colors::TextLight + ";"
                "}"
                "QLabel {"
                "    color: " + Colors::TextLight + ";"
                "}"
                "QPushButton {"
                "    background: " + Colors::DarkTertiary + ";"
                "    color: " + Colors::TextLight + ";"
                "    border: 1px solid " + Colors::TextDarkGray + ";"
                "    border-radius: " + Colors::RadiusExtraSmall + ";"
                "    padding: 5px 10px;"
                "}"
            );
            msgBox.setWindowTitle(Strings::Info);
            msgBox.setText("Избранное '" + nameToRemove + "' успешно удалено");
            msgBox.setIcon(QMessageBox::Information);
            msgBox.exec();
        }
    }
}

void FavoritesAction::loadFavorites()
{
    favorites.clear();

    int size = settings.beginReadArray("Favorites");
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        QString name = settings.value("name").toString();
        QString path = settings.value("path").toString();
        if (!name.isEmpty() && !path.isEmpty()) {
            favorites.insert(name, path);
        }
    }
    settings.endArray();
}

void FavoritesAction::saveFavorites()
{
    settings.beginWriteArray("Favorites");
    int index = 0;
    for (auto it = favorites.begin(); it != favorites.end(); ++it) {
        settings.setArrayIndex(index);
        settings.setValue("name", it.key());
        settings.setValue("path", it.value());
        index++;
    }
    settings.endArray();
}

QString FavoritesAction::getCurrentPath() const
{
    MainWindow *mainWindow = qobject_cast<MainWindow*>(parent());
    if (mainWindow) {
        return mainWindow->getCurrentPath();
    }
    return QString();
}

QWidget* FavoritesAction::getMainWindow() const
{
    // Ищем главное окно в родительской цепочке
    QWidget *parentWidget = qobject_cast<QWidget*>(parent());
    while (parentWidget && !qobject_cast<MainWindow*>(parentWidget)) {
        parentWidget = parentWidget->parentWidget();
    }
    return parentWidget;
}