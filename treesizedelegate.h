#pragma once

#include <QStyledItemDelegate>
#include <QPainter>
#include <QFileSystemModel>

class TreeSizeDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit TreeSizeDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};