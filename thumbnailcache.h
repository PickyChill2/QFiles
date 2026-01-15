// thumbnailcache.h
#pragma once

#include <QObject>
#include <QCache>
#include <QMap>
#include <QString>
#include <QPixmap>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QStandardPaths>

class ThumbnailCache : public QObject
{
    Q_OBJECT

public:
    static ThumbnailCache& instance();
    
    bool hasThumbnail(const QString& filePath) const;
    QPixmap getThumbnail(const QString& filePath) const;
    void storeThumbnail(const QString& filePath, const QPixmap& thumbnail);
    void removeThumbnail(const QString& filePath);
    void clearExpiredThumbnails(int maxAgeDays = 30);
    void clearCache();

    QString getCachePath() const { return cacheDir.path(); }

private:
    ThumbnailCache();
    ~ThumbnailCache() = default;
    
    QString getThumbnailPath(const QString& filePath) const;
    QString generateThumbnailKey(const QString& filePath) const;
    bool isThumbnailValid(const QString& thumbnailPath, const QString& originalFilePath) const;

    QDir cacheDir;
    mutable QCache<QString, QPixmap> memoryCache;
};