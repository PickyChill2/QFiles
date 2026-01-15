#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QIcon>

class ReplaceFileDialog : public QDialog
{
    Q_OBJECT

public:
    enum Result {
        Replace,
        Skip,
        Copy,
        Cancel
    };

    static constexpr int MIN_WIDTH = 200;
    static constexpr int MIN_HEIGHT = 150;
    static constexpr int FONT_SIZE_TITLE = 12;
    static constexpr int FONT_SIZE_MESSAGE = 14;
    static constexpr int FONT_SIZE_CHECKBOX = 10;
    static constexpr int FONT_SIZE_BUTTON = 12;
    static constexpr int CHECKBOX_IND_SIZE = 14;
    static constexpr int BUTTON_MIN_WIDTH = 90;
    static constexpr int PADDING_LARGE = 15;
    static constexpr int PADDING_MEDIUM = 10;
    static constexpr int PADDING_SMALL = 6;
    static constexpr int MARGIN_BUTTON = 4;

    explicit ReplaceFileDialog(const QString &fileName, QWidget *parent = nullptr);

    Result getResult() const { return result; }
    bool shouldApplyToAll() const { return applyToAll; }

private slots:
    void onReplace();
    void onSkip();
    void onCopy();
    void onCancel();

private:
    Result result = Cancel;
    bool applyToAll = false;
    QCheckBox *applyToAllCheck;
    QPushButton *replaceButton;
    QPushButton *skipButton;
    QPushButton *copyButton;
    QPushButton *cancelButton;
};