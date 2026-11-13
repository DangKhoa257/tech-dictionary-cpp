#include "word_dialog.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

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

void WordDialog::setupUi() {
    setWindowTitle(mode_ == Mode::Add ? QStringLiteral("Thêm từ")
                                      : QStringLiteral("Sửa nghĩa từ"));
    setModal(true);
    resize(420, 180);

    auto* mainLayout = new QVBoxLayout(this);
    auto* formLayout = new QFormLayout();

    englishEdit_ = new QLineEdit(this);
    vietnameseEdit_ = new QLineEdit(this);
    validationLabel_ = new QLabel(this);
    saveButton_ = new QPushButton(QStringLiteral("Lưu"), this);
    auto* cancelButton = new QPushButton(QStringLiteral("Hủy"), this);

    englishEdit_->setPlaceholderText(QStringLiteral("Nhập từ tiếng Anh"));
    vietnameseEdit_->setPlaceholderText(QStringLiteral("Nhập nghĩa tiếng Việt"));
    validationLabel_->setWordWrap(true);

    formLayout->addRow(QStringLiteral("Từ tiếng Anh:"), englishEdit_);
    formLayout->addRow(QStringLiteral("Nghĩa tiếng Việt:"), vietnameseEdit_);

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

void WordDialog::setValidationMessage(const QString& message) {
    validationLabel_->setText(message);
}

void WordDialog::showSaveError(const QString& message) {
    setValidationMessage(message);
}

void WordDialog::handleSave() {
    if (hasInvalidText(englishEdit_->text()) || hasInvalidText(vietnameseEdit_->text())) {
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
    emit saveRequested(english, vietnamese);
}
