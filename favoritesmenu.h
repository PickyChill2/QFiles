#pragma once

#include <QWidgetAction>
#include <QMenu>
#include <QAction>
#include <QMap>
#include <QSettings>
#include <QDialog>
#include <QListWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QPainter>
#include <QGraphicsDropShadowEffect>

class FavoritesMenu : public QMenu
{
    Q_OBJECT

public:
    explicit FavoritesMenu(const QString &title, QWidget *parent = nullptr);

    signals:
        void favoriteMiddleClicked(const QString &path);

protected:
    //void paintEvent(QPaintEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void setupStyle();
};

class ManageFavoritesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ManageFavoritesDialog(const QMap<QString, QString> &favorites, QWidget *parent = nullptr);
    QString getSelectedFavorite() const { return selectedFavorite; }

private slots:
    void onRemoveClicked();
    void onItemSelectionChanged();

private:
    QListWidget *listWidget;
    QPushButton *removeButton;
    QString selectedFavorite;
    QMap<QString, QString> favorites;
};

class FavoritesAction : public QWidgetAction
{
    Q_OBJECT

public:
    explicit FavoritesAction(QWidget *parent = nullptr);
    ~FavoritesAction();

    void updateFavorites();

    signals:
        void favoriteClicked(const QString &path);
    void favoriteMiddleClicked(const QString &path);

private slots:
    void updateMenu();
    void onFavoriteTriggered();
    void addCurrentFolderToFavorites();
    void manageFavorites();

private:
    void loadFavorites();
    void saveFavorites();
    QString getCurrentPath() const;
    QWidget* getMainWindow() const;

    FavoritesMenu *favoritesMenu;
    QMap<QString, QString> favorites; // name -> path
    QSettings settings;
    QWidget *mainWindow;
};