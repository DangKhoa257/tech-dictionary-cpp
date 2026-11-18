#include "main_window.h"

#include <QFile>
#include <QListWidgetItem>
#include <QSplitter>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) { setupUi(); }
MainWindow::~MainWindow() = default;

void MainWindow::setupUi() {
    setWindowTitle(QStringLiteral("Tech Dictionary"));
    setMinimumSize(800, 560);
    resize(1120, 730);
    QFile styleFile(QStringLiteral(":/styles.qss"));
    if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text))
        setStyleSheet(QString::fromUtf8(styleFile.readAll()));

    centralWidget = new QWidget(this);
    centralWidget->setObjectName(QStringLiteral("root"));
    setCentralWidget(centralWidget);
    auto* mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    sidebarFrame = new QFrame(centralWidget);
    sidebarFrame->setObjectName(QStringLiteral("sidebar"));
    sidebarFrame->setFixedWidth(218);
    auto* sidebarLayout = new QVBoxLayout(sidebarFrame);
    sidebarLayout->setContentsMargins(18, 26, 18, 20);
    sidebarLayout->setSpacing(8);
    brandLabel = new QLabel(QStringLiteral("Tech Dictionary"), sidebarFrame);
    brandLabel->setObjectName(QStringLiteral("brand"));
    sidebarLayout->addWidget(brandLabel);
    auto* tagline = new QLabel(QStringLiteral("TỪ ĐIỂN CÔNG NGHỆ"), sidebarFrame);
    tagline->setObjectName(QStringLiteral("brandTagline"));
    sidebarLayout->addWidget(tagline);
    sidebarLayout->addSpacing(28);
    eyebrowLabel = new QLabel(QStringLiteral("DUYỆT THUẬT NGỮ"), sidebarFrame);
    eyebrowLabel->setObjectName(QStringLiteral("eyebrow"));
    sidebarLayout->addWidget(eyebrowLabel);

    const QStringList filters = {QStringLiteral("All Words"), QStringLiteral("A - E"),
        QStringLiteral("F - J"), QStringLiteral("K - O"), QStringLiteral("P - T"),
        QStringLiteral("U - Z")};
    for (const QString& filter : filters) {
        auto* button = new QPushButton(filter == QLatin1String("All Words")
            ? QStringLiteral("Tất cả từ") : filter, sidebarFrame);
        button->setObjectName(QStringLiteral("navButton"));
        button->setProperty("filterKey", filter);
        button->setCheckable(true);
        button->setChecked(filter == QLatin1String("All Words"));
        button->setEnabled(false);
        button->setCursor(Qt::PointingHandCursor);
        filterButtons_.append(button);
        connect(button, &QPushButton::clicked, this, &MainWindow::onNavButtonClicked);
        sidebarLayout->addWidget(button);
    }
    sidebarLayout->addStretch();
    totalWordCountLabel_ = new QLabel(QStringLiteral("Tổng cộng: 0 từ"), sidebarFrame);
    totalWordCountLabel_->setObjectName(QStringLiteral("totalWordCount"));
    sidebarLayout->addWidget(totalWordCountLabel_);
    sidebarFooterLabel = new QLabel(QStringLiteral("Phiên bản 0.2.0"), sidebarFrame);
    sidebarFooterLabel->setObjectName(QStringLiteral("sidebarFooter"));
    sidebarLayout->addWidget(sidebarFooterLabel);
    mainLayout->addWidget(sidebarFrame);

    contentWidget = new QWidget(centralWidget);
    contentWidget->setObjectName(QStringLiteral("content"));
    auto* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(28, 24, 28, 24);
    contentLayout->setSpacing(18);
    auto* headerLayout = new QHBoxLayout();
    auto* titleLayout = new QVBoxLayout();
    titleLayout->setSpacing(4);
    headingLabel = new QLabel(QStringLiteral("Từ điển thuật ngữ CNTT"), contentWidget);
    headingLabel->setObjectName(QStringLiteral("heading"));
    titleLayout->addWidget(headingLabel);
    descriptionLabel = new QLabel(QStringLiteral("Tra cứu và quản lý thuật ngữ công nghệ Anh – Việt"), contentWidget);
    descriptionLabel->setObjectName(QStringLiteral("description"));
    descriptionLabel->setWordWrap(true);
    titleLayout->addWidget(descriptionLabel);
    headerLayout->addLayout(titleLayout, 1);
    btnAdd = new QPushButton(QStringLiteral("+  Thêm từ mới"), contentWidget);
    btnAdd->setObjectName(QStringLiteral("addButton"));
    btnAdd->setEnabled(false);
    btnAdd->setCursor(Qt::PointingHandCursor);
    connect(btnAdd, &QPushButton::clicked, this, &MainWindow::onAddClicked);
    headerLayout->addWidget(btnAdd, 0, Qt::AlignTop);
    contentLayout->addLayout(headerLayout);

    auto* searchContainer = new QFrame(contentWidget);
    searchContainer->setObjectName(QStringLiteral("searchCard"));
    searchContainerLayout = new QVBoxLayout(searchContainer);
    searchContainerLayout->setContentsMargins(12, 10, 12, 10);
    contentLayout->addWidget(searchContainer);

    auto* splitter = new QSplitter(Qt::Horizontal, contentWidget);
    splitter->setObjectName(QStringLiteral("contentSplitter"));
    splitter->setChildrenCollapsible(false);
    auto* listCard = new QFrame(splitter);
    listCard->setObjectName(QStringLiteral("listCard"));
    auto* listLayout = new QVBoxLayout(listCard);
    listLayout->setContentsMargins(18, 18, 18, 18);
    listLayout->setSpacing(12);
    auto* listHeader = new QHBoxLayout();
    auto* listTitle = new QLabel(QStringLiteral("Danh sách thuật ngữ"), listCard);
    listTitle->setObjectName(QStringLiteral("panelTitle"));
    listHeader->addWidget(listTitle);
    listHeader->addStretch();
    visibleCountLabel_ = new QLabel(QStringLiteral("0 từ"), listCard);
    visibleCountLabel_->setObjectName(QStringLiteral("visibleCount"));
    listHeader->addWidget(visibleCountLabel_);
    listLayout->addLayout(listHeader);
    emptyStateLabel_ = new QLabel(QStringLiteral("Chưa có thuật ngữ trong mục này.\nHãy thêm từ mới hoặc mở một tệp từ điển."), listCard);
    emptyStateLabel_->setObjectName(QStringLiteral("emptyState"));
    emptyStateLabel_->setAlignment(Qt::AlignCenter);
    emptyStateLabel_->setWordWrap(true);
    listLayout->addWidget(emptyStateLabel_, 1);
    wordListWidget = new QListWidget(listCard);
    wordListWidget->setObjectName(QStringLiteral("wordList"));
    wordListWidget->setAlternatingRowColors(false);
    wordListWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    connect(wordListWidget, &QListWidget::itemSelectionChanged, this, &MainWindow::onItemSelectionChanged);
    listLayout->addWidget(wordListWidget, 1);

    auto* detailCard = new QFrame(splitter);
    detailCard->setObjectName(QStringLiteral("detailCard"));
    auto* detailLayout = new QVBoxLayout(detailCard);
    detailLayout->setContentsMargins(22, 18, 22, 18);
    detailLayout->setSpacing(12);
    auto* detailTitle = new QLabel(QStringLiteral("CHI TIẾT THUẬT NGỮ"), detailCard);
    detailTitle->setObjectName(QStringLiteral("panelEyebrow"));
    detailLayout->addWidget(detailTitle);
    wordDetailTextEdit = new QTextEdit(detailCard);
    wordDetailTextEdit->setObjectName(QStringLiteral("wordDetail"));
    wordDetailTextEdit->setReadOnly(true);
    wordDetailTextEdit->setFocusPolicy(Qt::NoFocus);
    detailLayout->addWidget(wordDetailTextEdit, 1);
    auto* actionLayout = new QHBoxLayout();
    btnEdit = new QPushButton(QStringLiteral("Sửa nghĩa"), detailCard);
    btnDelete = new QPushButton(QStringLiteral("Xóa từ"), detailCard);
    btnEdit->setObjectName(QStringLiteral("editButton"));
    btnDelete->setObjectName(QStringLiteral("deleteButton"));
    btnEdit->setEnabled(false);
    btnDelete->setEnabled(false);
    btnEdit->setCursor(Qt::PointingHandCursor);
    btnDelete->setCursor(Qt::PointingHandCursor);
    connect(btnEdit, &QPushButton::clicked, this, &MainWindow::onEditClicked);
    connect(btnDelete, &QPushButton::clicked, this, &MainWindow::onDeleteClicked);
    actionLayout->addWidget(btnEdit);
    actionLayout->addWidget(btnDelete);
    actionLayout->addStretch();
    detailLayout->addLayout(actionLayout);
    splitter->addWidget(listCard);
    splitter->addWidget(detailCard);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 3);
    splitter->setSizes({330, 500});
    contentLayout->addWidget(splitter, 1);
    mainLayout->addWidget(contentWidget, 1);
    setWordDetail({}, {});
    setWordList({});
}

void MainWindow::onNavButtonClicked() {
    auto* button = qobject_cast<QPushButton*>(sender());
    if (!button) return;
    for (auto* nav : filterButtons_) nav->setChecked(nav == button);
    emit filterRequested(button->property("filterKey").toString());
}

void MainWindow::onAddClicked() { emit addWordRequested(); }
void MainWindow::onEditClicked() { emit editWordRequested(); }
void MainWindow::onDeleteClicked() { emit deleteWordRequested(); }

void MainWindow::onItemSelectionChanged() {
    auto* item = wordListWidget->currentItem();
    btnEdit->setEnabled(interactionEnabled_ && item);
    btnDelete->setEnabled(interactionEnabled_ && item);
    if (item) emit wordSelected(item->text());
    else setWordDetail({}, {});
}

void MainWindow::setWordList(const QStringList& words) {
    wordListWidget->clear();
    wordListWidget->addItems(words);
    visibleCountLabel_->setText(QStringLiteral("%1 từ").arg(words.size()));
    emptyStateLabel_->setVisible(words.isEmpty());
    wordListWidget->setVisible(!words.isEmpty());
    onItemSelectionChanged();
}

void MainWindow::setTotalWordCount(int count) {
    totalWordCountLabel_->setText(QStringLiteral("Tổng cộng: %1 từ").arg(count));
}

void MainWindow::setWordDetail(const QString& title, const QString& definition,
                               const QString& partOfSpeech, const QString& example,
                               const QString& category) {
    if (title.isEmpty()) {
        wordDetailTextEdit->setHtml(QStringLiteral("<div style='color:#64748B; margin-top:26px; font-size:14px;'>Chọn một thuật ngữ trong danh sách để xem nghĩa tiếng Việt.</div>"));
        return;
    }
    QString html = QStringLiteral(
        "<div style='font-family:Segoe UI; color:#172B4D;'>"
        "<h2 style='font-size:25px; margin-bottom:10px;'>%1</h2>")
        .arg(title.toHtmlEscaped());
    if (!partOfSpeech.isEmpty())
        html += QStringLiteral("<p style='font-size:13px; color:#526783;'>%1</p>")
                    .arg(partOfSpeech.toHtmlEscaped());
    html += QStringLiteral(
        "<div style='font-size:12px; color:#64748B;'>NGHĨA TIẾNG VIỆT</div>"
        "<p style='font-size:16px; line-height:1.5;'>%1</p>")
        .arg(definition.toHtmlEscaped());
    if (!example.isEmpty())
        html += QStringLiteral(
            "<div style='font-size:12px; color:#64748B;'>VÍ DỤ TIẾNG ANH</div>"
            "<p style='font-size:15px; line-height:1.5;'>%1</p>")
            .arg(example.toHtmlEscaped());
    if (!category.isEmpty())
        html += QStringLiteral("<p style='font-size:13px; color:#526783;'>CHỦ ĐỀ · %1</p>")
                    .arg(category.toHtmlEscaped());
    wordDetailTextEdit->setHtml(html + QStringLiteral("</div>"));
}

QString MainWindow::selectedWord() const {
    const auto* item = wordListWidget->currentItem();
    return item ? item->text() : QString{};
}

void MainWindow::setInteractionEnabled(bool enabled) {
    interactionEnabled_ = enabled;
    for (auto* button : filterButtons_) button->setEnabled(enabled);
    btnAdd->setEnabled(enabled);
    const bool selected = wordListWidget->currentItem() != nullptr;
    btnEdit->setEnabled(enabled && selected);
    btnDelete->setEnabled(enabled && selected);
}

void MainWindow::setActiveFilter(const QString& filter) {
    for (auto* button : filterButtons_)
        button->setChecked(button->property("filterKey").toString() == filter);
}

QVBoxLayout* MainWindow::getSearchContainerLayout() const { return searchContainerLayout; }
