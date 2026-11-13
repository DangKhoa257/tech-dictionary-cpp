#include "app_controller.h"
#include "search_widget.h"
#include "word_dialog.h"

#include <QApplication>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QLineEdit>
#include <QListWidget>
#include <QMetaObject>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTimer>

#include <iostream>

int main(int argc, char* argv[]) {
    QApplication application(argc, argv);
    QStandardPaths::setTestModeEnabled(true);
    QTemporaryDir temporary(QDir::current().filePath(QStringLiteral("app-controller-test-XXXXXX")));
    int failures = 0;
    const auto expect = [&failures](bool condition, const char* name) {
        if (!condition) { std::cerr << "Failed: " << name << '\n'; ++failures; }
    };
    expect(temporary.isValid(), "temporary directory");
    const QString path = temporary.filePath(QString::fromUtf8("từ-điển.txt"));
    const QString badPath = temporary.filePath(QStringLiteral("bad.txt"));

    {
        AppController controller(application);
        auto* list = controller.window().findChild<QListWidget*>(QStringLiteral("wordList"));
        expect(list != nullptr, "word list exists");
        expect(controller.searchWord({}).status == ControllerStatus::InvalidInput, "empty search");
        expect(controller.addWord(QStringLiteral("9bad"), QStringLiteral("nghĩa")).status == ControllerStatus::InvalidInput, "invalid key");
        expect(controller.addWord(QStringLiteral("Algorithm"), {}).status == ControllerStatus::InvalidInput, "empty meaning");
        expect(controller.updateWord({}, QStringLiteral("nghĩa")).status == ControllerStatus::InvalidInput, "empty update");
        expect(controller.deleteWord({}).status == ControllerStatus::InvalidInput, "empty delete");
        expect(controller.loadFromFile({}).status == ControllerStatus::InvalidInput, "empty load path");
        expect(controller.saveToFile({}).status == ControllerStatus::InvalidInput, "empty save path");
        expect(controller.searchWord(QStringLiteral("Missing")).status == ControllerStatus::NotFound, "missing search");
        expect(controller.updateWord(QStringLiteral("Missing"), QStringLiteral("nghĩa")).status == ControllerStatus::NotFound, "missing update");
        expect(controller.deleteWord(QStringLiteral("Missing")).status == ControllerStatus::NotFound, "missing delete");

        expect(controller.addWord(QStringLiteral("Algorithm"), QString::fromUtf8("thuật toán")).status == ControllerStatus::Success, "add");
        expect(controller.isDirty(), "dirty after add");
        expect(controller.searchWord(QStringLiteral("algorithm")).vietnamese == QString::fromUtf8("thuật toán"), "case insensitive UTF-8 search");
        expect(list && list->count() == 1 && list->item(0)->text() == QStringLiteral("Algorithm"), "UI list after add");
        expect(controller.addWord(QStringLiteral("ALGORITHM"), QStringLiteral("duplicate")).status == ControllerStatus::InvalidInput, "duplicate");
        expect(controller.updateWord(QStringLiteral("ALGORITHM"), QString::fromUtf8("giải thuật")).status == ControllerStatus::Success, "update");
        expect(controller.searchWord(QStringLiteral("Algorithm")).vietnamese == QString::fromUtf8("giải thuật"), "updated meaning");
        expect(controller.undo().status == ControllerStatus::Success, "undo update");
        expect(controller.searchWord(QStringLiteral("Algorithm")).vietnamese == QString::fromUtf8("thuật toán"), "undo restored meaning");
        expect(controller.redo().status == ControllerStatus::Success, "redo update");
        expect(controller.searchWord(QStringLiteral("Algorithm")).vietnamese == QString::fromUtf8("giải thuật"), "redo restored meaning");

        expect(controller.addWord(QStringLiteral("Kernel"), QString::fromUtf8("nhân hệ điều hành")).status == ControllerStatus::Success, "add K");
        expect(controller.addWord(QStringLiteral("Thread"), QString::fromUtf8("luồng")).status == ControllerStatus::Success, "add T");
        QMetaObject::invokeMethod(&controller.window(), "filterRequested", Q_ARG(QString, QStringLiteral("A - E")));
        expect(list && list->count() == 1 && list->item(0)->text() == QStringLiteral("Algorithm"), "A-E filter");
        QMetaObject::invokeMethod(&controller.window(), "filterRequested", Q_ARG(QString, QStringLiteral("K - O")));
        expect(list && list->count() == 1 && list->item(0)->text() == QStringLiteral("Kernel"), "K-O filter");
        QMetaObject::invokeMethod(&controller.window(), "filterRequested", Q_ARG(QString, QStringLiteral("U - Z")));
        expect(list && list->count() == 0, "empty filter");
        QMetaObject::invokeMethod(&controller.window(), "filterRequested", Q_ARG(QString, QStringLiteral("All Words")));
        expect(list && list->count() == 3, "all words filter");

        expect(controller.saveToFile(path).status == ControllerStatus::Success, "Unicode filename save");
        expect(!controller.isDirty() && controller.currentPath() == path, "clean after save");
        expect(controller.saveToFile(path).status == ControllerStatus::Success, "repeated save");
        expect(controller.deleteWord(QStringLiteral("Algorithm")).status == ControllerStatus::Success, "delete");
        expect(controller.isDirty() && list && list->count() == 2, "UI and dirty after delete");
        expect(controller.undo().status == ControllerStatus::Success, "undo delete");
        expect(!controller.isDirty(), "undo back to saved state is clean");
        expect(controller.redo().status == ControllerStatus::Success, "redo delete");
        expect(controller.isDirty(), "redo makes dirty");

        QFile bad(badPath);
        expect(bad.open(QIODevice::WriteOnly), "create corrupt file");
        bad.write("Bad\t\xff\n");
        bad.close();
        expect(controller.loadFromFile(badPath).status == ControllerStatus::Failed, "corrupt load fails");
        expect(controller.searchWord(QStringLiteral("Kernel")).status == ControllerStatus::Success, "failed load preserves data");
        expect(controller.loadFromFile(path).status == ControllerStatus::Success, "Unicode filename load");
        expect(!controller.isDirty() && list && list->count() == 3, "load resets state and UI");
        expect(controller.searchWord(QStringLiteral("Algorithm")).vietnamese == QString::fromUtf8("giải thuật"), "round trip UTF-8");
        expect(controller.undo().status == ControllerStatus::Unavailable, "load clears history");
        expect(controller.addWord(QStringLiteral("Ephemeral"), QStringLiteral("temporary")).status == ControllerStatus::Success, "temporary add");
        expect(controller.deleteWord(QStringLiteral("Ephemeral")).status == ControllerStatus::Success && !controller.isDirty(), "return to saved state is clean");
        expect(controller.updateWord(QStringLiteral("Algorithm"), QString::fromUtf8("giải thuật")).status == ControllerStatus::Success && !controller.isDirty(), "unchanged meaning stays clean");

        auto* searchInput = controller.window().findChild<QLineEdit*>(QStringLiteral("searchInput"));
        auto* searchButton = controller.window().findChild<QPushButton*>(QStringLiteral("searchButton"));
        auto* searchResults = controller.window().findChild<QListWidget*>(QStringLiteral("searchResults"));
        expect(searchInput && searchButton && searchResults, "search UI wired");
        if (searchInput && searchButton && searchResults) {
            searchInput->setText(QStringLiteral("Algorithm"));
            searchButton->click();
            expect(searchResults->count() > 0, "UI existing-term search");
            searchInput->setText(QStringLiteral("Absent"));
            searchButton->click();
            expect(searchResults->count() == 0, "UI not-found search");
        }
        auto* addButton = controller.window().findChild<QPushButton*>(QStringLiteral("addButton"));
        auto* editButton = controller.window().findChild<QPushButton*>(QStringLiteral("editButton"));
        expect(addButton && editButton, "edit buttons exist");
        if (addButton && editButton && list) {
            addButton->click();
            auto* dialog = controller.window().findChild<WordDialog*>();
            expect(dialog != nullptr, "add dialog opened");
            if (dialog) {
                const auto fields = dialog->findChildren<QLineEdit*>();
                expect(fields.size() == 2, "add dialog fields");
                if (fields.size() == 2) {
                    fields[0]->setText(QStringLiteral("Widget"));
                    fields[1]->setText(QString::fromUtf8("tiện ích"));
                    for (auto* button : dialog->findChildren<QPushButton*>())
                        if (button->text() == QString::fromUtf8("Lưu")) button->click();
                }
                expect(controller.searchWord(QStringLiteral("Widget")).status == ControllerStatus::Success, "UI add flow");
            }
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            const auto matches = list->findItems(QStringLiteral("Widget"), Qt::MatchExactly);
            if (!matches.isEmpty()) list->setCurrentItem(matches.front());
            editButton->click();
            dialog = controller.window().findChild<WordDialog*>();
            expect(dialog != nullptr, "edit dialog opened");
            if (dialog) {
                const auto fields = dialog->findChildren<QLineEdit*>();
                if (fields.size() == 2) {
                    fields[1]->setText(QString::fromUtf8("thành phần giao diện"));
                    for (auto* button : dialog->findChildren<QPushButton*>())
                        if (button->text() == QString::fromUtf8("Lưu")) button->click();
                }
                expect(controller.searchWord(QStringLiteral("Widget")).vietnamese == QString::fromUtf8("thành phần giao diện"), "UI edit flow");
            }
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        }
        controller.window().show();
        QTimer::singleShot(0, [] {
            auto* box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
            if (box && box->button(QMessageBox::Cancel)) box->button(QMessageBox::Cancel)->click();
        });
        expect(!controller.window().close() && controller.window().isVisible(), "cancel unsaved close");
    }
    // Recreate and destroy the controller after loading owned nodes: ASan/Valgrind can inspect this scope.
    {
        AppController controller(application);
        expect(controller.loadFromFile(path).status == ControllerStatus::Success, "reload after shutdown");
    }
    {
        AppController controller(application);
        QTimer::singleShot(0, &application, &QCoreApplication::quit);
        expect(controller.run() == 0, "application event loop");
        expect(controller.searchWord(QStringLiteral("Algorithm")).status == ControllerStatus::Success,
               "startup loads bundled dictionary");
    }
    return failures == 0 ? 0 : 1;
}
