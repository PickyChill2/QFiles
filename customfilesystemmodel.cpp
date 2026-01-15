#include "customfilesystemmodel.h"
#include <QFileInfo>
#include <QDebug>
#include <windows.h>
#include <shlobj.h>
#include <propvarutil.h>
#include <propkey.h>

CustomFileSystemModel::CustomFileSystemModel(QObject *parent)
    : QFileSystemModel(parent)
{
}

bool CustomFileSystemModel::isLnkFile(const QFileInfo &fileInfo) const
{
    return fileInfo.suffix().toLower() == "lnk";
}

QString CustomFileSystemModel::getLnkFileName(const QString &path) const
{
    // Возвращаем имя файла .lnk без пути
    QFileInfo fileInfo(path);
    return fileInfo.fileName();
}

QVariant CustomFileSystemModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole && index.column() == 0) {
        QFileInfo fileInfo = this->fileInfo(index);
        
        // Если это .lnk файл, возвращаем имя самого ярлыка
        if (isLnkFile(fileInfo)) {
            return getLnkFileName(fileInfo.absoluteFilePath());
        }
    }
    
    // Для всех остальных случаев используем стандартное поведение
    return QFileSystemModel::data(index, role);
}

QString CustomFileSystemModel::fileName(const QModelIndex &index) const
{
    QFileInfo fileInfo = this->fileInfo(index);
    
    // Если это .lnk файл, возвращаем имя самого ярлыка
    if (isLnkFile(fileInfo)) {
        return getLnkFileName(fileInfo.absoluteFilePath());
    }
    
    return QFileSystemModel::fileName(index);
}