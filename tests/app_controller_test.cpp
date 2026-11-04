#include <QApplication>

#include <iostream>

#include "app_controller.h"

int main(int argc, char* argv[]) {
    QApplication application(argc, argv);
    AppController controller(application);
    int failures = 0;

    const auto check = [&failures](const ControllerResult& result,
                                   ControllerStatus expected, const char* caseName) {
        if (result.status != expected || result.message.isEmpty() ||
            !result.vietnamese.isEmpty()) {
            std::cerr << "Failed: " << caseName << '\n';
            ++failures;
        }
    };

    check(controller.searchWord(QStringLiteral("  ")), ControllerStatus::InvalidInput,
          "empty search");
    check(controller.addWord(QStringLiteral("Algorithm"), {}), ControllerStatus::InvalidInput,
          "empty meaning");
    check(controller.updateWord({}, QStringLiteral("thuật toán")),
          ControllerStatus::InvalidInput, "empty update word");
    check(controller.deleteWord({}), ControllerStatus::InvalidInput,
          "empty delete word");
    check(controller.loadFromFile({}), ControllerStatus::InvalidInput,
          "empty load path");
    check(controller.saveToFile({}), ControllerStatus::InvalidInput,
          "empty save path");

    check(controller.searchWord(QStringLiteral("Algorithm")), ControllerStatus::Unavailable,
          "search unavailable");
    check(controller.addWord(QStringLiteral("Algorithm"), QStringLiteral("thuật toán")),
          ControllerStatus::Unavailable, "add unavailable");
    check(controller.updateWord(QStringLiteral("Algorithm"), QStringLiteral("thuật toán")),
          ControllerStatus::Unavailable, "update unavailable");
    check(controller.deleteWord(QStringLiteral("Algorithm")), ControllerStatus::Unavailable,
          "delete unavailable");
    check(controller.loadFromFile(QStringLiteral("dictionary.txt")),
          ControllerStatus::Unavailable, "load unavailable");
    check(controller.saveToFile(QStringLiteral("dictionary.txt")),
          ControllerStatus::Unavailable, "save unavailable");

    return failures == 0 ? 0 : 1;
}
