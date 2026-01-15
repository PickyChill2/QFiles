#include "treesizedelegate.h"
#include "disksizeutils.h"
#include <QPainter>
//#include <QFileSystemModel>
#include <QApplication>
#include <QDebug>
#include "customfilesystemmodel.h"

TreeSizeDelegate::TreeSizeDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void TreeSizeDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                            const QModelIndex &index) const
{
    // Если это не столбец "Size" (индекс 1), используем стандартный делегат
    if (index.column() != 1) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

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

    bool isDisk = fileInfo.isDir() && QDir(filePath).isRoot();

    // Рисуем фон только для выделенного элемента
    if (option.state & QStyle::State_Selected) {
        painter->fillRect(rect, option.palette.highlight());
    }
    // Не рисуем фон для невыделенных элементов - оставляем прозрачный

    // Рисуем текст
    painter->setPen(option.state & QStyle::State_Selected ?
                   option.palette.highlightedText().color() :
                   option.palette.text().color());

    if (isDisk) {
        // Для дисков отображаем компактную информацию о размере
        QString diskSize = DiskSizeUtils::getDiskSizeInfoCompact(filePath);
        if (!diskSize.isEmpty()) {
            QFontMetrics metrics(painter->font());
            QString elidedText = metrics.elidedText(diskSize, Qt::ElideRight, rect.width() - 4);
            painter->drawText(rect.adjusted(2, 0, -2, 0), Qt::AlignVCenter | Qt::AlignLeft, elidedText);
        }
    } else {
        // Для файлов используем стандартное отображение
        QString sizeText = fsModel->data(index, Qt::DisplayRole).toString();
        painter->drawText(rect.adjusted(2, 0, -2, 0), Qt::AlignVCenter | Qt::AlignLeft, sizeText);
    }

    painter->restore();
}

QSize TreeSizeDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)
    return QSize(150, 20);
}