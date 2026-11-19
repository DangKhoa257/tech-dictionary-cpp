#include <QApplication>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QAbstractItemView>
#include <QGuiApplication>
#include <QScreen>
#include <QScrollBar>
#include <QTextEdit>
#include <QTest>

#include <cstdlib>
#include <iostream>

#include "word_dialog.h"

int main(int argc, char* argv[]) {
    QApplication application(argc, argv);
    int failures = 0;
    const auto check = [&failures](bool condition, const char* description) {
        if (!condition) {
            std::cerr << "Failed: " << description << '\n';
            ++failures;
        }
    };

    WordDialog add(WordDialog::Mode::Add);
    const auto fields = add.findChildren<QLineEdit*>();
    const auto buttons = add.findChildren<QPushButton*>();
    check(fields.size() == 2 && buttons.size() == 2, "dialog controls exist");
    if (fields.size() != 2 || buttons.size() != 2) return 1;
    int requests = 0;
    QObject::connect(&add, &WordDialog::saveRequested, &add,
                     [&requests](const QString&, const QString&) { ++requests; });
    add.show();
    buttons[0]->click();
    check(requests == 0, "empty input does not request save");
    fields[0]->setText(QStringLiteral("Algorithm"));
    fields[1]->setText(QStringLiteral("bad") + QChar(0x7F));
    buttons[0]->click();
    check(requests == 0, "control character does not request save");
    fields[1]->setText(QStringLiteral("Thuật toán"));
    buttons[0]->click();
    check(requests == 1 && add.isVisible(), "valid save request keeps dialog open");
    auto* example = add.findChild<QTextEdit*>(QStringLiteral("exampleInput"));
    const auto categories = add.findChildren<QComboBox*>();
    int richRequests = 0;
    QObject::connect(&add, &WordDialog::richSaveRequested, &add,
                     [&richRequests](const QString&, const QString&, const QString& part,
                                     const QString& sentence, const QString& category) {
        if (part == QStringLiteral("Noun") &&
            sentence == QStringLiteral("The algorithm sorts the array.") &&
            category == QStringLiteral("Algorithms")) ++richRequests;
    });
    check(example && categories.size() == 2, "enriched fields exist");
    if (example && categories.size() == 2) {
        auto* part = add.findChild<QComboBox*>(QStringLiteral("partOfSpeechInput"));
        auto* category = add.findChild<QComboBox*>(QStringLiteral("categoryInput"));
        check(part && category, "named metadata controls exist");
        if (!part || !category) return 1;
        part->showPopup();
        QCoreApplication::processEvents();
        auto* partView = part->view();
        auto* partPopup = partView->window();
        const QColor partText = partView->palette().color(QPalette::Text);
        const QColor partBase = partView->palette().color(QPalette::Base);
        check(partPopup->isVisible() && partPopup->height() < 300 &&
                  QGuiApplication::primaryScreen()->availableGeometry().contains(partPopup->geometry()) &&
                  std::abs(partText.lightness() - partBase.lightness()) >= 100,
              "part popup is visible, bounded and readable");
        const QModelIndex noun = partView->model()->index(part->findText(QStringLiteral("Noun")), 0);
        QTest::mouseClick(partView->viewport(), Qt::LeftButton, Qt::NoModifier,
                          partView->visualRect(noun).center());
        check(part->currentText() == QStringLiteral("Noun"), "mouse selects part of speech");

        category->showPopup();
        QCoreApplication::processEvents();
        auto* categoryView = category->view();
        auto* categoryPopup = categoryView->window();
        check(categoryPopup->isVisible() && categoryPopup->height() < 300 &&
                  QGuiApplication::primaryScreen()->availableGeometry().contains(categoryPopup->geometry()) &&
                  categoryView->verticalScrollBar() &&
                  categoryView->verticalScrollBar()->maximum() > 0,
              "category popup is bounded and scrollable");
        categoryView->setCurrentIndex(categoryView->model()->index(
            category->findText(QStringLiteral("Algorithms")), 0));
        QTest::keyClick(categoryView, Qt::Key_Return);
        check(category->currentText() == QStringLiteral("Algorithms"),
              "keyboard selects category");
        example->setPlainText(QStringLiteral("The algorithm sorts the array."));
        buttons[0]->click();
        check(richRequests == 1, "enriched save signal includes metadata");
    }
    add.showSaveError(QStringLiteral("Save failed"));
    bool feedbackVisible = false;
    for (QLabel* label : add.findChildren<QLabel*>()) {
        if (label->text() == QStringLiteral("Save failed")) feedbackVisible = true;
    }
    check(feedbackVisible && add.isVisible(), "owner can show failure without closing");
    add.accept();
    check(add.result() == QDialog::Accepted, "owner can close after success");

    WordDialog edit(WordDialog::Mode::Edit, QStringLiteral("Algorithm"),
                    QStringLiteral("Thuật toán"));
    const auto editFields = edit.findChildren<QLineEdit*>();
    check(editFields.size() == 2 && editFields[0]->isReadOnly(),
          "edit mode locks English key");
    edit.show();
    const auto editButtons = edit.findChildren<QPushButton*>();
    if (editButtons.size() == 2) editButtons[1]->click();
    check(edit.result() == QDialog::Rejected && !edit.isVisible(),
          "cancel closes without saving");
    return failures == 0 ? 0 : 1;
}
