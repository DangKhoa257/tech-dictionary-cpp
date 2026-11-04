#include "main_window.h"

#include <QFile>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("Tech Dictionary"));
    setMinimumSize(760, 520);
    resize(1080, 700);

    QFile styleFile(QStringLiteral(":/styles.qss"));
    if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setStyleSheet(QString::fromUtf8(styleFile.readAll()));
    }

    auto* root = new QWidget(this);
    root->setObjectName(QStringLiteral("root"));
    setCentralWidget(root);

    auto* rootLayout = new QHBoxLayout(root);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto* sidebar = new QFrame(root);
    sidebar->setObjectName(QStringLiteral("sidebar"));
    sidebar->setFixedWidth(220);
    auto* sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(20, 28, 20, 24);
    sidebarLayout->setSpacing(10);

    auto* brand = new QLabel(QStringLiteral("TECH\nDICTIONARY"), sidebar);
    brand->setObjectName(QStringLiteral("brand"));
    sidebarLayout->addWidget(brand);
    sidebarLayout->addSpacing(38);

    auto* navigationLabel = new QLabel(QStringLiteral("NAVIGATION"), sidebar);
    navigationLabel->setObjectName(QStringLiteral("eyebrow"));
    sidebarLayout->addWidget(navigationLabel);

    const QString navNames[] = {
        QStringLiteral("Dictionary · Soon"),
        QStringLiteral("A–Z · Soon"),
        QStringLiteral("Settings · Soon")
    };
    for (const QString& name : navNames) {
        auto* button = new QPushButton(name, sidebar);
        button->setObjectName(QStringLiteral("navButton"));
        button->setEnabled(false);
        sidebarLayout->addWidget(button);
    }
    sidebarLayout->addStretch();

    auto* version = new QLabel(QStringLiteral("Version 0.1.0 · Foundation"), sidebar);
    version->setObjectName(QStringLiteral("sidebarFooter"));
    sidebarLayout->addWidget(version);
    rootLayout->addWidget(sidebar);

    auto* content = new QWidget(root);
    content->setObjectName(QStringLiteral("content"));
    auto* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(48, 42, 48, 48);
    contentLayout->setSpacing(24);

    auto* sectionLabel = new QLabel(QStringLiteral("ENGLISH ↔ VIETNAMESE IT TERMS"), content);
    sectionLabel->setObjectName(QStringLiteral("eyebrow"));
    contentLayout->addWidget(sectionLabel);

    auto* heading = new QLabel(QStringLiteral("Your technical vocabulary,\nall in one place."), content);
    heading->setObjectName(QStringLiteral("heading"));
    heading->setWordWrap(true);
    contentLayout->addWidget(heading);

    auto* description = new QLabel(
        QString::fromUtf8("Tra cứu thuật ngữ công nghệ thông tin Anh–Việt."), content);
    description->setObjectName(QStringLiteral("description"));
    description->setWordWrap(true);
    contentLayout->addWidget(description);

    auto* searchField = new QLineEdit(content);
    searchField->setObjectName(QStringLiteral("searchField"));
    searchField->setPlaceholderText(QStringLiteral("Search for a term (coming soon)"));
    searchField->setEnabled(false);
    contentLayout->addWidget(searchField);

    auto* card = new QFrame(content);
    card->setObjectName(QStringLiteral("welcomeCard"));
    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(32, 30, 32, 30);
    cardLayout->setSpacing(12);
    auto* cardTag = new QLabel(QStringLiteral("WELCOME"), card);
    cardTag->setObjectName(QStringLiteral("eyebrow"));
    cardLayout->addWidget(cardTag);
    auto* cardTitle = new QLabel(QStringLiteral("A dictionary built for learning."), card);
    cardTitle->setObjectName(QStringLiteral("cardTitle"));
    cardTitle->setWordWrap(true);
    cardLayout->addWidget(cardTitle);
    auto* cardText = new QLabel(
        QStringLiteral("The dictionary, search, editing and file storage modules are in development. "
                       "This first version establishes the application layout."), card);
    cardText->setObjectName(QStringLiteral("cardText"));
    cardText->setWordWrap(true);
    cardLayout->addWidget(cardText);
    contentLayout->addWidget(card);
    contentLayout->addStretch();
    rootLayout->addWidget(content, 1);
}
