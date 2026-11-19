#include <QApplication>
#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QComboBox>
#include <QCompleter>
#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QFileInfo>
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QGuiApplication>
#include <QLineEdit>
#include <QListWidget>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QScreen>
#include <QTemporaryDir>
#include <QTextEdit>
#include <QTimer>

#include <cstdlib>

#include "app_controller.h"
#include "word_dialog.h"

namespace {
bool captureDialogState(QWidget& mainWindow, WordDialog& dialog, QComboBox& combo,
                        const QString& path) {
    QWidget* popup = combo.view()->window();
    if (!popup || !popup->isVisible()) return false;
    const QRect mainRect(mainWindow.mapToGlobal(QPoint(0, 0)), mainWindow.size());
    const QRect dialogRect(dialog.mapToGlobal(QPoint(0, 0)), dialog.size());
    const QRect popupRect(popup->mapToGlobal(QPoint(0, 0)), popup->size());
    const QRect canvasRect = mainRect.united(dialogRect).united(popupRect);
    const qreal scale = mainWindow.devicePixelRatioF();
    QPixmap canvas(QSize(qRound(canvasRect.width() * scale),
                         qRound(canvasRect.height() * scale)));
    canvas.setDevicePixelRatio(scale);
    canvas.fill(Qt::white);
    QPainter painter(&canvas);
    painter.drawPixmap(mainRect.topLeft() - canvasRect.topLeft(), mainWindow.grab());
    painter.drawPixmap(dialogRect.topLeft() - canvasRect.topLeft(), dialog.grab());
    painter.drawPixmap(popupRect.topLeft() - canvasRect.topLeft(), popup->grab());
    painter.end();
    return canvas.save(path, "PNG") && QFileInfo(path).size() > 1000;
}

bool popupUsable(QComboBox& combo) {
    auto* view = combo.view();
    QWidget* popup = view ? view->window() : nullptr;
    const QScreen* screen = popup ? QGuiApplication::screenAt(popup->geometry().center()) : nullptr;
    if (!view || !popup || !popup->isVisible() || !screen) return false;
    const QColor text = view->palette().color(QPalette::Text);
    const QColor background = view->palette().color(QPalette::Base);
    const int lightnessGap = std::abs(text.lightness() - background.lightness());
    return view->model() && view->model()->rowCount() == combo.count() &&
           popup->height() <= 7 * 38 + 18 &&
           screen->availableGeometry().contains(popup->geometry()) &&
           lightnessGap >= 100;
}
}  // namespace

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
        const QString addPopupShot = shotDir.filePath(QStringLiteral("tech-dictionary-v2-add-part-popup.png"));
        const QString editPopupShot = shotDir.filePath(QStringLiteral("tech-dictionary-v2-edit-category-popup.png"));
        const QString framesDir = shotDir.filePath(QStringLiteral("anim_frames"));
        QDir().mkpath(framesDir);

        QTimer::singleShot(250, &application, [&application, &controller, screenshot, dashShot, autoShot,
                                                detailShot, addPopupShot, editPopupShot, framesDir] {
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
                    auto* fade = qobject_cast<QGraphicsOpacityEffect*>(detail->graphicsEffect());
                    const qreal fadeStart = fade ? fade->opacity() : 1.0;
                    for (int i = 0; i < 8; ++i) {
                        captureFrame();
                        delay(25);
                    }
                    okay = okay && fade && fadeStart < 0.6 && fade->opacity() > 0.98;
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

                // Capture real Windows popup widgets while Add and Edit dialogs are open.
                auto* addButton = controller.window().findChild<QPushButton*>(QStringLiteral("addButton"));
                if (okay && addButton) addButton->click();
                delay(70);
                auto* addDialog = controller.window().findChild<WordDialog*>();
                auto* part = addDialog ? addDialog->findChild<QComboBox*>(QStringLiteral("partOfSpeechInput")) : nullptr;
                auto* category = addDialog ? addDialog->findChild<QComboBox*>(QStringLiteral("categoryInput")) : nullptr;
                auto* addExample = addDialog ? addDialog->findChild<QTextEdit*>(QStringLiteral("exampleInput")) : nullptr;
                const auto addFields = addDialog ? addDialog->findChildren<QLineEdit*>() : QList<QLineEdit*>{};
                okay = okay && addDialog && part && category && addExample &&
                       addFields.size() == 2 && addDialog->isVisible();
                if (addDialog && part && category) {
                    part->showPopup();
                    delay(70);
                    okay = okay && popupUsable(*part) &&
                           captureDialogState(controller.window(), *addDialog, *part, addPopupShot);
                    part->hidePopup();
                    part->setCurrentText(QStringLiteral("Noun"));
                    category->setCurrentText(QStringLiteral("Programming"));
                    if (addFields.size() == 2) {
                        addFields[0]->setText(QStringLiteral("VisualSmokeOnly"));
                        addFields[1]->setText(QStringLiteral("Chỉ dùng để thử giao diện"));
                    }
                    if (addExample) addExample->setPlainText(
                        QStringLiteral("The visual smoke check uses temporary data."));
                    okay = okay && addDialog->partOfSpeech() == QStringLiteral("Noun") &&
                           addDialog->category() == QStringLiteral("Programming") &&
                           addDialog->example().contains(QStringLiteral("temporary data"));
                    addDialog->reject();
                }
                okay = okay && controller.searchWord(QStringLiteral("VisualSmokeOnly")).status ==
                       ControllerStatus::NotFound;

                if (okay) list->setCurrentItem(items.front());
                auto* editButton = controller.window().findChild<QPushButton*>(QStringLiteral("editButton"));
                if (okay && editButton) editButton->click();
                delay(70);
                auto* editDialog = controller.window().findChild<WordDialog*>();
                auto* editCategory = editDialog ? editDialog->findChild<QComboBox*>(QStringLiteral("categoryInput")) : nullptr;
                auto* editPart = editDialog ? editDialog->findChild<QComboBox*>(QStringLiteral("partOfSpeechInput")) : nullptr;
                auto* editExample = editDialog ? editDialog->findChild<QTextEdit*>(QStringLiteral("exampleInput")) : nullptr;
                okay = okay && editDialog && editCategory && editPart && editExample &&
                       editDialog->isVisible() &&
                       editDialog->englishWord() == QStringLiteral("Algorithm") &&
                       editDialog->partOfSpeech() == QStringLiteral("Noun") &&
                       editDialog->category() == QStringLiteral("Algorithms");
                if (editDialog && editCategory && editPart && editExample) {
                    editCategory->showPopup();
                    delay(70);
                    okay = okay && popupUsable(*editCategory) &&
                           captureDialogState(controller.window(), *editDialog, *editCategory,
                                              editPopupShot);
                    editCategory->hidePopup();
                    editPart->setCurrentText(QStringLiteral("Verb"));
                    editCategory->setCurrentText(QStringLiteral("Programming"));
                    editExample->setPlainText(QStringLiteral("The algorithm processes each record."));
                    const auto save = editDialog->findChild<QPushButton*>(QStringLiteral("primaryButton"));
                    if (save) save->click();
                    const auto edited = controller.searchWord(QStringLiteral("Algorithm"));
                    okay = okay && save && edited.status == ControllerStatus::Success &&
                           edited.partOfSpeech == QStringLiteral("Verb") &&
                           edited.category == QStringLiteral("Programming") &&
                           edited.example == QStringLiteral("The algorithm processes each record.");
                    okay = okay && controller.undo().status == ControllerStatus::Success &&
                           controller.searchWord(QStringLiteral("Algorithm")).category ==
                               QStringLiteral("Algorithms");
                    okay = okay && controller.redo().status == ControllerStatus::Success &&
                           controller.searchWord(QStringLiteral("Algorithm")).category ==
                               QStringLiteral("Programming");
                }
                okay = okay && QFileInfo(addPopupShot).size() > 1000 &&
                       QFileInfo(editPopupShot).size() > 1000;
            }
            application.exit(okay ? 0 : 3);
        });
    }
    return controller.run();
}
