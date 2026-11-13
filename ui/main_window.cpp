#include "main_window.h"

#include <QFile>
#include <QListWidgetItem>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setupUi();
}

MainWindow::~MainWindow() {}

void MainWindow::setupUi() {
    setWindowTitle(QStringLiteral("Tech Dictionary"));
    setMinimumSize(800, 560);
    resize(1100, 720);
    QFile styleFile(QStringLiteral(":/styles.qss"));
    if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setStyleSheet(QString::fromUtf8(styleFile.readAll()));
    }
    // Root Widget (Khớp với styles.qss)
    centralWidget = new QWidget(this);
    centralWidget->setObjectName("root");
    setCentralWidget(centralWidget);

    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ==========================================
    // 1. SIDEBAR BÊN TRÁI (Kèm setObjectName)
    // ==========================================
    sidebarFrame = new QFrame(this);
    sidebarFrame->setObjectName("sidebar");
    sidebarFrame->setFixedWidth(240);

    QVBoxLayout *sidebarLayout = new QVBoxLayout(sidebarFrame);
    sidebarLayout->setContentsMargins(16, 20, 16, 20);

    brandLabel = new QLabel("TECH DICT", sidebarFrame);
    brandLabel->setObjectName("brand");
    sidebarLayout->addWidget(brandLabel);

    eyebrowLabel = new QLabel("BỘ LỌC A-Z", sidebarFrame);
    eyebrowLabel->setObjectName("eyebrow");
    sidebarLayout->addWidget(eyebrowLabel);

    // Nút Filter A-Z
    QStringList filters = {"All Words", "A - E", "F - J", "K - O", "P - T", "U - Z"};
    for (const QString &filter : filters) {
        QPushButton *btn = new QPushButton(filter, sidebarFrame);
        btn->setObjectName("navButton");
        btn->setEnabled(false);
        filterButtons_.append(btn);
        connect(btn, &QPushButton::clicked, this, &MainWindow::onNavButtonClicked);
        sidebarLayout->addWidget(btn);
    }

    sidebarLayout->addStretch();

    sidebarFooterLabel = new QLabel("v0.1.0 - Technical Dictionary", sidebarFrame);
    sidebarFooterLabel->setObjectName("sidebarFooter");
    sidebarLayout->addWidget(sidebarFooterLabel);

    mainLayout->addWidget(sidebarFrame);

    // ==========================================
    // 2. VÙNG NỘI DUNG CHÍNH (BÊN PHẢI)
    // ==========================================
    contentWidget = new QWidget(this);
    contentWidget->setObjectName("content");
    QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(20, 20, 20, 20);
    contentLayout->setSpacing(15);

    headingLabel = new QLabel("Từ Điển Thuật Ngữ Công Nghệ", contentWidget);
    headingLabel->setObjectName("heading");
    headingLabel->setWordWrap(true);
    contentLayout->addWidget(headingLabel);

    descriptionLabel = new QLabel("Tra cứu và quản lý thuật ngữ chuyên ngành CNTT", contentWidget);
    descriptionLabel->setObjectName("description");
    descriptionLabel->setWordWrap(true);
    contentLayout->addWidget(descriptionLabel);

    // VÙNG NHÚNG SEARCH WIDGET CỦA TV3
    QWidget *searchContainer = new QWidget(contentWidget);
    searchContainerLayout = new QVBoxLayout(searchContainer);
    searchContainerLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->addWidget(searchContainer);

    // VÙNG CHỨA DANH SÁCH & CHI TIẾT
    QHBoxLayout *dataLayout = new QHBoxLayout();
    
    wordListWidget = new QListWidget(contentWidget);
    wordListWidget->setObjectName("wordList");
    connect(wordListWidget, &QListWidget::itemSelectionChanged, this, &MainWindow::onItemSelectionChanged);
    dataLayout->addWidget(wordListWidget, 1);

    wordDetailTextEdit = new QTextEdit(contentWidget);
    wordDetailTextEdit->setObjectName("wordDetail");
    wordDetailTextEdit->setReadOnly(true);
    wordDetailTextEdit->setPlainText(QStringLiteral("Select a word to view its meaning."));
    dataLayout->addWidget(wordDetailTextEdit, 2);

    contentLayout->addLayout(dataLayout, 1);

    // NÚT THAO TÁC THÊM / SỬA / XÓA (KẾT NỐI VỚI TV4)
    QHBoxLayout *actionLayout = new QHBoxLayout();
    btnAdd = new QPushButton("Thêm từ mới", contentWidget);
    btnEdit = new QPushButton("Sửa từ", contentWidget);
    btnDelete = new QPushButton("Xóa từ", contentWidget);
    btnAdd->setObjectName("addButton");
    btnEdit->setObjectName("editButton");
    btnDelete->setObjectName("deleteButton");
    btnAdd->setEnabled(false);
    btnEdit->setEnabled(false);
    btnDelete->setEnabled(false);

    connect(btnAdd, &QPushButton::clicked, this, &MainWindow::onAddClicked);
    connect(btnEdit, &QPushButton::clicked, this, &MainWindow::onEditClicked);
    connect(btnDelete, &QPushButton::clicked, this, &MainWindow::onDeleteClicked);

    actionLayout->addWidget(btnAdd);
    actionLayout->addWidget(btnEdit);
    actionLayout->addWidget(btnDelete);
    actionLayout->addStretch();

    contentLayout->addLayout(actionLayout);
    mainLayout->addWidget(contentWidget, 1);
}

void MainWindow::onNavButtonClicked() {
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (btn) {
        emit filterRequested(btn->text());
    }
}

void MainWindow::onAddClicked() { emit addWordRequested(); }
void MainWindow::onEditClicked() { emit editWordRequested(); }
void MainWindow::onDeleteClicked() { emit deleteWordRequested(); }

void MainWindow::onItemSelectionChanged() {
    QListWidgetItem *item = wordListWidget->currentItem();
    btnEdit->setEnabled(interactionEnabled_ && item != nullptr);
    btnDelete->setEnabled(interactionEnabled_ && item != nullptr);
    wordDetailTextEdit->setPlainText(QStringLiteral("Select a word to view its meaning."));
    if (item) {
        emit wordSelected(item->text());
    }
}

void MainWindow::setWordList(const QStringList &words) {
    wordListWidget->clear();
    wordListWidget->addItems(words);
    onItemSelectionChanged();
}

void MainWindow::setWordDetail(const QString &title, const QString &definition) {
    if (title.isEmpty() && definition.isEmpty()) {
        wordDetailTextEdit->setPlainText(QStringLiteral("Select a word to view its meaning."));
        return;
    }
    wordDetailTextEdit->setHtml(QStringLiteral("<h2>%1</h2><p>%2</p>")
        .arg(title.toHtmlEscaped(), definition.toHtmlEscaped()));
}

QString MainWindow::selectedWord() const {
    const QListWidgetItem* item = wordListWidget->currentItem();
    return item == nullptr ? QString{} : item->text();
}

void MainWindow::setInteractionEnabled(bool enabled) {
    interactionEnabled_ = enabled;
    for (QPushButton* button : filterButtons_) button->setEnabled(enabled);
    btnAdd->setEnabled(enabled);
    const bool hasSelection = wordListWidget->currentItem() != nullptr;
    btnEdit->setEnabled(enabled && hasSelection);
    btnDelete->setEnabled(enabled && hasSelection);
}

QVBoxLayout* MainWindow::getSearchContainerLayout() const {
    return searchContainerLayout;
}
