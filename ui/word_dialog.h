#pragma once
#include <QDialog>
#include <QString>

class QLineEdit;
class QPushButton;
class QLabel;

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

signals:
    void saveRequested(const QString& english, const QString& vietnamese);

private slots:
    void handleSave();

private:
    void setupUi();
    void setValidationMessage(const QString& message);

    Mode mode_;
    QLineEdit* englishEdit_;
    QLineEdit* vietnameseEdit_;
    QLabel* validationLabel_;
    QPushButton* saveButton_;
};
