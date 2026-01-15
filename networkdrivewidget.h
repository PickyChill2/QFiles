#pragma once

#include <QWidget>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QMessageBox>
#include <QProcess>
#include <windows.h>

class NetworkDriveDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NetworkDriveDialog(QWidget *parent = nullptr);
    QString getDrivePath() const;
    QString getDriveLetter() const;
    QString getUsername() const;
    QString getPassword() const;

private:
    QLineEdit *drivePathEdit;
    QComboBox *driveLetterCombo;
    QLineEdit *usernameEdit;
    QLineEdit *passwordEdit;
};

class NetworkDriveWidget : public QWidget
{
    Q_OBJECT

public:
    explicit NetworkDriveWidget(QWidget *parent = nullptr);

    signals:
        void driveConnected(const QString &path);

private slots:
    void showConnectDialog();
    void connectNetworkDrive(const QString &path, const QString &letter,
                           const QString &username, const QString &password);
    void disconnectNetworkDrive();

private:
    void setupUI();
    void populateConnectedDrives();

    QPushButton *connectButton;
    QPushButton *disconnectButton;
    QComboBox *connectedDrivesCombo;
};