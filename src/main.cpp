#include <QApplication>
#include <QAbstractItemModel>
#include <QCompleter>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QLineEdit>
#include <QListWidget>
#include <QTemporaryDir>
#include <QTextEdit>
#include <QTimer>

#include "app_controller.h"

int main(int argc, char* argv[]) {
    QApplication application(argc, argv);
    application.setApplicationName("Tech Dictionary");
    application.setOrganizationName("Tech Dictionary Team");

    const bool smoke = argc == 3 && QString::fromLocal8Bit(argv[1]) == QLatin1String("--ci-smoke");
    QTemporaryDir isolatedData;
    if (smoke && !isolatedData.isValid()) return 2;
    const QString seed = QDir(QCoreApplication::applicationDirPath())
                             .filePath(QStringLiteral("data/dictionary.txt"));
    AppController controller(application,
                             smoke ? isolatedData.filePath(QStringLiteral("dictionary.txt")) : QString{},
                             smoke ? seed : QString{});
    if (smoke) {
        const QString screenshot = QString::fromLocal8Bit(argv[2]);
        QTimer::singleShot(500, &application, [&application, &controller, screenshot] {
            auto* list = controller.window().findChild<QListWidget*>(QStringLiteral("wordList"));
            auto* detail = controller.window().findChild<QTextEdit*>(QStringLiteral("wordDetail"));
            auto* input = controller.window().findChild<QLineEdit*>(QStringLiteral("searchInput"));
            bool okay = list && detail && input && list->count() >= 550;
            if (okay) {
                const auto items = list->findItems(QStringLiteral("Algorithm"), Qt::MatchExactly);
                okay = !items.isEmpty();
                if (okay) list->setCurrentItem(items.front());
                okay = okay && detail->toPlainText().contains(
                    QStringLiteral("The algorithm sorts the array in ascending order."));
                input->setText(QStringLiteral("comp"));
                okay = okay && input->completer() &&
                       input->completer()->model()->rowCount() > 0;
                okay = okay && controller.window().grab().save(screenshot, "PNG");
                okay = okay && QFileInfo(screenshot).size() > 1000;
            }
            application.exit(okay ? 0 : 3);
        });
    }
    return controller.run();
}
