#include "app_controller.h"
#include "search_widget.h"
#include "word_dialog.h"

#include <QAction>
#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QLineEdit>
#include <QListWidget>
#include <QMetaObject>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTimer>
#include <QTextEdit>

#include <iostream>
#include <new>

namespace {

class FaultySaveController : public AppController {
public:
    using AppController::AppController;
    bool failSnapshot = false;

protected:
    DictionaryHistory::Snapshot snapshotForSave() const override {
        if (failSnapshot) throw std::bad_alloc();
        return AppController::snapshotForSave();
    }
};

bool writeBytes(const QString& path, const QByteArray& data) {
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(data) == data.size();
}

QByteArray readBytes(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return {};
    return file.readAll();
}

QAction* actionNamed(MainWindow& window, const QString& name) {
    for (auto* action : window.findChildren<QAction*>())
        if (action->text() == name) return action;
    return nullptr;
}

}  // namespace

int main(int argc, char* argv[]) {
    QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    QApplication application(argc, argv);
    QStandardPaths::setTestModeEnabled(true);
    QTemporaryDir temporary(QDir::current().filePath(QStringLiteral("app-controller-test-XXXXXX")));
    int failures = 0;
    const auto expect = [&failures](bool condition, const char* name) {
        if (!condition) { std::cerr << "Failed: " << name << '\n'; ++failures; }
    };
    const auto queueOpenChoice = [&expect](QMessageBox::StandardButton choice, const QString& path) {
        QTimer::singleShot(0, [&, choice, path] {
            auto* prompt = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
            expect(prompt != nullptr, "unsaved Open prompt appears");
            if (!prompt) return;
            QTimer::singleShot(0, [&, path] {
                auto* picker = qobject_cast<QFileDialog*>(QApplication::activeModalWidget());
                expect(picker != nullptr, "Open file picker appears");
                if (picker) {
                    picker->selectFile(path);
                    static_cast<QDialog*>(picker)->accept();
                }
            });
            auto* button = prompt->button(choice);
            expect(button != nullptr, "unsaved Open choice exists");
            if (button) button->click();
        });
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
        expect(controller.undo().status == ControllerStatus::NotFound, "load clears history");
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
        auto* openAction = actionNamed(controller.window(), QStringLiteral("&Open…"));
        expect(openAction != nullptr, "Open menu action exists");
        if (openAction) {
            QTimer::singleShot(0, [] {
                auto* box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
                if (box && box->button(QMessageBox::Cancel)) box->button(QMessageBox::Cancel)->click();
            });
            openAction->trigger();
            expect(controller.isDirty() && controller.searchWord(QStringLiteral("Widget")).status == ControllerStatus::Success,
                   "cancelled Open keeps unsaved dictionary");
        }
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
        AppController controller(application, temporary.filePath(QStringLiteral("startup-user.txt")),
                                 QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("data/dictionary.txt")));
        QTimer::singleShot(0, &application, &QCoreApplication::quit);
        expect(controller.run() == 0, "application event loop");
        expect(controller.searchWord(QStringLiteral("Algorithm")).status == ControllerStatus::Success,
               "startup loads bundled dictionary");
    }

    const QString userPath = temporary.filePath(QStringLiteral("user-dictionary.txt"));
    const QString seedPath = temporary.filePath(QStringLiteral("bundled-seed.txt"));
    const QString recoveredPath = temporary.filePath(QStringLiteral("recovered.txt"));
    const QByteArray seedData = "Seed\tH\xE1\xBA\xA1t gi\xE1\xBB\x91ng\n";
    const QByteArray corruptData = "Broken\t\xFF\n";
    expect(writeBytes(seedPath, seedData), "create isolated seed");
    {
        AppController controller(application, userPath, seedPath);
        QTimer::singleShot(0, &application, &QCoreApplication::quit);
        expect(controller.run() == 0, "startup without user file");
        expect(controller.searchWord(QStringLiteral("Seed")).status == ControllerStatus::Success,
               "bundled seed loads by default");
        expect(controller.currentPath() == userPath, "seed maps to editable path");
        expect(controller.saveToFile(seedPath).status == ControllerStatus::InvalidInput,
               "seed cannot be overwritten");
        expect(readBytes(seedPath) == seedData, "bundled seed unchanged");
    }
    expect(writeBytes(userPath, "User\tNg\xC6\xB0\xE1\xBB\x9Di d\xC3\xB9ng\n"), "create user dictionary");
    {
        AppController controller(application, userPath, seedPath);
        QTimer::singleShot(0, &application, &QCoreApplication::quit);
        expect(controller.run() == 0, "startup with valid user file");
        expect(controller.searchWord(QStringLiteral("User")).status == ControllerStatus::Success,
               "valid user dictionary takes priority");
        expect(controller.searchWord(QStringLiteral("Seed")).status == ControllerStatus::NotFound,
               "seed not merged with user file");
        expect(controller.currentPath() == userPath, "valid user file active");
    }
    expect(writeBytes(userPath, corruptData), "create corrupt user dictionary");
    {
        AppController controller(application, userPath, seedPath);
        QTimer::singleShot(0, [&] {
            auto* box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
            expect(box != nullptr, "corrupt startup offers recovery");
            if (box) {
                for (auto* button : box->buttons())
                    if (button->text() == QStringLiteral("Load bundled dictionary")) button->click();
            }
            QTimer::singleShot(0, &application, &QCoreApplication::quit);
        });
        expect(controller.run() == 0, "startup seed recovery");
        expect(controller.searchWord(QStringLiteral("Seed")).status == ControllerStatus::Success,
               "seed recovery loads real data");
        expect(controller.currentPath().isEmpty(), "recovery leaves protected path inactive");
        expect(readBytes(userPath) == corruptData, "corrupt original preserved");
        expect(controller.saveToFile(userPath).status == ControllerStatus::InvalidInput,
               "direct save cannot overwrite corrupt original");
        expect(controller.saveToFile(recoveredPath).status == ControllerStatus::Success,
               "recovery can save to new path");
        expect(readBytes(userPath) == corruptData, "new save leaves corrupt original intact");
    }
    {
        AppController controller(application, userPath, temporary.filePath(QStringLiteral("missing-seed.txt")));
        QTimer::singleShot(0, [&] {
            auto* box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
            expect(box != nullptr, "recovery offered without seed");
            if (box) {
                for (auto* button : box->buttons())
                    if (button->text() == QStringLiteral("Start empty")) button->click();
            }
            QTimer::singleShot(0, &application, &QCoreApplication::quit);
        });
        expect(controller.run() == 0, "empty recovery when seed missing");
        expect(controller.searchWord(QStringLiteral("Seed")).status == ControllerStatus::NotFound,
               "empty recovery keeps dictionary usable");
        expect(controller.currentPath().isEmpty() && readBytes(userPath) == corruptData,
               "empty recovery protects corrupt file");
    }
    {
        AppController controller(application, userPath, seedPath);
        QTimer::singleShot(0, [&] {
            auto* box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
            expect(box != nullptr, "recovery dialog before Open another file");
            QTimer::singleShot(0, [&] {
                auto* picker = qobject_cast<QFileDialog*>(QApplication::activeModalWidget());
                expect(picker != nullptr, "recovery file picker opened");
                if (picker) {
                    picker->selectFile(recoveredPath);
                    static_cast<QDialog*>(picker)->accept();
                }
                QTimer::singleShot(0, &application, &QCoreApplication::quit);
            });
            if (box) {
                for (auto* button : box->buttons())
                    if (button->text() == QStringLiteral("Open another file…")) button->click();
            }
        });
        expect(controller.run() == 0, "open another file during recovery");
        expect(controller.searchWord(QStringLiteral("Seed")).status == ControllerStatus::Success &&
               controller.currentPath() == recoveredPath, "recovery Open activates selected file");
        expect(readBytes(userPath) == corruptData, "recovery Open preserves corrupt original");
    }
    {
        AppController controller(application, temporary.filePath(QStringLiteral("missing-user.txt")),
                                 temporary.filePath(QStringLiteral("missing-seed.txt")));
        QTimer::singleShot(0, &application, &QCoreApplication::quit);
        expect(controller.run() == 0, "startup with both files missing");
        expect(controller.addWord(QStringLiteral("New"), QStringLiteral("mới")).status == ControllerStatus::Success,
               "missing files still allow editing");
    }

    const QString saveA = temporary.filePath(QStringLiteral("save-a.txt"));
    const QString saveB = temporary.filePath(QStringLiteral("save-b.txt"));
    {
        FaultySaveController controller(application);
        auto* undoAction = actionNamed(controller.window(), QStringLiteral("&Undo"));
        auto* redoAction = actionNamed(controller.window(), QStringLiteral("&Redo"));
        expect(undoAction && redoAction && !undoAction->isEnabled() && !redoAction->isEnabled(),
               "history actions initially disabled");
        expect(controller.addWord(QStringLiteral("Alpha"), QStringLiteral("một")).status == ControllerStatus::Success,
               "add before save");
        expect(undoAction && undoAction->isEnabled() && redoAction && !redoAction->isEnabled(),
               "undo enabled after add");
        expect(controller.undo().status == ControllerStatus::Success &&
               controller.searchWord(QStringLiteral("Alpha")).status == ControllerStatus::NotFound &&
               redoAction && redoAction->isEnabled(), "undo add restores empty dictionary");
        expect(controller.redo().status == ControllerStatus::Success &&
               controller.searchWord(QStringLiteral("Alpha")).status == ControllerStatus::Success,
               "redo add restores term");
        auto* wordList = controller.window().findChild<QListWidget*>(QStringLiteral("wordList"));
        if (wordList) {
            const auto matches = wordList->findItems(QStringLiteral("Alpha"), Qt::MatchExactly);
            if (!matches.isEmpty()) wordList->setCurrentItem(matches.front());
        }
        auto* detail = controller.window().findChild<QTextEdit*>(QStringLiteral("wordDetail"));
        expect(controller.window().selectedWord() == QStringLiteral("Alpha") && detail &&
               detail->toPlainText().contains(QStringLiteral("một")), "selected detail after add");
        expect(controller.saveToFile(saveA).status == ControllerStatus::Success && !controller.isDirty(),
               "normal save sets clean state");
        expect(controller.updateWord(QStringLiteral("Alpha"), QStringLiteral("hai")).status == ControllerStatus::Success,
               "update before Save As");
        expect(controller.window().selectedWord() == QStringLiteral("Alpha") && detail &&
               detail->toPlainText().contains(QStringLiteral("hai")), "selected detail after update");
        controller.failSnapshot = true;
        expect(controller.saveToFile(saveB).status == ControllerStatus::Failed,
               "snapshot failure aborts before file write");
        expect(!QFileInfo::exists(saveB) && controller.currentPath() == saveA && controller.isDirty(),
               "snapshot failure keeps path and dirty state");
        expect(controller.undo().status == ControllerStatus::Success && !controller.isDirty(),
               "undo after failed save returns to persisted state");
        expect(controller.window().selectedWord() == QStringLiteral("Alpha") && detail &&
               detail->toPlainText().contains(QStringLiteral("một")), "selected detail after undo");
        expect(controller.redo().status == ControllerStatus::Success && controller.isDirty(),
               "redo after failed save returns to unsaved state");
        controller.failSnapshot = false;
        expect(controller.saveToFile(saveB).status == ControllerStatus::Success,
               "Save As writes new path");
        expect(controller.currentPath() == saveB && !controller.isDirty(), "Save As changes active path");
        expect(controller.saveToFile(saveB).status == ControllerStatus::Success, "repeated Save As path save");
        expect(controller.undo().status == ControllerStatus::Success && controller.isDirty(),
               "undo after Save differs from persisted snapshot");
        expect(controller.redo().status == ControllerStatus::Success && !controller.isDirty(),
               "redo returns to saved snapshot");
        expect(controller.addWord(QStringLiteral("Beta"), QStringLiteral("ba")).status == ControllerStatus::Success,
               "new mutation after save");
        const QString impossible = temporary.filePath(QStringLiteral("missing-dir/save.txt"));
        expect(controller.saveToFile(impossible).status == ControllerStatus::Failed,
               "unwritable destination fails");
        expect(controller.currentPath() == saveB && controller.isDirty() && !QFileInfo::exists(impossible),
               "failed write keeps metadata and destination absent");
        expect(readBytes(saveB).contains("Alpha\thai\n"), "failed write keeps prior saved data");
        expect(controller.undo().status == ControllerStatus::Success && !controller.isDirty(),
               "undo after failed write reaches saved state");
        expect(controller.redo().status == ControllerStatus::Success && controller.isDirty(),
               "redo after failed write remains unsaved");
        expect(controller.undo().status == ControllerStatus::Success, "prepare redo invalidation");
        expect(controller.addWord(QStringLiteral("Gamma"), QStringLiteral("bốn")).status == ControllerStatus::Success,
               "new history branch");
        expect(redoAction && !redoAction->isEnabled(), "new mutation clears redo");
        expect(controller.deleteWord(QStringLiteral("Gamma")).status == ControllerStatus::Success,
               "delete after new branch");
        expect(controller.window().selectedWord().isEmpty() && detail &&
               detail->toPlainText().contains(QStringLiteral("Select a word")),
               "delete clears stale selection and detail");
    }

    {
        AppController controller(application);
        expect(controller.loadFromFile(saveA).status == ControllerStatus::Success, "load for close-save test");
        expect(controller.addWord(QStringLiteral("CloseSave"), QStringLiteral("đã lưu")).status == ControllerStatus::Success,
               "dirty before close-save");
        controller.window().show();
        QTimer::singleShot(0, [] {
            auto* box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
            if (box && box->button(QMessageBox::Save)) box->button(QMessageBox::Save)->click();
        });
        expect(controller.window().close() && !controller.isDirty(), "close Save persists data");
        expect(readBytes(saveA).contains("CloseSave\t"), "close Save updated file");
    }
    {
        const QByteArray before = readBytes(saveB);
        AppController controller(application);
        expect(controller.loadFromFile(saveB).status == ControllerStatus::Success, "load for close-discard test");
        expect(controller.addWord(QStringLiteral("DiscardMe"), QStringLiteral("tạm")).status == ControllerStatus::Success,
               "dirty before close-discard");
        controller.window().show();
        QTimer::singleShot(0, [] {
            auto* box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
            if (box && box->button(QMessageBox::Discard)) box->button(QMessageBox::Discard)->click();
        });
        expect(controller.window().close(), "close Discard proceeds");
        expect(readBytes(saveB) == before, "close Discard leaves file unchanged");
    }
    {
        const QByteArray before = readBytes(saveA);
        AppController controller(application);
        expect(controller.loadFromFile(saveA).status == ControllerStatus::Success, "load for Open Discard");
        expect(controller.addWord(QStringLiteral("DiscardOnOpen"), QStringLiteral("tạm")).status == ControllerStatus::Success,
               "dirty before Open Discard");
        auto* open = actionNamed(controller.window(), QStringLiteral("&Open…"));
        expect(open != nullptr, "Open action for Discard");
        if (open) {
            queueOpenChoice(QMessageBox::Discard, saveB);
            open->trigger();
        }
        expect(controller.currentPath() == saveB && !controller.isDirty() &&
               controller.searchWord(QStringLiteral("DiscardOnOpen")).status == ControllerStatus::NotFound,
               "Open Discard replaces in-memory data");
        expect(readBytes(saveA) == before, "Open Discard does not save old file");
    }
    {
        AppController controller(application);
        expect(controller.loadFromFile(saveA).status == ControllerStatus::Success, "load for Open Save");
        expect(controller.addWord(QStringLiteral("SaveOnOpen"), QStringLiteral("đã lưu")).status == ControllerStatus::Success,
               "dirty before Open Save");
        auto* open = actionNamed(controller.window(), QStringLiteral("&Open…"));
        expect(open != nullptr, "Open action for Save");
        if (open) {
            queueOpenChoice(QMessageBox::Save, saveB);
            open->trigger();
        }
        expect(controller.currentPath() == saveB && !controller.isDirty() &&
               controller.searchWord(QStringLiteral("SaveOnOpen")).status == ControllerStatus::NotFound,
               "Open Save activates selected dictionary");
        expect(readBytes(saveA).contains("SaveOnOpen\t"), "Open Save persists old dictionary first");
    }
    {
        const QString vanishedDir = temporary.filePath(QStringLiteral("vanished"));
        const QString vanishedFile = QDir(vanishedDir).filePath(QStringLiteral("dictionary.txt"));
        expect(QDir().mkpath(vanishedDir) && writeBytes(vanishedFile, "Safe\told\n"),
               "create file for failed close-save");
        AppController controller(application);
        expect(controller.loadFromFile(vanishedFile).status == ControllerStatus::Success,
               "load file before destination disappears");
        expect(QFile::remove(vanishedFile) && QDir().rmdir(vanishedDir), "remove test destination");
        expect(controller.addWord(QStringLiteral("Unsaved"), QStringLiteral("chưa lưu")).status == ControllerStatus::Success,
               "dirty before failed close-save");
        controller.window().show();
        QTimer::singleShot(0, [] {
            auto* box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
            QTimer::singleShot(0, [] {
                auto* error = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
                if (error && error->button(QMessageBox::Ok)) error->button(QMessageBox::Ok)->click();
            });
            if (box && box->button(QMessageBox::Save)) box->button(QMessageBox::Save)->click();
        });
        expect(!controller.window().close() && controller.window().isVisible() && controller.isDirty(),
               "failed close-save prevents data loss");
        expect(controller.searchWord(QStringLiteral("Unsaved")).status == ControllerStatus::Success,
               "failed close-save keeps in-memory data");
    }
    return failures == 0 ? 0 : 1;
}
