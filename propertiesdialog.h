#pragma once

#include <QDialog>
#include <QTabWidget>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QGroupBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileInfo>
#include <QDateTime>
#include <QFile>
#include <QDir>
#include <QMessageBox>
#include <QApplication>
#include <QThread>
#include <QProgressBar>
#include <QTextEdit>
#include <QComboBox>
#include <QListWidget>
#include <QTreeWidget>
#include <QHeaderView>
#include <QTimer>
#include <QFileIconProvider>

#ifdef Q_OS_WIN
#include <windows.h>
#include <aclapi.h>
#include <accctrl.h>
#include <sddl.h>
#include <shellapi.h>
#include <shlobj.h>
#include <objbase.h>
#include <propvarutil.h>
#include <propkey.h>
#include <shobjidl.h>
#include <wrl/client.h>
#endif

#include "colors.h"
#include "strings.h"

class PropertiesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PropertiesDialog(const QString &filePath, QWidget *parent = nullptr);
    ~PropertiesDialog();

private slots:
    void applyChanges();
    void okClicked();
    void cancelClicked();
    void loadSecurityInfo();
    void updateAdvancedAttributes();
    void calculateSize();
    void onRunAsAdminToggled(bool checked);
    void onHiddenToggled(bool checked);
    void onReadOnlyToggled(bool checked);
    void onArchiveToggled(bool checked);
    void onCompressToggled(bool checked);
    void onEncryptToggled(bool checked);
    void onFileNameChanged(const QString &text);

private:
    void setupUI();
    void loadFileInfo();
    void applyFileAttributes();
    bool setFileAttributes(const QString &path, DWORD attributes);
    DWORD getFileAttributes(const QString &path);
    QString formatSize(qint64 bytes) const;
    QString getFileType(const QString &filePath) const;
    bool isExecutableFile(const QString &filePath) const;
    void updateUI();
    void updateApplyButton();

    // Новые методы для работы с elevation
#ifdef Q_OS_WIN
    bool setElevationRequirement(const QString &filePath, bool requireElevation);
    bool hasElevationRequirement(const QString &filePath);
    bool createElevatedShortcut(const QString &filePath);
    bool removeElevatedShortcut(const QString &filePath);
    bool setRunAsAdminRegistry(const QString &filePath, bool runAsAdmin);
    bool checkRunAsAdminRegistry(const QString &filePath);
    bool setManifestElevation(const QString &filePath, bool requireElevation);
    HRESULT setElevationIconOverlay(const QString &filePath, bool requireElevation);
#endif

    QString filePath;
    QFileInfo fileInfo;

    // UI Elements
    QTabWidget *tabWidget;

    // General Tab
    QWidget *generalTab;
    QLabel *iconLabel;
    QLabel *fileNameLabel;
    QLineEdit *fileNameEdit;
    QLabel *fileTypeLabel;
    QLabel *fileLocationLabel;
    QLabel *fileSizeLabel;
    QLabel *fileSizeOnDiskLabel;
    QLabel *createdLabel;
    QLabel *modifiedLabel;
    QLabel *accessedLabel;
    QCheckBox *readOnlyCheckBox;
    QCheckBox *hiddenCheckBox;
    QCheckBox *archiveCheckBox;

    // Security Tab
    QWidget *securityTab;
    QListWidget *securityList;
    QTextEdit *securityDescription;
    QPushButton *advancedSecurityButton;

    // Advanced Tab
    QWidget *advancedTab;
    QCheckBox *runAsAdminCheckBox;
    QCheckBox *compressCheckBox;
    QCheckBox *encryptCheckBox;
    QGroupBox *attributesGroup;

    // Информация о методе elevation
    QLabel *elevationMethodLabel;

    // Buttons
    QPushButton *applyButton;
    QPushButton *okButton;
    QPushButton *cancelButton;

    // Original attributes
    DWORD originalAttributes;
    bool originalRunAsAdmin;
    QString originalFileName;
    bool isFolder;
    qint64 totalSize;
    qint64 sizeOnDisk;
    bool hasChanges;
};