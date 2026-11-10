#include "delete_confirm.h"

#include <QMessageBox>
#include <QWidget>

bool confirmDeleteWord(QWidget* parent, const QString& englishWord) {
    const QString message = QStringLiteral("Bạn có chắc muốn xóa từ '%1' không?")
                                .arg(englishWord);

    return QMessageBox::question(parent,
                                 QStringLiteral("Xác nhận xóa"),
                                 message,
                                 QMessageBox::Yes | QMessageBox::No,
                                 QMessageBox::No) == QMessageBox::Yes;
}
