#include <QApplication>
#include <QAbstractItemModel>
#include <QCompleter>
#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QFileInfo>
#include <QFrame>
#include <QLineEdit>
#include <QListWidget>
#include <QPainter>
#include <QPixmap>
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
        const QFileInfo shotInfo(screenshot);
        const QDir shotDir = shotInfo.dir();
        const QString dashShot = shotDir.filePath(QStringLiteral("tech-dictionary-v2-dashboard.png"));
        const QString autoShot = shotDir.filePath(QStringLiteral("tech-dictionary-v2-autocomplete.png"));
        const QString detailShot = shotDir.filePath(QStringLiteral("tech-dictionary-v2-detail.png"));
        const QString framesDir = shotDir.filePath(QStringLiteral("anim_frames"));
        QDir().mkpath(framesDir);

        QTimer::singleShot(250, &application, [&application, &controller, screenshot, dashShot, autoShot, detailShot, framesDir] {
            const auto delay = [](int ms) {
                QEventLoop loop;
                QTimer::singleShot(ms, &loop, &QEventLoop::quit);
                loop.exec();
            };

            auto* list = controller.window().findChild<QListWidget*>(QStringLiteral("wordList"));
            auto* detail = controller.window().findChild<QTextEdit*>(QStringLiteral("wordDetail"));
            auto* input = controller.window().findChild<QLineEdit*>(QStringLiteral("searchInput"));
            auto* detailCard = controller.window().findChild<QFrame*>(QStringLiteral("detailCard"));

            bool okay = list && detail && input && list->count() >= 550;
            if (okay) {
                // Step 1: Verify data & select Algorithm
                const auto items = list->findItems(QStringLiteral("Algorithm"), Qt::MatchExactly);
                okay = !items.isEmpty();
                if (okay) list->setCurrentItem(items.front());
                delay(220); // Wait for detail panel fade-in animation

                okay = okay && detail->toPlainText().contains(
                    QStringLiteral("The algorithm sorts the array in ascending order."));

                // Verify Prompt & Socket
                const auto prompt = list->findItems(QStringLiteral("Prompt"), Qt::MatchExactly);
                if (okay && !prompt.isEmpty()) {
                    list->setCurrentItem(prompt.front());
                    delay(100);
                    okay = okay && detail->toPlainText().contains(
                        QStringLiteral("Lời nhắc đầu vào cho mô hình AI")) &&
                        detail->toPlainText().contains(QStringLiteral("Artificial Intelligence"));
                }
                const auto socket = list->findItems(QStringLiteral("Socket"), Qt::MatchExactly);
                if (okay && !socket.isEmpty()) {
                    list->setCurrentItem(socket.front());
                    delay(100);
                    okay = okay && detail->toPlainText().contains(
                        QStringLiteral("Điểm cuối giao tiếp mạng")) &&
                        detail->toPlainText().contains(QStringLiteral("Networking"));
                }

                // Return to Algorithm for Dashboard screenshot
                if (okay) list->setCurrentItem(items.front());
                delay(220);

                // Save Dashboard screenshot & Rich detail view
                controller.window().grab().save(dashShot, "PNG");
                if (detailCard) {
                    detailCard->grab().save(detailShot, "PNG");
                }

                // Step 2: Capture live animation frames of selecting Computer
                int frameIndex = 0;
                const auto captureFrame = [&]() {
                    QPixmap p = controller.window().grab();
                    if (input && input->completer() && input->completer()->popup() &&
                        input->completer()->popup()->isVisible()) {
                        QWidget* popup = input->completer()->popup();
                        QPoint relPos = popup->mapToGlobal(QPoint(0, 0)) - controller.window().mapToGlobal(QPoint(0, 0));
                        QPainter painter(&p);
                        popup->render(&painter, relPos);
                        painter.end();
                    }
                    const QString fname = QStringLiteral("frame_%1.png").arg(frameIndex++, 3, 10, QLatin1Char('0'));
                    p.save(QDir(framesDir).filePath(fname), "PNG");
                };

                // Hold initial state for GIF
                for (int i = 0; i < 4; ++i) {
                    captureFrame();
                    delay(40);
                }

                // Select Computer and capture the detail transition animation
                const auto compItems = list->findItems(QStringLiteral("Computer"), Qt::MatchExactly);
                if (!compItems.isEmpty()) {
                    list->setCurrentItem(compItems.front());
                    for (int i = 0; i < 8; ++i) {
                        captureFrame();
                        delay(25);
                    }
                }

                // Step 3: Type "comp" and capture Autocomplete reveal
                input->setFocus();
                input->setText(QStringLiteral("comp"));
                okay = okay && input->completer() &&
                       input->completer()->model()->rowCount() > 0;

                input->completer()->complete();
                QCoreApplication::processEvents();

                // Capture autocomplete reveal animation frames
                for (int i = 0; i < 8; ++i) {
                    captureFrame();
                    delay(25);
                }

                // Highlight first suggestion
                if (input->completer()->popup()) {
                    input->completer()->popup()->setCurrentIndex(
                        input->completer()->popup()->model()->index(0, 0));
                }
                delay(50);

                // Hold final autocomplete state for GIF
                for (int i = 0; i < 5; ++i) {
                    captureFrame();
                    delay(40);
                }

                // Save Autocomplete screenshot and primary smoke screenshot
                QPixmap autoPix = controller.window().grab();
                if (input->completer() && input->completer()->popup() &&
                    input->completer()->popup()->isVisible()) {
                    QWidget* popup = input->completer()->popup();
                    QPoint relPos = popup->mapToGlobal(QPoint(0, 0)) - controller.window().mapToGlobal(QPoint(0, 0));
                    QPainter painter(&autoPix);
                    popup->render(&painter, relPos);
                    painter.end();
                }
                autoPix.save(autoShot, "PNG");
                autoPix.save(screenshot, "PNG");

                okay = okay && QFileInfo(screenshot).size() > 1000 &&
                       QFileInfo(dashShot).size() > 1000 &&
                       QFileInfo(autoShot).size() > 1000;
            }
            application.exit(okay ? 0 : 3);
        });
    }
    return controller.run();
}
