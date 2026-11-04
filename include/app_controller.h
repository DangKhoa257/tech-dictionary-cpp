#pragma once

#include <QApplication>

#include "main_window.h"

// Owns the initial window. Dictionary integration belongs here in a later milestone.
class AppController {
public:
    explicit AppController(QApplication& application);
    int run();

private:
    QApplication& application_;
    MainWindow mainWindow_;
};
