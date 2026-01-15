// colors.h
#ifndef COLORS_H
#define COLORS_H

#include <QString>

//favoritesmenu - +
//mainwindow - ?
//searchwidget - +
//networkdrivewidget - ?
//thumbnailview - ?
//quickaccesswidget - ?
//recyclebinwidget - ?


namespace Colors {

    // Основные цвета темной темы
    // Используется в: favoritesmenu.cpp, folderviewsettings.cpp, mainwindow.cpp
    const QString DarkPrimary = "#2d3748";           // Основной фон диалогов и меню
    const QString DarkSecondary = "#1a202c";         // Вторичный фон градиентов
    const QString DarkTertiary = "#4a5568";          // Фон списков, кнопок
    const QString DarkQuaternary = "#5a6578";        // Ховер-состояния

    // Цвета для состояний кнопок
    const QString ButtonPressedStart = "#3d4554";    // Начало градиента для нажатой кнопки
    const QString ButtonPressedEnd = "#2d3748";      // Конец градиента для нажатой кнопки

    // Текст
    // Используется в: favoritesmenu.cpp, searchwidget.cpp, networkdrivewidget.cpp
    const QString TextLight = "#e2e8f0";             // Основной текст
    const QString TextMuted = "#a0aec0";             // Выключенный текст
    const QString TextWhite = "#ffffff";             // Белый текст
    const QString TextBlue = "#3182ce";              // Акцентный синий текст
    const QString TextBlueLight = "#90cdf4";         // Светлый синий текст
    const QString TextGray = "#cccccc";              // Серый текст (поиск)
    const QString TextDarkGray = "#718096";          // Темно-серый текст

    // Акцентные цвета
    // Используется в: favoritesmenu.cpp, networkdrivewidget.cpp
    const QString AccentGreenStart = "#38a169";      // Начало градиента зеленых кнопок
    const QString AccentGreenEnd = "#2f855a";        // Конец градиента зеленых кнопок
    const QString AccentGreenHoverStart = "#48bb78"; // Ховер начало
    const QString AccentGreenHoverEnd = "#38a169";   // Ховер конец
    const QString AccentGreenPressedStart = "#2d784d"; // Нажатое состояние начало
    const QString AccentGreenPressedEnd = "#276741"; // Нажатое состояние конец
    const QString AccentBlue = "#0078d4";            // Синий акцент (индикатор загрузки)
    const QString AccentRed = "#e53e3e";             // Красный акцент (отключение)

    // Градиенты
    // Используется в: favoritesmenu.cpp
    const QString GradientDark = "qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #2d3748, stop: 1 #1a202c)";
    const QString GradientGreen = "qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #38a169, stop: 1 #2f855a)";
    const QString GradientGreenHover = "qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #48bb78, stop: 1 #38a169)";
    const QString GradientGreenPressed = "qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #2d784d, stop: 1 #276741)";

    // Прозрачности
    // Используется в: favoritesmenu.cpp (меню избранного)
    const QString TransparentDark95 = "rgba(45, 55, 72, 0.95)"; // Полупрозрачное меню
    const QString TransparentDark240 = "rgba(45, 55, 72, 240)"; // Почти непрозрачный фон
    const QString TransparentBlue30 = "rgba(49, 130, 206, 0.3)"; // Выделение элементов меню
    const QString TransparentBlack80 = "rgba(0, 0, 0, 80)";     // Тень меню

    // Скругления
    // Используется в: favoritesmenu.cpp, searchwidget.cpp, thumbnailview.cpp
    const QString RadiusLarge = "12px";      // Большие скругления (диалоги, меню)
    const QString RadiusMedium = "8px";      // Средние скругления (кнопки, элементы)
    const QString RadiusSmall = "6px";       // Малые скругления (инпуты)
    const QString RadiusExtraSmall = "4px";  // Очень малые скругления

    // Размеры теней
    // Используется в: favoritesmenu.cpp (тень меню)
    const QString ShadowBlurRadius = "20px"; // Радиус размытия тени
    const QString ShadowOffset = "0, 4px";   // Смещение тени

    // Цвета для поиска
    // Используется в: searchwidget.cpp
    const QString SearchBackground = "#1e1e1e";      // Фон виджета поиска
    const QString SearchBorder = "#333";             // Граница поиска
    const QString SearchText = "#cccccc";            // Текст в поиске

    // Цвета для индикаторов
    // Используется в: searchwidget.cpp (индикатор загрузки)
    const QString LoadingIndicator = "#0078d4";      // Цвет индикатора загрузки

    // Цвета для сетевых дисков
    // Используется в: networkdrivewidget.cpp
    const QString NetworkConnect = "#38a169";        // Кнопка подключения
    const QString NetworkDisconnect = "#e53e3e";     // Кнопка отключения

    // Цвета для кнопок быстрого доступа
    // Используется в: homepagewidget.cpp, quickaccesswidget.cpp
    const QString QuickAccessBackground = "#4a5568"; // Фон кнопок быстрого доступа
    const QString QuickAccessHover = "#5a6578";      // Ховер кнопок быстрого доступа

    // Цвета для корзины
    // Используется в: recyclebinwidget.cpp
    const QString RecycleBinBackground = "#2d3748";  // Фон корзины
    const QString RecycleBinBorder = "#4a5568";      // Граница корзины

    // Цвета для миниатюр
    // Используется в: thumbnailview.cpp, thumbnaildelegate.cpp
    const QString ThumbnailPlaceholder = "#c8d1e0";  // Заглушка для миниатюр
    const QString ThumbnailBackground = "#e6f3ff";   // Фон миниатюр

    //уведомления
    const QString NotificationGreen = "#4CAF50";
    const QString NotificationRed = "#F44336";
    const QString NotificationOrange = "#FF9800";

    //Копирование/Замена
    static const QString GradientRedStart = "#ff4164";
    static const QString GradientRedEnd = "#ff4b2b";
    static const QString BorderRed = "#e63946";
    static const QString GradientBlueStart = "#2193b0";
    static const QString GradientBlueEnd = "#6dd5ed";
    static const QString BorderBlue = "#1e90ff";
    static const QString GradientBlue = "#0065b0";
    static const QString GradientGreenEnd = "#96c93d";
    static const QString BorderGreen = "#32cd32";
    static const QString TextRed = "#ff6b6b";
    static const QString TextYellow = "#ffd93d";
    static const QString BorderDark = "#3a3a3a";

}

namespace Radius {
    // Целочисленные значения скруглений (для использования в коде)
    // Используется в: favoritesmenu.cpp, thumbnailview.cpp
    const int Large = 12;        // Большие скругления
    const int Medium = 8;        // Средние скругления
    const int Small = 6;         // Малые скругления
    const int ExtraSmall = 4;    // Очень малые скругления
}

namespace Alpha {
    // Значения прозрачности (0-255)
    // Используется в: favoritesmenu.cpp
    const int Opaque = 255;      // Полностью непрозрачный
    const int High = 240;        // Высокая непрозрачность
    const int Medium = 128;      // Средняя прозрачность
    const int Low = 80;          // Низкая непрозрачность
    const int VeryLow = 30;      // Очень низкая непрозрачность
}

    namespace Shadow {
        // Параметры теней
        const int BlurRadius = 20;        // Радиус размытия тени
        const int OffsetX = 0;            // Смещение по X
        const int OffsetY = 4;            // Смещение по Y
    }

#endif // COLORS_H