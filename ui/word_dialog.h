#pragma once

#include <QDialog>
#include <QString>

class QLineEdit;
class QPushButton;
class QLabel;
class QComboBox;
class QTextEdit;

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
    WordDialog(Mode mode, const QString& english, const QString& vietnamese,
               const QString& partOfSpeech, const QString& example,
               const QString& category, QWidget* parent = nullptr);

    QString englishWord() const;
    QString vietnameseMeaning() const;
    QString partOfSpeech() const;
    QString example() const;
    QString category() const;
    void showSaveError(const QString& message);

signals:
    void saveRequested(const QString& english, const QString& vietnamese);
    void richSaveRequested(const QString& english, const QString& vietnamese,
                           const QString& partOfSpeech, const QString& example,
                           const QString& category);

private slots:
    void handleSave();

private:
    void setupUi();
    void setValidationMessage(const QString& message);

    Mode mode_;
    QLineEdit* englishEdit_;
    QLineEdit* vietnameseEdit_;
    QComboBox* partOfSpeechEdit_;
    QTextEdit* exampleEdit_;
    QComboBox* categoryEdit_;
    QLabel* validationLabel_;
    QPushButton* saveButton_;
};

