#include "disksizeutils.h"
#include <QFileInfo>
#include <QDir>
#include <QDebug>

QString DiskSizeUtils::formatSize(qint64 bytes)
{
    const qint64 KB = 1024;
    const qint64 MB = KB * 1024;
    const qint64 GB = MB * 1024;
    const qint64 TB = GB * 1024;

    if (bytes >= TB)
        return QString("%1 ТБ").arg(QString::number(bytes / (double)TB, 'f', 1));
    else if (bytes >= GB)
        return QString("%1 ГБ").arg(QString::number(bytes / (double)GB, 'f', 1));
    else if (bytes >= MB)
        return QString("%1 МБ").arg(QString::number(bytes / (double)MB, 'f', 1));
    else if (bytes >= KB)
        return QString("%1 КБ").arg(QString::number(bytes / (double)KB, 'f', 1));
    else
        return QString("%1 Б").arg(bytes);
}

QString DiskSizeUtils::getDiskSizeInfo(const QString &path)
{
    QFileInfo fileInfo(path);
    if (!fileInfo.isDir() || !QDir(path).isRoot()) {
        return QString();
    }

    QStorageInfo storage(path);
    if (!storage.isValid() || !storage.isReady()) {
        return QString();
    }

    qint64 total = storage.bytesTotal();
    qint64 free = storage.bytesFree();
    qint64 used = total - free;

    QString totalStr = formatSize(total);
    QString freeStr = formatSize(free);

    return QString("%1\\%2 (всего %3, свободно %4)")
        .arg(totalStr)
        .arg(freeStr)
        .arg(totalStr)
        .arg(freeStr);
}

QString DiskSizeUtils::getDiskSizeInfoCompact(const QString &path)
{
    QFileInfo fileInfo(path);
    if (!fileInfo.isDir() || !QDir(path).isRoot()) {
        return QString();
    }

    QStorageInfo storage(path);
    if (!storage.isValid() || !storage.isReady()) {
        return QString();
    }

    qint64 total = storage.bytesTotal();
    qint64 free = storage.bytesFree();

    QString totalStr = formatSize(total);
    QString freeStr = formatSize(free);

    // Формат: 100ГБ\50ГБ
    return QString("%1\\%2").arg(totalStr).arg(freeStr);
}

QString DiskSizeUtils::getDiskSizeInfoForThumbnail(const QString &path)
{
    QFileInfo fileInfo(path);
    if (!fileInfo.isDir() || !QDir(path).isRoot()) {
        return QString();
    }

    QStorageInfo storage(path);
    if (!storage.isValid() || !storage.isReady()) {
        return QString();
    }

    qint64 total = storage.bytesTotal();
    qint64 free = storage.bytesFree();

    QString totalStr = formatSize(total);
    QString freeStr = formatSize(free);

    return QString("%1\\%2").arg(totalStr).arg(freeStr);
}