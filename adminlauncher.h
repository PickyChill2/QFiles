#pragma once

#include <QObject>
#include <QString>

#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#endif

class AdminLauncher : public QObject
{
    Q_OBJECT

public:
    explicit AdminLauncher(QObject *parent = nullptr);
    
    bool runAsAdmin(const QString &filePath, const QString &arguments = "");
    bool runAsAdminWithUAC(const QString &filePath, const QString &arguments = "");
    
private:
#ifdef Q_OS_WIN
    bool isUserAdmin();
    bool isProcessElevated();
#endif
};