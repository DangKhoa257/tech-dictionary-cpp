#include <QApplication>

#include "app_controller.h"

int main(int argc, char* argv[]) {
    QApplication application(argc, argv);
    application.setApplicationName("Tech Dictionary");
    application.setOrganizationName("Tech Dictionary Team");

    AppController controller(application);
    return controller.run();
}
