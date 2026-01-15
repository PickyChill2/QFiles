#pragma once

#include <QString>

namespace Strings {
    // Quick Access Widget
    const QString Pictures = "Изображения";
    const QString Documents = "Документы";
    const QString Downloads = "Загрузки";
    const QString Desktop = "Рабочий стол";
    const QString Music = "Музыка";
    const QString Videos = "Видео";

    // Emojis for Quick Access
    const QString EmojiPictures = "🖼️";
    const QString EmojiDocuments = "📄";
    const QString EmojiDownloads = "📥";
    const QString EmojiDesktop = "🖥️";
    const QString EmojiMusic = "🎵";
    const QString EmojiVideos = "🎬";
    const QString UndoDeleteTitle = "Отменить удаление";

    // Toolbar actions
    const QString Back = "◀ Назад";
    const QString Forward = "▶ Вперед";
    const QString Up = "⬆ Вверх";
    const QString Home = "🏠 Домой";
    const QString Refresh = "🔄 обновить";
    const QString ListView = "☰ Список";
    const QString TreeView = "🌳 Дерево";
    const QString ThumbnailView = "🖼️ Миниатюры";
    const QString NewFolder = "📁 Новая папка";
    const QString Rename = "✏️ Переименовать";
    const QString MakeNewTab = "➕ Новая вкладка";
    const QString CloseTab = "❌ Закрыть вкладку";
    const QString Hidden = "👁️ Скрытые";

    // Context menu actions
    const QString Open = "📂 Открыть";
    const QString OpenFile = "📄 Открыть";
    const QString OpenWith = "🔧 Открыть в";
    const QString Cut = "✂️ Вырезать";
    const QString Copy = "📋 Копировать";
    const QString Paste = "📄 Вставить";
    const QString Delete = "🗑️ В корзину";
    const QString Properties = "📊 Свойства";
    const QString ShowInExplorer = "🔍 В проводнике";
    const QString Create = "🐊 Создать";
    const QString NewTextFile = "🗒️ Текстовый файл";
    const QString NewDocFile = "📄 Word";
    const QString Edit = "📝 Изменить";
    const QString ExtractHere = "🗃️ Извлечь сюда";
    const QString CreateShortcut = "☠️ Создать ярлык";
    const QString CreateArchive = "📇 Создать архив";

    const QString PowerShell1 = "🚂 Запустить в PowerShell";
    const QString PowerShell2 = "🚄 Запустить в PowerShell от имени администратора";

    const QString RenameTitle = "Переименование";
    const QString RenamePrompt = "Введите новое имя:";

    // Recycle Bin actions
    const QString EmptyRecycleBin = "🧹 Очистить корзину";
    const QString OpenRecycleBin = "📂 Открыть корзину";
    const QString RestoreItem = "📤 Восстановить";
    const QString DeletePermanently = "💀 Удалить навсегда";


    //Корзина
    const QString RecycleBinPath = "recycle://";
    const QString RecycleBinTitle = "Корзина";
    const QString EmojiRecycleBin = "🗑️";

    const QString Favorites = "⭐ Избранное";

    // Сортировка
    const QString SortByName = "По имени";
    const QString SortBySize = "По размеру";
    const QString SortByDate = "По дате";
    const QString SortByType = "По типу";
    const QString SortAscending = "По возрастанию";
    const QString SortDescending = "По убыванию";

    //__________________________________________________________________________________________________________________

    // Messages
    const QString AppName = "QFiles";
    const QString HomePage = "Home";
    const QString OpenWithTitle = "Open With";
    const QString OpenWithMessage = "Open with functionality would go here";
    const QString CutTitle = "Cut";
    const QString CutMessage = "%1 items cut";
    const QString CopyTitle = "Copy";
    const QString CopyMessage = "%1 items copied";
    const QString PasteTitle = "Paste";
    const QString PasteMessage = "Would %1 %2 items to %3";
    const QString DeleteTitle = "Delete";
    const QString DeleteMessage = "Delete %1 selected items?";
    const QString PropertiesTitle = "Properties";
    const QString PropertiesTemplate = "Name: %1\nSize: %2 bytes\nType: %3\nModified: %4";
    const QString Error = "Error";
    const QString ParentWindowError = "Parent window is not set";
    const QString NavigateError = "Please navigate to a folder first";
    const QString DirectoryError = "Directory does not exist";
    const QString FolderError = "Failed to create folder";
    const QString NavigationError = "Cannot navigate to: %1";
    const QString Info = "Info";

    // File Operations
    const QString DeleteError = "Failed to delete: %1";
    const QString PasteError = "Failed to paste: %1";
    const QString PastePartialError = "Successfully processed %1 items, failed: %2";
    const QString RenameExistsError = "A file or folder with name '%1' already exists";
    const QString RenameError = "Failed to rename: %1";

    // Network Drive
    const QString NetworkDriveConnect = "🌐 Подкл. сетевой диск";
    const QString NetworkDriveDisconnect = "❌ Откл. сетевой диск";

    // Recycle Bin messages // Добавлено
    const QString EmptyRecycleBinTitle = "Очистка корзины";
    const QString EmptyRecycleBinMessage = "Вы уверены, что хотите очистить корзину?";
    const QString EmptyRecycleBinSuccess = "Корзина успешно очищена";
    const QString EmptyRecycleBinError = "Ошибка при очистке корзины";
    const QString RestoreSuccess = "Элементы успешно восстановлены";
    const QString RestoreError = "Ошибка при восстановлении элементов";
    const QString DeletePermanentlyTitle = "Удаление навсегда";
    const QString DeletePermanentlyMessage = "Вы уверены, что хотите удалить выбранные элементы навсегда?";

    const QString AddToFavorites = "Добавить текущую папку в избранное";
    const QString ManageFavorites = "Управление избранным";
    const QString EnterFavoriteName = "Введите название для избранного:";
    const QString FavoriteExists = "Избранное с таким названием уже существует!";
    const QString NoFavorites = "Нет сохраненных избранных папок.";
    const QString SelectFavoriteToRemove = "Выберите избранное для удаления:";
    const QString Warning = "Предупреждение";
    const QString Path = QObject::tr("Путь");
}
