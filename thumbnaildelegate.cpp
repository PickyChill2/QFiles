#include "thumbnaildelegate.h"
#include "thumbnailview.h"
#include "styles.h"
#include "disksizeutils.h"
#include <QPainter>
//#include <QFileSystemModel>
#include <QApplication>
#include <QDebug>
#include "customfilesystemmodel.h"

static const QStringList SUPPORTED_IMAGE_FORMATS = {
    "png", "jpg", "jpeg", "bmp", "gif", "tiff", "tif", "webp", "ico", "svg"
};

ThumbnailDelegate::ThumbnailDelegate(ThumbnailView *thumbnailView, QObject *parent)
    : QStyledItemDelegate(parent)
    , m_thumbnailView(thumbnailView)
{
}

void ThumbnailDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                             const QModelIndex &index) const
{
    painter->save();

    QRect rect = option.rect;

    // Получаем модель и путь к файлу
    CustomFileSystemModel *fsModel = qobject_cast<CustomFileSystemModel*>(const_cast<QAbstractItemModel*>(index.model()));
    if (!fsModel) {
        QStyledItemDelegate::paint(painter, option, index);
        painter->restore();
        return;
    }

    QString filePath = fsModel->filePath(index);
    QFileInfo fileInfo(filePath);
    QIcon icon = fsModel->fileIcon(index);

    // Получаем размер миниатюры из ThumbnailView с учетом масштаба
    QSize thumbSize = m_thumbnailView->getThumbnailSize();

    // Определяем, является ли элемент папкой или файлом без миниатюры
    bool isDirectory = fileInfo.isDir();
    bool isImageWithThumbnail = false;
    bool isDisk = isDirectory && QDir(filePath).isRoot(); // Проверяем, является ли путь корнем (диском)

    QPixmap thumbnail;
    if (m_thumbnailView && fileInfo.isFile() && isImageFile(filePath)) {
        thumbnail = m_thumbnailView->getThumbnail(filePath);
        isImageWithThumbnail = !thumbnail.isNull();
    }

    // Для папок и файлов без миниатюр используем уменьшенный размер
    QSize displayThumbSize = (isDirectory || !isImageWithThumbnail) ? thumbSize / 2 : thumbSize;

    // Рассчитываем область для миниатюры с центрированием по вертикали и горизонтали
    int thumbX = rect.x() + (rect.width() - displayThumbSize.width()) / 2;
    int thumbY;

    if (isDisk) {
        // Для дисков делаем отступ меньше, т.к. будет две строки текста
        thumbY = rect.y() + (rect.height() - displayThumbSize.height() - 2 * Styles::ThumbnailDelegateTextHeight - Styles::ThumbnailDelegateMargin) / 2;
    } else {
        thumbY = rect.y() + (rect.height() - displayThumbSize.height() - Styles::ThumbnailDelegateTextHeight - Styles::ThumbnailDelegateMargin) / 2;
    }

    QRect thumbRect(thumbX, thumbY, displayThumbSize.width(), displayThumbSize.height());

    // Область для текста
    int textHeight = Styles::ThumbnailDelegateTextHeight;
    int textY = thumbRect.bottom() + Styles::ThumbnailDelegateMargin;
    int textWidth = rect.width() - 2 * Styles::ThumbnailDelegateMargin;
    int textX = rect.x() + Styles::ThumbnailDelegateMargin;

    QRect textRect(textX, textY, textWidth, textHeight);

    // Рисуем фон для всего элемента (включая миниатюру и текст)
    if (option.state & QStyle::State_Selected) {
        painter->fillRect(rect, option.palette.highlight());
    } else {
        painter->fillRect(rect, option.palette.window());
    }

    // Рисуем миниатюру или иконку
    if (isImageWithThumbnail) {
        // Масштабируем миниатюру до нужного размера с сохранением пропорций
        QSize scaledSize = thumbnail.size();
        scaledSize.scale(displayThumbSize, Qt::KeepAspectRatio);

        QRect centeredRect(thumbRect.x() + (thumbRect.width() - scaledSize.width()) / 2,
                          thumbRect.y() + (thumbRect.height() - scaledSize.height()) / 2,
                          scaledSize.width(),
                          scaledSize.height());

        // Рисуем миниатюру с плавным преобразованием
        painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter->drawPixmap(centeredRect, thumbnail);
    } else {
        // Для папок и файлов без миниатюр используем уменьшенную иконку
        QPixmap iconPixmap = icon.pixmap(displayThumbSize);
        iconPixmap = iconPixmap.scaled(displayThumbSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        QRect centeredRect(thumbRect.x() + (thumbRect.width() - iconPixmap.width()) / 2,
                          thumbRect.y() + (thumbRect.height() - iconPixmap.height()) / 2,
                          iconPixmap.width(),
                          iconPixmap.height());
        painter->drawPixmap(centeredRect, iconPixmap);
    }

    // Рисуем текст (имя файла)
    painter->setPen(option.state & QStyle::State_Selected ?
                   option.palette.highlightedText().color() :
                   option.palette.text().color());

    QString fileName = fsModel->fileName(index);
    QFontMetrics metrics(painter->font());

    if (isDisk) {
        // Для дисков отображаем две строки
        QString diskName = fileName;
        QString diskSize = DiskSizeUtils::getDiskSizeInfoForThumbnail(filePath);

        if (!diskSize.isEmpty()) {
            // Первая строка: имя диска
            QRect nameRect = textRect;
            nameRect.setHeight(textHeight);
            QString elidedName = metrics.elidedText(diskName, Qt::ElideMiddle, nameRect.width());
            painter->drawText(nameRect, Qt::AlignCenter, elidedName);

            // Вторая строка: размер диска
            QRect sizeRect = textRect;
            sizeRect.setTop(nameRect.bottom());
            sizeRect.setHeight(textHeight);
            QString elidedSize = metrics.elidedText(diskSize, Qt::ElideMiddle, sizeRect.width());
            painter->drawText(sizeRect, Qt::AlignCenter, elidedSize);
        } else {
            QString elidedText = metrics.elidedText(fileName, Qt::ElideMiddle, textRect.width());
            painter->drawText(textRect, Qt::AlignCenter, elidedText);
        }
    } else {
        // Для обычных файлов и папок - одна строка
        QString elidedText = metrics.elidedText(fileName, Qt::ElideMiddle, textRect.width());
        painter->drawText(textRect, Qt::AlignCenter, elidedText);
    }

    painter->restore();
}

QSize ThumbnailDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)
    // Используем текущий размер сетки из ThumbnailView
    return m_thumbnailView->getGridSize();
}

bool ThumbnailDelegate::isImageFile(const QString& filePath) const
{
    QFileInfo fileInfo(filePath);
    QString suffix = fileInfo.suffix().toLower();
    return SUPPORTED_IMAGE_FORMATS.contains(suffix);
}