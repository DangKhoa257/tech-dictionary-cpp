#include <QApplication>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

#include <iostream>

#include "main_window.h"
#include "search_widget.h"

int main(int argc, char* argv[]) {
    QApplication application(argc, argv);
    int failures = 0;
    const auto check = [&failures](bool condition, const char* name) {
        if (!condition) {
            std::cerr << "Failed: " << name << '\n';
            ++failures;
        }
    };

    MainWindow window;
    check(window.windowTitle() == QStringLiteral("Tech Dictionary"), "window title");
    check(window.minimumWidth() >= 760 && window.minimumHeight() >= 520,
          "minimum size");
    check(window.width() >= window.minimumWidth(), "default size");
    QFile style(QStringLiteral(":/styles.qss"));
    check(style.open(QIODevice::ReadOnly) && !window.styleSheet().isEmpty() &&
          window.styleSheet().contains(QStringLiteral("#2563EB")),
          "embedded stylesheet applied");

    auto* list = window.findChild<QListWidget*>(QStringLiteral("wordList"));
    auto* detail = window.findChild<QTextEdit*>(QStringLiteral("wordDetail"));
    auto* add = window.findChild<QPushButton*>(QStringLiteral("addButton"));
    auto* edit = window.findChild<QPushButton*>(QStringLiteral("editButton"));
    auto* remove = window.findChild<QPushButton*>(QStringLiteral("deleteButton"));
    check(list && detail && add && edit && remove, "main controls exist");
    if (!list || !detail || !add || !edit || !remove) return 1;
    check(!add->isEnabled() && !edit->isEnabled() && !remove->isEnabled(),
          "unwired actions disabled");

    QStringList filters;
    QString selected;
    int adds = 0, edits = 0, deletes = 0;
    QObject::connect(&window, &MainWindow::filterRequested, &window,
                     [&filters](const QString& value) { filters.append(value); });
    QObject::connect(&window, &MainWindow::wordSelected, &window,
                     [&selected](const QString& value) { selected = value; });
    QObject::connect(&window, &MainWindow::addWordRequested, &window,
                     [&adds]() { ++adds; });
    QObject::connect(&window, &MainWindow::editWordRequested, &window,
                     [&edits]() { ++edits; });
    QObject::connect(&window, &MainWindow::deleteWordRequested, &window,
                     [&deletes]() { ++deletes; });

    window.setInteractionEnabled(true);
    const auto nav = window.findChildren<QPushButton*>(QStringLiteral("navButton"));
    check(nav.size() == 6 && nav[0]->isChecked(), "initial filter highlight");
    for (QPushButton* button : nav) button->click();
    check(filters == QStringList({"All Words", "A - E", "F - J", "K - O",
                                  "P - T", "U - Z"}), "filter signals");
    check(nav.size() == 6 && nav[5]->isChecked() && !nav[0]->isChecked(), "selected filter highlight");
    add->click();
    check(adds == 1 && !edit->isEnabled() && !remove->isEnabled(),
          "add signal and selection guards");

    window.setWordList({QStringLiteral("<vector>"), QStringLiteral("Algorithm")});
    check(list->count() == 2, "list population");
    list->setCurrentRow(0);
    check(selected == QStringLiteral("<vector>") &&
          window.selectedWord() == QStringLiteral("<vector>"), "selection signal");
    edit->click();
    remove->click();
    check(edits == 1 && deletes == 1, "edit and delete signals");

    window.setWordDetail(QStringLiteral("<vector> A < B & \"quoted\""),
                         QStringLiteral("<b>Xin & chào</b>"));
    check(detail->toPlainText().contains(QStringLiteral("<vector> A < B & \"quoted\"")) &&
          detail->toPlainText().contains(QStringLiteral("<b>Xin & chào</b>")),
          "literal UTF-8 detail rendering");
    window.setWordList({QStringLiteral("New")});
    check(list->count() == 1 && window.selectedWord().isEmpty() &&
          detail->toPlainText().contains(QStringLiteral("Chọn một thuật ngữ")),
          "replacement clears stale detail");
    window.setWordList({});
    check(list->count() == 0 && !edit->isEnabled() && !remove->isEnabled(),
          "empty list disables selection actions");
    auto* empty = window.findChild<QLabel*>(QStringLiteral("emptyState"));
    auto* total = window.findChild<QLabel*>(QStringLiteral("totalWordCount"));
    window.setTotalWordCount(288);
    check(empty && !empty->isHidden() && total && total->text().contains(QStringLiteral("288")),
          "empty state and total count");

    auto* search = new SearchWidget(&window);
    window.getSearchContainerLayout()->addWidget(search);
    check(search->parentWidget() != nullptr &&
          window.getSearchContainerLayout()->count() == 1,
          "TV3 search widget container");
    auto* input = search->findChild<QLineEdit*>(QStringLiteral("searchInput"));
    auto* searchButton = search->findChild<QPushButton*>(QStringLiteral("searchButton"));
    check(input && searchButton && input->placeholderText().contains(QStringLiteral("C++")),
          "search interface");
    int requests = 0;
    QObject::connect(search, &SearchWidget::searchRequested, search,
                     [&requests](const QString& term) { if (term == QStringLiteral("<vector>")) ++requests; });
    if (input && searchButton) { input->setText(QStringLiteral("<vector>")); searchButton->click(); }
    check(requests == 1, "search request preserves special characters");
    window.resize(800, 560);
    check(window.width() >= window.minimumWidth() && window.height() >= window.minimumHeight(),
          "minimum window resize");
    if (qEnvironmentVariableIsSet("TECH_DICTIONARY_UI_SCREENSHOT")) {
        window.setWordList({QStringLiteral("Algorithm"), QStringLiteral("C++"), QStringLiteral("<vector>")});
        list->setCurrentRow(1);
        window.setWordDetail(QStringLiteral("C++"), QStringLiteral("Ngôn ngữ lập trình"));
        window.resize(1120, 730);
        window.show();
        QApplication::processEvents();
        check(window.grab().save(qEnvironmentVariable("TECH_DICTIONARY_UI_SCREENSHOT")),
              "offscreen UI screenshot");
    }
    return failures == 0 ? 0 : 1;
}
