#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QDirIterator>
#include <QThread>
#include <QDebug>
#include <QElapsedTimer>

class SearchWorker : public QObject
{
    Q_OBJECT

public:
    explicit SearchWorker(QObject *parent = nullptr);

public slots:
    void search(const QString &text, const QString &startPath, bool searchInAllDrives,
                bool caseSensitive, bool searchInNamesOnly);

    signals:
        void searchFinished(const QStringList &results, bool timeout);
    void progressUpdate(int count);
};