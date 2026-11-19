#include "search_widget.h"
#include "icon_helper.h"

#include <QAbstractItemView>
#include <QColor>
#include <QCompleter>
#include <QEasingCurve>
#include <QFont>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsOpacityEffect>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QListWidget>
#include <QPainter>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QShowEvent>
#include <QStringListModel>
#include <QStyledItemDelegate>
#include <QVBoxLayout>

namespace {

class SuggestionItemDelegate : public QStyledItemDelegate {
public:
    explicit SuggestionItemDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        Q_UNUSED(index);
        return QSize(option.rect.width(), 36);
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);

        const bool isSelected = (option.state & QStyle::State_Selected);
        const bool isHovered = (option.state & QStyle::State_MouseOver);

        QRect itemRect = option.rect.adjusted(6, 2, -6, -2);

        if (isSelected) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor(QStringLiteral("#DBEAFE")));
            painter->drawRoundedRect(itemRect, 6, 6);
        } else if (isHovered) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor(QStringLiteral("#F1F5F9")));
            painter->drawRoundedRect(itemRect, 6, 6);
        }

        // Book icon
        const QColor iconColor = isSelected ? QColor(QStringLiteral("#1D4ED8")) : QColor(QStringLiteral("#64748B"));
        QIcon book = IconHelper::bookIcon(iconColor, 16);
        QRect iconRect(itemRect.left() + 8, itemRect.top() + (itemRect.height() - 16) / 2, 16, 16);
        book.paint(painter, iconRect);

        // Text
        QFont font = painter->font();
        font.setPointSize(10);
        font.setWeight(isSelected ? QFont::DemiBold : QFont::Normal);
        painter->setFont(font);

        const QColor textColor = isSelected ? QColor(QStringLiteral("#1D4ED8")) : QColor(QStringLiteral("#1E293B"));
        painter->setPen(textColor);

        QRect textRect = itemRect.adjusted(32, 0, -8, 0);
        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, index.data().toString());

        painter->restore();
    }
};

class SuggestionPopupView : public QListView {
public:
    explicit SuggestionPopupView(QWidget* parent = nullptr) : QListView(parent) {
        setObjectName(QStringLiteral("autocompletePopup"));
        setViewportMargins(0, 26, 0, 0);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        setSelectionMode(QAbstractItemView::SingleSelection);
        setEditTriggers(QAbstractItemView::NoEditTriggers);
        setMouseTracking(true);
        setStyleSheet(QStringLiteral(
            "QListView#autocompletePopup { "
            "  background-color: #FFFFFF; "
            "  color: #1E293B; "
            "  border: 1px solid #CBD5E1; "
            "  border-radius: 10px; "
            "  padding: 4px; "
            "  outline: none; "
            "}"));

        auto* shadow = new QGraphicsDropShadowEffect(this);
        shadow->setBlurRadius(18);
        shadow->setColor(QColor(16, 36, 64, 45));
        shadow->setOffset(0, 4);
        setGraphicsEffect(shadow);
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        QListView::paintEvent(event);
        // Draw the header after the viewport so its background cannot cover it.
        QPainter p(this);
        p.setPen(QColor(QStringLiteral("#64748B")));
        QFont font = p.font();
        font.setPointSize(9);
        font.setWeight(QFont::Medium);
        p.setFont(font);

        const int count = model() ? model()->rowCount() : 0;
        const QString header = QStringLiteral("%1 gợi ý phù hợp").arg(count);
        p.drawText(QRect(14, 6, width() - 28, 18), Qt::AlignLeft | Qt::AlignVCenter, header);
    }

    void showEvent(QShowEvent* event) override {
        QListView::showEvent(event);
        if (QGuiApplication::platformName() != QLatin1String("offscreen") &&
            !qEnvironmentVariableIsSet("TECH_DICTIONARY_NO_ANIMATION")) {
            auto* anim = new QPropertyAnimation(this, "windowOpacity", this);
            anim->setDuration(160);
            anim->setStartValue(0.0);
            anim->setEndValue(1.0);
            anim->setEasingCurve(QEasingCurve::OutCubic);
            anim->start(QAbstractAnimation::DeleteWhenStopped);
        }
    }
};

}  // namespace

SearchWidget::SearchWidget(QWidget* parent)
    : QWidget(parent),
      searchInput_(new QLineEdit(this)),
      searchButton_(new QPushButton(QStringLiteral("Tìm kiếm"), this)),
      statusLabel_(new QLabel(QStringLiteral("Nhập thuật ngữ tiếng Anh để tra cứu."), this)),
      resultList_(new QListWidget(this)),
      completer_(new QCompleter(this)),
      suggestionModel_(new QStringListModel(this)) {
    setObjectName(QStringLiteral("searchWidget"));

    searchInput_->setObjectName(QStringLiteral("searchInput"));
    searchInput_->setPlaceholderText(
        QStringLiteral("Tìm thuật ngữ, ví dụ: C++, algorithm, compiler, cloud..."));
    searchInput_->addAction(IconHelper::searchIcon(QColor(QStringLiteral("#64748B")), 18),
                            QLineEdit::LeadingPosition);
    searchInput_->setClearButtonEnabled(true);

    searchButton_->setObjectName(QStringLiteral("searchButton"));
    searchButton_->setIcon(IconHelper::searchIcon(Qt::white, 16));
    searchButton_->setCursor(Qt::PointingHandCursor);

    statusLabel_->setObjectName(QStringLiteral("searchStatus"));
    resultList_->setObjectName(QStringLiteral("searchResults"));
    resultList_->setSelectionMode(QListWidget::NoSelection);
    resultList_->setMaximumHeight(48);
    resultList_->hide();

    setupCompleterPopup();

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(4);

    auto* inputLayout = new QHBoxLayout();
    inputLayout->setContentsMargins(0, 0, 0, 0);
    inputLayout->setSpacing(10);
    inputLayout->addWidget(searchInput_, 1);
    inputLayout->addWidget(searchButton_);
    rootLayout->addLayout(inputLayout);
    rootLayout->addWidget(statusLabel_);
    rootLayout->addWidget(resultList_);

    connect(searchButton_, &QPushButton::clicked, this, &SearchWidget::requestSearch);
    connect(searchInput_, &QLineEdit::returnPressed, this, &SearchWidget::requestSearch);
}

void SearchWidget::setupCompleterPopup() {
    auto* popup = new SuggestionPopupView(this);
    completer_->setPopup(popup);
    completer_->setModel(suggestionModel_);
    completer_->setCaseSensitivity(Qt::CaseInsensitive);
    completer_->setCompletionMode(QCompleter::PopupCompletion);
    completer_->setMaxVisibleItems(8);

    searchInput_->setCompleter(completer_);
    auto* suggestionDelegate = new SuggestionItemDelegate(popup);
    suggestionDelegate->setObjectName(QStringLiteral("suggestionItemDelegate"));
    popup->setItemDelegate(suggestionDelegate);

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
}

void SearchWidget::requestSearch() {
    const QString english = searchInput_->text().trimmed();
    resultList_->clear();
    resultList_->hide();

    if (english.isEmpty()) {
        statusLabel_->setText(QStringLiteral("Vui lòng nhập thuật ngữ cần tìm."));
        return;
    }

    statusLabel_->setText(QStringLiteral("Đang tìm kiếm…"));
    emit searchRequested(english);
}

void SearchWidget::showFoundResult(const QString& english, const QString& vietnamese) {
    resultList_->clear();
    resultList_->addItem(english + QStringLiteral(" — ") + vietnamese);
    resultList_->show();
    statusLabel_->setText(QStringLiteral("Đã tìm thấy thuật ngữ."));
}

void SearchWidget::showNotFound(const QString& english) {
    resultList_->clear();
    resultList_->hide();
    statusLabel_->setText(QStringLiteral("Không tìm thấy “%1”.").arg(english));
}

void SearchWidget::showDisconnected() {
    resultList_->clear();
    resultList_->hide();
    statusLabel_->setText(QStringLiteral("Chưa kết nối dữ liệu từ điển."));
}

void SearchWidget::clearResult() {
    resultList_->clear();
    resultList_->hide();
    statusLabel_->setText(QStringLiteral("Nhập thuật ngữ tiếng Anh để tra cứu."));
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
