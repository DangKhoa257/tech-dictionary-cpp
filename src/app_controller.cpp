#include "app_controller.h"

namespace {

ControllerResult invalidInput(const QString& message) {
    return {ControllerStatus::InvalidInput, message, {}};
}

ControllerResult unavailable(const QString& operation) {
    return {ControllerStatus::Unavailable,
            operation + QStringLiteral(" is not available yet."), {}};
}

}  // namespace

AppController::AppController(QApplication& application) : application_(application) {}

int AppController::run() {
    mainWindow_.show();
    return application_.exec();
}

ControllerResult AppController::searchWord(const QString& english) const {
    if (english.trimmed().isEmpty()) {
        return invalidInput(QStringLiteral("Enter an English word to search."));
    }
    return unavailable(QStringLiteral("Search"));
}

ControllerResult AppController::addWord(const QString& english, const QString& vietnamese) {
    if (english.trimmed().isEmpty() || vietnamese.trimmed().isEmpty()) {
        return invalidInput(QStringLiteral("Enter an English word and Vietnamese meaning."));
    }
    return unavailable(QStringLiteral("Add word"));
}

ControllerResult AppController::updateWord(const QString& english, const QString& vietnamese) {
    if (english.trimmed().isEmpty() || vietnamese.trimmed().isEmpty()) {
        return invalidInput(QStringLiteral("Enter an English word and Vietnamese meaning."));
    }
    return unavailable(QStringLiteral("Update word"));
}

ControllerResult AppController::deleteWord(const QString& english) {
    if (english.trimmed().isEmpty()) {
        return invalidInput(QStringLiteral("Enter an English word to delete."));
    }
    return unavailable(QStringLiteral("Delete word"));
}

ControllerResult AppController::loadFromFile(const QString& path) {
    if (path.trimmed().isEmpty()) {
        return invalidInput(QStringLiteral("Choose a dictionary file to load."));
    }
    return unavailable(QStringLiteral("Load dictionary"));
}

ControllerResult AppController::saveToFile(const QString& path) const {
    if (path.trimmed().isEmpty()) {
        return invalidInput(QStringLiteral("Choose a dictionary file to save."));
    }
    return unavailable(QStringLiteral("Save dictionary"));
}
