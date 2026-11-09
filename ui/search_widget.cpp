#include "search_widget.h"

#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

SearchWidget::SearchWidget(QWidget* parent)
    : QWidget(parent),
      searchInput_(new QLineEdit(this)),
      searchButton_(new QPushButton(QStringLiteral("Search"), this)),
      statusLabel_(new QLabel(
          QStringLiteral("Enter an English technical term."), this)),
      resultList_(new QListWidget(this)) {
    setObjectName(QStringLiteral("searchWidget"));

    searchInput_->setObjectName(QStringLiteral("searchInput"));
    searchInput_->setPlaceholderText(
        QStringLiteral("Search for a term, e.g. Computer"));

    searchButton_->setObjectName(QStringLiteral("searchButton"));
    statusLabel_->setObjectName(QStringLiteral("searchStatus"));
    resultList_->setObjectName(QStringLiteral("searchResults"));
    resultList_->setSelectionMode(QListWidget::NoSelection);

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(16, 16, 16, 16);
    rootLayout->setSpacing(10);

    auto* inputLayout = new QHBoxLayout();
    inputLayout->addWidget(searchInput_, 1);
    inputLayout->addWidget(searchButton_);
    rootLayout->addLayout(inputLayout);
    rootLayout->addWidget(statusLabel_);
    rootLayout->addWidget(resultList_, 1);

    connect(searchButton_, &QPushButton::clicked,
            this, &SearchWidget::requestSearch);
    connect(searchInput_, &QLineEdit::returnPressed,
            this, &SearchWidget::requestSearch);
}

void SearchWidget::requestSearch() {
    const QString english = searchInput_->text().trimmed();
    resultList_->clear();

    if (english.isEmpty()) {
        statusLabel_->setText(
            QStringLiteral("Please enter a word to search."));
        return;
    }

    statusLabel_->setText(QStringLiteral("Searching..."));
    emit searchRequested(english);
}

void SearchWidget::showFoundResult(const QString& english,
                                   const QString& vietnamese) {
    resultList_->clear();
    resultList_->addItem(english + QStringLiteral(" — ") + vietnamese);
    statusLabel_->setText(QStringLiteral("1 result found."));
}

void SearchWidget::showNotFound(const QString& english) {
    resultList_->clear();
    statusLabel_->setText(
        QStringLiteral("No result found for \"%1\".").arg(english));
}

void SearchWidget::showDisconnected() {
    resultList_->clear();
    statusLabel_->setText(
        QStringLiteral("Search data is not connected yet."));
}

void SearchWidget::clearResult() {
    resultList_->clear();
    statusLabel_->setText(
        QStringLiteral("Enter an English technical term."));
}
