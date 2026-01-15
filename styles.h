#pragma once

#include <QSize>
#include <QString>
#include "colors.h"

namespace Styles {
    // Основные размеры
    const int ThumbnailWidth = 150;
    const int ThumbnailHeight = ThumbnailWidth;
    const int ThumbnailGridWidth = 100;
    const int ThumbnailGridHeight = ThumbnailGridWidth;
    const int ThumbnailIconWidth = 96;
    const int ThumbnailIconHeight = ThumbnailIconWidth;
    const int ThumbnailSpacing = 10;

    // Размеры для делегата миниатюр
    const int ThumbnailDelegateTextHeight = 30;
    const int ThumbnailDelegateMargin = 5;
    const int ThumbnailDelegatePadding = 10;

    // Размеры кнопок быстрого доступа
    const int QuickAccessButtonWidth = 100;
    const int QuickAccessButtonHeight = 80;
    const int QuickAccessLayoutSpacing = 10;
    const int QuickAccessLayoutMargins = 10;

    // Размеры домашней страницы
    const int HomePageButtonWidth = 100;
    const int HomePageButtonHeight = 80;
    const int HomePageLayoutSpacing = 2;
    const int HomePageLayoutMargins = 2;
    const int HomePageWidgetHeight = 150;

    // Размеры шрифтов
    const int QuickAccessFontSize = 11;
    const int HomePageLabelFontSize = 14;
    const int NetworkDriveFontSize = 10;

    // Размеры сетевых дисков
    const int NetworkDriveComboWidth = 120;
    const int NetworkDriveComboMaxWidth = 150;

    // Тайминги
    const int LoadThumbnailsDelay = 200;
    const int ScrollLoadDelay = 150;

    // Стили для SearchWidget (темная тема, соответствующая основному окну)
    const QString SearchWidgetStyle =
    "SearchWidget {"
    "    background-color: " + Colors::DarkPrimary + ";"
    "    border: 2px solid " + Colors::AccentBlue + ";"
    "    border-radius: " + Colors::RadiusMedium + ";"
    "    color: " + Colors::TextWhite + ";"
    "}"
    "QLineEdit {"
    "    padding: 8px 12px;"
    "    border: 1px solid " + Colors::DarkTertiary + ";"
    "    border-radius: " + Colors::RadiusSmall + ";"
    "    background-color: " + Colors::DarkSecondary + ";"
    "    color: " + Colors::TextLight + ";"
    "    font-size: 14px;"
    "    selection-background-color: " + Colors::AccentBlue + ";"
    "    selection-color: " + Colors::TextWhite + ";"
    "}"
    "QPushButton {"
    "    padding: 8px 16px;"
    "    border: 1px solid " + Colors::DarkTertiary + ";"
    "    border-radius: " + Colors::RadiusSmall + ";"
    "    background-color: " + Colors::DarkSecondary + ";"
    "    color: " + Colors::TextLight + ";"
    "    font-size: 12px;"
    "    min-height: 20px;"
    "}"
    "QPushButton:hover {"
    "    background-color: " + Colors::DarkQuaternary + ";"
    "}"
    "QPushButton:disabled {"
    "    background-color: " + Colors::DarkPrimary + ";"
    "    color: " + Colors::TextMuted + ";"
    "}"
    "QPushButton#closeButton {"
    "    background-color: " + Colors::AccentRed + ";"
    "    color: " + Colors::TextWhite + ";"
    "    font-weight: bold;"
    "    font-size: 14px;"
    "    padding: 0px;"
    "}"
    "QPushButton#closeButton:hover {"
    "    background-color: #ff4444;" // Более светлый оттенок красного для ховера
    "}"
    "QCheckBox {"
    "    spacing: 8px;"
    "    background-color: transparent;"
    "    color: " + Colors::TextLight + ";"
    "    font-size: 12px;"
    "}"
    "QCheckBox::indicator {"
    "    width: 16px;"
    "    height: 16px;"
    "    border: 1px solid " + Colors::DarkTertiary + ";"
    "    border-radius: " + Colors::RadiusExtraSmall + ";"
    "    background-color: " + Colors::DarkSecondary + ";"
    "}"
    "QCheckBox::indicator:checked {"
    "    background-color: " + Colors::AccentBlue + ";"
    "    border: 1px solid " + Colors::AccentBlue + ";"
    "}"
    "QComboBox {"
    "    padding: 6px 10px;"
    "    border: 1px solid " + Colors::DarkTertiary + ";"
    "    border-radius: " + Colors::RadiusSmall + ";"
    "    background-color: " + Colors::DarkSecondary + ";"
    "    color: " + Colors::TextLight + ";"
    "    font-size: 12px;"
    "    min-height: 20px;"
    "}"
    "QComboBox::drop-down {"
    "    border: 0px;"
    "    width: 20px;"
    "}"
    "QComboBox::down-arrow {"
    "    image: none;"
    "    border-left: 1px solid " + Colors::DarkTertiary + ";"
    "    width: 15px;"
    "    background-color: " + Colors::DarkSecondary + ";"
    "}"
    "QComboBox QAbstractItemView {"
    "    background-color: " + Colors::DarkSecondary + ";"
    "    color: " + Colors::TextLight + ";"
    "    border: 1px solid " + Colors::DarkTertiary + ";"
    "    selection-background-color: " + Colors::AccentBlue + ";"
    "    font-size: 12px;"
    "}"
    "QListWidget {"
    "    background-color: " + Colors::DarkSecondary + ";"
    "    border: 1px solid " + Colors::DarkTertiary + ";"
    "    border-radius: " + Colors::RadiusSmall + ";"
    "    outline: none;"
    "    color: " + Colors::TextLight + ";"
    "    font-size: 13px;"
    "    padding: 4px;"
    "}"
    "QListWidget::item {"
    "    padding: 8px 12px;"
    "    border-bottom: 1px solid " + Colors::DarkTertiary + ";"
    "    background-color: " + Colors::DarkSecondary + ";"
    "}"
    "QListWidget::item:selected {"
    "    background-color: " + Colors::AccentBlue + ";"
    "    color: " + Colors::TextWhite + ";"
    "    border-radius: " + Colors::RadiusExtraSmall + ";"
    "}"
    "QListWidget::item:hover {"
    "    background-color: " + Colors::DarkQuaternary + ";"
    "    border-radius: " + Colors::RadiusExtraSmall + ";"
    "}"
    "QProgressBar {"
    "    border: none;"
    "    background-color: " + Colors::DarkSecondary + ";"
    "    border-radius: " + Colors::RadiusExtraSmall + ";"
    "    min-height: 4px;"
    "    max-height: 4px;"
    "}"
    "QProgressBar::chunk {"
    "    background-color: " + Colors::AccentBlue + ";"
    "    border-radius: " + Colors::RadiusExtraSmall + ";"
    "}"
    "QLabel {"
    "    color: " + Colors::TextGray + ";"
    "    font-size: 11px;"
    "    background: transparent;"
    "}"
    "QMenu {"
    "    background-color: " + Colors::DarkSecondary + ";"
    "    border: 1px solid " + Colors::DarkTertiary + ";"
    "    color: " + Colors::TextWhite + ";"
    "}"
    "QMenu::item {"
    "    padding: 5px 15px;"
    "}"
    "QMenu::item:selected {"
    "    background-color: " + Colors::AccentBlue + ";"
    "}"
    "/* Анимация для индикатора загрузки */"
    "@keyframes spin {"
    "    0% { transform: rotate(0deg); }"
    "    100% { transform: rotate(360deg); }"
    "}";

    // Стили для кнопок
    namespace Buttons {
        const QString QuickAccess =
            "QPushButton {"
            "    border: 1px solid " + Colors::DarkTertiary + ";"
            "    border-radius: " + Colors::RadiusSmall + ";"
            "    padding: 8px;"
            "    font-size: 11px;"
            "    background-color: " + Colors::QuickAccessBackground + ";"
            "    color: " + Colors::TextLight + ";"
            "}"
            "QPushButton:hover {"
            "    border-color: " + Colors::AccentBlue + ";"
            "    background-color: " + Colors::QuickAccessHover + ";"
            "}";

        const QString NetworkConnect =
            "QPushButton {"
            "    padding: 2px 5px;"
            "    font-size: 10px;"
            "    border: 1px solid " + Colors::DarkTertiary + ";"
            "    border-radius: " + Colors::RadiusExtraSmall + ";"
            "    background-color: " + Colors::NetworkConnect + ";"
            "    color: " + Colors::TextWhite + ";"
            "}"
            "QPushButton:hover {"
            "    border-color: " + Colors::AccentBlue + ";"
            "    background-color: " + Colors::AccentGreenHoverStart + ";"
            "}";

        const QString NetworkDisconnect =
            "QPushButton {"
            "    padding: 2px 5px;"
            "    font-size: 10px;"
            "    border: 1px solid " + Colors::DarkTertiary + ";"
            "    border-radius: " + Colors::RadiusExtraSmall + ";"
            "    background-color: " + Colors::NetworkDisconnect + ";"
            "    color: " + Colors::TextWhite + ";"
            "}"
            "QPushButton:hover {"
            "    border-color: " + Colors::AccentRed + ";"
            "    background-color: #ff4444;" // Более светлый оттенок красного для ховера
            "}";

        const QString SearchClose =
            "QPushButton {"
            "    border: 1px solid " + Colors::DarkTertiary + ";"
            "    border-radius: " + Colors::RadiusExtraSmall + ";"
            "    background-color: " + Colors::AccentRed + ";"
            "    font-weight: bold;"
            "    color: " + Colors::TextWhite + ";"
            "}"
            "QPushButton:hover {"
            "    background-color: #ff4444;" // Более светлый оттенок красного для ховера
            "}";

        const QString SearchAction =
            "QPushButton {"
            "    padding: 6px 12px;"
            "    border: 1px solid " + Colors::DarkTertiary + ";"
            "    border-radius: " + Colors::RadiusExtraSmall + ";"
            "    background-color: " + Colors::AccentBlue + ";"
            "    color: " + Colors::TextWhite + ";"
            "}"
            "QPushButton:hover {"
            "    background-color: " + Colors::TextBlue + ";"
            "}"
            "QPushButton:disabled {"
            "    background-color: " + Colors::DarkTertiary + ";"
            "    color: " + Colors::TextMuted + ";"
            "}";
    }
    
    // Стили для комбобоксов
    namespace ComboBoxes {
        const QString NetworkDrive =
            "QComboBox {"
            "    padding: 2px;"
            "    font-size: 10px;"
            "    background-color: " + Colors::DarkSecondary + ";"
            "    color: " + Colors::TextLight + ";"
            "    border: 1px solid " + Colors::DarkTertiary + ";"
            "    border-radius: " + Colors::RadiusExtraSmall + ";"
            "}";
    }
    
    // Стили для меток
    namespace Labels {
        const QString HomePage =
            "QLabel {"
            "    font-size: 14px;"
            "    font-weight: bold;"
            "    margin: 2px;"
            "    color: " + Colors::TextLight + ";"
            "    background: transparent;"
            "}";
    }
}