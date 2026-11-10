#include "main_window.h"

#include <QFile>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("Tech Dictionary"));
    setMinimumSize(760, 520);
    resize(980, 620);

    // Load file QSS của TV5
    QFile styleFile(QStringLiteral(":/styles.qss"));
    if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setStyleSheet(QString::fromUtf8(styleFile.readAll()));
        styleFile.close();
    }

    auto* rootWidget = new QWidget(this);
    setCentralWidget(rootWidget);

    auto* rootLayout = new QHBoxLayout(rootWidget);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // Dựng 2 phần chính: Sidebar bên trái & Content bên phải
    setupSidebar(rootLayout);
    setupMainContent(rootLayout);
}

void MainWindow::setupSidebar(QHBoxLayout* rootLayout) {
    auto* sidebar = new QFrame(this);
    sidebar->setFixedWidth(200);
    
    auto* sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(16, 20, 16, 20);
    sidebarLayout->setSpacing(10);

    auto* brandLabel = new QLabel(QStringLiteral("TECH DICTIONARY"), sidebar);
    sidebarLayout->addWidget(brandLabel);

    auto* filterLabel = new QLabel(QStringLiteral("FILTER A-Z"), sidebar);
    sidebarLayout->addWidget(filterLabel);

    // Bộ lọc A-Z theo yêu cầu TV5
    const QString filters[] = { QStringLiteral("All Words"), QStringLiteral("A - E"), QStringLiteral("F - J"), QStringLiteral("K - O"), QStringLiteral("P - T"), QStringLiteral("U - Z") };
    for (const QString& name : filters) {
        auto* btn = new QPushButton(name, sidebar);
        sidebarLayout->addWidget(btn);
    }

    sidebarLayout->addStretch();
    rootLayout->addWidget(sidebar);
}

void MainWindow::setupMainContent(QHBoxLayout* rootLayout) {
    auto* contentWidget = new QWidget(this);
    auto* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(24, 24, 24, 24);
    contentLayout->setSpacing(16);

    // 1. Vùng dành riêng để nhúng SearchWidget của TV3
    searchField = new QLineEdit(contentWidget);
    searchField->setPlaceholderText(QStringLiteral("Nhập từ vựng cần tìm kiếm (SearchWidget)..."));
    contentLayout->addWidget(searchField);

    // 2. Vùng trung tâm: Danh sách từ (Trái) & Nghĩa (Phải)
    auto* bodyLayout = new QHBoxLayout();

    wordList = new QListWidget(contentWidget);
    wordList->addItem(QStringLiteral("Algorithm"));
    wordList->addItem(QStringLiteral("Database"));
    wordList->addItem(QStringLiteral("Pointer"));
    bodyLayout->addWidget(wordList, 1); // Tỷ lệ 1

    auto* rightPanel = new QWidget(contentWidget);
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    definitionBox = new QTextEdit(rightPanel);
    definitionBox->setReadOnly(true);
    definitionBox->setPlaceholderText(QStringLiteral("Nghĩa tiếng Việt sẽ hiển thị ở đây..."));
    rightLayout->addWidget(definitionBox);

    // 3. Các nút chức năng để kết nối với WordDialog của TV4
    auto* btnLayout = new QHBoxLayout();
    btnAddWord = new QPushButton(QStringLiteral("Thêm từ"), rightPanel);
    btnEditWord = new QPushButton(QStringLiteral("Sửa từ"), rightPanel);
    btnDeleteWord = new QPushButton(QStringLiteral("Xóa từ"), rightPanel);

    btnLayout->addWidget(btnAddWord);
    btnLayout->addWidget(btnEditWord);
    btnLayout->addWidget(btnDeleteWord);
    rightLayout->addLayout(btnLayout);

    bodyLayout->addWidget(rightPanel, 2); // Tỷ lệ 2
    contentLayout->addLayout(bodyLayout);

    rootLayout->addWidget(contentWidget, 1);
}

MainWindow::~MainWindow() {}