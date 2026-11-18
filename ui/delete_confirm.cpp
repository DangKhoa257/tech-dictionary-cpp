#include "delete_confirm.h"

#include <QMessageBox>
#include <QPushButton>
#include <QWidget>

bool confirmDeleteWord(QWidget* parent, const QString& englishWord) {
    const QString message =
        QStringLiteral("Bạn có chắc muốn xóa từ '%1' không?").arg(englishWord);

    QMessageBox box(QMessageBox::Question, QStringLiteral("Xác nhận xóa"), message,
                    QMessageBox::NoButton, parent);
    auto* cancel = box.addButton(QStringLiteral("Hủy"), QMessageBox::RejectRole);
    auto* remove = box.addButton(QStringLiteral("Xóa từ"), QMessageBox::DestructiveRole);
    box.setDefaultButton(qobject_cast<QPushButton*>(cancel));
    box.exec();
    return box.clickedButton() == remove;
}
