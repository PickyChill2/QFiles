#include "networkdrivewidget.h"

#include "styles.h"

#include <QDir>

#include <QFileInfo>

#include <QStorageInfo>

#include <QDebug>



NetworkDriveDialog::NetworkDriveDialog(QWidget *parent)

    : QDialog(parent)

    , drivePathEdit(new QLineEdit(this))

    , driveLetterCombo(new QComboBox(this))

    , usernameEdit(new QLineEdit(this))

    , passwordEdit(new QLineEdit(this))

{

    setWindowTitle("Connect Network Drive");

    setModal(true);

    resize(350, 180);



    // Настройка поля пути

    drivePathEdit->setPlaceholderText("\\\\server\\share");



    // Заполнение доступных букв дисков

    for (char letter = 'A'; letter <= 'Z'; ++letter) {

        QString drive = QString(letter) + ":";

        QStorageInfo storage(drive);

        if (!storage.isValid() || storage.rootPath().isEmpty()) {

            driveLetterCombo->addItem(drive);

        }

    }

    driveLetterCombo->insertItem(0, "Auto");



    // Настройка полей авторизации

    usernameEdit->setPlaceholderText("Username (local computer account)");

    passwordEdit->setPlaceholderText("Password");

    passwordEdit->setEchoMode(QLineEdit::Password);



    // Создание layout с меньшими отступами

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    mainLayout->setContentsMargins(10, 10, 10, 10);

    mainLayout->setSpacing(5);



    mainLayout->addWidget(new QLabel("Network Path:"));

    mainLayout->addWidget(drivePathEdit);



    mainLayout->addWidget(new QLabel("Drive Letter:"));

    mainLayout->addWidget(driveLetterCombo);



    mainLayout->addWidget(new QLabel("Username (optional):"));

    mainLayout->addWidget(usernameEdit);



    mainLayout->addWidget(new QLabel("Password (optional):"));

    mainLayout->addWidget(passwordEdit);



    // Кнопки

    QHBoxLayout *buttonLayout = new QHBoxLayout();

    buttonLayout->setSpacing(5);

    QPushButton *connectBtn = new QPushButton("Connect", this);

    QPushButton *cancelBtn = new QPushButton("Cancel", this);



    buttonLayout->addWidget(connectBtn);

    buttonLayout->addWidget(cancelBtn);

    mainLayout->addLayout(buttonLayout);



    // Подключение сигналов

    connect(connectBtn, &QPushButton::clicked, this, &NetworkDriveDialog::accept);

    connect(cancelBtn, &QPushButton::clicked, this, &NetworkDriveDialog::reject);

}



QString NetworkDriveDialog::getDrivePath() const

{

    return drivePathEdit->text().trimmed();

}



QString NetworkDriveDialog::getDriveLetter() const

{

    QString letter = driveLetterCombo->currentText();

    return letter == "Auto" ? "" : letter;

}



QString NetworkDriveDialog::getUsername() const

{

    return usernameEdit->text().trimmed();

}



QString NetworkDriveDialog::getPassword() const

{

    return passwordEdit->text();

}



NetworkDriveWidget::NetworkDriveWidget(QWidget *parent)

    : QWidget(parent)

    , connectButton(new QPushButton("🌐 Connect", this))

    , disconnectButton(new QPushButton("❌ Disconnect", this))

    , connectedDrivesCombo(new QComboBox(this))

{

    setupUI();

    populateConnectedDrives();

}



void NetworkDriveWidget::setupUI()

{

    QHBoxLayout *layout = new QHBoxLayout(this);

    layout->setContentsMargins(0, 0, 0, 0);

    layout->setSpacing(3);



    layout->addWidget(connectedDrivesCombo);

    layout->addWidget(connectButton);

    layout->addWidget(disconnectButton);



    connectButton->setStyleSheet(Styles::Buttons::NetworkConnect); // Используем стили из Styles

    disconnectButton->setStyleSheet(Styles::Buttons::NetworkDisconnect); // Используем стили из Styles



    connectedDrivesCombo->setMinimumWidth(Styles::NetworkDriveComboWidth); // Используем константы

    connectedDrivesCombo->setMaximumWidth(Styles::NetworkDriveComboMaxWidth); // Используем константы

    connectedDrivesCombo->setStyleSheet(Styles::ComboBoxes::NetworkDrive); // Используем стили из Styles



    connect(connectButton, &QPushButton::clicked, this, &NetworkDriveWidget::showConnectDialog);

    connect(disconnectButton, &QPushButton::clicked, this, &NetworkDriveWidget::disconnectNetworkDrive);

}



void NetworkDriveWidget::populateConnectedDrives()

{

    connectedDrivesCombo->clear();



    foreach (const QStorageInfo &storage, QStorageInfo::mountedVolumes()) {

        if (storage.isValid() && storage.isReady()) {

            QString rootPath = storage.rootPath();

            if (rootPath.length() == 3 && rootPath[1] == ':' && rootPath[2] == '\\') {

                DWORD driveType = GetDriveTypeW((LPCWSTR)rootPath.utf16());

                if (driveType == DRIVE_REMOTE) {

                    QString displayText = QString("%1 (%2)").arg(rootPath.left(2)).arg(storage.displayName());

                    connectedDrivesCombo->addItem(displayText, rootPath.left(2));

                }

            }

        }

    }



    disconnectButton->setEnabled(connectedDrivesCombo->count() > 0);

}



void NetworkDriveWidget::showConnectDialog()

{

    NetworkDriveDialog dialog(this);

    if (dialog.exec() == QDialog::Accepted) {

        connectNetworkDrive(dialog.getDrivePath(),

                          dialog.getDriveLetter(),

                          dialog.getUsername(),

                          dialog.getPassword());

    }

}



void NetworkDriveWidget::connectNetworkDrive(const QString &path, const QString &letter,

                                           const QString &username, const QString &password)

{

    if (path.isEmpty()) {

        QMessageBox::warning(this, "Error", "Please enter network path");

        return;

    }



    // Пробуем несколько подходов

    bool success = false;

    QString connectedDrive;

    QString lastError;



    // Подход 1: Используем net use напрямую (как в терминале)

    if (!success) {

        QString command;

        if (!letter.isEmpty()) {

            command = QString("net use %1 \"%2\"").arg(letter).arg(path);

        } else {

            command = QString("net use \"%1\"").arg(path);

        }



        if (!username.isEmpty()) {

            command += QString(" /user:%1").arg(username);

            if (!password.isEmpty()) {

                command += QString(" %1").arg(password);

            }

        }



        qDebug() << "Trying approach 1:" << command;



        QProcess process;

        process.setProcessChannelMode(QProcess::MergedChannels);

        process.start("cmd.exe", QStringList() << "/c" << command);

        process.waitForFinished(10000);



        QByteArray output = process.readAll();

        QString errorOutput = QString::fromLocal8Bit(output);

        qDebug() << "Output 1:" << errorOutput;



        if (process.exitCode() == 0) {

            success = true;

            if (!letter.isEmpty()) {

                connectedDrive = letter + "\\";

            } else {

                // Пытаемся найти букву в выводе

                QRegularExpression re("([A-Z]:)");

                QRegularExpressionMatch match = re.match(errorOutput);

                if (match.hasMatch()) {

                    connectedDrive = match.captured(1) + "\\";

                } else {

                    connectedDrive = path;

                }

            }

        } else {

            lastError = errorOutput;

        }

    }



    // Подход 2: Используем Windows API напрямую

    if (!success) {

        qDebug() << "Trying approach 2: Windows API";



        QString remotePath = path;

        if (!remotePath.endsWith("\\")) {

            remotePath += "\\";

        }



        QString localName = letter.isEmpty() ? nullptr : (letter + ":");

        QString user = username.isEmpty() ? nullptr : username;

        QString pwd = password.isEmpty() ? nullptr : password;



        DWORD result = WNetUseConnectionW(

            nullptr, // hwndOwner

            nullptr, // lpNetResource

            (LPCWSTR)pwd.utf16(), // lpPassword

            (LPCWSTR)user.utf16(), // lpUserID

            CONNECT_INTERACTIVE, // dwFlags

            localName.isEmpty() ? nullptr : (LPWSTR)localName.utf16(), // lpAccessName

            nullptr, // lpBufferSize

            nullptr // lpResult

        );



        if (result == NO_ERROR) {

            success = true;

            connectedDrive = localName.isEmpty() ? path : localName + "\\";

            qDebug() << "Windows API success";

        } else {

            qDebug() << "Windows API failed:" << result;

            lastError = QString("Windows API error: %1").arg(result);

        }

    }



    // Подход 3: Пробуем без обратного слэша в конце

    if (!success) {

        QString cleanPath = path;

        if (cleanPath.endsWith("\\")) {

            cleanPath = cleanPath.left(cleanPath.length() - 1);

        }



        QString command;

        if (!letter.isEmpty()) {

            command = QString("net use %1 \"%2\"").arg(letter).arg(cleanPath);

        } else {

            command = QString("net use \"%1\"").arg(cleanPath);

        }



        qDebug() << "Trying approach 3:" << command;



        QProcess process;

        process.setProcessChannelMode(QProcess::MergedChannels);

        process.start("cmd.exe", QStringList() << "/c" << command);

        process.waitForFinished(10000);



        QByteArray output = process.readAll();

        QString errorOutput = QString::fromLocal8Bit(output);

        qDebug() << "Output 3:" << errorOutput;



        if (process.exitCode() == 0) {

            success = true;

            if (!letter.isEmpty()) {

                connectedDrive = letter + "\\";

            } else {

                connectedDrive = cleanPath;

            }

        } else {

            lastError = errorOutput;

        }

    }



    // Подход 4: Пробуем через PowerShell

    if (!success) {

        QString command;

        if (!letter.isEmpty()) {

            command = QString("New-PSDrive -Name \"%1\" -PSProvider \"FileSystem\" -Root \"%2\" -Persist").arg(letter.left(1)).arg(path);

        } else {

            // PowerShell требует имя для диска

            QString autoLetter = "Z";

            for (char letter = 'Y'; letter >= 'A'; --letter) {

                QString testDrive = QString(letter) + ":";

                QStorageInfo storage(testDrive);

                if (!storage.isValid() || storage.rootPath().isEmpty()) {

                    autoLetter = QString(letter);

                    break;

                }

            }

            command = QString("New-PSDrive -Name \"%1\" -PSProvider \"FileSystem\" -Root \"%2\" -Persist").arg(autoLetter).arg(path);

        }



        qDebug() << "Trying approach 4 (PowerShell):" << command;



        QProcess process;

        process.setProcessChannelMode(QProcess::MergedChannels);

        process.start("powershell.exe", QStringList() << "-Command" << command);

        process.waitForFinished(10000);



        QByteArray output = process.readAll();

        QString errorOutput = QString::fromLocal8Bit(output);

        qDebug() << "Output 4:" << errorOutput;



        if (process.exitCode() == 0) {

            success = true;

            if (!letter.isEmpty()) {

                connectedDrive = letter + "\\";

            } else {

                // Ищем букву в выводе PowerShell

                QRegularExpression re("([A-Z]:)");

                QRegularExpressionMatch match = re.match(errorOutput);

                if (match.hasMatch()) {

                    connectedDrive = match.captured(1) + "\\";

                } else {

                    connectedDrive = path;

                }

            }

        } else {

            lastError = errorOutput;

        }

    }



    if (success) {

        QMessageBox::information(this, "Success", "Network drive connected successfully");

        populateConnectedDrives();

        emit driveConnected(connectedDrive);

    } else {

        QString errorMessage;



        if (lastError.contains("67", Qt::CaseInsensitive) || lastError.contains("53", Qt::CaseInsensitive)) {

            errorMessage = "Network path not found.\n\n"

                          "Please check:\n"

                          "• Computer name and share name are correct\n"

                          "• Network connectivity\n"

                          "• Firewall settings\n"

                          "• Network discovery is enabled\n"

                          "• The share exists and is accessible";

        } else {

            errorMessage = "Failed to connect network drive:\n" + lastError;

        }



        QMessageBox::warning(this, "Error", errorMessage);

    }

}



void NetworkDriveWidget::disconnectNetworkDrive()

{

    if (connectedDrivesCombo->currentIndex() < 0) {

        return;

    }



    QString driveLetter = connectedDrivesCombo->currentData().toString();



    QMessageBox::StandardButton reply = QMessageBox::question(

        this,

        "Confirmation",

        QString("Disconnect network drive %1?").arg(driveLetter),

        QMessageBox::Yes | QMessageBox::No

    );



    if (reply == QMessageBox::Yes) {

        // Пробуем несколько подходов для отключения

        bool success = false;



        // Подход 1: net use

        QString command = QString("net use %1 /delete").arg(driveLetter);

        QProcess process;

        process.setProcessChannelMode(QProcess::MergedChannels);

        process.start("cmd.exe", QStringList() << "/c" << command);

        process.waitForFinished(5000);



        if (process.exitCode() == 0) {

            success = true;

        } else {

            // Подход 2: PowerShell

            QString psCommand = QString("Remove-PSDrive -Name \"%1\"").arg(driveLetter.left(1));

            QProcess psProcess;

            psProcess.setProcessChannelMode(QProcess::MergedChannels);

            psProcess.start("powershell.exe", QStringList() << "-Command" << psCommand);

            psProcess.waitForFinished(5000);



            success = (psProcess.exitCode() == 0);

        }



        if (success) {

            QMessageBox::information(this, "Success", "Network drive disconnected");

            populateConnectedDrives();

        } else {

            QString errorMessage = "Failed to disconnect network drive";

            QMessageBox::warning(this, "Error", errorMessage);

        }

    }

}