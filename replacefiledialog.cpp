#include "replacefiledialog.h"
#include "colors.h"
#include <QApplication>
#include <QDebug>

ReplaceFileDialog::ReplaceFileDialog(const QString &fileName, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Подтверждение замены");
    setModal(true);
    setMinimumWidth(MIN_WIDTH);
    setMinimumHeight(MIN_HEIGHT);

    // Устанавливаем стиль для диалога в темной теме
    setStyleSheet(
        "QDialog {"
        "    background: " + Colors::GradientDark + ";"
        "    border: 2px solid " + Colors::BorderDark + ";"
        "    border-radius: " + Colors::RadiusLarge + ";"
        "    color: " + Colors::TextLight + ";"
        "}"
        "QLabel {"
        "    color: " + Colors::TextLight + ";"
        "    font-size: " + QString::number(FONT_SIZE_MESSAGE) + "px;"
        "    font-weight: normal;"
        "    padding: " + QString::number(PADDING_MEDIUM) + "px;"
        "    background: transparent;"
        "}"
        "QCheckBox {"
        "    color: " + Colors::TextLight + ";"
        "    font-size: " + QString::number(FONT_SIZE_CHECKBOX) + "px;"
        "    padding: " + QString::number(PADDING_SMALL) + "px;"
        "    background: transparent;"
        "    spacing: " + QString::number(PADDING_SMALL) + "px;"
        "}"
        "QCheckBox::indicator {"
        "    width: " + QString::number(CHECKBOX_IND_SIZE) + "px;"
        "    height: " + QString::number(CHECKBOX_IND_SIZE) + "px;"
        "    border: 2px solid " + Colors::TextDarkGray + ";"
        "    border-radius: " + Colors::RadiusExtraSmall + ";"
        "    background: " + Colors::DarkTertiary + ";"
        "}"
        "QCheckBox::indicator:checked {"
        "    background: " + Colors::GradientBlue + ";"
        "    border: 2px solid " + Colors::GradientBlue + ";"
        "}"
        "QCheckBox::indicator:hover {"
        "    border: 2px solid " + Colors::TextBlueLight + ";"
        "}"
        "QPushButton {"
        "    background: " + Colors::DarkTertiary + ";"
        "    color: " + Colors::TextLight + ";"
        "    border: 2px solid " + Colors::TextDarkGray + ";"
        "    padding: " + QString::number(PADDING_MEDIUM) + "px " + QString::number(PADDING_LARGE) + "px;"
        "    border-radius: " + Colors::RadiusSmall + ";"
        "    font-weight: bold;"
        "    font-size: " + QString::number(FONT_SIZE_BUTTON) + "px;"
        "    min-width: " + QString::number(BUTTON_MIN_WIDTH) + "px;"
        "    margin: " + QString::number(MARGIN_BUTTON) + "px;"
        "}"
        "QPushButton:hover {"
        "    background: " + Colors::DarkQuaternary + ";"
        "    border: 2px solid " + Colors::TextBlueLight + ";"
        "    color: " + Colors::TextWhite + ";"
        "}"
        "QPushButton:pressed {"
        "    background: " + Colors::ButtonPressedStart + ";"
        "    border: 2px solid " + Colors::TextBlue + ";"
        "}"
        "QPushButton:focus {"
        "    border: 2px solid " + Colors::TextBlueLight + ";"
        "    outline: none;"
        "}"
    );

    // Создаем основной layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(PADDING_LARGE, PADDING_LARGE, PADDING_LARGE, PADDING_LARGE);
    mainLayout->setSpacing(PADDING_MEDIUM);

    // Заголовок и сообщение
    QLabel *titleLabel = new QLabel("Конфликт файлов", this);
    titleLabel->setStyleSheet(
        "QLabel {"
        "    color: " + Colors::TextWhite + ";"
        "    font-size: " + QString::number(FONT_SIZE_TITLE) + "px;"
        "    font-weight: bold;"
        "    padding-bottom: " + QString::number(PADDING_MEDIUM) + "px;"
        "}"
    );

    QLabel *messageLabel = new QLabel(
        QString("Файл <b>\"%1\"</b> уже существует в целевой папке.\n\nВыберите действие:").arg(fileName), this);
    messageLabel->setWordWrap(true);
    messageLabel->setStyleSheet(
        "QLabel {"
        "    color: " + Colors::TextLight + ";"
        "    font-size: " + QString::number(FONT_SIZE_MESSAGE) + "px;"
        "    line-height: 1.4;"
        "}"
    );

    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(messageLabel);

    // Чекбокс "Применить ко всем"
    applyToAllCheck = new QCheckBox("Применить это действие ко всем конфликтующим файлам", this);
    mainLayout->addWidget(applyToAllCheck);

    mainLayout->addSpacing(PADDING_MEDIUM);

    // Кнопки действий
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(PADDING_SMALL);

    replaceButton = new QPushButton("Заменить", this);
    replaceButton->setToolTip("Заменяет существующий файл новым");
    replaceButton->setCursor(Qt::PointingHandCursor);

    skipButton = new QPushButton("Пропустить", this);
    skipButton->setToolTip("Пропускает этот файл");
    skipButton->setCursor(Qt::PointingHandCursor);

    copyButton = new QPushButton("Создать копию", this);
    copyButton->setToolTip("Создает копию файла с другим именем");
    copyButton->setCursor(Qt::PointingHandCursor);

    cancelButton = new QPushButton("Отмена", this);
    cancelButton->setToolTip("Отменяет всю операцию");
    cancelButton->setCursor(Qt::PointingHandCursor);

    buttonLayout->addWidget(replaceButton);
    buttonLayout->addWidget(skipButton);
    buttonLayout->addWidget(copyButton);
    buttonLayout->addWidget(cancelButton);

    // Устанавливаем специальные стили для кнопок
    replaceButton->setStyleSheet(
        "QPushButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 " + Colors::GradientRedStart + ", stop:1 " + Colors::GradientRedEnd + ");"
        "    border: 2px solid " + Colors::BorderRed + ";"
        "    color: " + Colors::TextWhite + ";"
        "}"
        "QPushButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 " + Colors::GradientRedEnd + ", stop:1 " + Colors::GradientRedStart + ");"
        "    border: 2px solid " + Colors::TextRed + ";"
        "}"
    );

    copyButton->setStyleSheet(
        "QPushButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 " + Colors::GradientBlueStart + ", stop:1 " + Colors::GradientBlueEnd + ");"
        "    border: 2px solid " + Colors::BorderBlue + ";"
        "    color: " + Colors::TextWhite + ";"
        "}"
        "QPushButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 " + Colors::GradientBlueEnd + ", stop:1 " + Colors::GradientBlueStart + ");"
        "    border: 2px solid " + Colors::TextBlueLight + ";"
        "}"
    );

    skipButton->setStyleSheet(
        "QPushButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 " + Colors::DarkQuaternary + ", stop:1 " + Colors::DarkTertiary + ");"
        "    border: 2px solid " + Colors::BorderDark + ";"
        "}"
    );

    cancelButton->setStyleSheet(
        "QPushButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 " + Colors::DarkQuaternary + ", stop:1 " + Colors::DarkTertiary + ");"
        "    border: 2px solid " + Colors::BorderDark + ";"
        "}"
        "QPushButton:hover {"
        "    border: 2px solid " + Colors::TextYellow + ";"
        "}"
    );

    mainLayout->addLayout(buttonLayout);

    // Подключаем сигналы
    connect(replaceButton, &QPushButton::clicked, this, &ReplaceFileDialog::onReplace);
    connect(skipButton, &QPushButton::clicked, this, &ReplaceFileDialog::onSkip);
    connect(copyButton, &QPushButton::clicked, this, &ReplaceFileDialog::onCopy);
    connect(cancelButton, &QPushButton::clicked, this, &ReplaceFileDialog::onCancel);

    // Настраиваем поведение по умолчанию
    replaceButton->setFocus();

    // Устанавливаем сочетания клавиш
    replaceButton->setShortcut(QKeySequence("Alt+R"));
    skipButton->setShortcut(QKeySequence("Alt+S"));
    copyButton->setShortcut(QKeySequence("Alt+C"));
    cancelButton->setShortcut(QKeySequence("Esc"));

    qDebug() << "ReplaceFileDialog created for file:" << fileName;
}

void ReplaceFileDialog::onReplace()
{
    result = Replace;
    applyToAll = applyToAllCheck->isChecked();
    qDebug() << "ReplaceFileDialog: Replace selected, apply to all:" << applyToAll;
    accept();
}

void ReplaceFileDialog::onSkip()
{
    result = Skip;
    applyToAll = applyToAllCheck->isChecked();
    qDebug() << "ReplaceFileDialog: Skip selected, apply to all:" << applyToAll;
    accept();
}

void ReplaceFileDialog::onCopy()
{
    result = Copy;
    applyToAll = applyToAllCheck->isChecked();
    qDebug() << "ReplaceFileDialog: Copy selected, apply to all:" << applyToAll;
    accept();
}

void ReplaceFileDialog::onCancel()
{
    result = Cancel;
    applyToAll = false;
    qDebug() << "ReplaceFileDialog: Cancel selected";
    reject();
}