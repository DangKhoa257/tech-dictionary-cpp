#include "delete_confirm.h"

#include <QMessageBox>
#include <QPushButton>
#include <QWidget>

bool confirmDeleteWord(QWidget* parent, const QString& englishWord) {
    const QString message =
        QStringLiteral("Bạn có chắc muốn xóa thuật ngữ “%1” khỏi từ điển không?\nThao tác này có thể hoàn tác qua menu Chỉnh sửa.").arg(englishWord);

    QMessageBox box(QMessageBox::Warning, QStringLiteral("Xác nhận xóa thuật ngữ"), message,
                    QMessageBox::NoButton, parent);
    auto* cancel = box.addButton(QStringLiteral("Hủy"), QMessageBox::RejectRole);
    auto* remove = box.addButton(QStringLiteral("Xóa từ"), QMessageBox::DestructiveRole);

    remove->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: #DC2626; color: #FFFFFF; border: 1px solid #DC2626; "
        "border-radius: 8px; padding: 8px 18px; font-weight: 600; font-size: 13px; }"
        "QPushButton:hover { background-color: #B91C1C; }"
        "QPushButton:pressed { background-color: #991B1B; }"));

    cancel->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: #FFFFFF; color: #334155; border: 1px solid #CBD5E1; "
        "border-radius: 8px; padding: 8px 18px; font-weight: 600; font-size: 13px; }"
        "QPushButton:hover { background-color: #F1F5F9; }"));

    box.setDefaultButton(qobject_cast<QPushButton*>(cancel));
    box.exec();
    return box.clickedButton() == remove;
}
