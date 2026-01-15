#include "notificationwidget.h"
#include <QPainter>
#include <QDebug>

NotificationWidget::NotificationWidget(QWidget *parent)
    : QWidget(parent)
    , m_type(Info)
    , m_autoClose(false)
    , m_timeoutMs(0)
    , m_opacity(1.0f)
    , m_autoCloseTimer(nullptr)
{
    setupUI();
}

NotificationWidget::~NotificationWidget()
{
    if (m_autoCloseTimer) {
        m_autoCloseTimer->stop();
        delete m_autoCloseTimer;
    }
}

void NotificationWidget::setupUI()
{
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setFixedSize(350, 80);

    // Фон с закругленными углами
    m_background = new QWidget(this);
    m_background->setGeometry(0, 0, width(), height());

    // Основной layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(0);

    // Верхняя строка: иконка, заголовок и кнопка закрытия
    QHBoxLayout *headerLayout = new QHBoxLayout();

    m_iconLabel = new QLabel(this);
    m_iconLabel->setFixedSize(16, 16);
    m_iconLabel->setAlignment(Qt::AlignCenter);

    m_titleLabel = new QLabel(this);
    m_titleLabel->setStyleSheet(
        QString(
            "QLabel {"
            "    font-weight: bold;"
            "    color: %1;"
            "    font-size: 12px;"
            "}"
        ).arg(Colors::TextLight)
    );

    m_closeButton = new QPushButton("✕", this);
    m_closeButton->setFixedSize(20, 20);
    m_closeButton->setStyleSheet(
        QString(
            "QPushButton {"
            "    background: transparent;"
            "    color: %1;"
            "    border: none;"
            "    font-size: 14px;"
            "    padding: 0px;"
            "    margin: 0px;"
            "}"
            "QPushButton:hover {"
            "    background: %2;"
            "    border-radius: 3px;"
            "}"
        ).arg(Colors::TextLight, Colors::DarkQuaternary)
    );
    connect(m_closeButton, &QPushButton::clicked, this, &NotificationWidget::onCloseClicked);

    headerLayout->addWidget(m_iconLabel);
    headerLayout->addWidget(m_titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(m_closeButton);

    // Сообщение
    m_messageLabel = new QLabel(this);
    m_messageLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_messageLabel->setStyleSheet(
        QString(
            "QLabel {"
            "    color: %1;"
            "    font-size: 11px;"
            "    padding: 0px;"
            "    margin: 0px;"
            "    qproperty-wordWrap: false;"
            "}"
        ).arg(Colors::TextLight)
    );
    m_messageLabel->setWordWrap(false);

    // Прогресс-бар (скрыт по умолчанию)
    m_progressWidget = new QWidget(this);
    m_progressWidget->setVisible(false);
    QHBoxLayout *progressLayout = new QHBoxLayout(m_progressWidget);
    progressLayout->setContentsMargins(0, 4, 0, 0);

    m_progressBar = new QProgressBar(m_progressWidget);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(false);
    m_progressBar->setFixedHeight(6);
    m_progressBar->setStyleSheet(
        QString(
            "QProgressBar {"
            "    border: none;"
            "    border-radius: %1;"
            "    background: %2;"
            "}"
            "QProgressBar::chunk {"
            "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
            "        stop:0 %3, stop:0.5 %4, stop:1 %3);"
            "    border-radius: %1;"
            "}"
        ).arg(Colors::RadiusSmall, Colors::DarkTertiary,
              Colors::TextBlue, Colors::TextBlueLight)
    );
    progressLayout->addWidget(m_progressBar);

    // Добавляем элементы в основной layout
    mainLayout->addLayout(headerLayout);
    mainLayout->addWidget(m_messageLabel);
    mainLayout->addWidget(m_progressWidget);

    // Эффект прозрачности для анимации
    m_opacityEffect = new QGraphicsOpacityEffect(this);
    m_opacityEffect->setOpacity(m_opacity);
    setGraphicsEffect(m_opacityEffect);

    updateStyle();
}

void NotificationWidget::setTitle(const QString &title)
{
    m_titleLabel->setText(title);
}

void NotificationWidget::setMessage(const QString &message)
{
    // Убираем переносы строк
    QString singleLineMessage = message;
    singleLineMessage.replace('\n', ' ');
    singleLineMessage = singleLineMessage.simplified();
    m_messageLabel->setText(singleLineMessage);
}

void NotificationWidget::setType(Type type)
{
    m_type = type;
    updateStyle();
}

void NotificationWidget::setProgress(int percent)
{
    if (m_type == Progress) {
        bool showProgress = (percent >= 0 && percent < 100);
        m_progressWidget->setVisible(showProgress);

        if (showProgress) {
            m_progressBar->setValue(percent);

            // Обновляем сообщение с прогрессом
            QString message = m_messageLabel->text();
            QString baseMessage = message;
            int percentIndex = message.indexOf(" - ");
            if (percentIndex != -1) {
                baseMessage = message.left(percentIndex);
            }

            if (percent > 0 && percent < 100) {
                m_messageLabel->setText(baseMessage + " - " + QString::number(percent) + "%");
            } else {
                m_messageLabel->setText(baseMessage);
            }
        }
    }
}

void NotificationWidget::setAutoClose(bool autoClose, int timeoutMs)
{
    m_autoClose = autoClose;
    m_timeoutMs = timeoutMs;

    if (m_autoCloseTimer) {
        m_autoCloseTimer->stop();
        delete m_autoCloseTimer;
        m_autoCloseTimer = nullptr;
    }

    if (m_autoClose && m_timeoutMs > 0) {
        m_autoCloseTimer = new QTimer(this);
        connect(m_autoCloseTimer, &QTimer::timeout, this, &NotificationWidget::onAutoCloseTimeout);
        m_autoCloseTimer->start(m_timeoutMs);
    }
}

void NotificationWidget::setPosition(const QPoint &position)
{
    move(position);
}

void NotificationWidget::setOpacity(float opacity)
{
    m_opacity = opacity;
    if (m_opacityEffect) {
        m_opacityEffect->setOpacity(opacity);
    }
}

QSize NotificationWidget::sizeHint() const
{
    return QSize(350, 80);
}

void NotificationWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
}

void NotificationWidget::showEvent(QShowEvent *event)
{
    Q_UNUSED(event)
    // Анимация появления
    QPropertyAnimation *animation = new QPropertyAnimation(this, "opacity");
    animation->setDuration(300);
    animation->setStartValue(0.0);
    animation->setEndValue(1.0);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void NotificationWidget::updateStyle()
{
    QString backgroundColor = Colors::DarkPrimary;
    QString borderColor = Colors::DarkQuaternary;
    QString iconColor = Colors::TextLight;
    QString iconText = getIconForType();

    switch (m_type) {
        case Success:
            iconColor = Colors::NotificationGreen; // Зеленый
            break;
        case Error:
            iconColor = Colors::NotificationRed; // Красный
            break;
        case Warning:
            iconColor = Colors::NotificationOrange; // Оранжевый
            break;
        case Info:
            iconColor = Colors::TextBlue; // Синий
            break;
        case Progress:
            iconColor = Colors::TextBlue;
            break;
    }

    m_iconLabel->setText(iconText);
    m_iconLabel->setStyleSheet(
        QString(
            "QLabel {"
            "    color: %1;"
            "    font-size: 14px;"
            "    font-weight: bold;"
            "    margin: 0px;"
            "    padding: 0px;"
            "}"
        ).arg(iconColor)
    );

    m_background->setStyleSheet(
        QString(
            "QWidget {"
            "    background: %1;"
            "    border: 2px solid %2;"
            "    border-radius: %3;"
            "}"
        ).arg(backgroundColor, borderColor, Colors::RadiusMedium)
    );
}

QString NotificationWidget::getIconForType() const
{
    switch (m_type) {
    case Success: return "✓";
    case Error: return "✗";
    case Warning: return "⚠";
    case Progress: return "⟳";
    case Info: return "ℹ";
    default: return "";
    }
}

void NotificationWidget::onCloseClicked()
{
    // Анимация закрытия
    QPropertyAnimation *animation = new QPropertyAnimation(this, "opacity");
    animation->setDuration(300);
    animation->setStartValue(m_opacity);
    animation->setEndValue(0.0);
    connect(animation, &QPropertyAnimation::finished, this, [this]() {
        emit closeRequested();
        emit closed();
        deleteLater();
    });
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void NotificationWidget::onAutoCloseTimeout()
{
    if (m_autoClose) {
        onCloseClicked();
    }
}