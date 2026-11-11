#include "main_window.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setupUi();
}

MainWindow::~MainWindow() {}

void MainWindow::setupUi() {
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
        connect(btn, &QPushButton::clicked, this, &MainWindow::onNavButtonClicked);
        sidebarLayout->addWidget(btn);
    }

    sidebarLayout->addStretch();

    sidebarFooterLabel = new QLabel("v1.0 - Technical Dictionary", sidebarFrame);
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
    contentLayout->addWidget(headingLabel);

    descriptionLabel = new QLabel("Tra cứu và quản lý thuật ngữ chuyên ngành CNTT", contentWidget);
    descriptionLabel->setObjectName("description");
    contentLayout->addWidget(descriptionLabel);

    // VÙNG NHÚNG SEARCH WIDGET CỦA TV3
    QWidget *searchContainer = new QWidget(contentWidget);
    searchContainerLayout = new QVBoxLayout(searchContainer);
    searchContainerLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->addWidget(searchContainer);

    // VÙNG CHỨA DANH SÁCH & CHI TIẾT
    QHBoxLayout *dataLayout = new QHBoxLayout();
    
    wordListWidget = new QListWidget(contentWidget);
    connect(wordListWidget, &QListWidget::itemSelectionChanged, this, &MainWindow::onItemSelectionChanged);
    dataLayout->addWidget(wordListWidget, 1);

    wordDetailTextEdit = new QTextEdit(contentWidget);
    wordDetailTextEdit->setReadOnly(true);
    dataLayout->addWidget(wordDetailTextEdit, 2);

    contentLayout->addLayout(dataLayout);

    // NÚT THAO TÁC THÊM / SỬA / XÓA (KẾT NỐI VỚI TV4)
    QHBoxLayout *actionLayout = new QHBoxLayout();
    btnAdd = new QPushButton("Thêm từ mới", contentWidget);
    btnEdit = new QPushButton("Sửa từ", contentWidget);
    btnDelete = new QPushButton("Xóa từ", contentWidget);

    connect(btnAdd, &QPushButton::clicked, this, &MainWindow::onAddClicked);
    connect(btnEdit, &QPushButton::clicked, this, &MainWindow::onEditClicked);
    connect(btnDelete, &QPushButton::clicked, this, &MainWindow::onDeleteClicked);

    actionLayout->addWidget(btnAdd);
    actionLayout->addWidget(btnEdit);
    actionLayout->addWidget(btnDelete);
    actionLayout->addStretch();

    contentLayout->addLayout(actionLayout);
    mainLayout->addWidget(contentWidget);
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
    if (item) {
        emit wordSelected(item->text());
    }
}

void MainWindow::setWordList(const QStringList &words) {
    wordListWidget->clear();
    wordListWidget->addItems(words);
}

void MainWindow::setWordDetail(const QString &title, const QString &definition) {
    wordDetailTextEdit->setHtml(QString("<h2>%1</h2><p>%2</p>").arg(title, definition));
}

QVBoxLayout* MainWindow::getSearchContainerLayout() const {
    return searchContainerLayout;
}
