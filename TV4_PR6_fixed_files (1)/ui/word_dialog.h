#pragma once

#include <QDialog>
#include <QString>

class QLabel;
class QLineEdit;
class QPushButton;

class WordDialog : public QDialog {
    Q_OBJECT

public:
    enum class Mode {
        Add,
        Edit
    };

    explicit WordDialog(Mode mode, QWidget* parent = nullptr);
    WordDialog(Mode mode, const QString& english, const QString& vietnamese,
               QWidget* parent = nullptr);

    QString englishWord() const;
    QString vietnameseMeaning() const;

    // The caller/controller reports the operation result through this method.
    // The dialog intentionally stays open until the caller calls accept().
    void setValidationMessage(const QString& message);

signals:
    void saveRequested(const QString& english, const QString& vietnamese);

private slots:
    void handleSave();

private:
    void setupUi();

    Mode mode_;
    QLineEdit* englishEdit_;
    QLineEdit* vietnameseEdit_;
    QLabel* validationLabel_;
    QPushButton* saveButton_;
};
