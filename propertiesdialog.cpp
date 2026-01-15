#include "propertiesdialog.h"
#include <QSettings>
#include <QDebug>
#include <QDirIterator>
#include <QStorageInfo>
#include <QProcess>
#include <QFileDialog>

#ifdef Q_OS_WIN
#include <shellapi.h>
#include <shlobj.h>
#include <accctrl.h>
#include <aclapi.h>
#include <objbase.h>
#include <propvarutil.h>
#include <propkey.h>
#include <shobjidl.h>

// Убираем pragma comment для MinGW
// #pragma comment(lib, "propsys.lib")
// #pragma comment(lib, "ole32.lib")
// #pragma comment(lib, "shell32.lib")
#endif

PropertiesDialog::PropertiesDialog(const QString &filePath, QWidget *parent)
    : QDialog(parent), filePath(filePath), fileInfo(filePath), totalSize(0), sizeOnDisk(0), hasChanges(false)
{
    setupUI();
    loadFileInfo();
    updateUI();

    // Start size calculation in background
    QTimer::singleShot(100, this, &PropertiesDialog::calculateSize);
}

PropertiesDialog::~PropertiesDialog()
{
}

void PropertiesDialog::setupUI()
{
    setWindowTitle(Strings::PropertiesTitle + " - " + fileInfo.fileName());
    setMinimumSize(500, 650);

    // Apply dark theme styling
    setStyleSheet(
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
        "    border-radius: " + Colors::RadiusSmall + ";"
        "    padding: 4px;"
        "    color: " + Colors::TextLight + ";"
        "}"
        "QCheckBox {"
        "    color: " + Colors::TextLight + ";"
        "    spacing: 8px;"
        "}"
        "QCheckBox::indicator {"
        "    width: 16px;"
        "    height: 16px;"
        "}"
        "QCheckBox::indicator:unchecked {"
        "    background: " + Colors::DarkTertiary + ";"
        "    border: 1px solid " + Colors::TextDarkGray + ";"
        "    border-radius: 2px;"
        "}"
        "QCheckBox::indicator:checked {"
        "    background: " + Colors::TextBlue + ";"
        "    border: 1px solid " + Colors::TextBlue + ";"
        "    border-radius: 2px;"
        "}"
        "QPushButton {"
        "    background: " + Colors::DarkTertiary + ";"
        "    color: " + Colors::TextLight + ";"
        "    border: 1px solid " + Colors::TextDarkGray + ";"
        "    border-radius: " + Colors::RadiusSmall + ";"
        "    padding: 8px 16px;"
        "    min-width: 80px;"
        "}"
        "QPushButton:hover {"
        "    background: " + Colors::DarkQuaternary + ";"
        "    border: 1px solid " + Colors::TextBlueLight + ";"
        "}"
        "QPushButton:pressed {"
        "    background: " + Colors::ButtonPressedStart + ";"
        "}"
        "QTabWidget::pane {"
        "    border: 1px solid " + Colors::DarkTertiary + ";"
        "    background: " + Colors::DarkPrimary + ";"
        "}"
        "QTabWidget::tab-bar {"
        "    alignment: center;"
        "}"
        "QTabBar::tab {"
        "    background: " + Colors::DarkTertiary + ";"
        "    color: " + Colors::TextLight + ";"
        "    padding: 8px 16px;"
        "    margin: 2px;"
        "    border-radius: " + Colors::RadiusSmall + ";"
        "}"
        "QTabBar::tab:selected {"
        "    background: " + Colors::TextBlue + ";"
        "    color: " + Colors::TextWhite + ";"
        "}"
        "QTabBar::tab:hover {"
        "    background: " + Colors::DarkQuaternary + ";"
        "}"
        "QGroupBox {"
        "    color: " + Colors::TextLight + ";"
        "    border: 1px solid " + Colors::DarkTertiary + ";"
        "    border-radius: " + Colors::RadiusMedium + ";"
        "    margin-top: 1ex;"
        "    padding-top: 10px;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    subcontrol-position: top center;"
        "    padding: 0 8px;"
        "}"
        "QListWidget, QTextEdit {"
        "    background: " + Colors::DarkTertiary + ";"
        "    border: 1px solid " + Colors::TextDarkGray + ";"
        "    border-radius: " + Colors::RadiusSmall + ";"
        "    color: " + Colors::TextLight + ";"
        "}"
    );

    // Create tab widget
    tabWidget = new QTabWidget(this);

    // Create tabs
    generalTab = new QWidget();
    securityTab = new QWidget();
    advancedTab = new QWidget();

    tabWidget->addTab(generalTab, "Общие");
    tabWidget->addTab(securityTab, "Безопасность");
    tabWidget->addTab(advancedTab, "Дополнительно");

    // Setup General Tab
    QVBoxLayout *generalLayout = new QVBoxLayout(generalTab);

    // File icon and name
    QHBoxLayout *nameLayout = new QHBoxLayout();
    iconLabel = new QLabel();
    QFileIconProvider iconProvider;
    QIcon icon = iconProvider.icon(fileInfo);
    iconLabel->setPixmap(icon.pixmap(64, 64));
    iconLabel->setAlignment(Qt::AlignTop);

    QVBoxLayout *nameInfoLayout = new QVBoxLayout();
    fileNameLabel = new QLabel("Имя файла:");
    fileNameEdit = new QLineEdit(fileInfo.fileName());
    nameInfoLayout->addWidget(fileNameLabel);
    nameInfoLayout->addWidget(fileNameEdit);

    nameLayout->addWidget(iconLabel);
    nameLayout->addLayout(nameInfoLayout);
    nameLayout->addStretch();

    generalLayout->addLayout(nameLayout);

    // File information
    QFormLayout *infoLayout = new QFormLayout();

    fileTypeLabel = new QLabel();
    fileLocationLabel = new QLabel();
    fileSizeLabel = new QLabel("Вычисление...");
    fileSizeOnDiskLabel = new QLabel("Вычисление...");
    createdLabel = new QLabel();
    modifiedLabel = new QLabel();
    accessedLabel = new QLabel();

    infoLayout->addRow("Тип:", fileTypeLabel);
    infoLayout->addRow("Расположение:", fileLocationLabel);
    infoLayout->addRow("Размер:", fileSizeLabel);
    infoLayout->addRow("На диске:", fileSizeOnDiskLabel);
    infoLayout->addRow("Создан:", createdLabel);
    infoLayout->addRow("Изменен:", modifiedLabel);
    infoLayout->addRow("Открыт:", accessedLabel);

    generalLayout->addLayout(infoLayout);

    // Attributes
    QGroupBox *attributesGroup = new QGroupBox("Атрибуты");
    QVBoxLayout *attributesLayout = new QVBoxLayout(attributesGroup);

    readOnlyCheckBox = new QCheckBox("Только для чтения");
    hiddenCheckBox = new QCheckBox("Скрытый");
    archiveCheckBox = new QCheckBox("Архивный");

    attributesLayout->addWidget(readOnlyCheckBox);
    attributesLayout->addWidget(hiddenCheckBox);
    attributesLayout->addWidget(archiveCheckBox);

    generalLayout->addWidget(attributesGroup);
    generalLayout->addStretch();

    // Setup Security Tab
    QVBoxLayout *securityLayout = new QVBoxLayout(securityTab);

    securityList = new QListWidget();
    securityDescription = new QTextEdit();
    securityDescription->setReadOnly(true);
    securityDescription->setMaximumHeight(150);

    advancedSecurityButton = new QPushButton("Дополнительно...");
    advancedSecurityButton->setEnabled(false);

    securityLayout->addWidget(new QLabel("Группы или пользователи:"));
    securityLayout->addWidget(securityList);
    securityLayout->addWidget(new QLabel("Разрешения для выбранного элемента:"));
    securityLayout->addWidget(securityDescription);
    securityLayout->addWidget(advancedSecurityButton, 0, Qt::AlignRight);
    securityLayout->addStretch();

    // Setup Advanced Tab
    QVBoxLayout *advancedLayout = new QVBoxLayout(advancedTab);

    runAsAdminCheckBox = new QCheckBox("Запускать эту программу от имени администратора");

    // Проверяем, является ли файл исполняемым
    bool isExecutable = isExecutableFile(filePath);
    runAsAdminCheckBox->setVisible(isExecutable && !fileInfo.isDir());

    if (isExecutable && !fileInfo.isDir()) {
        QLabel *adminHint = new QLabel("При включении этой опции при запуске файла будет запрашиваться повышение прав администратора.");
        adminHint->setStyleSheet("color: " + Colors::TextMuted + "; font-size: 10px;");
        adminHint->setWordWrap(true);
        advancedLayout->addWidget(adminHint);

        // Информация о методе elevation
        elevationMethodLabel = new QLabel();
        elevationMethodLabel->setStyleSheet("color: " + Colors::TextGray + "; font-size: 10px;"); // Changed from TextGreen to TextGray
        elevationMethodLabel->setWordWrap(true);
        advancedLayout->addWidget(elevationMethodLabel);
    }

    attributesGroup = new QGroupBox("Дополнительные атрибуты");
    QVBoxLayout *advancedAttributesLayout = new QVBoxLayout(attributesGroup);

    compressCheckBox = new QCheckBox("Сжимать содержимое для экономии места на диске");
    encryptCheckBox = new QCheckBox("Шифровать содержимое для защиты данных");

    advancedAttributesLayout->addWidget(compressCheckBox);
    advancedAttributesLayout->addWidget(encryptCheckBox);

    advancedLayout->addWidget(runAsAdminCheckBox);
    advancedLayout->addWidget(attributesGroup);
    advancedLayout->addStretch();

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    applyButton = new QPushButton("Применить");
    okButton = new QPushButton("OK");
    cancelButton = new QPushButton("Отмена");

    buttonLayout->addStretch();
    buttonLayout->addWidget(applyButton);
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);

    // Main layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(tabWidget);
    mainLayout->addLayout(buttonLayout);

    // Connect signals
    connect(applyButton, &QPushButton::clicked, this, &PropertiesDialog::applyChanges);
    connect(okButton, &QPushButton::clicked, this, &PropertiesDialog::okClicked);
    connect(cancelButton, &QPushButton::clicked, this, &PropertiesDialog::cancelClicked);
    connect(fileNameEdit, &QLineEdit::textChanged, this, &PropertiesDialog::onFileNameChanged);
    connect(runAsAdminCheckBox, &QCheckBox::toggled, this, &PropertiesDialog::onRunAsAdminToggled);
    connect(hiddenCheckBox, &QCheckBox::toggled, this, &PropertiesDialog::onHiddenToggled);
    connect(readOnlyCheckBox, &QCheckBox::toggled, this, &PropertiesDialog::onReadOnlyToggled);
    connect(archiveCheckBox, &QCheckBox::toggled, this, &PropertiesDialog::onArchiveToggled);
    connect(compressCheckBox, &QCheckBox::toggled, this, &PropertiesDialog::onCompressToggled);
    connect(encryptCheckBox, &QCheckBox::toggled, this, &PropertiesDialog::onEncryptToggled);
}

void PropertiesDialog::loadFileInfo()
{
    isFolder = fileInfo.isDir();
    originalFileName = fileInfo.fileName();

    // Load basic file info
    fileTypeLabel->setText(getFileType(filePath));
    fileLocationLabel->setText(fileInfo.absolutePath());

    if (fileInfo.birthTime().isValid()) {
        createdLabel->setText(fileInfo.birthTime().toString("dd.MM.yyyy hh:mm:ss"));
    } else {
        createdLabel->setText("Неизвестно");
    }

    if (fileInfo.lastModified().isValid()) {
        modifiedLabel->setText(fileInfo.lastModified().toString("dd.MM.yyyy hh:mm:ss"));
    } else {
        modifiedLabel->setText("Неизвестно");
    }

    if (fileInfo.lastRead().isValid()) {
        accessedLabel->setText(fileInfo.lastRead().toString("dd.MM.yyyy hh:mm:ss"));
    } else {
        accessedLabel->setText("Неизвестно");
    }

    // Load file attributes
#ifdef Q_OS_WIN
    originalAttributes = getFileAttributes(filePath);
    readOnlyCheckBox->setChecked(originalAttributes & FILE_ATTRIBUTE_READONLY);
    hiddenCheckBox->setChecked(originalAttributes & FILE_ATTRIBUTE_HIDDEN);
    archiveCheckBox->setChecked(originalAttributes & FILE_ATTRIBUTE_ARCHIVE);
    compressCheckBox->setChecked(originalAttributes & FILE_ATTRIBUTE_COMPRESSED);
    encryptCheckBox->setChecked(originalAttributes & FILE_ATTRIBUTE_ENCRYPTED);
#else
    readOnlyCheckBox->setChecked(!fileInfo.isWritable());
    hiddenCheckBox->setChecked(fileInfo.fileName().startsWith('.'));
    archiveCheckBox->setChecked(false);
    compressCheckBox->setEnabled(false);
    encryptCheckBox->setEnabled(false);
#endif

    // Проверяем, является ли файл исполняемым
    bool isExecutable = isExecutableFile(filePath);

    if (isExecutable && !fileInfo.isDir()) {
#ifdef Q_OS_WIN
        // Проверяем текущие настройки elevation
        bool hasRegistrySetting = checkRunAsAdminRegistry(filePath);
        bool hasManifestElevation = hasElevationRequirement(filePath);

        originalRunAsAdmin = hasRegistrySetting || hasManifestElevation;

        // Отображаем информацию о методе
        QString methodInfo;
        if (hasManifestElevation) {
            methodInfo = "Используется манифест файла (рекомендуемый метод)";
            elevationMethodLabel->setText("✓ " + methodInfo);
        } else if (hasRegistrySetting) {
            methodInfo = "Используется настройка реестра";
            elevationMethodLabel->setText("⚠ " + methodInfo);
        } else {
            methodInfo = "Используется ярлык с elevation (создается автоматически)";
            elevationMethodLabel->setText("⚡ " + methodInfo);
        }
#endif
    } else {
        originalRunAsAdmin = false;
    }

    runAsAdminCheckBox->setChecked(originalRunAsAdmin);
    runAsAdminCheckBox->setVisible(isExecutable && !fileInfo.isDir());

    // Load security info
    loadSecurityInfo();
}

bool PropertiesDialog::isExecutableFile(const QString &filePath) const
{
    QFileInfo info(filePath);
    QString extension = info.suffix().toLower();
    QStringList executableExtensions = {"exe", "com", "bat", "cmd", "msi", "ps1", "scr", "vbs", "js", "wsf"};

    return executableExtensions.contains(extension);
}

#ifdef Q_OS_WIN
// Метод 1: Установка требования elevation через реестр
bool PropertiesDialog::setRunAsAdminRegistry(const QString &filePath, bool runAsAdmin)
{
    HKEY hKey = nullptr;
    LSTATUS result = ERROR_SUCCESS;
    QString keyPath = QString("Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers");

    // Открываем или создаем ключ реестра
    result = RegOpenKeyExW(HKEY_CURRENT_USER,
                          keyPath.toStdWString().c_str(),
                          0,
                          KEY_WRITE,
                          &hKey);

    if (result != ERROR_SUCCESS) {
        result = RegCreateKeyExW(HKEY_CURRENT_USER,
                                keyPath.toStdWString().c_str(),
                                0,
                                nullptr,
                                REG_OPTION_NON_VOLATILE,
                                KEY_WRITE,
                                nullptr,
                                &hKey,
                                nullptr);
    }

    if (result == ERROR_SUCCESS && hKey != nullptr) {
        QString valueName = QDir::toNativeSeparators(filePath);

        if (runAsAdmin) {
            // Устанавливаем значение RUNASADMIN
            std::wstring value = L"RUNASADMIN";
            result = RegSetValueExW(hKey,
                                   valueName.toStdWString().c_str(),
                                   0,
                                   REG_SZ,
                                   reinterpret_cast<const BYTE*>(value.c_str()),
                                   static_cast<DWORD>((value.length() + 1) * sizeof(wchar_t)));
        } else {
            // Удаляем значение
            result = RegDeleteValueW(hKey, valueName.toStdWString().c_str());
            if (result == ERROR_FILE_NOT_FOUND) {
                result = ERROR_SUCCESS;
            }
        }

        RegCloseKey(hKey);
        return (result == ERROR_SUCCESS);
    }

    return false;
}

bool PropertiesDialog::checkRunAsAdminRegistry(const QString &filePath)
{
    HKEY hKey = nullptr;
    QString keyPath = QString("Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers");
    QString valueName = QDir::toNativeSeparators(filePath);

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
            return value.contains("RUNASADMIN", Qt::CaseInsensitive);
        }
    }

    return false;
}

// Метод 2: Проверка манифеста файла
bool PropertiesDialog::hasElevationRequirement(const QString &filePath)
{
    // Проверяем, содержит ли файл манифест с требованием elevation
    HMODULE hModule = LoadLibraryExW(filePath.toStdWString().c_str(),
                                     nullptr,
                                     LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE);

    if (hModule == nullptr) {
        return false;
    }

    HRSRC hRes = FindResourceW(hModule, MAKEINTRESOURCE(1), RT_MANIFEST);
    bool hasElevation = false;

    if (hRes != nullptr) {
        HGLOBAL hResData = LoadResource(hModule, hRes);
        if (hResData != nullptr) {
            DWORD size = SizeofResource(hModule, hRes);
            LPVOID data = LockResource(hResData);

            if (data != nullptr && size > 0) {
                // Преобразуем данные в QString
                QString manifest = QString::fromUtf16(static_cast<const char16_t*>(data), size / 2);
                // Проверяем наличие требований администратора в манифесте
                if (manifest.contains("requireAdministrator", Qt::CaseInsensitive) ||
                    manifest.contains("level=\"requireAdministrator\"", Qt::CaseInsensitive)) {
                    hasElevation = true;
                }
            }
        }
    }

    FreeLibrary(hModule);
    return hasElevation;
}

// Метод 3: Создание ярлыка с elevation (упрощенная версия для MinGW)
bool PropertiesDialog::createElevatedShortcut(const QString &filePath)
{
    // В MinGW нет полной поддержки COM для создания ярлыков с флагами
    // Используем более простой метод: создаем файл .vbs скрипт, который запускает с правами админа
    QString shortcutPath = QDir::tempPath() + "/" +
                          QFileInfo(filePath).baseName() +
                          "_Elevated.vbs";

    // Создаем VBS скрипт для запуска с правами администратора
    QFile vbsFile(shortcutPath);
    if (vbsFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&vbsFile);
        out << "Set UAC = CreateObject(\"Shell.Application\")\n";
        out << "UAC.ShellExecute \"" << QDir::toNativeSeparators(filePath) << "\", \"\", \"\", \"runas\", 1\n";
        vbsFile.close();

        // Также создаем bat файл для удобства
        QString batPath = QDir::tempPath() + "/" +
                         QFileInfo(filePath).baseName() +
                         "_Elevated.bat";
        QFile batFile(batPath);
        if (batFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream batOut(&batFile);
            batOut << "@echo off\n";
            batOut << "echo Запуск " << QFileInfo(filePath).fileName() << " от имени администратора...\n";
            batOut << "start \"\" \"" << shortcutPath << "\"\n";
            batFile.close();

            return true;
        }
    }

    return false;
}

bool PropertiesDialog::removeElevatedShortcut(const QString &filePath)
{
    QString shortcutPath = QDir::tempPath() + "/" +
                          QFileInfo(filePath).baseName() +
                          "_Elevated.vbs";

    QString batPath = QDir::tempPath() + "/" +
                     QFileInfo(filePath).baseName() +
                     "_Elevated.bat";

    bool removed1 = QFile::remove(shortcutPath);
    bool removed2 = QFile::remove(batPath);

    return removed1 || removed2;
}

// Метод 4: Установка иконки щита на файл (упрощенная версия)
HRESULT PropertiesDialog::setElevationIconOverlay(const QString &filePath, bool requireElevation)
{
    Q_UNUSED(requireElevation);

    // В MinGW сложно работать с COM свойствами файлов
    // Вместо этого просто обновим иконку через Shell API
    SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATHW, filePath.toStdWString().c_str(), nullptr);

    return S_OK;
}
#endif

void PropertiesDialog::loadSecurityInfo()
{
    securityList->clear();
    securityDescription->clear();

#ifdef Q_OS_WIN
    PSECURITY_DESCRIPTOR pSD = NULL;
    DWORD dwResult = GetNamedSecurityInfoW(
        filePath.toStdWString().c_str(),
        SE_FILE_OBJECT,
        OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
        NULL, NULL, NULL, NULL, &pSD);

    if (dwResult == ERROR_SUCCESS) {
        PACL pDacl = NULL;
        BOOL bDaclPresent = FALSE;
        BOOL bDaclDefaulted = FALSE;

        if (GetSecurityDescriptorDacl(pSD, &bDaclPresent, &pDacl, &bDaclDefaulted) && bDaclPresent && pDacl) {
            for (DWORD i = 0; i < pDacl->AceCount; i++) {
                LPVOID pAce = NULL;
                if (GetAce(pDacl, i, &pAce)) {
                    PACE_HEADER pAceHeader = (PACE_HEADER)pAce;
                    if (pAceHeader->AceType == ACCESS_ALLOWED_ACE_TYPE) {
                        PACCESS_ALLOWED_ACE pAllowedAce = (PACCESS_ALLOWED_ACE)pAce;
                        PSID pSid = (PSID)&pAllowedAce->SidStart;

                        // Get account name from SID
                        DWORD nameSize = 0;
                        DWORD domainSize = 0;
                        SID_NAME_USE sidType;

                        // First call to get buffer sizes
                        LookupAccountSidW(NULL, pSid, NULL, &nameSize, NULL, &domainSize, &sidType);

                        if (nameSize > 0 && domainSize > 0) {
                            wchar_t *name = new wchar_t[nameSize];
                            wchar_t *domain = new wchar_t[domainSize];

                            if (LookupAccountSidW(NULL, pSid, name, &nameSize, domain, &domainSize, &sidType)) {
                                QString accountName = QString::fromWCharArray(domain) + "\\" + QString::fromWCharArray(name);
                                securityList->addItem(accountName);

                                // Add permissions description
                                QString permissions;
                                if (pAllowedAce->Mask & FILE_READ_DATA) permissions += "Чтение, ";
                                if (pAllowedAce->Mask & FILE_WRITE_DATA) permissions += "Запись, ";
                                if (pAllowedAce->Mask & FILE_EXECUTE) permissions += "Выполнение, ";
                                if (pAllowedAce->Mask & FILE_DELETE_CHILD) permissions += "Удаление, ";
                                if (pAllowedAce->Mask & GENERIC_ALL) permissions += "Полный доступ, ";

                                if (!permissions.isEmpty()) {
                                    permissions.chop(2); // Remove last comma and space
                                    securityDescription->append(accountName + ": " + permissions);
                                }
                            }

                            delete[] name;
                            delete[] domain;
                        }
                    }
                }
            }
        }

        LocalFree(pSD);
    } else {
        securityList->addItem("Не удалось загрузить информацию о безопасности");
        securityDescription->setText("Ошибка при получении информации о безопасности: " + QString::number(dwResult));
    }
#else
    securityList->addItem("Информация о безопасности доступна только в Windows");
#endif

    if (securityList->count() == 0) {
        securityList->addItem("Нет информации о разрешениях");
    }
}

void PropertiesDialog::calculateSize()
{
    totalSize = 0;
    sizeOnDisk = 0;

    if (fileInfo.isDir()) {
        // Calculate directory size recursively using QDirIterator
        QDirIterator it(filePath, QDir::Files | QDir::Hidden | QDir::System, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();
            QFileInfo fileInfo = it.fileInfo();
            if (fileInfo.isFile()) {
                totalSize += fileInfo.size();
#ifdef Q_OS_WIN
                DWORD sizeHigh = 0;
                DWORD sizeLow = GetCompressedFileSizeW(fileInfo.absoluteFilePath().toStdWString().c_str(), &sizeHigh);
                if (sizeLow != INVALID_FILE_SIZE) {
                    sizeOnDisk += (static_cast<qint64>(sizeHigh) << 32) | sizeLow;
                } else {
                    sizeOnDisk += fileInfo.size();
                }
#else
                sizeOnDisk += fileInfo.size();
#endif
            }
        }
    } else {
        totalSize = fileInfo.size();
#ifdef Q_OS_WIN
        DWORD sizeHigh = 0;
        DWORD sizeLow = GetCompressedFileSizeW(filePath.toStdWString().c_str(), &sizeHigh);
        if (sizeLow != INVALID_FILE_SIZE) {
            sizeOnDisk = (static_cast<qint64>(sizeHigh) << 32) | sizeLow;
        } else {
            sizeOnDisk = totalSize;
        }
#else
        sizeOnDisk = totalSize;
#endif
    }

    fileSizeLabel->setText(formatSize(totalSize));
    fileSizeOnDiskLabel->setText(formatSize(sizeOnDisk));
}

QString PropertiesDialog::formatSize(qint64 bytes) const
{
    const qint64 KB = 1024;
    const qint64 MB = KB * 1024;
    const qint64 GB = MB * 1024;

    if (bytes >= GB) {
        return QString("%1 ГБ (%2 байт)").arg(QString::number(bytes / (double)GB, 'f', 2)).arg(bytes);
    } else if (bytes >= MB) {
        return QString("%1 МБ (%2 байт)").arg(QString::number(bytes / (double)MB, 'f', 2)).arg(bytes);
    } else if (bytes >= KB) {
        return QString("%1 КБ (%2 байт)").arg(QString::number(bytes / (double)KB, 'f', 2)).arg(bytes);
    } else {
        return QString("%1 байт").arg(bytes);
    }
}

QString PropertiesDialog::getFileType(const QString &filePath) const
{
    QFileInfo info(filePath);
    if (info.isDir()) {
        return "Папка с файлами";
    }

    QString extension = info.suffix().toLower();
    if (extension == "exe" || extension == "com" || extension == "bat") {
        return "Приложение";
    } else if (extension == "txt") {
        return "Текстовый документ";
    } else if (extension == "doc" || extension == "docx") {
        return "Документ Microsoft Word";
    } else if (extension == "pdf") {
        return "Документ PDF";
    } else if (extension == "jpg" || extension == "jpeg" || extension == "png" || extension == "bmp") {
        return "Изображение";
    } else if (extension == "zip" || extension == "rar" || extension == "7z") {
        return "Архив";
    } else if (extension == "mp3" || extension == "wav" || extension == "flac") {
        return "Аудио файл";
    } else if (extension == "mp4" || extension == "avi" || extension == "mkv") {
        return "Видео файл";
    } else if (extension == "cmd" || extension == "ps1") {
        return "Скрипт";
    } else if (extension == "msi") {
        return "Установщик Windows";
    }

    return QString("Файл %1").arg(extension.toUpper());
}

void PropertiesDialog::updateUI()
{
    applyButton->setEnabled(false);
}

void PropertiesDialog::updateApplyButton()
{
    bool hasFileNameChange = fileNameEdit->text() != originalFileName;
    bool hasAttributeChanges = false;
    bool hasAdminChanges = runAsAdminCheckBox->isChecked() != originalRunAsAdmin;

#ifdef Q_OS_WIN
    DWORD currentAttributes = 0;
    if (readOnlyCheckBox->isChecked()) currentAttributes |= FILE_ATTRIBUTE_READONLY;
    if (hiddenCheckBox->isChecked()) currentAttributes |= FILE_ATTRIBUTE_HIDDEN;
    if (archiveCheckBox->isChecked()) currentAttributes |= FILE_ATTRIBUTE_ARCHIVE;

    hasAttributeChanges = currentAttributes != originalAttributes;
#endif

    hasChanges = hasFileNameChange || hasAttributeChanges || hasAdminChanges;
    applyButton->setEnabled(hasChanges);
}

void PropertiesDialog::applyChanges()
{
    bool success = true;
    QString errorMessage;

    // Apply file name change
    if (fileNameEdit->text() != originalFileName) {
        QString newPath = fileInfo.absolutePath() + "/" + fileNameEdit->text();
        if (QFile::rename(filePath, newPath)) {
            filePath = newPath;
            fileInfo.setFile(newPath);
            originalFileName = fileNameEdit->text();
            setWindowTitle(Strings::PropertiesTitle + " - " + fileInfo.fileName());

            // Update file type and location
            fileTypeLabel->setText(getFileType(filePath));
            fileLocationLabel->setText(fileInfo.absolutePath());
        } else {
            errorMessage = "Не удалось переименовать файл";
            success = false;
            fileNameEdit->setText(originalFileName);
        }
    }

    if (success) {
        // Apply file attributes
#ifdef Q_OS_WIN
        DWORD newAttributes = 0;
        if (readOnlyCheckBox->isChecked()) newAttributes |= FILE_ATTRIBUTE_READONLY;
        if (hiddenCheckBox->isChecked()) newAttributes |= FILE_ATTRIBUTE_HIDDEN;
        if (archiveCheckBox->isChecked()) newAttributes |= FILE_ATTRIBUTE_ARCHIVE;

        if (newAttributes != originalAttributes) {
            if (setFileAttributes(filePath, newAttributes)) {
                originalAttributes = newAttributes;
            } else {
                errorMessage = "Не удалось изменить атрибуты файла";
                success = false;
                // Restore checkbox states
                readOnlyCheckBox->setChecked(originalAttributes & FILE_ATTRIBUTE_READONLY);
                hiddenCheckBox->setChecked(originalAttributes & FILE_ATTRIBUTE_HIDDEN);
                archiveCheckBox->setChecked(originalAttributes & FILE_ATTRIBUTE_ARCHIVE);
            }
        }
#endif

        // Apply run as admin setting - только для исполняемых файлов
        bool isExecutable = isExecutableFile(filePath);
        if (isExecutable && !fileInfo.isDir()) {
            bool runAsAdmin = runAsAdminCheckBox->isChecked();

            if (runAsAdmin != originalRunAsAdmin) {
#ifdef Q_OS_WIN
                // Комбинированный подход для MinGW
                bool method1Success = setRunAsAdminRegistry(filePath, runAsAdmin);
                bool method2Success = false;

                if (runAsAdmin) {
                    method2Success = createElevatedShortcut(filePath);
                    // Показываем информацию пользователю о создании скриптов
                    if (method2Success) {
                        elevationMethodLabel->setText("✓ Созданы скрипты для запуска с правами администратора в папке Temp");
                    }
                } else {
                    method2Success = removeElevatedShortcut(filePath);
                }

                if (!method1Success && !method2Success) {
                    errorMessage = "Не удалось установить настройки запуска от администратора";
                    success = false;
                } else {
                    originalRunAsAdmin = runAsAdmin;

                    // Обновляем кэш иконок
                    SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATHW, filePath.toStdWString().c_str(), nullptr);
                }
#endif
            }
        }

        // Apply advanced attributes
        updateAdvancedAttributes();
    }

    if (success) {
        hasChanges = false;
        applyButton->setEnabled(false);

        // Показываем информацию пользователю
        if (runAsAdminCheckBox->isChecked() && isExecutableFile(filePath)) {
            QMessageBox::information(this, "Настройки применены",
                "Настройки запуска от имени администратора применены.\n\n"
                "При запуске файла теперь будет запрашиваться повышение прав.\n"
                "В папке Temp созданы скрипты для гарантированного запуска.");
        }
    } else if (!errorMessage.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", errorMessage);
    }
}

void PropertiesDialog::updateAdvancedAttributes()
{
#ifdef Q_OS_WIN
    // Handle compression
    if (compressCheckBox->isChecked() != !!(originalAttributes & FILE_ATTRIBUTE_COMPRESSED)) {
        HANDLE hFile = CreateFileW(
            filePath.toStdWString().c_str(),
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS,
            NULL
        );

        if (hFile != INVALID_HANDLE_VALUE) {
            DWORD bytesReturned;
            USHORT compressionState = compressCheckBox->isChecked() ? COMPRESSION_FORMAT_DEFAULT : COMPRESSION_FORMAT_NONE;
            if (DeviceIoControl(
                hFile,
                FSCTL_SET_COMPRESSION,
                &compressionState,
                sizeof(compressionState),
                NULL,
                0,
                &bytesReturned,
                NULL
            )) {
                // Success - update the original attributes
                if (compressCheckBox->isChecked()) {
                    originalAttributes |= FILE_ATTRIBUTE_COMPRESSED;
                } else {
                    originalAttributes &= ~FILE_ATTRIBUTE_COMPRESSED;
                }
            } else {
                QMessageBox::warning(this, "Ошибка", "Не удалось изменить настройки сжатия");
                compressCheckBox->setChecked(originalAttributes & FILE_ATTRIBUTE_COMPRESSED);
            }
            CloseHandle(hFile);
        }
    }

    // Note: Encryption requires additional Windows APIs and user consent
    // For now, we'll just update the attribute display
    if (encryptCheckBox->isChecked() != !!(originalAttributes & FILE_ATTRIBUTE_ENCRYPTED)) {
        QMessageBox::information(this, "Информация",
            "Шифрование файлов требует дополнительных настроек системы.\n"
            "Эта функция будет реализована в будущих версиях.");
        encryptCheckBox->setChecked(originalAttributes & FILE_ATTRIBUTE_ENCRYPTED);
    }
#endif
}

bool PropertiesDialog::setFileAttributes(const QString &path, DWORD attributes)
{
#ifdef Q_OS_WIN
    return SetFileAttributesW(path.toStdWString().c_str(), attributes);
#else
    Q_UNUSED(path)
    Q_UNUSED(attributes)
    return false;
#endif
}

DWORD PropertiesDialog::getFileAttributes(const QString &path)
{
#ifdef Q_OS_WIN
    return GetFileAttributesW(path.toStdWString().c_str());
#else
    Q_UNUSED(path)
    return 0;
#endif
}

void PropertiesDialog::okClicked()
{
    if (hasChanges) {
        applyChanges();
    }
    accept();
}

void PropertiesDialog::cancelClicked()
{
    reject();
}

void PropertiesDialog::onFileNameChanged(const QString &text)
{
    Q_UNUSED(text)
    updateApplyButton();
}

void PropertiesDialog::onRunAsAdminToggled(bool checked)
{
    Q_UNUSED(checked)
    updateApplyButton();
}

void PropertiesDialog::onHiddenToggled(bool checked)
{
    Q_UNUSED(checked)
    updateApplyButton();
}

void PropertiesDialog::onReadOnlyToggled(bool checked)
{
    Q_UNUSED(checked)
    updateApplyButton();
}

void PropertiesDialog::onArchiveToggled(bool checked)
{
    Q_UNUSED(checked)
    updateApplyButton();
}

void PropertiesDialog::onCompressToggled(bool checked)
{
    Q_UNUSED(checked)
    updateApplyButton();
}

void PropertiesDialog::onEncryptToggled(bool checked)
{
    Q_UNUSED(checked)
    updateApplyButton();
}