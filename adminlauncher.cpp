#include "adminlauncher.h"
#include <QProcess>
#include <QMessageBox>
#include <QCoreApplication>

AdminLauncher::AdminLauncher(QObject *parent)
    : QObject(parent)
{
}

#ifdef Q_OS_WIN
bool AdminLauncher::isUserAdmin()
{
    BOOL isAdmin = FALSE;
    PSID adminGroupSid = NULL;
    
    // Создаем SID для группы администраторов
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&ntAuthority, 2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0,
        &adminGroupSid)) {
        
        CheckTokenMembership(NULL, adminGroupSid, &isAdmin);
        FreeSid(adminGroupSid);
    }
    
    return isAdmin != FALSE;
}

bool AdminLauncher::isProcessElevated()
{
    HANDLE token = NULL;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
        return false;
    
    TOKEN_ELEVATION elevation;
    DWORD size;
    bool isElevated = false;
    
    if (GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size)) {
        isElevated = elevation.TokenIsElevated != 0;
    }
    
    CloseHandle(token);
    return isElevated;
}
#endif

bool AdminLauncher::runAsAdmin(const QString &filePath, const QString &arguments)
{
#ifdef Q_OS_WIN
    SHELLEXECUTEINFO sei = { sizeof(sei) };
    sei.lpVerb = L"runas";  // Это ключевой параметр для запроса UAC
    sei.lpFile = filePath.toStdWString().c_str();
    sei.lpParameters = arguments.isEmpty() ? NULL : arguments.toStdWString().c_str();
    sei.nShow = SW_SHOWNORMAL;
    
    if (!ShellExecuteEx(&sei)) {
        DWORD error = GetLastError();
        if (error == ERROR_CANCELLED) {
            // Пользователь отменил UAC запрос
            return false;
        }
        return false;
    }
    
    return true;
#else
    Q_UNUSED(filePath);
    Q_UNUSED(arguments);
    return false;
#endif
}

bool AdminLauncher::runAsAdminWithUAC(const QString &filePath, const QString &arguments)
{
    return runAsAdmin(filePath, arguments);
}