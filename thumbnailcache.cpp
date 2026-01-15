#include "thumbnailcache.h"
#include <QCryptographicHash>
#include <QDebug>

ThumbnailCache& ThumbnailCache::instance()
{
    static ThumbnailCache instance;
    return instance;
}

ThumbnailCache::ThumbnailCache()
    : memoryCache(1000) // Кэш на 1000 миниатюр в памяти
{
    // Определяем путь для кэша
    QString cachePath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (cachePath.isEmpty()) {
        cachePath = QDir::tempPath() + "/QFiles/thumbnails";
    } else {
        cachePath += "/thumbnails";
    }
    
    cacheDir.setPath(cachePath);
    if (!cacheDir.exists()) {
        cacheDir.mkpath(".");
    }
    
    qDebug() << "Thumbnail cache directory:" << cacheDir.absolutePath();
    
    // Очищаем устаревшие миниатюры при запуске
    clearExpiredThumbnails();
}

QString ThumbnailCache::generateThumbnailKey(const QString& filePath) const
{
    QFileInfo fileInfo(filePath);
    QString keyData = filePath + "_" + 
                     QString::number(fileInfo.lastModified().toSecsSinceEpoch()) + "_" +
                     QString::number(fileInfo.size());
    
    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(keyData.toUtf8());
    return hash.result().toHex();
}

QString ThumbnailCache::getThumbnailPath(const QString& filePath) const
{
    QString key = generateThumbnailKey(filePath);
    return cacheDir.filePath(key + ".png");
}

bool ThumbnailCache::isThumbnailValid(const QString& thumbnailPath, const QString& originalFilePath) const
{
    QFileInfo thumbnailInfo(thumbnailPath);
    QFileInfo originalInfo(originalFilePath);
    
    if (!thumbnailInfo.exists() || !originalInfo.exists()) {
        return false;
    }
    
    // Проверяем, что оригинальный файл не изменился
    return thumbnailInfo.lastModified() >= originalInfo.lastModified();
}

bool ThumbnailCache::hasThumbnail(const QString& filePath) const
{
    // Проверяем в памяти
    if (memoryCache.contains(filePath)) {
        return true;
    }
    
    // Проверяем на диске
    QString thumbnailPath = getThumbnailPath(filePath);
    return isThumbnailValid(thumbnailPath, filePath);
}

QPixmap ThumbnailCache::getThumbnail(const QString& filePath) const
{
    // Сначала проверяем в памяти
    if (memoryCache.contains(filePath)) {
        return *memoryCache.object(filePath);
    }
    
    // Затем проверяем на диске
    QString thumbnailPath = getThumbnailPath(filePath);
    if (isThumbnailValid(thumbnailPath, filePath)) {
        QPixmap thumbnail;
        if (thumbnail.load(thumbnailPath)) {
            // Сохраняем в памяти для будущих запросов
            ThumbnailCache* nonConstThis = const_cast<ThumbnailCache*>(this);
            nonConstThis->memoryCache.insert(filePath, new QPixmap(thumbnail));
            return thumbnail;
        }
    }
    
    return QPixmap(); // Пустая миниатюра
}

void ThumbnailCache::storeThumbnail(const QString& filePath, const QPixmap& thumbnail)
{
    if (thumbnail.isNull()) {
        return;
    }
    
    // Сохраняем в памяти
    memoryCache.insert(filePath, new QPixmap(thumbnail));
    
    // Сохраняем на диск
    QString thumbnailPath = getThumbnailPath(filePath);
    if (!thumbnail.save(thumbnailPath, "PNG")) {
        qDebug() << "Failed to save thumbnail to:" << thumbnailPath;
    }
}

void ThumbnailCache::removeThumbnail(const QString& filePath)
{
    memoryCache.remove(filePath);
    
    QString thumbnailPath = getThumbnailPath(filePath);
    if (QFile::exists(thumbnailPath)) {
        QFile::remove(thumbnailPath);
    }
}

void ThumbnailCache::clearExpiredThumbnails(int maxAgeDays)
{
    QDateTime cutoff = QDateTime::currentDateTime().addDays(-maxAgeDays);
    
    QStringList filters;
    filters << "*.png";
    QFileInfoList thumbnails = cacheDir.entryInfoList(filters, QDir::Files);
    
    int removedCount = 0;
    for (const QFileInfo& thumbnailInfo : thumbnails) {
        if (thumbnailInfo.lastModified() < cutoff) {
            if (QFile::remove(thumbnailInfo.absoluteFilePath())) {
                removedCount++;
            }
        }
    }
    
    if (removedCount > 0) {
        qDebug() << "Cleared" << removedCount << "expired thumbnails";
    }
}

void ThumbnailCache::clearCache()
{
    memoryCache.clear();
    
    QStringList filters;
    filters << "*.png";
    QFileInfoList thumbnails = cacheDir.entryInfoList(filters, QDir::Files);
    
    for (const QFileInfo& thumbnailInfo : thumbnails) {
        QFile::remove(thumbnailInfo.absoluteFilePath());
    }
    
    qDebug() << "Thumbnail cache cleared";
}