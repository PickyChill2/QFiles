#include <QApplication>
#include <QDir>
#include <QDebug>
#include <QIcon>
#include <QMessageBox>
#include <QSystemSemaphore>
#include <QWidget>
#include <QCommandLineParser>
#include <QFile>
#include <QFileInfo>
#include <QLocalServer>
#include <QLocalSocket>
#include "mainwindow.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <winreg.h>
#endif

class SingleApplication : public QApplication
{
    Q_OBJECT

public:
    SingleApplication(int &argc, char **argv) : QApplication(argc, argv)
    {
        // Уникальный идентификатор сервера для обмена данными
        serverName = "QFiles_SingleApp_Server";

        // Пробуем подключиться к существующему серверу
        QLocalSocket socket;
        socket.connectToServer(serverName);

        if (socket.waitForConnected(500)) {
            isRunning = true;
        } else {
            isRunning = false;
            // Если сервера нет, создаем его (мы — первый экземпляр)
            server = new QLocalServer(this);
            QLocalServer::removeServer(serverName); // Очистка после возможного сбоя
            if (!server->listen(serverName)) {
                qDebug() << "Server could not start:" << server->errorString();
            }
            connect(server, &QLocalServer::newConnection, this, &SingleApplication::handleNewConnection);
        }
    }

    bool isAlreadyRunning() const { return isRunning; }

    // Метод для передачи аргументов (пути) уже работающему приложению
    void sendMessage(const QString &message)
    {
        QLocalSocket socket;
        socket.connectToServer(serverName);
        if (socket.waitForConnected(500)) {
            socket.write(message.toUtf8());
            socket.waitForBytesWritten();
        }
    }

    void activateExistingInstance()
    {
        QWidgetList widgets = QApplication::allWidgets();
        for (QWidget *widget : widgets) {
            MainWindow *mainWindow = qobject_cast<MainWindow*>(widget);
            if (mainWindow) {
                mainWindow->show();
                mainWindow->raise();
                mainWindow->activateWindow();

                if (mainWindow->windowState() & Qt::WindowMinimized) {
                    mainWindow->setWindowState(mainWindow->windowState() & ~Qt::WindowMinimized);
                }
                break;
            }
        }
    }

signals:
    void messageReceived(const QString &message);

private slots:
    void handleNewConnection()
    {
        QLocalSocket *socket = server->nextPendingConnection();
        connect(socket, &QLocalSocket::readyRead, [this, socket]() {
            QString message = QString::fromUtf8(socket->readAll());
            emit messageReceived(message);
            socket->deleteLater();
            activateExistingInstance();
        });
    }

private:
    bool isRunning;
    QString serverName;
    QLocalServer *server = nullptr;
};

bool performAdminFileOperation(const QStringList &args)
{
    QCommandLineParser parser;
    parser.setApplicationDescription("Admin file operations");

    QCommandLineOption adminFileOpOption("admin-file-operation", "Perform admin file operation");
    QCommandLineOption typeOption("type", "Operation type", "type");
    QCommandLineOption sourceOption("source", "Source path", "source");
    QCommandLineOption destOption("destination", "Destination path", "destination");

    parser.addOption(adminFileOpOption);
    parser.addOption(typeOption);
    parser.addOption(sourceOption);
    parser.addOption(destOption);

    if (!parser.parse(args)) {
        return false;
    }

    if (parser.isSet(adminFileOpOption)) {
        QString type = parser.value(typeOption);
        QString source = parser.value(sourceOption);
        QString destination = parser.value(destOption);

        if (type == "copy") {
            QFileInfo sourceInfo(source);
            if (sourceInfo.isDir()) {
                QDir sourceDir(source);
                QDir destDir(destination);
                if (!destDir.mkpath(".")) return false;

                QStringList files = sourceDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
                bool success = true;
                for (const QString &file : files) {
                    QString srcFilePath = source + "/" + file;
                    QString destFilePath = destination + "/" + file;
                    if (!performAdminFileOperation(QStringList() << "--admin-file-operation" << "--type" << "copy" << "--source" << srcFilePath << "--destination" << destFilePath)) {
                        success = false;
                    }
                }
                return success;
            } else {
                return QFile::copy(source, destination);
            }
        } else if (type == "move") {
            if (QFile::rename(source, destination)) return true;
            if (performAdminFileOperation(QStringList() << "--admin-file-operation" << "--type" << "copy" << "--source" << source << "--destination" << destination)) {
                QFileInfo sourceInfo(source);
                return sourceInfo.isDir() ? QDir(source).removeRecursively() : QFile::remove(source);
            }
            return false;
        } else if (type == "rename") {
            return QFile::rename(source, destination);
        } else if (type == "replace") {
            if (QFile::exists(destination)) QFile::remove(destination);
            return QFile::copy(source, destination);
        }
    }
    return false;
}

bool performBatchAdminOperation(const QStringList &args)
{
    QCommandLineParser parser;
    parser.setApplicationDescription("Batch admin operations");
    QCommandLineOption adminOpOption("admin-operation", "Perform batch admin operation");
    QCommandLineOption typeOption("type", "Operation type", "type");
    QCommandLineOption destOption("destination", "Destination directory", "destination");
    QCommandLineOption filesOption("files", "Files to process", "files");

    parser.addOption(adminOpOption);
    parser.addOption(typeOption);
    parser.addOption(destOption);
    parser.addOption(filesOption);

    if (!parser.parse(args)) return false;

    if (parser.isSet(adminOpOption)) {
        QString type = parser.value(typeOption);
        QString destination = parser.value(destOption);
        QStringList files = parser.values(filesOption);

        bool success = true;
        for (const QString &file : files) {
            QFileInfo fileInfo(file);
            QString destPath = destination + "/" + fileInfo.fileName();
            if (!performAdminFileOperation(QStringList() << "--admin-file-operation" << "--type" << type << "--source" << file << "--destination" << destPath)) {
                success = false;
            }
        }
        return success;
    }
    return false;
}

int main(int argc, char *argv[])
{
    QStringList args;
    for (int i = 0; i < argc; ++i) {
        args.append(QString::fromLocal8Bit(argv[i]));
    }

    // Режим администратора
    if (args.contains("--admin-file-operation")) {
        QCoreApplication adminApp(argc, argv);
        return performAdminFileOperation(args) ? 0 : 1;
    }
    if (args.contains("--admin-operation")) {
        QCoreApplication adminApp(argc, argv);
        return performBatchAdminOperation(args) ? 0 : 1;
    }

    // Обработка ссылки QFile: (исправленная версия для путей с пробелами)
    QString linkPath;
    int qfileStartIndex = -1;
    for (int i = 1; i < args.size(); ++i) {  // Пропускаем args[0] (путь к exe)
        if (args[i].startsWith("QFile:", Qt::CaseInsensitive)) {
            qfileStartIndex = i;
            break;
        }
    }

    if (qfileStartIndex != -1) {
        // Извлекаем путь после префикса
        linkPath = args[qfileStartIndex].mid(6);

        // Если путь с пробелами и не закавычен, собираем оставшиеся аргументы
        for (int j = qfileStartIndex + 1; j < args.size(); ++j) {
            linkPath += " " + args[j];
        }

        // Удаляем кавычки (если путь был закавычен)
        linkPath.remove("\"");
        linkPath = linkPath.trimmed();  // Убираем лишние пробелы по краям
    }

    QString appDir = QCoreApplication::applicationDirPath();
    QApplication::addLibraryPath(appDir + "/platforms");
    QApplication::addLibraryPath(appDir);
    qputenv("QT_PLUGIN_PATH", (appDir + "/platforms").toLocal8Bit());

#ifdef Q_OS_WIN
    // Отключение оптимизации во весь экран
    QString appPath = QCoreApplication::applicationFilePath();
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers", 0, KEY_WRITE | KEY_READ, &hKey) == ERROR_SUCCESS) {
        std::wstring value = L"DISABLEDXMAXIMIZEDWINDOWEDMODE";
        RegSetValueExW(hKey, appPath.toStdWString().c_str(), 0, REG_SZ, (const BYTE*)value.c_str(), (value.length() + 1) * sizeof(wchar_t));
        RegCloseKey(hKey);
    }
#endif

    SingleApplication app(argc, argv);

    // Если приложение уже запущено
    if (app.isAlreadyRunning()) {
        if (!linkPath.isEmpty()) {
            app.sendMessage(linkPath);
        } else {
            app.activateExistingInstance();
        }
        return 0;
    }

    app.setApplicationName("QFiles");
    app.setApplicationVersion("1.0");

    QIcon appIcon;
    if (QIcon::hasThemeIcon("QFiles")) {
        appIcon = QIcon::fromTheme("QFiles");
    } else {
        appIcon = QIcon(":/QFiles.png");
        if (appIcon.isNull()) appIcon = QIcon(appDir + "/QFiles.png");
    }
    if (!appIcon.isNull()) app.setWindowIcon(appIcon);

    MainWindow window;

    // Соединяем получение сообщения с открытием вкладки
    QObject::connect(&app, &SingleApplication::messageReceived, &window, &MainWindow::openInNewTab);

    if (!linkPath.isEmpty()) {
        window.openInNewTab(linkPath);
    } else {
        window.show();
    }

    return app.exec();
}

#include "main.moc"