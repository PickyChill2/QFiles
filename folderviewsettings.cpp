#include "folderviewsettings.h"
#include <QDebug>

FolderViewSettings::FolderViewSettings(QObject *parent)
    : QObject(parent)
{
    // Используем INI файл в папке приложения
    settings = new QSettings("QFiles", "FolderViewSettings", this);
    settings->setDefaultFormat(QSettings::IniFormat);

    qDebug() << "FolderViewSettings file:" << settings->fileName();

    // Загружаем все настройки в кэш при старте
    QStringList allKeys = settings->allKeys();
    for (const QString &key : allKeys) {
        if (key.startsWith("ViewModes/")) {
            QString folderPath = key.mid(10); // Убираем "ViewModes/"
            int viewMode = settings->value(key).toInt();
            viewModeCache[folderPath] = viewMode;
            qDebug() << "Loaded view mode for" << folderPath << ":" << viewMode;
        }
        else if (key.startsWith("SortColumn/")) {
            QString folderPath = key.mid(11); // Убираем "SortColumn/"
            int sortColumn = settings->value(key).toInt();
            QString orderKey = "SortOrder/" + folderPath;
            Qt::SortOrder sortOrder = static_cast<Qt::SortOrder>(settings->value(orderKey, static_cast<int>(Qt::AscendingOrder)).toInt());
            sortCache[folderPath] = qMakePair(sortColumn, sortOrder);
            qDebug() << "Loaded sort settings for" << folderPath << ":" << sortColumn << "," << sortOrder;
        }
    }
}

void FolderViewSettings::saveViewMode(const QString &folderPath, int viewMode)
{
    QString normalizedPath = normalizePath(folderPath);
    if (normalizedPath.isEmpty()) {
        return;
    }

    QString key = getSettingsKey(normalizedPath, "ViewModes");
    settings->setValue(key, viewMode);
    settings->sync(); // Немедленно сохраняем

    // Обновляем кэш
    viewModeCache[normalizedPath] = viewMode;

    qDebug() << "Saved view mode for" << normalizedPath << ":" << viewMode;
}

int FolderViewSettings::loadViewMode(const QString &folderPath) const
{
    QString normalizedPath = normalizePath(folderPath);
    if (normalizedPath.isEmpty()) {
        return getDefaultViewMode();
    }

    // Проверяем кэш сначала
    if (viewModeCache.contains(normalizedPath)) {
        return viewModeCache[normalizedPath];
    }

    // Загружаем из настроек
    QString key = getSettingsKey(normalizedPath, "ViewModes");
    int viewMode = settings->value(key, getDefaultViewMode()).toInt();

    // Сохраняем в кэш
    viewModeCache[normalizedPath] = viewMode;

    qDebug() << "Loaded view mode for" << normalizedPath << ":" << viewMode;
    return viewMode;
}

void FolderViewSettings::saveSortSettings(const QString &folderPath, int sortColumn, Qt::SortOrder sortOrder)
{
    QString normalizedPath = normalizePath(folderPath);
    if (normalizedPath.isEmpty()) {
        return;
    }

    QString columnKey = getSettingsKey(normalizedPath, "SortColumn");
    QString orderKey = getSettingsKey(normalizedPath, "SortOrder");

    settings->setValue(columnKey, sortColumn);
    settings->setValue(orderKey, static_cast<int>(sortOrder));
    settings->sync();

    // Обновляем кэш
    sortCache[normalizedPath] = qMakePair(sortColumn, sortOrder);

    qDebug() << "Saved sort settings for" << normalizedPath << ":" << sortColumn << "," << sortOrder;
}

void FolderViewSettings::loadSortSettings(const QString &folderPath, int &sortColumn, Qt::SortOrder &sortOrder) const
{
    QString normalizedPath = normalizePath(folderPath);
    if (normalizedPath.isEmpty()) {
        getDefaultSortSettings(sortColumn, sortOrder);
        return;
    }

    // Проверяем кэш сначала
    if (sortCache.contains(normalizedPath)) {
        sortColumn = sortCache[normalizedPath].first;
        sortOrder = sortCache[normalizedPath].second;
        return;
    }

    QString columnKey = getSettingsKey(normalizedPath, "SortColumn");
    QString orderKey = getSettingsKey(normalizedPath, "SortOrder");

    int defaultColumn;
    Qt::SortOrder defaultOrder;
    getDefaultSortSettings(defaultColumn, defaultOrder);

    sortColumn = settings->value(columnKey, defaultColumn).toInt();
    sortOrder = static_cast<Qt::SortOrder>(settings->value(orderKey, static_cast<int>(defaultOrder)).toInt());

    // Сохраняем в кэш
    sortCache[normalizedPath] = qMakePair(sortColumn, sortOrder);

    qDebug() << "Loaded sort settings for" << normalizedPath << ":" << sortColumn << "," << sortOrder;
}

void FolderViewSettings::getDefaultSortSettings(int &sortColumn, Qt::SortOrder &sortOrder) const
{
    sortColumn = 0; // По имени по умолчанию
    sortOrder = Qt::AscendingOrder; // По возрастанию
}

QString FolderViewSettings::normalizePath(const QString &path) const
{
    if (path.isEmpty()) {
        return "";
    }

    // Обрабатываем специальные пути
    if (path == "HomePage" || path == "RecycleBin") {
        return path;
    }

    // Нормализуем файловый путь
    QString normalized = QDir::cleanPath(path);

    // Для корневых дисков (C:\), оставляем как есть
    if (normalized.length() == 3 && normalized.endsWith(":/")) {
        return normalized;
    }

    // Убираем слеш в конце для папок (кроме корневых)
    if (normalized.endsWith('/') && normalized.length() > 3) {
        normalized.chop(1);
    }

    return normalized;
}

QString FolderViewSettings::getSettingsKey(const QString &folderPath, const QString &settingType) const
{
    return settingType + "/" + folderPath;
}

void FolderViewSettings::cleanupNonExistentFolders()
{
    QMap<QString, int> newViewModeCache;
    QMap<QString, QPair<int, Qt::SortOrder>> newSortCache;
    QStringList keysToRemove;

    // Очищаем кэш режимов просмотра
    for (auto it = viewModeCache.begin(); it != viewModeCache.end(); ++it) {
        const QString &folderPath = it.key();

        // Пропускаем специальные пути
        if (folderPath == "HomePage" || folderPath == "RecycleBin") {
            newViewModeCache[folderPath] = it.value();
            continue;
        }

        // Проверяем существование реальной папки
        if (QDir(folderPath).exists()) {
            newViewModeCache[folderPath] = it.value();
        } else {
            keysToRemove.append(getSettingsKey(folderPath, "ViewModes"));
            qDebug() << "Removing view settings for non-existent folder:" << folderPath;
        }
    }

    // Очищаем кэш сортировки
    for (auto it = sortCache.begin(); it != sortCache.end(); ++it) {
        const QString &folderPath = it.key();

        // Пропускаем специальные пути
        if (folderPath == "HomePage" || folderPath == "RecycleBin") {
            newSortCache[folderPath] = it.value();
            continue;
        }

        // Проверяем существование реальной папки
        if (QDir(folderPath).exists()) {
            newSortCache[folderPath] = it.value();
        } else {
            keysToRemove.append(getSettingsKey(folderPath, "SortColumn"));
            keysToRemove.append(getSettingsKey(folderPath, "SortOrder"));
            qDebug() << "Removing sort settings for non-existent folder:" << folderPath;
        }
    }

    // Удаляем из настроек
    for (const QString &key : keysToRemove) {
        settings->remove(key);
    }

    // Обновляем кэши
    viewModeCache = newViewModeCache;
    sortCache = newSortCache;
    
    if (!keysToRemove.isEmpty()) {
        settings->sync();
    }
}