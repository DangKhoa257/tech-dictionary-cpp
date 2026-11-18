#include "search_widget.h"

#include <QColor>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QPalette>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCompleter>
#include <QStringListModel>
#include <QAbstractItemView>

SearchWidget::SearchWidget(QWidget* parent)
    : QWidget(parent),
      searchInput_(new QLineEdit(this)),
      searchButton_(new QPushButton(QStringLiteral("Tìm kiếm"), this)),
      statusLabel_(new QLabel(
          QStringLiteral("Nhập thuật ngữ tiếng Anh để tra cứu."), this)),
      resultList_(new QListWidget(this)),
      completer_(new QCompleter(this)),
      suggestionModel_(new QStringListModel(this)) {
    setObjectName(QStringLiteral("searchWidget"));

    searchInput_->setObjectName(QStringLiteral("searchInput"));
    searchInput_->setPlaceholderText(
        QStringLiteral("Tìm thuật ngữ, ví dụ: C++, <vector>, Algorithm"));
    QPalette inputPalette = searchInput_->palette();
    inputPalette.setColor(QPalette::PlaceholderText, QColor(QStringLiteral("#64748B")));
    searchInput_->setPalette(inputPalette);
    completer_->setModel(suggestionModel_);
    completer_->setCaseSensitivity(Qt::CaseInsensitive);
    completer_->setCompletionMode(QCompleter::PopupCompletion);
    completer_->setMaxVisibleItems(8);
    completer_->popup()->setStyleSheet(QStringLiteral(
        "QListView { background:#FFFFFF; color:#172B4D; border:1px solid #CBD5E1; "
        "selection-background-color:#DBEAFE; selection-color:#123A75; font-size:13px; }"
        "QListView::item { min-height:27px; padding:3px 9px; }"));
    searchInput_->setCompleter(completer_);
    connect(searchInput_, &QLineEdit::textChanged, this, [this](const QString& text) {
        if (text.trimmed().isEmpty()) {
            suggestionModel_->setStringList({});
            completer_->popup()->hide();
            clearResult();
        } else {
            emit suggestionRequested(text);
        }
    });
    connect(completer_, QOverload<const QString&>::of(&QCompleter::activated),
            this, [this](const QString& term) {
        searchInput_->setText(term);
        emit searchRequested(term);
    });

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

void SearchWidget::setSuggestions(const QStringList& suggestions) {
    suggestionModel_->setStringList(suggestions);
    if (suggestions.isEmpty()) {
        completer_->popup()->hide();
        statusLabel_->setText(QStringLiteral("Không có gợi ý phù hợp. Nhấn Enter để tìm chính xác."));
    } else if (searchInput_->hasFocus()) {
        statusLabel_->setText(QStringLiteral("%1 gợi ý phù hợp.").arg(suggestions.size()));
        completer_->setCompletionPrefix(searchInput_->text().trimmed());
        completer_->complete();
    } else {
        statusLabel_->setText(QStringLiteral("%1 gợi ý phù hợp.").arg(suggestions.size()));
    }
}

QString SearchWidget::query() const { return searchInput_->text(); }
void SearchWidget::hideSuggestions() { completer_->popup()->hide(); }
