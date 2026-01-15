#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QComboBox>
#include <QListWidget>
#include <QProgressBar>
#include <QMovie>
#include <QThread>
#include <QMenu>
#include <QTimer>

class SearchWorker;

class SearchWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SearchWidget(QWidget *parent = nullptr);
    ~SearchWidget();

    void showAtPosition(const QPoint &pos);
    QString getSearchText() const;
    bool searchInCurrentFolder() const;
    bool searchInAllDrives() const;
    bool caseSensitive() const;
    bool searchInNamesOnly() const;
    void clearSearch();

    signals:
        void searchRequested(const QString &text);
    void closed();
    void resultSelected(const QString &path);
    void navigateToContainingFolder(const QString &filePath);
    void navigateToFile(const QString &filePath);
    void startSearchSignal(const QString &text, const QString &startPath, bool searchInAllDrives, bool caseSensitive, bool searchInNamesOnly);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void showEvent(QShowEvent *event) override;
    bool event(QEvent *event) override;

private slots:
    void onSearchClicked();
    void onCloseClicked();
    void onResultClicked(QListWidgetItem *item);
    void onSearchFinished(const QStringList &results, bool timeout);
    void onProgressUpdate(int count);
    void stopSearch();
    void showResultsContextMenu(const QPoint &pos);
    void onShowInContainingFolder();

private:
    void setupUI();
    void updateResultsHeight();
    void startSearch();

    QLineEdit *searchEdit;
    QPushButton *searchButton;
    QPushButton *closeButton;
    QCheckBox *caseSensitiveCheck;
    QCheckBox *namesOnlyCheck;
    QComboBox *searchScopeCombo;
    QListWidget *resultsList;
    QProgressBar *progressBar;
    QLabel *statusLabel;
    QLabel *loadingIndicator;
    QMenu *contextMenu;

    SearchWorker *searchWorker;
    QThread *searchThread;
    bool isSearching;
    QString currentRightClickedPath;
};