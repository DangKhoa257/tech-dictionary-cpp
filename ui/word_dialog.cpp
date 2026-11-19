#include "word_dialog.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QFormLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QPainter>
#include <QPalette>
#include <QPushButton>
#include <QScreen>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QTextEdit>
#include <QVBoxLayout>

#include <algorithm>

namespace {

class MetadataItemDelegate final : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        QSize hint = QStyledItemDelegate::sizeHint(option, index);
        hint.setHeight(std::max(hint.height(), 38));
        return hint;
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override {
        const bool selected = option.state & QStyle::State_Selected;
        const bool hovered = option.state & QStyle::State_MouseOver;
        painter->save();
        painter->fillRect(option.rect, selected ? QColor(QStringLiteral("#DBEAFE"))
                         : hovered ? QColor(QStringLiteral("#F1F5F9")) : Qt::white);
        painter->setPen(selected ? QColor(QStringLiteral("#1D4ED8"))
                                 : QColor(QStringLiteral("#102440")));
        painter->drawText(option.rect.adjusted(12, 0, -12, 0),
                          Qt::AlignLeft | Qt::AlignVCenter, index.data().toString());
        painter->restore();
    }
};

class MetadataComboBox final : public QComboBox {
public:
    explicit MetadataComboBox(QWidget* parent) : QComboBox(parent) {
        auto* list = new QListView(this);
        list->setObjectName(QStringLiteral("metadataPopup"));
        list->setUniformItemSizes(true);
        list->setMouseTracking(true);
        list->setVerticalScrollMode(QAbstractItemView::ScrollPerItem);
        list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        list->setItemDelegate(new MetadataItemDelegate(list));
        list->setAutoFillBackground(true);
        list->viewport()->setAutoFillBackground(true);
        QPalette popupPalette = list->palette();
        popupPalette.setColor(QPalette::Base, Qt::white);
        popupPalette.setColor(QPalette::Text, QColor(QStringLiteral("#102440")));
        popupPalette.setColor(QPalette::Highlight, QColor(QStringLiteral("#DBEAFE")));
        popupPalette.setColor(QPalette::HighlightedText, QColor(QStringLiteral("#1D4ED8")));
        list->setPalette(popupPalette);
        list->viewport()->setPalette(popupPalette);
        list->setStyleSheet(QStringLiteral(
            "QListView#metadataPopup { background: #FFFFFF; color: #102440; "
            "border: 1px solid #CBD5E1; outline: none; }"
            "QListView#metadataPopup::item { min-height: 38px; }"));
        setView(list);
        setMaxVisibleItems(7);
        setMinimumHeight(42);
    }

protected:
    void showPopup() override {
        QComboBox::showPopup();
        if (!view() || !view()->isVisible()) return;

        QWidget* popup = view()->window();
        const QPoint anchor = mapToGlobal(QPoint(0, height()));
        QScreen* screen = QGuiApplication::screenAt(anchor);
        if (!screen) screen = QGuiApplication::primaryScreen();
        if (!screen) return;

        const QRect available = screen->availableGeometry();
        const int rows = std::min(count(), maxVisibleItems());
        const int rowHeight = std::max(38, view()->sizeHintForRow(0));
        const int wantedHeight = std::min(available.height(), rows * rowHeight + 10);
        int wantedWidth = width();
        for (int i = 0; i < count(); ++i)
            wantedWidth = std::max(wantedWidth,
                                   view()->fontMetrics().horizontalAdvance(itemText(i)) + 44);
        wantedWidth = std::min(wantedWidth, available.width());

        popup->resize(wantedWidth, wantedHeight);
        const int x = std::clamp(anchor.x(), available.left(),
                                 available.right() - popup->width() + 1);
        int y = anchor.y();
        if (y + popup->height() > available.bottom() + 1)
            y = mapToGlobal(QPoint(0, 0)).y() - popup->height();
        y = std::clamp(y, available.top(), available.bottom() - popup->height() + 1);
        popup->move(x, y);
    }
};

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
      partOfSpeechEdit_(nullptr),
      exampleEdit_(nullptr),
      categoryEdit_(nullptr),
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
      partOfSpeechEdit_(nullptr),
      exampleEdit_(nullptr),
      categoryEdit_(nullptr),
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
    setWindowTitle(mode_ == Mode::Add ? QStringLiteral("Thêm từ mới")
                                      : QStringLiteral("Sửa nghĩa từ"));
    setObjectName(QStringLiteral("wordDialog"));
    setModal(true);
    setMinimumWidth(500);
    resize(570, 490);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(28, 24, 28, 24);
    mainLayout->setSpacing(14);

    auto* title = new QLabel(mode_ == Mode::Add
        ? QStringLiteral("Thêm thuật ngữ mới")
        : QStringLiteral("Chỉnh sửa thuật ngữ"), this);
    title->setObjectName(QStringLiteral("dialogTitle"));
    auto* subtitle = new QLabel(mode_ == Mode::Add
        ? QStringLiteral("Nhập thuật ngữ và thông tin để tra cứu dễ hơn.")
        : QStringLiteral("Cập nhật nghĩa và ví dụ của thuật ngữ đã chọn."), this);
    subtitle->setObjectName(QStringLiteral("dialogSubtitle"));
    subtitle->setWordWrap(true);
    mainLayout->addWidget(title);
    mainLayout->addWidget(subtitle);
    mainLayout->addSpacing(4);

    auto* formLayout = new QFormLayout();
    formLayout->setHorizontalSpacing(18);
    formLayout->setVerticalSpacing(12);
    formLayout->setLabelAlignment(Qt::AlignLeft);
    formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    formLayout->setRowWrapPolicy(QFormLayout::WrapLongRows);

    englishEdit_ = new QLineEdit(this);
    vietnameseEdit_ = new QLineEdit(this);
    englishEdit_->setMinimumHeight(42);
    vietnameseEdit_->setMinimumHeight(42);

    partOfSpeechEdit_ = new MetadataComboBox(this);
    partOfSpeechEdit_->setObjectName(QStringLiteral("partOfSpeechInput"));
    partOfSpeechEdit_->addItems({
        QStringLiteral("Chọn từ loại"),
        QStringLiteral("Noun"),
        QStringLiteral("Verb"),
        QStringLiteral("Adjective"),
        QStringLiteral("Adverb"),
        QStringLiteral("Noun phrase"),
        QStringLiteral("Verb phrase"),
        QStringLiteral("Abbreviation"),
        QStringLiteral("Other / Technical term")
    });

    exampleEdit_ = new QTextEdit(this);
    exampleEdit_->setObjectName(QStringLiteral("exampleInput"));
    exampleEdit_->setMinimumHeight(86);
    exampleEdit_->setMaximumHeight(118);

    categoryEdit_ = new MetadataComboBox(this);
    categoryEdit_->setObjectName(QStringLiteral("categoryInput"));
    categoryEdit_->addItems({
        QStringLiteral("Chọn chủ đề"),
        QStringLiteral("Basic Computing"),
        QStringLiteral("Programming"),
        QStringLiteral("Object-Oriented Programming"),
        QStringLiteral("Data Structures"),
        QStringLiteral("Algorithms"),
        QStringLiteral("Database"),
        QStringLiteral("Networking"),
        QStringLiteral("Operating Systems"),
        QStringLiteral("Cybersecurity"),
        QStringLiteral("Artificial Intelligence"),
        QStringLiteral("Machine Learning"),
        QStringLiteral("Data Science"),
        QStringLiteral("Software Engineering"),
        QStringLiteral("Cloud Computing"),
        QStringLiteral("Web Development")
    });

    validationLabel_ = new QLabel(this);
    validationLabel_->setObjectName(QStringLiteral("dialogValidation"));
    validationLabel_->hide();
    saveButton_ = new QPushButton(QStringLiteral("Lưu"), this);
    auto* cancelButton = new QPushButton(QStringLiteral("Hủy"), this);

    saveButton_->setObjectName(QStringLiteral("primaryButton"));
    cancelButton->setObjectName(QStringLiteral("secondaryButton"));
    saveButton_->setCursor(Qt::PointingHandCursor);
    cancelButton->setCursor(Qt::PointingHandCursor);
    saveButton_->setMinimumSize(108, 40);
    cancelButton->setMinimumSize(108, 40);

    englishEdit_->setPlaceholderText(QStringLiteral("Ví dụ: Algorithm, Compiler, Cloud..."));
    vietnameseEdit_->setPlaceholderText(QStringLiteral("Ví dụ: Thuật toán, Trình biên dịch..."));
    exampleEdit_->setPlaceholderText(QStringLiteral("Ví dụ câu tiếng Anh minh họa cách sử dụng..."));

    validationLabel_->setWordWrap(true);
    formLayout->addRow(QStringLiteral("Từ tiếng Anh:"), englishEdit_);
    formLayout->addRow(QStringLiteral("Nghĩa tiếng Việt:"), vietnameseEdit_);
    formLayout->addRow(QStringLiteral("Từ loại:"), partOfSpeechEdit_);
    formLayout->addRow(QStringLiteral("Ví dụ tiếng Anh:"), exampleEdit_);
    formLayout->addRow(QStringLiteral("Chủ đề:"), categoryEdit_);

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(10);
    buttonLayout->addStretch();
    buttonLayout->addWidget(saveButton_);
    buttonLayout->addWidget(cancelButton);

    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(validationLabel_);
    auto* divider = new QFrame(this);
    divider->setObjectName(QStringLiteral("dialogDivider"));
    divider->setFrameShape(QFrame::HLine);
    mainLayout->addWidget(divider);
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

QString WordDialog::partOfSpeech() const {
    return partOfSpeechEdit_->currentIndex() == 0 ? QString{} : partOfSpeechEdit_->currentText();
}
QString WordDialog::example() const { return exampleEdit_->toPlainText().trimmed(); }
QString WordDialog::category() const {
    return categoryEdit_->currentIndex() == 0 ? QString{} : categoryEdit_->currentText();
}

void WordDialog::setValidationMessage(const QString& message) {
    validationLabel_->setText(message);
    validationLabel_->setVisible(!message.isEmpty());
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
