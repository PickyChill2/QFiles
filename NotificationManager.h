#pragma once

#include <QObject>
#include <QList>
#include <QPoint>
#include <QMainWindow>
#include <QMap>
#include "notificationwidget.h"

namespace {
    const bool AUTO_CLOSE_ENABLED = true;
    const int AUTO_CLOSE_TIMEOUT = 5000;
    const bool AUTO_CLOSE_NOT_ENABLED = false;
    const int AUTO_CLOSE_NOT_TIMEOUT = 0;
}

class NotificationManager : public QObject
{
    Q_OBJECT

public:
    explicit NotificationManager(QMainWindow *parent = nullptr);
    ~NotificationManager();

    // Основные методы для работы с уведомлениями
    void showNotification(const QString &title, const QString &message,
                         NotificationWidget::Type type = NotificationWidget::Info,
                         bool autoClose = true, int timeoutMs = 5000);

    void showProgressNotification(const QString &operationId, const QString &title,
                             const QString &message, int progress = 0);

    void updateProgressNotification(const QString &operationId, const QString &message, int progress);

    void finishProgressNotification(const QString &operationId, const QString &title,
                                   const QString &message, NotificationWidget::Type type);

    void closeAllNotifications();
    void closeNotificationByOperationId(const QString &operationId);

private slots:
    void onNotificationClosed();
    void updatePositions();

private:
    QMainWindow *m_mainWindow;
    QList<NotificationWidget*> m_notifications;
    QMap<QString, NotificationWidget*> m_operationNotifications; // Карта операций и их уведомлений
    int m_spacing;

    QPoint calculatePosition(int index) const;
    void removeNotification(NotificationWidget *notification);
    QString generateOperationId(const QString &prefix);
};