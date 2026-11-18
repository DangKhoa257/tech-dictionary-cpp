#include "word_dialog.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QComboBox>
#include <QTextEdit>

namespace {

bool hasInvalidText(const QString& text) {
    for (qsizetype i = 0; i < text.size(); ++i) {
        const ushort value = text.at(i).unicode();
        if (value < 0x20 || value == 0x7F) return true;
        if (QChar::isHighSurrogate(value)) {
            if (i + 1 >= text.size() ||
                !QChar::isLowSurrogate(text.at(i + 1).unicode())) return true;
            ++i;
        } else if (QChar::isLowSurrogate(value)) {
            return true;
        }
    }
    return false;
}

bool hasInvalidExample(const QString& text) {
    for (qsizetype i = 0; i < text.size(); ++i) {
        const ushort value = text.at(i).unicode();
        if ((value < 0x20 && value != '\t' && value != '\n' && value != '\r') || value == 0x7F)
            return true;
        if (QChar::isHighSurrogate(value)) {
            if (++i >= text.size() || !QChar::isLowSurrogate(text.at(i).unicode()))
                return true;
        } else if (QChar::isLowSurrogate(value)) return true;
    }
    return false;
}

}  // namespace

WordDialog::WordDialog(Mode mode, QWidget* parent)
    : QDialog(parent),
      mode_(mode),
      englishEdit_(nullptr),
      vietnameseEdit_(nullptr),
      validationLabel_(nullptr),
      saveButton_(nullptr) {
    setupUi();
}

WordDialog::WordDialog(Mode mode, const QString& english, const QString& vietnamese,
                       QWidget* parent)
    : QDialog(parent),
      mode_(mode),
      englishEdit_(nullptr),
      vietnameseEdit_(nullptr),
      validationLabel_(nullptr),
      saveButton_(nullptr) {
    setupUi();
    englishEdit_->setText(english);
    vietnameseEdit_->setText(vietnamese);

    if (mode_ == Mode::Edit) {
        englishEdit_->setReadOnly(true);
    }
}

WordDialog::WordDialog(Mode mode, const QString& english, const QString& vietnamese,
                       const QString& partOfSpeech, const QString& example,
                       const QString& category, QWidget* parent)
    : WordDialog(mode, english, vietnamese, parent) {
    partOfSpeechEdit_->setCurrentText(partOfSpeech);
    exampleEdit_->setPlainText(example);
    categoryEdit_->setCurrentText(category);
}

void WordDialog::setupUi() {
    setWindowTitle(mode_ == Mode::Add ? QStringLiteral("Thêm từ")
                                      : QStringLiteral("Sửa nghĩa từ"));
    setModal(true);
    setMinimumWidth(440);
    resize(500, 350);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 22, 24, 22);
    mainLayout->setSpacing(16);
    auto* formLayout = new QFormLayout();
    formLayout->setSpacing(12);

    englishEdit_ = new QLineEdit(this);
    vietnameseEdit_ = new QLineEdit(this);
    partOfSpeechEdit_ = new QComboBox(this);
    partOfSpeechEdit_->addItems({QStringLiteral(""), QStringLiteral("Noun"),
        QStringLiteral("Verb"), QStringLiteral("Adjective"), QStringLiteral("Adverb"),
        QStringLiteral("Noun phrase"), QStringLiteral("Verb phrase"),
        QStringLiteral("Abbreviation"), QStringLiteral("Other / Technical term")});
    exampleEdit_ = new QTextEdit(this);
    exampleEdit_->setObjectName(QStringLiteral("exampleInput"));
    exampleEdit_->setMaximumHeight(76);
    categoryEdit_ = new QComboBox(this);
    categoryEdit_->addItems({QStringLiteral(""), QStringLiteral("Basic Computing"),
        QStringLiteral("Programming"), QStringLiteral("Object-Oriented Programming"),
        QStringLiteral("Data Structures"), QStringLiteral("Algorithms"),
        QStringLiteral("Database"), QStringLiteral("Networking"),
        QStringLiteral("Operating Systems"), QStringLiteral("Cybersecurity"),
        QStringLiteral("Artificial Intelligence"), QStringLiteral("Machine Learning"),
        QStringLiteral("Data Science"), QStringLiteral("Software Engineering"),
        QStringLiteral("Cloud Computing"), QStringLiteral("Web Development")});
    validationLabel_ = new QLabel(this);
    saveButton_ = new QPushButton(QStringLiteral("Lưu"), this);
    auto* cancelButton = new QPushButton(QStringLiteral("Hủy"), this);
    saveButton_->setObjectName(QStringLiteral("primaryButton"));
    cancelButton->setObjectName(QStringLiteral("secondaryButton"));

    englishEdit_->setPlaceholderText(QStringLiteral("Nhập từ tiếng Anh"));
    vietnameseEdit_->setPlaceholderText(QStringLiteral("Nhập nghĩa tiếng Việt"));
    validationLabel_->setWordWrap(true);
    validationLabel_->setStyleSheet(QStringLiteral("color:#B42318; font-size:12px;"));

    formLayout->addRow(QStringLiteral("Từ tiếng Anh:"), englishEdit_);
    formLayout->addRow(QStringLiteral("Nghĩa tiếng Việt:"), vietnameseEdit_);
    formLayout->addRow(QStringLiteral("Từ loại:"), partOfSpeechEdit_);
    formLayout->addRow(QStringLiteral("Ví dụ tiếng Anh:"), exampleEdit_);
    formLayout->addRow(QStringLiteral("Chủ đề:"), categoryEdit_);

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(saveButton_);
    buttonLayout->addWidget(cancelButton);

    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(validationLabel_);
    mainLayout->addLayout(buttonLayout);

    if (mode_ == Mode::Edit) {
        englishEdit_->setReadOnly(true);
    }

    connect(saveButton_, &QPushButton::clicked, this, &WordDialog::handleSave);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

QString WordDialog::englishWord() const {
    return englishEdit_->text().trimmed();
}

QString WordDialog::vietnameseMeaning() const {
    return vietnameseEdit_->text().trimmed();
}

QString WordDialog::partOfSpeech() const { return partOfSpeechEdit_->currentText(); }
QString WordDialog::example() const { return exampleEdit_->toPlainText().trimmed(); }
QString WordDialog::category() const { return categoryEdit_->currentText(); }

void WordDialog::setValidationMessage(const QString& message) {
    validationLabel_->setText(message);
}

void WordDialog::showSaveError(const QString& message) {
    setValidationMessage(message);
}

void WordDialog::handleSave() {
    if (hasInvalidText(englishEdit_->text()) || hasInvalidText(vietnameseEdit_->text()) ||
        hasInvalidExample(exampleEdit_->toPlainText())) {
        setValidationMessage(QStringLiteral("Không dùng ký tự điều khiển hoặc văn bản không hợp lệ."));
        return;
    }
    const QString english = englishWord();
    const QString vietnamese = vietnameseMeaning();

    if (english.isEmpty()) {
        setValidationMessage(QStringLiteral("Vui lòng nhập từ tiếng Anh."));
        englishEdit_->setFocus();
        return;
    }

    const QChar first = english.front();
    const bool isAsciiLetter = (first >= QLatin1Char('A') && first <= QLatin1Char('Z')) ||
                               (first >= QLatin1Char('a') && first <= QLatin1Char('z'));
    if (!isAsciiLetter) {
        setValidationMessage(QStringLiteral("Từ tiếng Anh phải bắt đầu bằng chữ cái A-Z."));
        englishEdit_->setFocus();
        return;
    }

    if (vietnamese.isEmpty()) {
        setValidationMessage(QStringLiteral("Vui lòng nhập nghĩa tiếng Việt."));
        vietnameseEdit_->setFocus();
        return;
    }

    setValidationMessage({});
    emit richSaveRequested(english, vietnamese, partOfSpeech(), example(), category());
    emit saveRequested(english, vietnamese);
}
