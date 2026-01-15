#pragma once

#include <QSize>

namespace IconSizes {
    // Размеры иконок для различных режимов отображения
    namespace ListView {
        const QSize Small = QSize(16, 16);
        const QSize Medium = QSize(24, 24);
        const QSize Large = QSize(32, 32);
        const QSize Default = Medium;
    }

    namespace TreeView {
        const QSize Small = QSize(16, 16);
        const QSize Medium = QSize(20, 20);
        const QSize Large = QSize(24, 24);
        const QSize Default = Medium;
    }

    namespace ThumbnailView {
        const QSize Small = QSize(48, 48);
        const QSize Medium = QSize(64, 64);
        const QSize Large = QSize(96, 96);
        const QSize Default = Large;
    }

    // Размеры для тулбара и других элементов интерфейса
    namespace ToolBar {
        const QSize Small = QSize(16, 16);
        const QSize Medium = QSize(24, 24);
        const QSize Large = QSize(32, 32);
        const QSize Default = Medium;
    }

    // Размеры для кнопок быстрого доступа
    namespace QuickAccess {
        const QSize Small = QSize(32, 32);
        const QSize Medium = QSize(48, 48);
        const QSize Large = QSize(64, 64);
        const QSize Default = Medium;
    }
}