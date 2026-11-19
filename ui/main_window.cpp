#include "main_window.h"
#include "icon_helper.h"

#include <QApplication>
#include <QEasingCurve>
#include <QFile>
#include <QGuiApplication>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidgetItem>
#include <QPropertyAnimation>
#include <QScreen>
#include <QSplitter>
#include <QVBoxLayout>

#include <algorithm>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setupUi();
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi() {
    setWindowTitle(QStringLiteral("Tech Dictionary"));
    setMinimumSize(800, 560);
    const QRect screen = QGuiApplication::primaryScreen()->availableGeometry();
    resize(std::min(1120, screen.width()), std::min(730, screen.height()));

    QFile styleFile(QStringLiteral(":/styles.qss"));
    if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setStyleSheet(QString::fromUtf8(styleFile.readAll()));
    }

    centralWidget = new QWidget(this);
    centralWidget->setObjectName(QStringLiteral("root"));
    setCentralWidget(centralWidget);

    auto* mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    setupSidebar(mainLayout);

    contentWidget = new QWidget(centralWidget);
    contentWidget->setObjectName(QStringLiteral("content"));
    auto* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(28, 22, 28, 22);
    contentLayout->setSpacing(16);

    setupHeader(contentLayout);
    setupSearchArea(contentLayout);
    setupStatsCards(contentLayout);
    setupContentSplitter(contentLayout);

    mainLayout->addWidget(contentWidget, 1);

    setWordDetail({}, {});
    setWordList({});
}

void MainWindow::setupSidebar(QHBoxLayout* mainLayout) {
    sidebarFrame = new QFrame(centralWidget);
    sidebarFrame->setObjectName(QStringLiteral("sidebar"));
    sidebarFrame->setFixedWidth(240);

    auto* sidebarLayout = new QVBoxLayout(sidebarFrame);
    sidebarLayout->setContentsMargins(16, 22, 16, 18);
    sidebarLayout->setSpacing(8);

    // Branding header
    auto* brandLayout = new QHBoxLayout();
    brandLayout->setContentsMargins(0, 0, 0, 0);
    brandLayout->setSpacing(12);

    auto* logoLabel = new QLabel(sidebarFrame);
    logoLabel->setPixmap(IconHelper::brandLogoPixmap(38));
    logoLabel->setFixedSize(38, 38);
    brandLayout->addWidget(logoLabel);

    auto* brandTextLayout = new QVBoxLayout();
    brandTextLayout->setContentsMargins(0, 0, 0, 0);
    brandTextLayout->setSpacing(2);

    brandLabel = new QLabel(QStringLiteral("Tech Dictionary"), sidebarFrame);
    brandLabel->setObjectName(QStringLiteral("brand"));
    brandTextLayout->addWidget(brandLabel);

    auto* tagline = new QLabel(QStringLiteral("TỪ ĐIỂN CÔNG NGHỆ"), sidebarFrame);
    tagline->setObjectName(QStringLiteral("brandTagline"));
    brandTextLayout->addWidget(tagline);
    brandLayout->addLayout(brandTextLayout);
    sidebarLayout->addLayout(brandLayout);

    sidebarLayout->addSpacing(18);

    // Active primary feature nav item: "Tra cứu từ điển"
    auto* navLookup = new QPushButton(QStringLiteral("  Tra cứu từ điển"), sidebarFrame);
    navLookup->setObjectName(QStringLiteral("navSectionLookup"));
    navLookup->setIcon(IconHelper::lookupIcon(Qt::white, 16));
    navLookup->setCursor(Qt::PointingHandCursor);
    sidebarLayout->addWidget(navLookup);
    connect(navLookup, &QPushButton::clicked, this, [this] {
        if (!filterButtons_.isEmpty()) {
            filterButtons_[0]->click();
        }
    });

    sidebarLayout->addSpacing(12);

    // Section title
    eyebrowLabel = new QLabel(QStringLiteral("DANH MỤC THEO CHỮ CÁI"), sidebarFrame);
    eyebrowLabel->setObjectName(QStringLiteral("eyebrow"));
    sidebarLayout->addWidget(eyebrowLabel);

    // 6 Alphabetical filter buttons
    const QStringList filters = {
        QStringLiteral("All Words"),
        QStringLiteral("A - E"),
        QStringLiteral("F - J"),
        QStringLiteral("K - O"),
        QStringLiteral("P - T"),
        QStringLiteral("U - Z")
    };

    for (int i = 0; i < filters.size(); ++i) {
        const QString& filter = filters[i];
        auto* button = new QPushButton(sidebarFrame);
        button->setObjectName(QStringLiteral("navButton"));
        button->setProperty("filterKey", filter);
        button->setCheckable(true);
        button->setChecked(i == 0);
        button->setEnabled(false);
        button->setCursor(Qt::PointingHandCursor);

        if (i == 0) {
            button->setText(QStringLiteral("  Tất cả từ"));
            button->setIcon(IconHelper::gridIcon(Qt::white, 16));
        } else {
            button->setText(QStringLiteral("  ") + filter);
            button->setIcon(IconHelper::folderIcon(QColor(QStringLiteral("#94A3B8")), 16));
        }

        filterButtons_.append(button);
        connect(button, &QPushButton::clicked, this, &MainWindow::onNavButtonClicked);
        sidebarLayout->addWidget(button);
    }

    sidebarLayout->addStretch();

    // Bottom total count and version
    totalWordCountLabel_ = new QLabel(QStringLiteral("Tổng cộng: 0 từ"), sidebarFrame);
    totalWordCountLabel_->setObjectName(QStringLiteral("totalWordCount"));
    sidebarLayout->addWidget(totalWordCountLabel_);

    sidebarFooterLabel = new QLabel(QStringLiteral("Phiên bản 0.2.0"), sidebarFrame);
    sidebarFooterLabel->setObjectName(QStringLiteral("sidebarFooter"));
    sidebarLayout->addWidget(sidebarFooterLabel);

    mainLayout->addWidget(sidebarFrame);
}

void MainWindow::setupHeader(QVBoxLayout* contentLayout) {
    auto* headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(14);

    // Header icon box (soft blue with laptop)
    auto* iconBox = new QFrame(contentWidget);
    iconBox->setObjectName(QStringLiteral("headerIconBox"));
    iconBox->setFixedSize(48, 48);
    auto* iconBoxLayout = new QVBoxLayout(iconBox);
    iconBoxLayout->setContentsMargins(0, 0, 0, 0);
    iconBoxLayout->setAlignment(Qt::AlignCenter);
    auto* iconLabel = new QLabel(iconBox);
    iconLabel->setPixmap(IconHelper::laptopPixmap(QColor(QStringLiteral("#2563EB")), 28));
    iconBoxLayout->addWidget(iconLabel);
    headerLayout->addWidget(iconBox);

    auto* titleLayout = new QVBoxLayout();
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(2);

    headingLabel = new QLabel(QStringLiteral("Từ điển thuật ngữ CNTT"), contentWidget);
    headingLabel->setObjectName(QStringLiteral("heading"));
    titleLayout->addWidget(headingLabel);

    descriptionLabel = new QLabel(
        QStringLiteral("Tra cứu và quản lý thuật ngữ công nghệ Anh – Việt"), contentWidget);
    descriptionLabel->setObjectName(QStringLiteral("description"));
    descriptionLabel->setWordWrap(true);
    titleLayout->addWidget(descriptionLabel);
    headerLayout->addLayout(titleLayout, 1);

    btnAdd = new QPushButton(QStringLiteral("+  Thêm từ mới"), contentWidget);
    btnAdd->setObjectName(QStringLiteral("addButton"));
    btnAdd->setEnabled(false);
    btnAdd->setCursor(Qt::PointingHandCursor);
    connect(btnAdd, &QPushButton::clicked, this, &MainWindow::onAddClicked);
    headerLayout->addWidget(btnAdd, 0, Qt::AlignVCenter);

    contentLayout->addLayout(headerLayout);
}

void MainWindow::setupSearchArea(QVBoxLayout* contentLayout) {
    auto* searchContainer = new QFrame(contentWidget);
    searchContainer->setObjectName(QStringLiteral("searchCard"));
    searchContainerLayout = new QVBoxLayout(searchContainer);
    searchContainerLayout->setContentsMargins(8, 8, 8, 8);
    contentLayout->addWidget(searchContainer);
}

void MainWindow::setupStatsCards(QVBoxLayout* contentLayout) {
    auto* statsLayout = new QHBoxLayout();
    statsLayout->setContentsMargins(0, 0, 0, 0);
    statsLayout->setSpacing(14);

    // Card 1: Terms
    auto* termsCard = new QFrame(contentWidget);
    termsCard->setObjectName(QStringLiteral("statCardTerms"));
    termsCard->setProperty("cardType", "statCard");
    auto* termsLayout = new QHBoxLayout(termsCard);
    termsLayout->setContentsMargins(14, 12, 14, 12);
    termsLayout->setSpacing(12);

    auto* termsIconBox = new QLabel(termsCard);
    termsIconBox->setObjectName(QStringLiteral("statIconBoxBlue"));
    termsIconBox->setFixedSize(38, 38);
    termsIconBox->setAlignment(Qt::AlignCenter);
    termsIconBox->setPixmap(IconHelper::renderPixmap(20, 20, [](QPainter& p, qreal w, qreal h) {
        QIcon b = IconHelper::bookIcon(QColor(QStringLiteral("#2563EB")), 20);
        b.paint(&p, QRect(0, 0, static_cast<int>(w), static_cast<int>(h)));
    }));
    termsLayout->addWidget(termsIconBox);

    auto* termsTextLayout = new QVBoxLayout();
    termsTextLayout->setContentsMargins(0, 0, 0, 0);
    termsTextLayout->setSpacing(1);
    statsTermsCountLabel_ = new QLabel(QStringLiteral("588 thuật ngữ"), termsCard);
    statsTermsCountLabel_->setObjectName(QStringLiteral("statValue"));
    termsTextLayout->addWidget(statsTermsCountLabel_);
    auto* termsSub = new QLabel(QStringLiteral("Tổng số từ trong từ điển"), termsCard);
    termsSub->setObjectName(QStringLiteral("statDesc"));
    termsTextLayout->addWidget(termsSub);
    termsLayout->addLayout(termsTextLayout);
    statsLayout->addWidget(termsCard);

    // Card 2: Categories
    auto* catCard = new QFrame(contentWidget);
    catCard->setObjectName(QStringLiteral("statCardCat"));
    catCard->setProperty("cardType", "statCard");
    auto* catLayout = new QHBoxLayout(catCard);
    catLayout->setContentsMargins(14, 12, 14, 12);
    catLayout->setSpacing(12);

    auto* catIconBox = new QLabel(catCard);
    catIconBox->setObjectName(QStringLiteral("statIconBoxPurple"));
    catIconBox->setFixedSize(38, 38);
    catIconBox->setAlignment(Qt::AlignCenter);
    catIconBox->setPixmap(IconHelper::layersPixmap(QColor(QStringLiteral("#7C3AED")), 22));
    catLayout->addWidget(catIconBox);

    auto* catTextLayout = new QVBoxLayout();
    catTextLayout->setContentsMargins(0, 0, 0, 0);
    catTextLayout->setSpacing(1);
    auto* catValue = new QLabel(QStringLiteral("15 chủ đề"), catCard);
    catValue->setObjectName(QStringLiteral("statValue"));
    catTextLayout->addWidget(catValue);
    auto* catSub = new QLabel(QStringLiteral("Phân loại theo lĩnh vực CNTT"), catCard);
    catSub->setObjectName(QStringLiteral("statDesc"));
    catTextLayout->addWidget(catSub);
    catLayout->addLayout(catTextLayout);
    statsLayout->addWidget(catCard);

    // Card 3: Quick Search
    auto* speedCard = new QFrame(contentWidget);
    speedCard->setObjectName(QStringLiteral("statCardSpeed"));
    speedCard->setProperty("cardType", "statCard");
    auto* speedLayout = new QHBoxLayout(speedCard);
    speedLayout->setContentsMargins(14, 12, 14, 12);
    speedLayout->setSpacing(12);

    auto* speedIconBox = new QLabel(speedCard);
    speedIconBox->setObjectName(QStringLiteral("statIconBoxGreen"));
    speedIconBox->setFixedSize(38, 38);
    speedIconBox->setAlignment(Qt::AlignCenter);
    speedIconBox->setPixmap(IconHelper::lightningPixmap(QColor(QStringLiteral("#059669")), 20));
    speedLayout->addWidget(speedIconBox);

    auto* speedTextLayout = new QVBoxLayout();
    speedTextLayout->setContentsMargins(0, 0, 0, 0);
    speedTextLayout->setSpacing(1);
    auto* speedValue = new QLabel(QStringLiteral("Tìm nhanh"), speedCard);
    speedValue->setObjectName(QStringLiteral("statValue"));
    speedTextLayout->addWidget(speedValue);
    auto* speedSub = new QLabel(QStringLiteral("Tra cứu tức thì, kết quả chính xác"), speedCard);
    speedSub->setObjectName(QStringLiteral("statDesc"));
    speedTextLayout->addWidget(speedSub);
    speedLayout->addLayout(speedTextLayout);
    statsLayout->addWidget(speedCard);

    contentLayout->addLayout(statsLayout);
}

void MainWindow::setupContentSplitter(QVBoxLayout* contentLayout) {
    auto* splitter = new QSplitter(Qt::Horizontal, contentWidget);
    splitter->setObjectName(QStringLiteral("contentSplitter"));
    splitter->setChildrenCollapsible(false);

    // Left List Panel
    auto* listCard = new QFrame(splitter);
    listCard->setObjectName(QStringLiteral("listCard"));
    auto* listLayout = new QVBoxLayout(listCard);
    listLayout->setContentsMargins(18, 16, 18, 16);
    listLayout->setSpacing(10);

    auto* listHeader = new QHBoxLayout();
    listHeader->setContentsMargins(0, 0, 0, 0);
    auto* listTitle = new QLabel(QStringLiteral("Danh sách thuật ngữ"), listCard);
    listTitle->setObjectName(QStringLiteral("panelTitle"));
    listHeader->addWidget(listTitle);
    listHeader->addStretch();

    visibleCountLabel_ = new QLabel(QStringLiteral("0 từ"), listCard);
    visibleCountLabel_->setObjectName(QStringLiteral("visibleCount"));
    listHeader->addWidget(visibleCountLabel_);
    listLayout->addLayout(listHeader);

    emptyStateLabel_ = new QLabel(
        QStringLiteral("Chưa có thuật ngữ trong mục này.\nHãy thêm từ mới hoặc mở một tệp từ điển."),
        listCard);
    emptyStateLabel_->setObjectName(QStringLiteral("emptyState"));
    emptyStateLabel_->setAlignment(Qt::AlignCenter);
    emptyStateLabel_->setWordWrap(true);
    listLayout->addWidget(emptyStateLabel_, 1);

    wordListWidget = new QListWidget(listCard);
    wordListWidget->setObjectName(QStringLiteral("wordList"));
    wordListWidget->setAlternatingRowColors(false);
    wordListWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    connect(wordListWidget, &QListWidget::itemSelectionChanged, this,
            &MainWindow::onItemSelectionChanged);
    listLayout->addWidget(wordListWidget, 1);

    // Right Detail Panel
    auto* detailCard = new QFrame(splitter);
    detailCard->setObjectName(QStringLiteral("detailCard"));
    auto* detailLayout = new QVBoxLayout(detailCard);
    detailLayout->setContentsMargins(22, 16, 22, 16);
    detailLayout->setSpacing(10);

    auto* detailHeaderLayout = new QHBoxLayout();
    detailHeaderLayout->setContentsMargins(0, 0, 0, 0);
    auto* detailTitle = new QLabel(QStringLiteral("Chi tiết thuật ngữ"), detailCard);
    detailTitle->setObjectName(QStringLiteral("panelEyebrow"));
    detailHeaderLayout->addWidget(detailTitle);
    detailHeaderLayout->addStretch();
    detailLayout->addLayout(detailHeaderLayout);

    wordDetailTextEdit = new QTextEdit(detailCard);
    wordDetailTextEdit->setObjectName(QStringLiteral("wordDetail"));
    wordDetailTextEdit->setReadOnly(true);
    wordDetailTextEdit->setFocusPolicy(Qt::NoFocus);

    detailOpacityEffect_ = new QGraphicsOpacityEffect(wordDetailTextEdit);
    detailOpacityEffect_->setOpacity(1.0);
    wordDetailTextEdit->setGraphicsEffect(detailOpacityEffect_);

    detailLayout->addWidget(wordDetailTextEdit, 1);

    auto* actionLayout = new QHBoxLayout();
    actionLayout->setContentsMargins(0, 6, 0, 0);
    actionLayout->setSpacing(10);

    btnEdit = new QPushButton(QStringLiteral("  Sửa nghĩa"), detailCard);
    btnDelete = new QPushButton(QStringLiteral("  Xóa từ"), detailCard);
    btnEdit->setObjectName(QStringLiteral("editButton"));
    btnDelete->setObjectName(QStringLiteral("deleteButton"));
    btnEdit->setIcon(IconHelper::editIcon(QColor(QStringLiteral("#2563EB")), 16));
    btnDelete->setIcon(IconHelper::deleteIcon(QColor(QStringLiteral("#DC2626")), 16));
    btnEdit->setEnabled(false);
    btnDelete->setEnabled(false);
    btnEdit->setCursor(Qt::PointingHandCursor);
    btnDelete->setCursor(Qt::PointingHandCursor);

    connect(btnEdit, &QPushButton::clicked, this, &MainWindow::onEditClicked);
    connect(btnDelete, &QPushButton::clicked, this, &MainWindow::onDeleteClicked);

    actionLayout->addStretch();
    actionLayout->addWidget(btnEdit);
    actionLayout->addWidget(btnDelete);
    detailLayout->addLayout(actionLayout);

    splitter->addWidget(listCard);
    splitter->addWidget(detailCard);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 3);
    splitter->setSizes({340, 520});

    contentLayout->addWidget(splitter, 1);
}

void MainWindow::onNavButtonClicked() {
    auto* button = qobject_cast<QPushButton*>(sender());
    if (!button) return;

    for (int i = 0; i < filterButtons_.size(); ++i) {
        auto* nav = filterButtons_[i];
        const bool checked = (nav == button);
        nav->setChecked(checked);

        // Update active icon
        if (i == 0) {
            nav->setIcon(IconHelper::gridIcon(Qt::white, 16));
        } else {
            nav->setIcon(IconHelper::folderIcon(checked ? Qt::white : QColor(QStringLiteral("#94A3B8")), 16));
        }
    }

    emit filterRequested(button->property("filterKey").toString());
}

void MainWindow::onAddClicked() { emit addWordRequested(); }
void MainWindow::onEditClicked() { emit editWordRequested(); }
void MainWindow::onDeleteClicked() { emit deleteWordRequested(); }

void MainWindow::onItemSelectionChanged() {
    auto* item = wordListWidget->currentItem();
    btnEdit->setEnabled(interactionEnabled_ && item);
    btnDelete->setEnabled(interactionEnabled_ && item);
    if (item) {
        emit wordSelected(item->text());
    } else {
        setWordDetail({}, {});
    }
}

void MainWindow::animateDetailTransition() {
    if (!detailOpacityEffect_) return;

    if (QGuiApplication::platformName() == QLatin1String("offscreen") ||
        qEnvironmentVariableIsSet("TECH_DICTIONARY_NO_ANIMATION")) {
        detailOpacityEffect_->setOpacity(1.0);
        return;
    }

    auto* anim = new QPropertyAnimation(detailOpacityEffect_, "opacity", this);
    anim->setDuration(190);
    anim->setStartValue(0.35);
    anim->setEndValue(1.0);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::setWordList(const QStringList& words) {
    wordListWidget->clear();

    const QIcon itemIcon = IconHelper::bookIcon(QColor(QStringLiteral("#2563EB")), 16);
    for (const auto& word : words) {
        auto* item = new QListWidgetItem(itemIcon, word);
        wordListWidget->addItem(item);
    }

    visibleCountLabel_->setText(QStringLiteral("%1 từ").arg(words.size()));
    emptyStateLabel_->setVisible(words.isEmpty());
    wordListWidget->setVisible(!words.isEmpty());
    onItemSelectionChanged();
}

void MainWindow::setTotalWordCount(int count) {
    totalWordCountLabel_->setText(QStringLiteral("Tổng cộng: %1 từ").arg(count));
    if (statsTermsCountLabel_) {
        statsTermsCountLabel_->setText(QStringLiteral("%1 thuật ngữ").arg(count));
    }
}

void MainWindow::setWordDetail(const QString& title, const QString& definition,
                               const QString& partOfSpeech, const QString& example,
                               const QString& category, const QStringList& relatedWords) {
    if (title.isEmpty()) {
        wordDetailTextEdit->setHtml(QStringLiteral(
            "<div style='color:#64748B; padding:40px 16px; text-align:center; font-family:\"Segoe UI\", sans-serif;'>"
            "<div style='font-size:16px; font-weight:600; color:#334155; margin-bottom:8px;'>Chọn một thuật ngữ</div>"
            "<div style='font-size:13px;'>Chọn một thuật ngữ trong danh sách để xem nghĩa tiếng Việt và ví dụ chi tiết.</div>"
            "</div>"));
        return;
    }

    QString html;
    html.reserve(1100);
    html += QStringLiteral(
        "<div style='font-family:Segoe UI; color:#102440;'>"
        "<p style='font-size:21pt; font-weight:700; margin:2px 0 6px 0;'>%1</p>")
        .arg(title.toHtmlEscaped());

    QStringList metadata;
    if (!partOfSpeech.isEmpty()) metadata.append(partOfSpeech.toHtmlEscaped());
    if (!category.isEmpty()) metadata.append(category.toHtmlEscaped());
    if (!metadata.isEmpty()) {
        html += QStringLiteral(
            "<p style='color:#1D4ED8; font-size:10pt; font-weight:600; margin:0 0 16px 0;'>%1</p>")
            .arg(metadata.join(QStringLiteral(" &nbsp; · &nbsp; ")));
    }

    html += QStringLiteral(
        "<p style='color:#475569; font-size:9pt; font-weight:600; margin:10px 0 5px 0;'>"
        "NGHĨA TIẾNG VIỆT</p>"
        "<table width='100%' cellspacing='0' cellpadding='10'><tr>"
        "<td bgcolor='#EAF7EE'><span style='color:#14532D; font-size:13pt; font-weight:700;'>"
        "%1</span></td></tr></table>")
        .arg(definition.toHtmlEscaped());

    if (!example.isEmpty()) {
        html += QStringLiteral(
            "<p style='color:#475569; font-size:9pt; font-weight:600; margin:16px 0 5px 0;'>"
            "VÍ DỤ TIẾNG ANH</p>"
            "<table width='100%' cellspacing='0' cellpadding='10'><tr>"
            "<td bgcolor='#F1F5F9'><span style='color:#334155; font-size:11pt; font-style:italic;'>"
            "%1</span></td></tr></table>")
            .arg(example.toHtmlEscaped());
    }

    if (!relatedWords.isEmpty()) {
        QStringList safeRelated;
        for (const auto& word : relatedWords) safeRelated.append(word.toHtmlEscaped());
        html += QStringLiteral(
            "<p style='color:#475569; font-size:9pt; font-weight:600; margin:16px 0 5px 0;'>"
            "THUẬT NGỮ LIÊN QUAN</p>"
            "<p style='color:#1D4ED8; font-size:10pt; margin:0;'>%1</p>")
            .arg(safeRelated.join(QStringLiteral(" &nbsp; · &nbsp; ")));
    }

    html += QStringLiteral("</div>");

    wordDetailTextEdit->setHtml(html);
    animateDetailTransition();
}

QString MainWindow::selectedWord() const {
    const auto* item = wordListWidget->currentItem();
    return item ? item->text() : QString{};
}

void MainWindow::setInteractionEnabled(bool enabled) {
    interactionEnabled_ = enabled;
    for (auto* button : filterButtons_) {
        button->setEnabled(enabled);
    }
    btnAdd->setEnabled(enabled);
    const bool selected = (wordListWidget->currentItem() != nullptr);
    btnEdit->setEnabled(enabled && selected);
    btnDelete->setEnabled(enabled && selected);
}

void MainWindow::setActiveFilter(const QString& filter) {
    for (int i = 0; i < filterButtons_.size(); ++i) {
        auto* button = filterButtons_[i];
        const bool match = (button->property("filterKey").toString() == filter);
        button->setChecked(match);
        if (i == 0) {
            button->setIcon(IconHelper::gridIcon(Qt::white, 16));
        } else {
            button->setIcon(IconHelper::folderIcon(match ? Qt::white : QColor(QStringLiteral("#94A3B8")), 16));
        }
    }
}

QVBoxLayout* MainWindow::getSearchContainerLayout() const {
    return searchContainerLayout;
}
