#pragma once

#include <QStyledItemDelegate>
#include <QPainter>
#include <QFileSystemModel>

class ListModeDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit ListModeDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};