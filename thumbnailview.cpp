#include "thumbnailview.h"
#include "thumbnaildelegate.h"
#include "styles.h"
#include <QResizeEvent>
#include <QPainter>
#include <QFuture>
#include <QtConcurrent>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QSvgRenderer>
#include <QApplication>
#include <QDir>
#include <QDebug>
#include <QScrollBar>
#include <QImageReader>
#include <QFile>
#include <QBuffer>
#include <QWheelEvent>
#include <QSettings>

// Windows includes
#include <windows.h>
#include <wincodec.h>
#pragma comment(lib, "windowscodecs.lib")

// Поддерживаемые форматы изображений
static const QStringList SUPPORTED_IMAGE_FORMATS = {
    "png", "jpg", "jpeg", "bmp", "gif", "tiff", "tif",
    "webp", "ico", "svg"
};

ThumbnailView::ThumbnailView(QWidget *parent)
    : QListView(parent)
    , delegate(new ThumbnailDelegate(this, this))
{
    setViewMode(QListView::IconMode);
    setResizeMode(QListView::Adjust);
    setMovement(QListView::Static);
    setWordWrap(true);
    setSelectionMode(QAbstractItemView::ExtendedSelection);

    // Устанавливаем начальный размер сетки
    updateGridSize();

    // Оптимизации для производительности
    setAttribute(Qt::WA_OpaquePaintEvent);

    // Устанавливаем кастомный делегат
    setItemDelegate(delegate);

    // Увеличиваем лимит кэша pixmap для больших изображений
    QPixmapCache::setCacheLimit(51200); // 50MB вместо стандартных 10MB

    // Таймер для отложенной загрузки миниатюр
    loadTimer = new QTimer(this);
    loadTimer->setSingleShot(true);
    loadTimer->setInterval(Styles::LoadThumbnailsDelay);
    connect(loadTimer, &QTimer::timeout, this, &ThumbnailView::loadVisibleThumbnails);

    // Таймер для обработки очереди миниатюр
    queueTimer = new QTimer(this);
    queueTimer->setInterval(100); // Быстрая обработка очереди
    connect(queueTimer, &QTimer::timeout, this, &ThumbnailView::processThumbnailQueue);

    // Подключаем скроллинг для динамической загрузки
    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, &ThumbnailView::onScroll);

    // Диагностика доступных форматов
    qDebug() << "Available image formats:" << QImageReader::supportedImageFormats();
}

ThumbnailView::~ThumbnailView()
{
    if (loadTimer->isActive()) {
        loadTimer->stop();
    }
    if (queueTimer->isActive()) {
        queueTimer->stop();
    }

    // Останавливаем все активные watchers
    for (auto watcher : activeWatchers) {
        if (watcher && !watcher->isFinished()) {
            watcher->waitForFinished();
        }
        watcher->deleteLater();
    }
    activeWatchers.clear();

    delete delegate;
}

void ThumbnailView::resizeEvent(QResizeEvent *event)
{
    QListView::resizeEvent(event);
    updateGridSize();
}

void ThumbnailView::showEvent(QShowEvent *event)
{
    QListView::showEvent(event);
    // Загружаем миниатюры при первом показе
    if (!initialLoadDone) {
        loadTimer->start();
        queueTimer->start(); // Запускаем обработчик очереди
        initialLoadDone = true;
    }
}

void ThumbnailView::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        // Изменяем масштаб при удержании Ctrl + прокрутка
        QPoint numDegrees = event->angleDelta() / 8;

        if (!numDegrees.isNull()) {
            double delta = numDegrees.y() > 0 ? SCALE_STEP : -SCALE_STEP;
            double newScaleFactor = thumbnailScaleFactor + delta;

            // Ограничиваем масштаб
            newScaleFactor = qMax(MIN_SCALE_FACTOR, qMin(MAX_SCALE_FACTOR, newScaleFactor));

            if (newScaleFactor != thumbnailScaleFactor) {
                setThumbnailScaleFactor(newScaleFactor);
            }
        }

        event->accept();
        return;
    }

    QListView::wheelEvent(event);
}

void ThumbnailView::setThumbnailScaleFactor(double factor)
{
    if (qFuzzyCompare(thumbnailScaleFactor, factor)) {
        return;
    }

    thumbnailScaleFactor = factor;

    // Сохраняем настройки для текущей папки
    saveThumbnailScaleFactor();

    // Обновляем размеры
    updateGridSize();

    // Обновляем отображение
    viewport()->update();

    qDebug() << "Thumbnail scale factor changed to:" << thumbnailScaleFactor;
}

void ThumbnailView::updateGridSize()
{
    // Базовые размеры для миниатюры
    int baseThumbWidth = Styles::ThumbnailWidth;
    int baseThumbHeight = Styles::ThumbnailHeight;

    // Масштабированные размеры миниатюры
    thumbnailSize = QSize(
        static_cast<int>(baseThumbWidth * thumbnailScaleFactor),
        static_cast<int>(baseThumbHeight * thumbnailScaleFactor)
    );

    // Рассчитываем размер сетки с гарантированными минимальными размерами
    int gridWidth = qMax(thumbnailSize.width() + 2 * Styles::ThumbnailDelegatePadding, 120);
    int gridHeight = qMax(thumbnailSize.height() + Styles::ThumbnailDelegateTextHeight + 3 * Styles::ThumbnailDelegateMargin, 150);

    setGridSize(QSize(gridWidth, gridHeight));

    // Обновляем размер иконок
    int baseIconWidth = Styles::ThumbnailIconWidth;
    int baseIconHeight = Styles::ThumbnailIconHeight;
    setIconSize(QSize(
        static_cast<int>(baseIconWidth * thumbnailScaleFactor),
        static_cast<int>(baseIconHeight * thumbnailScaleFactor)
    ));

    // Обновляем отступы с учетом масштаба, но не меньше минимальных
    setSpacing(qMax(static_cast<int>(Styles::ThumbnailSpacing * thumbnailScaleFactor), 5));
}

void ThumbnailView::onDirectoryLoaded(const QString &path)
{
    // Сохраняем текущий путь
    currentPath = path;

    // Загружаем масштаб для этой папки
    loadThumbnailScaleFactor();

    // НЕ очищаем глобальный кэш при смене директории - он общий для всех директорий
    pendingThumbnails.clear();
    thumbnailQueue.clear();

    // Останавливаем все активные watchers
    for (auto watcher : activeWatchers) {
        if (watcher && !watcher->isFinished()) {
            watcher->cancel();
        }
        watcher->deleteLater();
    }
    activeWatchers.clear();

    // Обновляем размеры с новым масштабом
    updateGridSize();

    if (isVisible()) {
        loadTimer->start();
        if (!queueTimer->isActive()) {
            queueTimer->start();
        }
    }
}

void ThumbnailView::onScroll()
{
    // При скроллинге загружаем видимые миниатюры
    if (isVisible()) {
        loadTimer->start(Styles::ScrollLoadDelay);
    }
}

void ThumbnailView::addToQueue(const QStringList& files)
{
    for (const QString& file : files) {
        if (!thumbnailQueue.contains(file) && !pendingThumbnails.contains(file)) {
            thumbnailQueue.enqueue(file);
            pendingThumbnails.insert(file);
        }
    }
}

void ThumbnailView::processThumbnailQueue()
{
    // Если уже есть максимальное количество активных загрузок, ждем
    if (activeWatchers.size() >= MAX_CONCURRENT_LOAD) {
        return;
    }

    // Ограничиваем общее количество элементов в очереди для экономии памяти
    if (thumbnailQueue.size() > 50) {
        // Очищаем старые элементы из очереди если их слишком много
        while (thumbnailQueue.size() > 25) {
            QString filePath = thumbnailQueue.dequeue();
            pendingThumbnails.remove(filePath);
        }
    }

    // Берем следующую пачку из очереди
    if (thumbnailQueue.isEmpty()) {
        return;
    }

    QStringList filesToLoad;
    for (int i = 0; i < BATCH_SIZE && !thumbnailQueue.isEmpty(); ++i) {
        QString filePath = thumbnailQueue.dequeue();
        // Проверяем, нет ли уже миниатюры в кэше
        if (!thumbnailCache.hasThumbnail(filePath)) {
            filesToLoad.append(filePath);
        } else {
            pendingThumbnails.remove(filePath);
        }
    }

    if (!filesToLoad.isEmpty()) {
        // Загружаем миниатюры асинхронно с обработкой исключений
        QFuture<QPair<QString, QPixmap>> future = QtConcurrent::mapped(
            filesToLoad,
            [this](const QString& filePath) -> QPair<QString, QPixmap> {
                try {
                    return qMakePair(filePath, generateThumbnail(filePath, thumbnailSize));
                }
                catch (...) {
                    return qMakePair(filePath, QPixmap());
                }
            }
        );

        // Создаем новый watcher для этой партии
        QFutureWatcher<QPair<QString, QPixmap>>* watcher = new QFutureWatcher<QPair<QString, QPixmap>>(this);
        connect(watcher, &QFutureWatcher<QPair<QString, QPixmap>>::finished, this,
            [this, watcher]() {
                this->onThumbnailBatchGenerated(watcher);
            });

        watcher->setFuture(future);
        activeWatchers.append(watcher);
    }
}

void ThumbnailView::loadVisibleThumbnails()
{
    QFileSystemModel *fsModel = qobject_cast<QFileSystemModel*>(model());
    if (!fsModel) {
        qDebug() << "No filesystem model";
        return;
    }

    QModelIndex rootIdx = rootIndex();
    int rowCount = fsModel->rowCount(rootIdx);
    if (rowCount <= 0) return;

    // Получаем видимую область
    QRect viewportRect = viewport()->rect();
    QModelIndex firstVisible = indexAt(viewportRect.topLeft());
    QModelIndex lastVisible = indexAt(viewportRect.bottomRight());

    int startRow = 0;
    int endRow = rowCount - 1;

    // Если есть видимые элементы, загружаем только их + запас
    if (firstVisible.isValid() && lastVisible.isValid()) {
        startRow = qMax(0, firstVisible.row() - 5);
        endRow = qMin(rowCount - 1, lastVisible.row() + 5);
    }

    // Собираем файлы для загрузки
    QStringList filesToLoad;
    for (int row = startRow; row <= endRow; ++row) {
        QModelIndex index = fsModel->index(row, 0, rootIdx);
        QString filePath = fsModel->filePath(index);

        if (isImageFile(filePath) && !thumbnailCache.hasThumbnail(filePath) && !pendingThumbnails.contains(filePath)) {
            filesToLoad.append(filePath);
        }
    }

    // Добавляем в очередь вместо немедленной загрузки
    if (!filesToLoad.isEmpty()) {
        addToQueue(filesToLoad);
    }
}

void ThumbnailView::loadThumbnails()
{
    QFileSystemModel *fsModel = qobject_cast<QFileSystemModel*>(model());
    if (!fsModel) return;

    QModelIndex rootIdx = rootIndex();
    int rowCount = fsModel->rowCount(rootIdx);

    // Собираем все файлы для загрузки
    QStringList filesToLoad;
    for (int row = 0; row < rowCount; ++row) {
        QModelIndex index = fsModel->index(row, 0, rootIdx);
        QString filePath = fsModel->filePath(index);

        if (isImageFile(filePath) && !thumbnailCache.hasThumbnail(filePath) && !pendingThumbnails.contains(filePath)) {
            filesToLoad.append(filePath);
        }
    }

    // Добавляем в очередь
    if (!filesToLoad.isEmpty()) {
        addToQueue(filesToLoad);
    }
}

QString ThumbnailView::checkFileSignature(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return "Cannot open file";
    }

    QByteArray header = file.read(8);
    file.close();

    if (header.size() < 2) return "File too small";

    QString signature;

    // Проверяем JPEG сигнатуру
    if (header.startsWith("\xFF\xD8")) {
        signature = "JPEG (FF D8)";
        if (header.size() >= 4) {
            if (header.startsWith("\xFF\xD8\xFF\xE0")) signature = "JPEG (JFIF)";
            else if (header.startsWith("\xFF\xD8\xFF\xE1")) signature = "JPEG (Exif)";
        }
    } else if (header.startsWith("\x89PNG")) {
        signature = "PNG";
    } else if (header.startsWith("BM")) {
        signature = "BMP";
    } else if (header.startsWith("GIF8")) {
        signature = "GIF";
    } else {
        signature = "Unknown";
    }

    return signature;
}

QPixmap ThumbnailView::loadJPEGViaWIC(const QString& filePath, const QSize& size)
{
    QPixmap result;

    // Инициализация COM
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) {
        qDebug() << "COM initialization failed:" << hr;
        return result;
    }

    IWICImagingFactory* pFactory = NULL;
    IWICBitmapDecoder* pDecoder = NULL;
    IWICBitmapFrameDecode* pFrame = NULL;
    IWICFormatConverter* pConverter = NULL;

    // Создаем фабрику WIC
    hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&pFactory)
    );

    if (SUCCEEDED(hr)) {
        // Конвертируем QString в wchar_t*
        std::wstring filePathW = filePath.toStdWString();

        // Создаем декодер для файла
        hr = pFactory->CreateDecoderFromFilename(
            filePathW.c_str(),
            NULL,
            GENERIC_READ,
            WICDecodeMetadataCacheOnLoad,
            &pDecoder
        );

        if (SUCCEEDED(hr)) {
            // Получаем первый кадр (для JPEG всегда один кадр)
            hr = pDecoder->GetFrame(0, &pFrame);

            if (SUCCEEDED(hr)) {
                // Создаем конвертер формата
                hr = pFactory->CreateFormatConverter(&pConverter);

                if (SUCCEEDED(hr)) {
                    // Инициализируем конвертер в формат 32bpp PBGRA (совместимый с QImage)
                    hr = pConverter->Initialize(
                        pFrame,
                        GUID_WICPixelFormat32bppPBGRA,
                        WICBitmapDitherTypeNone,
                        NULL,
                        0.0,
                        WICBitmapPaletteTypeCustom
                    );

                    if (SUCCEEDED(hr)) {
                        // Получаем размеры изображения
                        UINT width = 0, height = 0;
                        pConverter->GetSize(&width, &height);

                        if (width > 0 && height > 0) {
                            // Создаем QImage для хранения данных
                            QImage image(width, height, QImage::Format_ARGB32);

                            // Копируем пиксели в QImage
                            hr = pConverter->CopyPixels(
                                NULL,
                                width * 4, // stride
                                width * height * 4, // buffer size
                                reinterpret_cast<BYTE*>(image.bits())
                            );

                            if (SUCCEEDED(hr)) {
                                // Масштабируем до нужного размера
                                result = QPixmap::fromImage(image.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                            } else {
                                qDebug() << "WIC CopyPixels failed:" << hr;
                            }
                        }
                    } else {
                        qDebug() << "WIC format converter initialization failed:" << hr;
                    }
                }
            } else {
                qDebug() << "WIC GetFrame failed:" << hr;
            }
        } else {
            qDebug() << "WIC CreateDecoderFromFilename failed:" << hr;
        }
    } else {
        qDebug() << "WIC CoCreateInstance failed:" << hr;
    }

    // Освобождение ресурсов
    if (pConverter) pConverter->Release();
    if (pFrame) pFrame->Release();
    if (pDecoder) pDecoder->Release();
    if (pFactory) pFactory->Release();

    CoUninitialize();

    return result;
}

QPixmap ThumbnailView::generateThumbnail(const QString& filePath, const QSize& size)
{
    // Сначала проверяем кэш
    QPixmap cachedPixmap = thumbnailCache.getThumbnail(filePath);
    if (!cachedPixmap.isNull()) {
        // Если есть в кэше, масштабируем до нужного размера
        return cachedPixmap.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    QPixmap pixmap;

    try {
        // Для SVG файлов
        if (filePath.toLower().endsWith(".svg")) {
            QSvgRenderer renderer(filePath);
            if (renderer.isValid()) {
                QPixmap svgPixmap(size);
                svgPixmap.fill(Qt::transparent);
                QPainter painter(&svgPixmap);
                renderer.render(&painter);
                pixmap = svgPixmap;
            }
        }
        // Для JPEG файлов используем WIC
        else if (filePath.toLower().endsWith(".jpg") || filePath.toLower().endsWith(".jpeg")) {
            pixmap = loadJPEGViaWIC(filePath, size);

            if (pixmap.isNull()) {
                // Резервный метод через Qt
                QImageReader reader(filePath);
                if (reader.canRead()) {
                    QImage image = reader.read();
                    if (!image.isNull()) {
                        pixmap = QPixmap::fromImage(image.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                    }
                }
            }
        }
        // Для остальных форматов используем стандартные методы Qt
        else {
            QImageReader reader(filePath);
            if (reader.canRead()) {
                QSize imageSize = reader.size();
                if (imageSize.isValid()) {
                    // Ограничиваем размер загружаемого изображения для экономии памяти
                    QSize loadSize = imageSize;
                    if (loadSize.width() > 1024 || loadSize.height() > 1024) {
                        loadSize.scale(1024, 1024, Qt::KeepAspectRatio);
                        reader.setScaledSize(loadSize);
                    }

                    QImage image = reader.read();
                    if (!image.isNull()) {
                        pixmap = QPixmap::fromImage(image.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                    }
                }
            }

            // Резервный метод с ограничением памяти
            if (pixmap.isNull()) {
                QImage image(filePath);
                if (!image.isNull()) {
                    // Ограничиваем размер для экономии памяти
                    if (image.width() > 800 || image.height() > 800) {
                        image = image.scaled(800, 800, Qt::KeepAspectRatio, Qt::FastTransformation);
                    }
                    pixmap = QPixmap::fromImage(image.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                }
            }
        }
    }
    catch (const std::exception& e) {
        qDebug() << "Exception while generating thumbnail for" << filePath << ":" << e.what();
    }
    catch (...) {
        qDebug() << "Unknown exception while generating thumbnail for" << filePath;
    }

    // Финальная заглушка если все методы не сработали
    if (pixmap.isNull()) {
        QImage placeholder(size, QImage::Format_RGB32);
        placeholder.fill(QColor(200, 200, 200));

        QPainter painter(&placeholder);
        painter.setPen(Qt::darkGray);
        QFont font = painter.font();
        font.setPointSize(8);
        painter.setFont(font);
        painter.drawText(placeholder.rect(), Qt::AlignCenter, "No preview\n" + QFileInfo(filePath).suffix().toUpper());

        pixmap = QPixmap::fromImage(placeholder);
    }

    // Сохраняем в кэш оригинальный размер (максимальный для качественного масштабирования)
    if (!pixmap.isNull()) {
        // Сохраняем в максимальном размере для последующего качественного масштабирования
        QSize maxSize(512, 512);
        QPixmap originalPixmap = pixmap;
        if (pixmap.width() > maxSize.width() || pixmap.height() > maxSize.height()) {
            originalPixmap = pixmap.scaled(maxSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        thumbnailCache.storeThumbnail(filePath, originalPixmap);

        // Возвращаем версию нужного размера
        return pixmap;
    }

    return QPixmap();
}

bool ThumbnailView::isImageFile(const QString& filePath)
{
    QFileInfo fileInfo(filePath);
    if (!fileInfo.isFile()) return false;

    QString suffix = fileInfo.suffix().toLower();
    return SUPPORTED_IMAGE_FORMATS.contains(suffix);
}

void ThumbnailView::onThumbnailBatchGenerated(QFutureWatcher<QPair<QString, QPixmap>>* watcher)
{
    if (watcher->isFinished()) {
        QFuture<QPair<QString, QPixmap>> future = watcher->future();

        bool hasUpdates = false;
        for (const auto& result : future.results()) {
            const QString& filePath = result.first;
            const QPixmap& pixmap = result.second;

            if (!pixmap.isNull()) {
                // Миниатюра уже сохранена в кэш в generateThumbnail
                hasUpdates = true;
            }
            // Удаляем из ожидающих в любом случае
            pendingThumbnails.remove(filePath);
        }

        // Обновляем только если есть новые миниатюры
        if (hasUpdates) {
            viewport()->update();
        }

        // Очищаем watcher
        activeWatchers.removeAll(watcher);
        watcher->deleteLater();
    }
}

void ThumbnailView::saveThumbnailScaleFactor()
{
    if (currentPath.isEmpty()) return;

    QSettings settings;
    // Сохраняем масштаб для текущей папки
    QString key = "thumbnailScaleFactor/" + currentPath;
    settings.setValue(key, thumbnailScaleFactor);
}

void ThumbnailView::loadThumbnailScaleFactor()
{
    if (currentPath.isEmpty()) {
        thumbnailScaleFactor = 1.0;
        return;
    }

    QSettings settings;
    // Загружаем масштаб для текущей папки
    QString key = "thumbnailScaleFactor/" + currentPath;
    thumbnailScaleFactor = settings.value(key, 1.0).toDouble();

    // Ограничиваем значение в допустимых пределах
    thumbnailScaleFactor = qMax(MIN_SCALE_FACTOR, qMin(MAX_SCALE_FACTOR, thumbnailScaleFactor));
}