#pragma once

#include <QObject>
#include <QMap>
#include <QString>
#include <QSettings>
#include <QDir>

class FolderViewSettings : public QObject
{
    Q_OBJECT

public:
    explicit FolderViewSettings(QObject *parent = nullptr);

    // Сохранить режим отображения для папки
    void saveViewMode(const QString &folderPath, int viewMode);

    // Загрузить режим отображения для папки
    int loadViewMode(const QString &folderPath) const;

    // Сохранить настройки сортировки для папки
    void saveSortSettings(const QString &folderPath, int sortColumn, Qt::SortOrder sortOrder);

    // Загрузить настройки сортировки для папки
    void loadSortSettings(const QString &folderPath, int &sortColumn, Qt::SortOrder &sortOrder) const;

    // Получить режим отображения по умолчанию
    int getDefaultViewMode() const { return 0; } // 0 = List View

    // Получить настройки сортировки по умолчанию
    void getDefaultSortSettings(int &sortColumn, Qt::SortOrder &sortOrder) const;

    // Нормализовать путь (убирает слеши в конце и т.д.)
    QString normalizePath(const QString &path) const;

    // Удалить настройки для несуществующих папок
    void cleanupNonExistentFolders();

private:
    QSettings *settings;
    mutable QMap<QString, int> viewModeCache; // Кэш для быстрого доступа к режимам просмотра
    mutable QMap<QString, QPair<int, Qt::SortOrder>> sortCache; // Кэш для настроек сортировки

    // Ключ для настроек
    QString getSettingsKey(const QString &folderPath, const QString &settingType) const;
};