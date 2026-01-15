#pragma once

#include <QFileSystemModel>

class CustomFileSystemModel : public QFileSystemModel
{
    Q_OBJECT

public:
    explicit CustomFileSystemModel(QObject *parent = nullptr);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QString fileName(const QModelIndex &index) const;
    
private:
    bool isLnkFile(const QFileInfo &fileInfo) const;
    QString getLnkFileName(const QString &path) const;
};