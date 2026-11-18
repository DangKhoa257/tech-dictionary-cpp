#include "search_widget.h"

#include <QColor>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QPalette>
#include <QVBoxLayout>
#include <QHBoxLayout>

SearchWidget::SearchWidget(QWidget* parent)
    : QWidget(parent),
      searchInput_(new QLineEdit(this)),
      searchButton_(new QPushButton(QStringLiteral("Tìm kiếm"), this)),
      statusLabel_(new QLabel(
          QStringLiteral("Nhập thuật ngữ tiếng Anh để tra cứu."), this)),
      resultList_(new QListWidget(this)) {
    setObjectName(QStringLiteral("searchWidget"));

    searchInput_->setObjectName(QStringLiteral("searchInput"));
    searchInput_->setPlaceholderText(
        QStringLiteral("Tìm thuật ngữ, ví dụ: C++, <vector>, Algorithm"));
    QPalette inputPalette = searchInput_->palette();
    inputPalette.setColor(QPalette::PlaceholderText, QColor(QStringLiteral("#64748B")));
    searchInput_->setPalette(inputPalette);

    searchButton_->setObjectName(QStringLiteral("searchButton"));
    statusLabel_->setObjectName(QStringLiteral("searchStatus"));
    resultList_->setObjectName(QStringLiteral("searchResults"));
    resultList_->setSelectionMode(QListWidget::NoSelection);
    resultList_->setMaximumHeight(48);
    resultList_->hide();

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(6);

    auto* inputLayout = new QHBoxLayout();
    inputLayout->addWidget(searchInput_, 1);
    inputLayout->addWidget(searchButton_);
    rootLayout->addLayout(inputLayout);
    rootLayout->addWidget(statusLabel_);
    rootLayout->addWidget(resultList_);

    connect(searchButton_, &QPushButton::clicked,
            this, &SearchWidget::requestSearch);
    connect(searchInput_, &QLineEdit::returnPressed,
            this, &SearchWidget::requestSearch);
}

void SearchWidget::requestSearch() {
    const QString english = searchInput_->text().trimmed();
    resultList_->clear();
    resultList_->hide();

    if (english.isEmpty()) {
        statusLabel_->setText(
            QStringLiteral("Vui lòng nhập thuật ngữ cần tìm."));
        return;
    }

    statusLabel_->setText(QStringLiteral("Đang tìm kiếm…"));
    emit searchRequested(english);
}

void SearchWidget::showFoundResult(const QString& english,
                                   const QString& vietnamese) {
    resultList_->clear();
    resultList_->addItem(english + QStringLiteral(" — ") + vietnamese);
    resultList_->show();
    statusLabel_->setText(QStringLiteral("Đã tìm thấy thuật ngữ."));
}

void SearchWidget::showNotFound(const QString& english) {
    resultList_->clear();
    resultList_->hide();
    statusLabel_->setText(
        QStringLiteral("Không tìm thấy “%1”.").arg(english));
}

void SearchWidget::showDisconnected() {
    resultList_->clear();
    resultList_->hide();
    statusLabel_->setText(
        QStringLiteral("Chưa kết nối dữ liệu từ điển."));
}

void SearchWidget::clearResult() {
    resultList_->clear();
    resultList_->hide();
    statusLabel_->setText(
        QStringLiteral("Nhập thuật ngữ tiếng Anh để tra cứu."));
}
