#include <QApplication>
#include <QFile>
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
    check(style.open(QIODevice::ReadOnly) && !window.styleSheet().isEmpty(),
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
    for (QPushButton* button : nav) button->click();
    check(filters == QStringList({"All Words", "A - E", "F - J", "K - O",
                                  "P - T", "U - Z"}), "filter signals");
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
          detail->toPlainText().contains(QStringLiteral("Select a word")),
          "replacement clears stale detail");
    window.setWordList({});
    check(list->count() == 0 && !edit->isEnabled() && !remove->isEnabled(),
          "empty list disables selection actions");

    auto* search = new SearchWidget(&window);
    window.getSearchContainerLayout()->addWidget(search);
    check(search->parentWidget() != nullptr &&
          window.getSearchContainerLayout()->count() == 1,
          "TV3 search widget container");
    return failures == 0 ? 0 : 1;
}
