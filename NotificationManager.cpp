#include "notificationmanager.h"
#include <QGuiApplication>
#include <QScreen>
#include <QDateTime>
#include <QDebug>

NotificationManager::NotificationManager(QMainWindow *parent)
    : QObject(parent)
    , m_mainWindow(parent)
    , m_spacing(10) // Отступ между уведомлениями
{
}

NotificationManager::~NotificationManager()
{
    closeAllNotifications();
}

QString NotificationManager::generateOperationId(const QString &prefix)
{
    return QString("%1_%2").arg(prefix).arg(QDateTime::currentMSecsSinceEpoch());
}

void NotificationManager::showNotification(const QString &title, const QString &message,
                                         NotificationWidget::Type type,
                                         bool autoClose, int timeoutMs)
{
    NotificationWidget *notification = new NotificationWidget(m_mainWindow);
    notification->setTitle(title);
    notification->setMessage(message);
    notification->setType(type);
    notification->setAutoClose(autoClose, timeoutMs);

    connect(notification, &NotificationWidget::closed,
            this, &NotificationManager::onNotificationClosed);
    connect(notification, &NotificationWidget::closeRequested,
            this, [this, notification]() { removeNotification(notification); });

    m_notifications.append(notification);
    updatePositions();

    notification->show();
}

void NotificationManager::showProgressNotification(const QString &operationId, const QString &title,
                                                  const QString &message, int progress)
{
    // Закрываем предыдущее уведомление для этой операции, если оно есть
    if (m_operationNotifications.contains(operationId)) {
        NotificationWidget *oldNotification = m_operationNotifications[operationId];
        if (oldNotification) {
            oldNotification->close();
            removeNotification(oldNotification);
        }
    }

    NotificationWidget *notification = new NotificationWidget(m_mainWindow);
    notification->setTitle(title);
    notification->setMessage(message);
    notification->setType(NotificationWidget::Progress);
    notification->setProgress(progress);
    notification->setAutoClose(false, 0); // Прогресс-уведомления не закрываются автоматически

    connect(notification, &NotificationWidget::closed,
            this, &NotificationManager::onNotificationClosed);
    connect(notification, &NotificationWidget::closeRequested,
            this, [this, notification, operationId]() {
                m_operationNotifications.remove(operationId);
                removeNotification(notification);
            });

    m_notifications.append(notification);
    m_operationNotifications[operationId] = notification;
    updatePositions();

    notification->show();
}

void NotificationManager::updateProgressNotification(const QString &operationId, const QString &message, int progress)
{
    if (m_operationNotifications.contains(operationId)) {
        NotificationWidget *notification = m_operationNotifications[operationId];
        if (notification) {
            notification->setMessage(message);
            notification->setProgress(progress);
        }
    }
}

void NotificationManager::finishProgressNotification(const QString &operationId,
                                                    const QString &title,
                                                    const QString &message,
                                                    NotificationWidget::Type type)
{
    if (m_operationNotifications.contains(operationId)) {
        NotificationWidget *notification = m_operationNotifications[operationId];
        if (notification) {
            // Обновляем существующее уведомление вместо создания нового
            notification->setTitle(title);
            notification->setMessage(message);
            notification->setType(type);
            notification->setAutoClose(AUTO_CLOSE_ENABLED, AUTO_CLOSE_TIMEOUT);
            //notification->setAutoClose(AUTO_CLOSE_NOT_ENABLED, AUTO_CLOSE_NOT_TIMEOUT);

            // Удаляем прогресс-бар для завершенных операций
            notification->setProgress(-1); // Добавим метод для скрытия прогресс-бара

            // Удаляем из карты операций, так как операция завершена
            m_operationNotifications.remove(operationId);
        }
    }
}

void NotificationManager::closeAllNotifications()
{
    for (NotificationWidget *notification : m_notifications) {
        notification->close();
        notification->deleteLater();
    }
    m_notifications.clear();
    m_operationNotifications.clear();
}

void NotificationManager::closeNotificationByOperationId(const QString &operationId)
{
    if (m_operationNotifications.contains(operationId)) {
        NotificationWidget *notification = m_operationNotifications[operationId];
        if (notification) {
            notification->close();
            removeNotification(notification);
        }
        m_operationNotifications.remove(operationId);
    }
}

void NotificationManager::onNotificationClosed()
{
    NotificationWidget *notification = qobject_cast<NotificationWidget*>(sender());
    if (notification) {
        // Удаляем из карты операций
        QString operationId;
        for (auto it = m_operationNotifications.begin(); it != m_operationNotifications.end(); ++it) {
            if (it.value() == notification) {
                operationId = it.key();
                break;
            }
        }
        if (!operationId.isEmpty()) {
            m_operationNotifications.remove(operationId);
        }

        removeNotification(notification);
    }
}


void NotificationManager::updatePositions()
{
    if (!m_mainWindow) return;

    for (int i = 0; i < m_notifications.size(); ++i) {
        NotificationWidget *notification = m_notifications[i];
        QPoint position = calculatePosition(i);
        notification->setPosition(position);
    }
}

QPoint NotificationManager::calculatePosition(int index) const
{
    if (!m_mainWindow || index < 0 || index >= m_notifications.size()) {
        return QPoint(0, 0);
    }

    QRect mainRect = m_mainWindow->geometry();
    int notificationHeight = 80; // Высота уведомления
    int notificationWidth = 350; // Ширина уведомления

    // Позиция в правом верхнем углу
    int x = mainRect.right() - notificationWidth - m_spacing;

    // Вычисляем Y позицию: первое уведомление вверху, остальные под ним с отступом
    int y = mainRect.top() + m_spacing + (index * (notificationHeight + m_spacing));

    // Проверяем, чтобы уведомление не вышло за пределы экрана
    QScreen *screen = QGuiApplication::screenAt(m_mainWindow->pos());
    if (screen) {
        QRect screenRect = screen->geometry();
        if (y + notificationHeight > screenRect.bottom()) {
            // Если уведомления не помещаются, начинаем сдвигать вверх
            y = screenRect.bottom() - notificationHeight - m_spacing;
        }
    }

    return QPoint(x, y);
}

void NotificationManager::removeNotification(NotificationWidget *notification)
{
    if (m_notifications.removeOne(notification)) {
        notification->deleteLater();
        updatePositions();
    }
}