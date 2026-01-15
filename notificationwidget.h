#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTimer>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QProgressBar>
#include "colors.h"

class NotificationWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(float opacity READ opacity WRITE setOpacity)

public:
    enum Type {
        Info,
        Success,
        Warning,
        Error,
        Progress
    };

    explicit NotificationWidget(QWidget *parent = nullptr);
    ~NotificationWidget();

    void setTitle(const QString &title);
    void setMessage(const QString &message);
    void setType(Type type);
    void setProgress(int percent); // Только для Progress типа
    void setAutoClose(bool autoClose, int timeoutMs = 5000);
    void setPosition(const QPoint &position);

    float opacity() const { return m_opacity; }
    void setOpacity(float opacity);

    QSize sizeHint() const override;

    signals:
        void closed();
    void closeRequested();

protected:
    void paintEvent(QPaintEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void onCloseClicked();
    void onAutoCloseTimeout();

private:
    void setupUI();
    void updateStyle();
    QString getIconForType() const;

    QWidget *m_background;
    QLabel *m_iconLabel;
    QLabel *m_titleLabel;
    QLabel *m_messageLabel;
    QPushButton *m_closeButton;
    QWidget *m_progressWidget;
    QProgressBar *m_progressBar;

    Type m_type;
    bool m_autoClose;
    int m_timeoutMs;
    QTimer *m_autoCloseTimer;
    float m_opacity;
    QGraphicsOpacityEffect *m_opacityEffect;
};