#pragma once

#include <QListView>
#include <QFileSystemModel>
#include <QFutureWatcher>
#include <QPixmapCache>
#include <QHash>
#include <QSvgRenderer>
#include <QTimer>
#include <QSet>
#include <QQueue>
#include <QSettings>
#include "thumbnailcache.h"

class ThumbnailDelegate;

class ThumbnailView : public QListView
{
    Q_OBJECT

public:
    explicit ThumbnailView(QWidget *parent = nullptr);
    ~ThumbnailView();

    QPixmap getThumbnail(const QString& filePath) const {
        return thumbnailCache.getThumbnail(filePath);
    }

    void setThumbnailScaleFactor(double factor);
    double getThumbnailScaleFactor() const { return thumbnailScaleFactor; }
    QSize getThumbnailSize() const { return thumbnailSize; }
    QSize getGridSize() const { return gridSize(); }

    ThumbnailDelegate* getDelegate() const { return delegate; }

public slots:
    void onDirectoryLoaded(const QString &path);
    void loadThumbnails();
    void loadVisibleThumbnails();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private slots:
    void onScroll();
    void onThumbnailBatchGenerated(QFutureWatcher<QPair<QString, QPixmap>>* watcher);
    void processThumbnailQueue();

private:
    void updateGridSize();
    QPixmap generateThumbnail(const QString& filePath, const QSize& size);
    bool isImageFile(const QString& filePath);
    void addToQueue(const QStringList& files);
    QPixmap loadJPEGViaWIC(const QString& filePath, const QSize& size);
    QString checkFileSignature(const QString& filePath);
    void saveThumbnailScaleFactor();
    void loadThumbnailScaleFactor();

    QSize thumbnailSize;
    ThumbnailCache &thumbnailCache = ThumbnailCache::instance();
    QSet<QString> pendingThumbnails;
    QList<QFutureWatcher<QPair<QString, QPixmap>>*> activeWatchers;
    ThumbnailDelegate *delegate;
    QTimer *loadTimer;
    QTimer *queueTimer;
    QQueue<QString> thumbnailQueue;
    QString currentPath;
    bool initialLoadDone = false;
    const int MAX_CONCURRENT_LOAD = 3;
    const int BATCH_SIZE = 7;

    // Масштабирование миниатюр
    double thumbnailScaleFactor = 1.0;
    const double MIN_SCALE_FACTOR = 0.5;
    const double MAX_SCALE_FACTOR = 3;
    const double SCALE_STEP = 0.1;

    const int MAX_QUEUE_SIZE = 100;
    const int MAX_IMAGE_DIMENSION = 2048;
    const int MAX_CACHE_SIZE_MB = 100;
};