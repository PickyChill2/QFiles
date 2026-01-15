#include "contextmenu.h"
#include "filesystemtab.h"
#include "mainwindow.h"
#include "strings.h"
#include "colors.h"
#include <QFile>
#include <QDir>
#include <QTimer>
#include <QDesktopServices>
#include <QProcess>
#include <QMessageBox>
#include <QDebug>
#include <QSettings>
#include <QFileDialog>
#include <QStandardPaths>
#include <QInputDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTableWidget>
#include <QHeaderView>
#include <shlguid.h>
#include <shobjidl.h>

#include "propertiesdialog.h"

ContextMenu::ContextMenu(QWidget *parent)
    : QMenu(parent)
{
    // Загружаем ассоциации при создании меню
    loadAssociations();

    setupActions();
    setupStyle();
}

void ContextMenu::setupStyle()
{
    // Устанавливаем стиль с скруглениями для темной темы, как в FavoritesMenu
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

// Функция для получения стиля диалоговых окон
QString ContextMenu::getDialogStyle() const
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
        "}"
        "QProgressBar {"
        "    border: 1px solid " + Colors::TextDarkGray + ";"
        "    border-radius: " + Colors::RadiusSmall + ";"
        "    text-align: center;"
        "    background: " + Colors::DarkTertiary + ";"
        "    color: " + Colors::TextLight + ";"
        "}"
        "QProgressBar::chunk {"
        "    background: " + Colors::TextBlue + ";"
        "    border-radius: " + Colors::RadiusSmall + ";"
        "}";
}

// Функция для стилизации любого диалога
void ContextMenu::styleDialog(QDialog *dialog) const
{
    if (dialog) {
        dialog->setStyleSheet(getDialogStyle());
    }
}

// Функция для создания стилизованного QMessageBox
QMessageBox* ContextMenu::createStyledMessageBox(QWidget *parent, const QString &title, const QString &text, QMessageBox::Icon icon) const
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

void ContextMenu::setParentTab(FileSystemTab *parentTab)
{
    this->parentTab = parentTab;
}

void ContextMenu::setFileOperations(FileOperations *fileOps)
{
    this->fileOperations = fileOps;
    if (fileOps) {
        connect(fileOps, &FileOperations::operationCompleted, this, &ContextMenu::fileOperationCompleted);
    }
}

void ContextMenu::setupActions()
{
    openAction = addAction(Strings::Open);
    openWithAction = addAction(Strings::OpenWith);

    // Добавляем действие "Извлечь сюда" перед разделителем
    extractHereAction = addAction(Strings::ExtractHere);
    extractHereAction->setVisible(false);

    addSeparator();
    cutAction = addAction(Strings::Cut);
    copyAction = addAction(Strings::Copy);
    pasteAction = addAction(Strings::Paste);
    addSeparator();
    renameAction = addAction(Strings::Rename);

    // Создаем меню "Создать"
    createMenu = new QMenu(Strings::Create, this);
    createAction = this->addMenu(createMenu);

    // Применяем стиль к подменю "Создать"
    createMenu->setStyleSheet(
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
        "QMenu::separator {"
        "    height: 1px;"
        "    background: " + Colors::DarkTertiary + ";"
        "    margin: 4px 8px;"
        "}"
    );

    // Добавляем действия в меню "Создать"
    newFolderAction = createMenu->addAction(Strings::NewFolder);
    createTextFileAction = createMenu->addAction(Strings::NewTextFile);
    createDocFileAction = createMenu->addAction(Strings::NewDocFile);

    // Добавляем разделитель в подменю
    createMenu->addSeparator();

    // Добавляем новые действия в подменю "Создать"
    createArchiveAction = createMenu->addAction(Strings::CreateArchive);
    createShortcutAction = createMenu->addAction(Strings::CreateShortcut);

    // Добавляем действие "Изменить" для скриптовых файлов
    editScriptFileAction = addAction(Strings::Edit);
    editScriptFileAction->setVisible(false);

    // Добавляем действия для PowerShell
    runPowerShellAction = addAction(Strings::PowerShell1);
    runPowerShellAction->setVisible(false);

    runPowerShellAsAdminAction = addAction(Strings::PowerShell2);
    runPowerShellAsAdminAction->setVisible(false);

    addSeparator();
    propertiesAction = addAction(Strings::Properties);
    showInExplorerAction = addAction(Strings::ShowInExplorer);
    //addSeparator();
    //showHiddenAction = addAction(Strings::Hidden);
    //showHiddenAction->setCheckable(false);
    addSeparator();
    deleteAction = addAction(Strings::Delete);
    //addSeparator();
    //undoDeleteAction = addAction(Strings::UndoDeleteTitle);
    //undoDeleteAction->setEnabled(false);

    // Добавляем действие для настройки пути к 7-Zip
    //addSeparator();
    //QAction *configure7zAction = addAction("Настроить путь к 7-Zip");
    //connect(configure7zAction, &QAction::triggered, this, &ContextMenu::show7zPathDialog);

    // Существующие соединения
    connect(openAction, &QAction::triggered, this, &ContextMenu::open);
    connect(openWithAction, &QAction::triggered, this, &ContextMenu::openWith);
    connect(cutAction, &QAction::triggered, this, &ContextMenu::cut);
    connect(copyAction, &QAction::triggered, this, &ContextMenu::copy);
    connect(pasteAction, &QAction::triggered, this, &ContextMenu::paste);
    connect(renameAction, &QAction::triggered, this, &ContextMenu::rename);
    connect(deleteAction, &QAction::triggered, this, &ContextMenu::deleteFile);
    connect(newFolderAction, &QAction::triggered, this, &ContextMenu::newFolder);
    connect(propertiesAction, &QAction::triggered, this, &ContextMenu::properties);
    connect(showInExplorerAction, &QAction::triggered, this, &ContextMenu::showInExplorer);
    connect(showHiddenAction, &QAction::triggered, this, &ContextMenu::toggleHiddenFiles);
    connect(createTextFileAction, &QAction::triggered, this, &ContextMenu::createTextFile);
    connect(createDocFileAction, &QAction::triggered, this, &ContextMenu::createDocFile);
    connect(editScriptFileAction, &QAction::triggered, this, &ContextMenu::editScriptFile);
    connect(runPowerShellAction, &QAction::triggered, this, &ContextMenu::runPowerShell);
    connect(runPowerShellAsAdminAction, &QAction::triggered, this, &ContextMenu::runPowerShellAsAdmin);

    // Новое соединение для извлечения архивов
    connect(extractHereAction, &QAction::triggered, this, &ContextMenu::extractHere);

    // Соединения для новых действий
    connect(createShortcutAction, &QAction::triggered, this, &ContextMenu::createShortcut);
    connect(createArchiveAction, &QAction::triggered, this, &ContextMenu::createArchive);
}

// Получаем путь к файлу с ассоциациями
QString ContextMenu::getJsonFilePath() const
{
    // Используем папку AppData для хранения настроек
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir appDataDir(appDataPath);
    if (!appDataDir.exists()) {
        appDataDir.mkpath(".");
    }
    return appDataDir.filePath("open_with_associations.json");
}

// Загружаем ассоциации из JSON файла
void ContextMenu::loadAssociations()
{
    QString filePath = getJsonFilePath();
    QFile file(filePath);

    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        file.close();

        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isNull()) {
            associations = doc.object();
        }
    } else {
        // Файл не существует, создаем пустой объект
        associations = QJsonObject();
    }
}

// Добавляем программу для расширения
void ContextMenu::addProgramForExtension(const QString &extension, const QString &programPath)
{
    QJsonArray programArray;

    // Получаем текущий массив или создаем новый
    if (associations.contains(extension)) {
        programArray = associations[extension].toArray();
    }

    // Проверяем, нет ли уже этой программы в списке
    bool alreadyExists = false;
    for (int i = 0; i < programArray.size(); ++i) {
        if (programArray[i].toString() == programPath) {
            alreadyExists = true;
            break;
        }
    }

    // Если программы еще нет, добавляем ее
    if (!alreadyExists) {
        programArray.append(programPath);
        associations[extension] = programArray;
        saveAssociations();
    }
}

// Удаляем программу для расширения
void ContextMenu::removeProgramForExtension(const QString &extension, const QString &programPath)
{
    if (associations.contains(extension)) {
        QJsonArray programArray = associations[extension].toArray();
        QJsonArray newArray;

        for (int i = 0; i < programArray.size(); ++i) {
            if (programArray[i].toString() != programPath) {
                newArray.append(programArray[i]);
            }
        }

        associations[extension] = newArray;
        saveAssociations();
    }
}

// Получаем иконку программы из EXE файла
QIcon ContextMenu::getProgramIcon(const QString &programPath) const
{
    if (programPath.isEmpty() || !QFile::exists(programPath)) {
        return QIcon();
    }

    SHFILEINFO shfi;
    memset(&shfi, 0, sizeof(shfi));

    // Получаем иконку файла
    if (SHGetFileInfoW(reinterpret_cast<LPCWSTR>(programPath.utf16()),
                      0, &shfi, sizeof(shfi),
                      SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES)) {
        if (shfi.hIcon) {
            // Создаем QIcon из HICON
            QIcon icon = QIcon(QPixmap::fromImage(QImage::fromHICON(shfi.hIcon)));
            DestroyIcon(shfi.hIcon);
            return icon;
        }
    }

    return QIcon();
}

// Сохраняем ассоциации в JSON файл
void ContextMenu::saveAssociations()
{
    QString filePath = getJsonFilePath();
    QFile file(filePath);

    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(associations);
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
    } else {
        qDebug() << "Failed to save associations to" << filePath;
    }
}

QStringList ContextMenu::getProgramsForExtension(const QString &extension)
{
    QStringList programs;

    if (associations.contains(extension)) {
        QJsonArray programArray = associations[extension].toArray();
        for (int i = 0; i < programArray.size(); ++i) {
            programs.append(programArray[i].toString());
        }
    }

    return programs;
}

void ContextMenu::show7zPathDialog()
{
    // Создаем диалоговое окно
    QDialog *dialog = new QDialog(nullptr);
    dialog->setWindowTitle("Выбор пути к 7-Zip");
    dialog->setMinimumWidth(500);
    styleDialog(dialog);

    // Создаем layout
    QVBoxLayout *mainLayout = new QVBoxLayout(dialog);

    // Информационный текст
    QLabel *infoLabel = new QLabel("Укажите путь к исполняемому файлу 7z.exe:", dialog);
    mainLayout->addWidget(infoLabel);

    // Layout для пути и кнопки обзора
    QHBoxLayout *pathLayout = new QHBoxLayout();

    QLineEdit *pathEdit = new QLineEdit(dialog);
    pathEdit->setPlaceholderText("C:\\Program Files\\7-Zip\\7z.exe");

    // Загружаем текущий сохраненный путь
    QSettings settings;
    QString currentPath = settings.value("7z_path").toString();
    if (!currentPath.isEmpty()) {
        pathEdit->setText(currentPath);
    }

    QPushButton *browseButton = new QPushButton("Обзор...", dialog);

    pathLayout->addWidget(pathEdit);
    pathLayout->addWidget(browseButton);
    mainLayout->addLayout(pathLayout);

    // Подсказка
    QLabel *hintLabel = new QLabel("Обычно 7z.exe находится в C:\\Program Files\\7-Zip\\", dialog);
    hintLabel->setStyleSheet("color: " + Colors::TextMuted + "; font-size: 10px;");
    mainLayout->addWidget(hintLabel);

    // Кнопки OK и Cancel
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *okButton = new QPushButton("Сохранить", dialog);
    QPushButton *cancelButton = new QPushButton("Отмена", dialog);

    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    mainLayout->addLayout(buttonLayout);

    // Подключаем сигналы
    connect(browseButton, &QPushButton::clicked, this, [this, pathEdit]() {
        // Создаем диалог выбора файла
        QFileDialog fileDialog(nullptr, "Выберите 7z.exe");
        fileDialog.setFileMode(QFileDialog::ExistingFile);
        fileDialog.setNameFilter("7z.exe (7z.exe);;Все файлы (*.*)");
        fileDialog.setDirectory("C:\\Program Files");

        // Применяем стиль к диалогу
        styleDialog(&fileDialog);

        if (fileDialog.exec() == QDialog::Accepted) {
            QString selectedFile = fileDialog.selectedFiles().first();
            if (!selectedFile.isEmpty()) {
                pathEdit->setText(selectedFile);
            }
        }
    });

    connect(okButton, &QPushButton::clicked, dialog, [this, dialog, pathEdit]() {
        QString path = pathEdit->text().trimmed();
        if (!path.isEmpty() && QFile::exists(path)) {
            QSettings settings;
            settings.setValue("7z_path", path);
            dialog->accept();

            // Показываем сообщение об успехе
            QMessageBox *msgBox = createStyledMessageBox(nullptr, "Успех",
                "Путь к 7-Zip успешно сохранен!", QMessageBox::Information);
            msgBox->exec();
            msgBox->deleteLater();
        } else {
            QMessageBox *msgBox = createStyledMessageBox(nullptr, "Ошибка",
                "Указанный файл не существует или путь некорректен.", QMessageBox::Warning);
            msgBox->exec();
            msgBox->deleteLater();
        }
    });

    connect(cancelButton, &QPushButton::clicked, dialog, &QDialog::reject);

    dialog->exec();
    dialog->deleteLater();
}

void ContextMenu::setSelection(const QModelIndexList &indexes, const QString &currentPath)
{
    selectedIndexes = indexes;
    currentDirectory = currentPath;

    // Обновляем доступность действий в зависимости от выбора
    bool hasSelection = !selectedIndexes.isEmpty();
    bool singleSelection = selectedIndexes.size() == 1;

    openAction->setEnabled(hasSelection);
    openWithAction->setEnabled(hasSelection);
    cutAction->setEnabled(hasSelection);
    copyAction->setEnabled(hasSelection);
    renameAction->setEnabled(singleSelection);
    deleteAction->setEnabled(hasSelection);
    propertiesAction->setEnabled(singleSelection);
    showInExplorerAction->setEnabled(hasSelection);

    // Проверяем, выбран ли файл (не папка) для отображения действия создания ярлыка
    bool isFileSelected = false;
    bool isScriptFileSelected = false;
    bool isPowerShellFile = false;
    bool isArchive = false;

    if (singleSelection) {
        QString filePath = selectedIndexes.first().data(QFileSystemModel::FilePathRole).toString();
        QFileInfo fileInfo(filePath);

        if (fileInfo.isFile()) {
            isFileSelected = true;

            // Проверяем, является ли скриптовым файлом (bat, ps1, vbs, reg)
            if (isScriptFile(filePath)) {
                isScriptFileSelected = true;

                // Проверяем, является ли файлом PowerShell
                if (fileInfo.suffix().toLower() == "ps1") {
                    isPowerShellFile = true;
                }
            }
            // Проверяем, является ли файлом архивом
            if (isArchiveFile(filePath)) {
                isArchive = true;
            }
        }
    }

    // Управляем видимостью кнопок в подменю "Создать"
    // Показываем "Создать ярлык" только для файлов, а "Создать архив" для всех элементов
    createShortcutAction->setVisible(isFileSelected);
    createArchiveAction->setVisible(singleSelection);  // Для папок тоже можно создавать архив

    editScriptFileAction->setVisible(isScriptFileSelected);
    runPowerShellAction->setVisible(isPowerShellFile);
    runPowerShellAsAdminAction->setVisible(isPowerShellFile);
    extractHereAction->setVisible(isArchive);

    // Обновляем доступность Paste
    if (fileOperations) {
        pasteAction->setEnabled(fileOperations->hasFilesToPaste());
    } else {
        pasteAction->setEnabled(false);
    }

    // Для "Open" меняем текст в зависимости от типа
    if (singleSelection) {
        QFileInfo info(selectedIndexes.first().data(QFileSystemModel::FilePathRole).toString());
        openAction->setText(info.isDir() ? Strings::Open : Strings::OpenFile);
    }
}

void ContextMenu::createNewFile(const QString &extension)
{
    if (!parentTab) return;

    QString currentPath = currentDirectory;
    if (currentPath.isEmpty()) return;

    QDir dir(currentPath);

    // Определяем базовое имя файла в зависимости от типа
    QString baseName;
    if (extension == "txt") {
        baseName = "Новый текстовый документ";
    } else if (extension == "docx") {
        baseName = "Новый документ Word";
    } else {
        baseName = "Новый файл";
    }

    QString newFilePath;
    int counter = 1;
    do {
        newFilePath = currentPath + "/" + baseName;
        if (counter > 1) {
            newFilePath += " (" + QString::number(counter) + ")";
        }
        newFilePath += "." + extension;
        counter++;
    } while (QFile::exists(newFilePath));

    // Создаем файл
    QFile file(newFilePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.close();
        qDebug() << "File created:" << newFilePath;

        // Обновляем представление
        emit refreshRequested();

        // Запускаем переименование после небольшой задержки
        QTimer::singleShot(500, [this, newFilePath]() {
            if (parentTab) {
                MainWindow* mainWindow = parentTab->getMainWindow();
                if (mainWindow) {
                    // Находим индекс созданного файла
                    QFileSystemModel *model = parentTab->getFileSystemModel();
                    if (model) {
                        QModelIndex newIndex = model->index(newFilePath);
                        if (newIndex.isValid()) {
                            mainWindow->startRename(newIndex);
                        }
                    }
                }
            }
        });
    } else {
        // Используем стилизованное сообщение об ошибке
        QMessageBox *msgBox = createStyledMessageBox(nullptr, Strings::Error, "Не удалось создать файл", QMessageBox::Warning);
        msgBox->exec();
        msgBox->deleteLater();

        qDebug() << "Failed to create file:" << newFilePath;
    }
}

void ContextMenu::createTextFile()
{
    createNewFile("txt");
}

void ContextMenu::createDocFile()
{
    createNewFile("docx");
}

void ContextMenu::editScriptFile()
{
    if (selectedIndexes.isEmpty()) return;

    QString filePath = selectedIndexes.first().data(QFileSystemModel::FilePathRole).toString();

    // Открываем скриптовый файл в блокноте
    QProcess process;
    process.startDetached("notepad.exe", QStringList() << filePath);

    qDebug() << "Opening script file in notepad:" << filePath;
}

void ContextMenu::runPowerShell()
{
    if (selectedIndexes.isEmpty()) return;

    QString filePath = selectedIndexes.first().data(QFileSystemModel::FilePathRole).toString();

    // Запускаем PowerShell скрипт
    QProcess process;
    process.startDetached("powershell.exe", QStringList() << "-File" << filePath);

    qDebug() << "Running PowerShell script:" << filePath;
}

void ContextMenu::runPowerShellAsAdmin()
{
    if (selectedIndexes.isEmpty()) return;

    QString filePath = selectedIndexes.first().data(QFileSystemModel::FilePathRole).toString();

    // Запускаем PowerShell скрипт от имени администратора
    QProcess process;
    process.startDetached("powershell.exe", QStringList() << "Start-Process" << "powershell" <<
                         "-ArgumentList" << QString("\"-File '%1'\"").arg(filePath) <<
                         "-Verb" << "RunAs");

    qDebug() << "Running PowerShell script as admin:" << filePath;
}

// Вспомогательная функция для проверки архивных файлов
bool ContextMenu::isArchiveFile(const QString& filePath) const
{
    QFileInfo fileInfo(filePath);
    QString suffix = fileInfo.suffix().toLower();
    return (suffix == "rar" || suffix == "zip" || suffix == "7z" ||
            suffix == "tar" || suffix == "gz" || suffix == "bz2");
}

// Вспомогательная функция для проверки скриптовых файлов
bool ContextMenu::isScriptFile(const QString& filePath) const
{
    QFileInfo fileInfo(filePath);
    QString suffix = fileInfo.suffix().toLower();
    return (suffix == "bat" || suffix == "ps1" || suffix == "vbs" || suffix == "reg");
}

// Метод для получения пути к 7-Zip
QString ContextMenu::get7zPath()
{
    QSettings settings;
    QString savedPath = settings.value("7z_path").toString();

    // Проверяем сохраненный путь
    if (!savedPath.isEmpty() && QFile::exists(savedPath)) {
        return savedPath;
    }

    // Проверяем стандартные пути установки 7-Zip
    QStringList possiblePaths = {
        "C:\\Program Files\\7-Zip\\7z.exe",
        "C:\\Program Files (x86)\\7-Zip\\7z.exe",
        "R:\\Prog\\7-Zip\\7z.exe",
    };

    for (const QString &path : possiblePaths) {
        if (QFile::exists(path)) {
            settings.setValue("7z_path", path);
            return path;
        }
    }

    // Проверяем, доступен ли 7z в PATH
    QProcess process;
    process.start("where", QStringList() << "7z");
    if (process.waitForFinished(1000) && process.exitCode() == 0) {
        QString path = QString::fromLocal8Bit(process.readAllStandardOutput()).trimmed();
        if (!path.isEmpty() && QFile::exists(path)) {
            settings.setValue("7z_path", path);
            return path;
        }
    }

    // Если 7-Zip не найден, показываем наше диалоговое окно
    QMessageBox *msgBox = createStyledMessageBox(nullptr, "7-Zip не найден",
        "7-Zip не установлен или не найден в системе. Необходимо указать путь к 7z.exe для работы с архивами.",
        QMessageBox::Information);

    QPushButton *browseButton = msgBox->addButton("Указать путь", QMessageBox::ActionRole);
    QPushButton *cancelButton = msgBox->addButton("Отмена", QMessageBox::RejectRole);

    msgBox->exec();

    if (msgBox->clickedButton() == browseButton) {
        show7zPathDialog();
        // Повторно проверяем сохраненный путь
        savedPath = settings.value("7z_path").toString();
        if (!savedPath.isEmpty() && QFile::exists(savedPath)) {
            return savedPath;
        }
    }

    msgBox->deleteLater();
    return QString(); // Путь не найден
}

// Метод для извлечения архивов
void ContextMenu::extractHere()
{
    if (selectedIndexes.isEmpty()) return;

    QString archivePath = selectedIndexes.first().data(QFileSystemModel::FilePathRole).toString();
    QString extractPath = currentDirectory;

    // Получаем путь к 7-Zip
    QString sevenZipPath = get7zPath();
    if (sevenZipPath.isEmpty()) {
        QMessageBox *msgBox = createStyledMessageBox(nullptr, Strings::Error,
                                                   "7-Zip не найден. Операция извлечения отменена.",
                                                   QMessageBox::Warning);
        msgBox->exec();
        msgBox->deleteLater();
        return;
    }

    // Используем Archiver для извлечения
    MainWindow* mainWindow = parentTab ? parentTab->getMainWindow() : nullptr;
    if (mainWindow) {
        Archiver *archiver = mainWindow->getArchiver();
        if (archiver) {
            archiver->extractArchive(archivePath, extractPath);
        }
    }
}

// Реализация создания ярлыка
void ContextMenu::createShortcut()
{
    if (selectedIndexes.isEmpty()) return;

    QString targetPath = selectedIndexes.first().data(QFileSystemModel::FilePathRole).toString();
    QFileInfo targetInfo(targetPath);

    // Используем полное имя файла с расширением вместо baseName
    QString fileName = targetInfo.fileName(); // Получаем полное имя файла с расширением

    // Создаем имя для ярлыка в формате "Ярлык + имя_файла.расширение.lnk"
    QString shortcutName = "Ярлык " + fileName + ".lnk";
    QString shortcutPath = currentDirectory + "/" + shortcutName;

    // Проверяем, не существует ли уже ярлык с таким именем
    int counter = 1;
    while (QFile::exists(shortcutPath)) {
        shortcutName = "Ярлык " + fileName + " (" + QString::number(counter) + ").lnk";
        shortcutPath = currentDirectory + "/" + shortcutName;
        counter++;
    }

#ifdef Q_OS_WIN
    // Создаем ярлык используя Windows API
    HRESULT hres = CoInitialize(NULL);
    IShellLink* pShellLink = NULL;
    hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&pShellLink);

    if (SUCCEEDED(hres)) {
        pShellLink->SetPath(targetPath.toStdWString().c_str());

        IPersistFile* pPersistFile;
        hres = pShellLink->QueryInterface(IID_IPersistFile, (LPVOID*)&pPersistFile);

        if (SUCCEEDED(hres)) {
            hres = pPersistFile->Save(shortcutPath.toStdWString().c_str(), TRUE);
            pPersistFile->Release();

            if (SUCCEEDED(hres)) {
                qDebug() << "Shortcut created:" << shortcutPath;
                emit refreshRequested();

                QMessageBox *msgBox = createStyledMessageBox(nullptr, "Успех",
                    "Ярлык успешно создан", QMessageBox::Information);
                msgBox->exec();
                msgBox->deleteLater();
            }
        }
        pShellLink->Release();
    }
    CoUninitialize();

    if (!SUCCEEDED(hres)) {
        QMessageBox *msgBox = createStyledMessageBox(nullptr, Strings::Error,
            "Не удалось создать ярлык", QMessageBox::Warning);
        msgBox->exec();
        msgBox->deleteLater();
    }
#else
    // Для не-Windows систем можно создать симлинк или показать сообщение
    QMessageBox *msgBox = createStyledMessageBox(nullptr, Strings::Error,
        "Создание ярлыков поддерживается только в Windows", QMessageBox::Warning);
    msgBox->exec();
    msgBox->deleteLater();
#endif
}

// Реализация создания архива
void ContextMenu::createArchive()
{
    if (selectedIndexes.isEmpty()) return;

    QString targetPath = selectedIndexes.first().data(QFileSystemModel::FilePathRole).toString();
    QFileInfo targetInfo(targetPath);

    QString archiveName = targetInfo.fileName() + ".zip";
    QString archivePath = currentDirectory + "/" + archiveName;

    // Проверяем, не существует ли уже архив с таким именем
    if (QFile::exists(archivePath)) {
        QMessageBox *msgBox = createStyledMessageBox(nullptr, "Подтверждение",
            "Архив с таким именем уже существует. Перезаписать?",
            QMessageBox::Question);
        msgBox->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox->setDefaultButton(QMessageBox::No);

        int result = msgBox->exec();
        msgBox->deleteLater();

        if (result != QMessageBox::Yes) {
            return;
        }
    }

    // Получаем путь к 7-Zip
    QString sevenZipPath = get7zPath();
    if (sevenZipPath.isEmpty()) {
        // Показываем сообщение об ошибке
        QMessageBox *msgBox = createStyledMessageBox(nullptr, Strings::Error,
            "7-Zip не найден. Операция создания архива отменена.",
            QMessageBox::Warning);
        msgBox->exec();
        msgBox->deleteLater();
        return;
    }

    // Используем Archiver для создания архива
    MainWindow* mainWindow = parentTab ? parentTab->getMainWindow() : nullptr;
    if (mainWindow) {
        // Получаем Archiver из MainWindow
        Archiver *archiver = mainWindow->getArchiver();
        if (archiver) {
            // Проверяем, что объект archiver все еще существует
            if (archiver) {
                archiver->createArchive(targetPath, archivePath);
            }
        }
    }
}

void ContextMenu::toggleHiddenFiles()
{
    if (parentTab && parentTab->getMainWindow()) {
        parentTab->getMainWindow()->toggleHiddenFiles();
    }
}

QStringList ContextMenu::getSelectedFilePaths() const
{
    QStringList files;
    for (const QModelIndex &index : selectedIndexes) {
        QString path = index.data(QFileSystemModel::FilePathRole).toString();
        files << QDir::cleanPath(path);
    }
    return files;
}

bool ContextMenu::isSingleFileSelected() const
{
    return selectedIndexes.size() == 1 &&
           !selectedIndexes.first().data(QFileSystemModel::FilePathRole).toString().isEmpty();
}

bool ContextMenu::isSingleFolderSelected() const
{
    return selectedIndexes.size() == 1 &&
           selectedIndexes.first().data(QFileSystemModel::FilePathRole).toString().isEmpty();
}

void ContextMenu::open()
{
    if (selectedIndexes.isEmpty()) return;

    QString path = selectedIndexes.first().data(QFileSystemModel::FilePathRole).toString();
    QFileInfo info(path);

    if (info.isDir()) {
        emit navigateRequested(path);
    } else {
        // Проверяем, нужно ли запускать от имени администратора
        QSettings settings;
        bool runAsAdmin = settings.value("RunAsAdmin_" + path, false).toBool();

        if (runAsAdmin) {
            // Запуск от имени администратора
            #ifdef Q_OS_WIN
                qDebug() << "Running as admin:" << path;

                SHELLEXECUTEINFO sei;
                ZeroMemory(&sei, sizeof(sei));
                sei.cbSize = sizeof(sei);
                sei.lpVerb = L"runas";  // Это ключевой параметр для запуска от администратора
                sei.lpFile = reinterpret_cast<LPCWSTR>(path.utf16());
                sei.nShow = SW_SHOWNORMAL;
                sei.fMask = SEE_MASK_NOCLOSEPROCESS;

                if (!ShellExecuteEx(&sei)) {
                    DWORD error = GetLastError();
                    qDebug() << "Failed to run as admin, error:" << error;

                    // Показываем сообщение об ошибке
                    QMessageBox *msgBox = createStyledMessageBox(nullptr, Strings::Error,
                        "Не удалось запустить программу от имени администратора. "
                        "Убедитесь, что у вас есть необходимые права.",
                        QMessageBox::Warning);
                    msgBox->exec();
                    msgBox->deleteLater();
                } else {
                    qDebug() << "Program started as admin successfully";
                }
            #else
                // Для не-Windows систем используем стандартный способ
                QDesktopServices::openUrl(QUrl::fromLocalFile(path));
            #endif
        } else {
            // Стандартный запуск без прав администратора
            QDesktopServices::openUrl(QUrl::fromLocalFile(path));
        }
    }
}

// Запуск файла с программой
void ContextMenu::launchWithProgram(const QString &filePath, const QString &programPath)
{
    if (programPath.isEmpty() || !QFile::exists(programPath)) {
        QMessageBox *msgBox = createStyledMessageBox(nullptr, "Ошибка",
            "Не удалось запустить программу. Файл программы не найден.",
            QMessageBox::Warning);
        msgBox->exec();
        msgBox->deleteLater();
        return;
    }

    // Подготавливаем путь к файлу
    QString nativeFilePath = QDir::toNativeSeparators(filePath);
    QString nativeProgramPath = QDir::toNativeSeparators(programPath);

    // Запускаем программу с файлом
    QProcess process;
    process.setProgram(nativeProgramPath);
    process.setArguments(QStringList() << nativeFilePath);

    if (process.startDetached()) {
        qDebug() << "Program started:" << programPath << "with file:" << filePath;
    } else {
        qDebug() << "Failed to start program:" << programPath;

        // Попробуем через ShellExecute
        SHELLEXECUTEINFO sei = { sizeof(sei) };
        sei.fMask = SEE_MASK_DEFAULT;
        sei.lpVerb = L"open";
        sei.lpFile = reinterpret_cast<LPCWSTR>(nativeProgramPath.utf16());
        sei.lpParameters = reinterpret_cast<LPCWSTR>(nativeFilePath.utf16());
        sei.nShow = SW_SHOWDEFAULT;

        if (!ShellExecuteEx(&sei)) {
            DWORD error = GetLastError();
            qDebug() << "ShellExecuteEx failed, error:" << error;

            QMessageBox *msgBox = createStyledMessageBox(nullptr, "Ошибка",
                "Не удалось запустить программу. Убедитесь, что программа установлена корректно.",
                QMessageBox::Warning);
            msgBox->exec();
            msgBox->deleteLater();
        }
    }
}

// Диалог управления ассоциациями
void ContextMenu::showAssociationsDialog()
{
    // Загружаем текущие ассоциации
    loadAssociations();

    // Создаем диалог
    QDialog *dialog = new QDialog(nullptr);
    dialog->setWindowTitle("Управление ассоциациями программ");
    dialog->setMinimumSize(600, 400);
    styleDialog(dialog);

    QVBoxLayout *mainLayout = new QVBoxLayout(dialog);

    // Таблица для отображения ассоциаций
    QTableWidget *table = new QTableWidget(dialog);
    table->setColumnCount(3);
    table->setHorizontalHeaderLabels(QStringList() << "Расширение" << "Программа" << "Действие");
    table->horizontalHeader()->setStretchLastSection(true);

    // Заполняем таблицу
    QStringList extensions = associations.keys();
    table->setRowCount(extensions.size());

    for (int i = 0; i < extensions.size(); ++i) {
        QString extension = extensions[i];
        QJsonArray programArray = associations[extension].toArray();

        // Объединяем все программы для этого расширения в одну строку
        QStringList programList;
        for (int j = 0; j < programArray.size(); ++j) {
            QString programPath = programArray[j].toString();
            QFileInfo progInfo(programPath);
            programList.append(progInfo.baseName());
        }

        // Расширение
        QTableWidgetItem *extItem = new QTableWidgetItem("." + extension);
        table->setItem(i, 0, extItem);

        // Программы
        QTableWidgetItem *progItem = new QTableWidgetItem(programList.join(", "));
        progItem->setToolTip(programList.join("\n"));
        table->setItem(i, 1, progItem);

        // Кнопка удаления
        QWidget *widget = new QWidget();
        QHBoxLayout *btnLayout = new QHBoxLayout(widget);
        btnLayout->setContentsMargins(0, 0, 0, 0);

        QPushButton *deleteBtn = new QPushButton("Удалить", widget);
        deleteBtn->setProperty("extension", extension);
        connect(deleteBtn, &QPushButton::clicked, this, [this, dialog, table, extension]() {
            // Удаляем все ассоциации для этого расширения
            associations.remove(extension);
            saveAssociations();

            // Обновляем таблицу
            showAssociationsDialog();
            dialog->close();
            dialog->deleteLater();
        });

        btnLayout->addWidget(deleteBtn);
        btnLayout->addStretch();

        table->setCellWidget(i, 2, widget);
    }

    table->resizeColumnsToContents();
    mainLayout->addWidget(table);

    // Кнопки
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *closeButton = new QPushButton("Закрыть", dialog);
    connect(closeButton, &QPushButton::clicked, dialog, &QDialog::accept);

    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);
    mainLayout->addLayout(buttonLayout);

    dialog->exec();
    dialog->deleteLater();
}

// Показываем меню "Открыть с помощью"
void ContextMenu::showOpenWithMenu(const QString &filePath)
{
    QFileInfo fileInfo(filePath);
    QString extension = fileInfo.suffix().toLower();

    // Загружаем текущие ассоциации
    loadAssociations();

    // Получаем программы для этого расширения
    QStringList programs = getProgramsForExtension(extension);

    // Создаем меню
    QMenu openWithMenu("Открыть с помощью");
    openWithMenu.setStyleSheet(this->styleSheet()); // Применяем тот же стиль

    // Добавляем действия для каждой программы
    QMap<QAction*, QString> actionProgramMap; // Для связи действия с путем к программе

    for (const QString &programPath : programs) {
        if (QFile::exists(programPath)) {
            QFileInfo progInfo(programPath);
            QString displayName = progInfo.baseName(); // Используем имя файла без расширения

            // Создаем действие с иконкой программы
            QAction *action = openWithMenu.addAction(getProgramIcon(programPath), displayName);
            action->setData(programPath);
            actionProgramMap[action] = programPath;
        }
    }

    // Если есть программы, добавляем разделитель
    if (!programs.isEmpty()) {
        openWithMenu.addSeparator();
    }

    // Добавляем действие для выбора другой программы
    QAction *chooseAction = openWithMenu.addAction("Выбрать программу...");
    chooseAction->setIcon(QIcon(":/icons/choose_program.png")); // Можно добавить иконку

    // Добавляем действие для настройки ассоциаций
    QAction *manageAction = openWithMenu.addAction("Управление ассоциациями...");

    // Показываем меню
    QAction *selectedAction = openWithMenu.exec(QCursor::pos());

    if (selectedAction) {
        if (selectedAction == chooseAction) {
            // Выбор новой программы
            chooseProgramForExtension(extension);

            // После выбора программы показываем меню снова
            QTimer::singleShot(100, [this, filePath]() {
                showOpenWithMenu(filePath);
            });
        } else if (selectedAction == manageAction) {
            // Показываем диалог управления ассоциациями
            showAssociationsDialog();
        } else if (actionProgramMap.contains(selectedAction)) {
            // Запускаем файл с выбранной программой
            QString programPath = actionProgramMap[selectedAction];
            launchWithProgram(filePath, programPath);
        }
    }
}

// Метод для получения списка похожих расширений
QStringList ContextMenu::getSimilarExtensions(const QString &extension)
{
    QMap<QString, QStringList> extensionGroups = {
        // Изображения
        {"png", {"jpg", "jpeg", "bmp", "gif", "tiff", "tif", "ico", "webp", "svg", "raw", "nef", "cr2"}},
        {"jpg", {"jpeg", "png", "bmp", "gif", "tiff", "tif", "ico", "webp", "raw", "nef", "cr2"}},
        {"jpeg", {"jpg", "png", "bmp", "gif", "tiff", "tif", "ico", "webp", "raw", "nef", "cr2"}},
        {"bmp", {"png", "jpg", "jpeg", "gif", "tiff", "tif", "ico", "webp"}},
        {"gif", {"png", "jpg", "jpeg", "bmp", "webp"}},
        {"tiff", {"tif", "png", "jpg", "jpeg", "bmp", "raw"}},
        {"tif", {"tiff", "png", "jpg", "jpeg", "bmp", "raw"}},
        {"webp", {"png", "jpg", "jpeg", "gif"}},
        {"svg", {"png", "jpg", "jpeg"}},
        {"raw", {"nef", "cr2", "arw", "dng", "tiff", "tif"}},

        // Аудио
        {"mp3", {"wav", "m4a", "flac", "aac", "ogg", "wma", "opus", "mid", "midi"}},
        {"wav", {"mp3", "m4a", "flac", "aac", "ogg", "wma", "opus"}},
        {"m4a", {"mp3", "wav", "flac", "aac", "ogg", "wma", "opus"}},
        {"flac", {"mp3", "wav", "m4a", "aac", "ogg", "wma", "opus"}},
        {"aac", {"mp3", "wav", "m4a", "flac", "ogg", "wma", "opus"}},
        {"ogg", {"mp3", "wav", "m4a", "flac", "aac", "opus"}},
        {"wma", {"mp3", "wav", "m4a", "flac", "aac"}},
        {"opus", {"mp3", "wav", "m4a", "flac", "aac", "ogg"}},

        // Видео
        {"mp4", {"avi", "mkv", "mov", "wmv", "flv", "webm", "m4v", "mpg", "mpeg", "3gp"}},
        {"avi", {"mp4", "mkv", "mov", "wmv", "flv", "webm", "m4v", "mpg", "mpeg"}},
        {"mkv", {"mp4", "avi", "mov", "wmv", "flv", "webm", "m4v", "mpg", "mpeg"}},
        {"mov", {"mp4", "avi", "mkv", "wmv", "flv", "webm", "m4v"}},
        {"wmv", {"mp4", "avi", "mkv", "mov", "flv", "webm", "m4v"}},
        {"flv", {"mp4", "avi", "mkv", "mov", "wmv", "webm"}},
        {"webm", {"mp4", "avi", "mkv", "mov", "wmv", "flv", "m4v"}},
        {"m4v", {"mp4", "avi", "mkv", "mov", "wmv", "webm"}},
        {"mpg", {"mpeg", "mp4", "avi", "mkv"}},
        {"mpeg", {"mpg", "mp4", "avi", "mkv"}},

        // Документы
        {"pdf", {"doc", "docx", "txt", "rtf", "odt", "xls", "xlsx", "ppt", "pptx"}},
        {"doc", {"docx", "pdf", "txt", "rtf", "odt"}},
        {"docx", {"doc", "pdf", "txt", "rtf", "odt"}},
        {"txt", {"doc", "docx", "pdf", "rtf", "odt"}},
        {"rtf", {"doc", "docx", "pdf", "txt", "odt"}},
        {"odt", {"doc", "docx", "pdf", "txt", "rtf"}},
        {"xls", {"xlsx", "pdf", "ods", "csv"}},
        {"xlsx", {"xls", "pdf", "ods", "csv"}},
        {"csv", {"xls", "xlsx", "pdf", "ods"}},
        {"ppt", {"pptx", "pdf", "odp"}},
        {"pptx", {"ppt", "pdf", "odp"}},

        // Архивы
        {"zip", {"rar", "7z", "tar", "gz", "bz2", "xz", "lz", "lzma"}},
        {"rar", {"zip", "7z", "tar", "gz", "bz2", "xz"}},
        {"7z", {"zip", "rar", "tar", "gz", "bz2", "xz"}},
        {"tar", {"zip", "rar", "7z", "gz", "bz2", "xz"}},
        {"gz", {"zip", "rar", "7z", "tar", "bz2", "xz"}},
        {"bz2", {"zip", "rar", "7z", "tar", "gz", "xz"}},
        {"xz", {"zip", "rar", "7z", "tar", "gz", "bz2"}},

        // Скрипты
        {"ps1", {"bat", "cmd", "vbs", "js", "py", "sh"}},
        {"bat", {"cmd", "ps1", "vbs", "js", "py"}},
        {"cmd", {"bat", "ps1", "vbs", "js", "py"}},
        {"vbs", {"bat", "cmd", "ps1", "js", "py"}},
        {"js", {"ps1", "bat", "cmd", "vbs", "py"}},
        {"py", {"ps1", "bat", "cmd", "vbs", "js"}},
        {"sh", {"bash", "ps1", "py"}},

        // Разное
        {"ini", {"cfg", "conf", "config", "xml", "json", "yaml", "yml"}},
        {"cfg", {"ini", "conf", "config", "xml", "json"}},
        {"conf", {"ini", "cfg", "config", "xml", "json"}},
        {"xml", {"ini", "cfg", "conf", "json", "yaml", "yml"}},
        {"json", {"ini", "cfg", "conf", "xml", "yaml", "yml"}},
        {"yaml", {"yml", "xml", "json", "ini"}},
        {"yml", {"yaml", "xml", "json", "ini"}}
    };

    return extensionGroups.value(extension, QStringList());
}

// Метод для предложения программы для похожих расширений
void ContextMenu::suggestProgramForSimilarExtensions(const QString &extension, const QString &programPath)
{
    QStringList similarExtensions = getSimilarExtensions(extension);
    if (similarExtensions.isEmpty()) {
        return;
    }

    // Находим расширения, для которых эта программа еще не добавлена
    QStringList extensionsToSuggest;
    for (const QString &similarExt : similarExtensions) {
        QStringList existingPrograms = getProgramsForExtension(similarExt);
        if (!existingPrograms.contains(programPath)) {
            extensionsToSuggest.append(similarExt);
        }
    }

    if (extensionsToSuggest.isEmpty()) {
        return;
    }

    // Сортируем расширения для лучшего отображения
    extensionsToSuggest.sort();

    // Создаем диалог предложения
    QDialog *suggestionDialog = new QDialog(nullptr);
    suggestionDialog->setWindowTitle("Добавить программу для похожих форматов?");
    suggestionDialog->setMinimumWidth(500);
    styleDialog(suggestionDialog);

    QVBoxLayout *mainLayout = new QVBoxLayout(suggestionDialog);

    // Сообщение
    QFileInfo progInfo(programPath);
    QString programName = progInfo.baseName();

    QLabel *messageLabel = new QLabel(
        QString("Добавить программу <b>%1</b> для следующих похожих форматов?").arg(programName),
        suggestionDialog
    );
    messageLabel->setWordWrap(true);
    mainLayout->addWidget(messageLabel);

    // Список расширений с чекбоксами
    QListWidget *extensionsList = new QListWidget(suggestionDialog);
    extensionsList->setSelectionMode(QAbstractItemView::NoSelection);

    for (const QString &ext : extensionsToSuggest) {
        QListWidgetItem *item = new QListWidgetItem("." + ext, extensionsList);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Checked);
        item->setData(Qt::UserRole, ext);
    }

    mainLayout->addWidget(extensionsList);

    // Чекбокс "Больше не показывать"
    QCheckBox *dontShowAgainCheckbox = new QCheckBox("Больше не предлагать для похожих форматов", suggestionDialog);
    mainLayout->addWidget(dontShowAgainCheckbox);

    // Кнопки
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *yesButton = new QPushButton("Добавить выбранные", suggestionDialog);
    QPushButton *noButton = new QPushButton("Нет, только для ." + extension, suggestionDialog);
    QPushButton *cancelButton = new QPushButton("Отмена", suggestionDialog);

    buttonLayout->addStretch();
    buttonLayout->addWidget(yesButton);
    buttonLayout->addWidget(noButton);
    buttonLayout->addWidget(cancelButton);
    mainLayout->addLayout(buttonLayout);

    // Подключаем сигналы
    connect(yesButton, &QPushButton::clicked, suggestionDialog, [this, suggestionDialog, extensionsList, programPath, dontShowAgainCheckbox]() {
        // Сохраняем настройку "не показывать"
        QSettings settings;
        if (dontShowAgainCheckbox->isChecked()) {
            settings.setValue("dont_suggest_similar_extensions", true);
        }

        // Добавляем программу для выбранных расширений
        for (int i = 0; i < extensionsList->count(); ++i) {
            QListWidgetItem *item = extensionsList->item(i);
            if (item->checkState() == Qt::Checked) {
                QString ext = item->data(Qt::UserRole).toString();
                addProgramForExtension(ext, programPath);
            }
        }

        suggestionDialog->accept();
    });

    connect(noButton, &QPushButton::clicked, suggestionDialog, [this, suggestionDialog, extension, programPath, dontShowAgainCheckbox]() {
        // Сохраняем настройку "не показывать"
        QSettings settings;
        if (dontShowAgainCheckbox->isChecked()) {
            settings.setValue("dont_suggest_similar_extensions", true);
        }

        suggestionDialog->accept();
    });

    connect(cancelButton, &QPushButton::clicked, suggestionDialog, &QDialog::reject);

    // Показываем диалог
    suggestionDialog->exec();
    suggestionDialog->deleteLater();
}

// Выбор программы для расширения
void ContextMenu::chooseProgramForExtension(const QString &extension)
{
    // Открываем диалог выбора файла
    QString programPath = QFileDialog::getOpenFileName(
        nullptr,
        "Выберите программу для открытия файлов ." + extension,
        "C:\\Program Files",
        "Исполняемые файлы (*.exe);;Все файлы (*.*)"
    );

    if (!programPath.isEmpty()) {
        // Проверяем, что файл существует
        if (QFile::exists(programPath)) {
            // Добавляем программу для текущего расширения
            addProgramForExtension(extension, programPath);

            // Проверяем, нужно ли предлагать для похожих расширений
            QSettings settings;
            bool dontSuggest = settings.value("dont_suggest_similar_extensions", false).toBool();

            if (!dontSuggest) {
                // Предлагаем для похожих расширений
                suggestProgramForSimilarExtensions(extension, programPath);
            }

            // Показываем сообщение об успехе
            QMessageBox *msgBox = createStyledMessageBox(nullptr, "Успех",
                QString("Программа успешно добавлена для открытия файлов .%1").arg(extension),
                QMessageBox::Information);
            msgBox->exec();
            msgBox->deleteLater();
        } else {
            QMessageBox *msgBox = createStyledMessageBox(nullptr, "Ошибка",
                "Выбранный файл не существует.",
                QMessageBox::Warning);
            msgBox->exec();
            msgBox->deleteLater();
        }
    }
}

// Измененная функция openWith()
void ContextMenu::openWith()
{
    if (selectedIndexes.isEmpty()) return;

    QString filePath = selectedIndexes.first().data(QFileSystemModel::FilePathRole).toString();

    // Проверяем, существует ли файл
    if (!QFile::exists(filePath)) {
        QMessageBox *msgBox = createStyledMessageBox(nullptr, "Ошибка",
            "Файл не найден.",
            QMessageBox::Warning);
        msgBox->exec();
        msgBox->deleteLater();
        return;
    }

    // Показываем наше меню с программами
    showOpenWithMenu(filePath);
}

void ContextMenu::cut()
{
    if (selectedIndexes.isEmpty() || !fileOperations) {
        qDebug() << "Cut: No selection or fileOperations not set";
        return;
    }
    fileOperations->cutFiles(getSelectedFilePaths());
}

void ContextMenu::copy()
{
    if (selectedIndexes.isEmpty() || !fileOperations) {
        qDebug() << "Copy: No selection or fileOperations not set";
        return;
    }
    fileOperations->copyFiles(getSelectedFilePaths());
}

void ContextMenu::paste()
{
    if (!fileOperations) {
        qDebug() << "Paste: fileOperations not set";
        return;
    }
    fileOperations->pasteFiles(currentDirectory);
}

void ContextMenu::rename()
{
    if (selectedIndexes.size() != 1) {
        qDebug() << "Rename: Single selection required";
        return;
    }

    if (parentTab) {
        // Получаем индекс для переименования
        QModelIndex index = selectedIndexes.first();

        // Используем прямое обращение к MainWindow
        MainWindow* mainWindow = parentTab->getMainWindow();
        if (mainWindow) {
            mainWindow->startRename(index);
        }
    } else {
        qDebug() << "Rename: Parent tab not set";

        QMessageBox *msgBox = createStyledMessageBox(nullptr, Strings::Error, "Parent tab is not set", QMessageBox::Warning);
        msgBox->exec();
        msgBox->deleteLater();
    }
}

void ContextMenu::deleteFile()
{
    if (selectedIndexes.isEmpty() || !fileOperations) {
        qDebug() << "Delete: No selection or fileOperations not set";
        return;
    }
    fileOperations->deleteFiles(getSelectedFilePaths());
}

void ContextMenu::newFolder()
{
    if (parentTab) {
        // Используем прямое обращение к MainWindow
        MainWindow* mainWindow = parentTab->getMainWindow();
        if (mainWindow) {
            mainWindow->startCreateNewFolder();
        }
    } else {
        qDebug() << "New Folder: Parent tab not set";

        QMessageBox *msgBox = createStyledMessageBox(nullptr, Strings::Error, "Parent tab is not set", QMessageBox::Warning);
        msgBox->exec();
        msgBox->deleteLater();
    }
}

void ContextMenu::properties()
{
    if (selectedIndexes.size() != 1) return;

    QString path = selectedIndexes.first().data(QFileSystemModel::FilePathRole).toString();

    // Create and show properties dialog
    PropertiesDialog *propertiesDialog = new PropertiesDialog(path, nullptr);
    propertiesDialog->setAttribute(Qt::WA_DeleteOnClose);
    propertiesDialog->exec();
}

void ContextMenu::showInExplorer()
{
    if (selectedIndexes.isEmpty()) return;

    QString path = selectedIndexes.first().data(QFileSystemModel::FilePathRole).toString();
    QProcess::startDetached("explorer", {"/select,", QDir::toNativeSeparators(path)});
}