#pragma once

#include <QStyledItemDelegate>
#include <QPainter>
#include <QFileSystemModel>

class ThumbnailView;

class ThumbnailDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit ThumbnailDelegate(ThumbnailView *thumbnailView, QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    bool isImageFile(const QString& filePath) const;
    ThumbnailView *m_thumbnailView;
};