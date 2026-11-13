#include <QApplication>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

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
