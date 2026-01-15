#pragma once

#include <QString>
#include <QStorageInfo>

class DiskSizeUtils
{
public:
    static QString formatSize(qint64 bytes);
    static QString getDiskSizeInfo(const QString &path);
    static QString getDiskSizeInfoCompact(const QString &path);
    static QString getDiskSizeInfoForThumbnail(const QString &path);
};