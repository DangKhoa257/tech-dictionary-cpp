#include "app_controller.h"

AppController::AppController(QApplication& application) : application_(application) {}

int AppController::run() {
    mainWindow_.show();
    return application_.exec();
}
