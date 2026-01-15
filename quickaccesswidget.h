#pragma once



#include <QWidget>

#include <QHBoxLayout>

#include <QPushButton>

#include <QLabel>

#include <QStandardPaths>



class QuickAccessWidget : public QWidget

{

    Q_OBJECT



public:

    explicit QuickAccessWidget(QWidget *parent = nullptr);



    signals:

        void folderClicked(const QString &path);



private slots:

    void onFolderClicked();



private:

    void setupUI();

    void populateQuickFolders();



    QHBoxLayout *mainLayout;

};