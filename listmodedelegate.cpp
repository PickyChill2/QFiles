#include "listmodedelegate.h"
#include "disksizeutils.h"
#include <QPainter>
//#include <QFileSystemModel>
#include <QApplication>
#include <QDebug>
#include "customfilesystemmodel.h"

ListModeDelegate::ListModeDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void ListModeDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                            const QModelIndex &index) const
{
    painter->save();

    QRect rect = option.rect;

    CustomFileSystemModel *fsModel = qobject_cast<CustomFileSystemModel*>(const_cast<QAbstractItemModel*>(index.model()));
    if (!fsModel) {
        QStyledItemDelegate::paint(painter, option, index);
        painter->restore();
        return;
    }

    QString filePath = fsModel->filePath(index);
    QFileInfo fileInfo(filePath);
    QIcon icon = fsModel->fileIcon(index);
    QString fileName = fsModel->fileName(index);

    bool isDisk = fileInfo.isDir() && QDir(filePath).isRoot();

    // Рисуем фон только для выделенного элемента
    if (option.state & QStyle::State_Selected) {
        painter->fillRect(rect, option.palette.highlight());
    }
    // Не рисуем фон для невыделенных элементов - оставляем прозрачный

    // Рисуем иконку
    int iconSize = option.decorationSize.width();
    QRect iconRect(rect.x() + 2, rect.y() + (rect.height() - iconSize) / 2, iconSize, iconSize);
    icon.paint(painter, iconRect);

    // Рисуем текст
    painter->setPen(option.state & QStyle::State_Selected ?
                   option.palette.highlightedText().color() :
                   option.palette.text().color());

    QRect textRect(iconRect.right() + 4, rect.y(), rect.width() - iconRect.width() - 6, rect.height());

    if (isDisk) {
        // Для дисков: имя + размер в одной строке
        QString diskSize = DiskSizeUtils::getDiskSizeInfoCompact(filePath);
        if (!diskSize.isEmpty()) {
            QString displayText = QString("%1 - %2").arg(fileName).arg(diskSize);
            QFontMetrics metrics(painter->font());
            QString elidedText = metrics.elidedText(displayText, Qt::ElideRight, textRect.width());
            painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, elidedText);
        } else {
            QFontMetrics metrics(painter->font());
            QString elidedText = metrics.elidedText(fileName, Qt::ElideRight, textRect.width());
            painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, elidedText);
        }
    } else {
        // Для обычных файлов и папок
        QFontMetrics metrics(painter->font());
        QString elidedText = metrics.elidedText(fileName, Qt::ElideRight, textRect.width());
        painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, elidedText);
    }

    painter->restore();
}

QSize ListModeDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)
    return QSize(200, 24);
}