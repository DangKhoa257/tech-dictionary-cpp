#pragma once

#include <QApplication>
#include <QString>

#include "dictionary.h"
#include "main_window.h"

enum class ControllerStatus {
    InvalidInput,
    Unavailable,
    NotFound,
    Failed,
    Success
};

// A search result uses vietnamese; other operations leave it empty.
struct ControllerResult {
    ControllerStatus status;
    QString message;
    QString vietnamese;
};

// Owns application state and translates UI requests into dictionary operations.
// No dictionary operation is linked yet; valid requests explicitly report Unavailable.
class AppController {
public:
    explicit AppController(QApplication& application);
    AppController(const AppController&) = delete;
    AppController& operator=(const AppController&) = delete;

    int run();

    ControllerResult searchWord(const QString& english) const;
    ControllerResult addWord(const QString& english, const QString& vietnamese);
    ControllerResult updateWord(const QString& english, const QString& vietnamese);
    ControllerResult deleteWord(const QString& english);
    ControllerResult loadFromFile(const QString& path);
    ControllerResult saveToFile(const QString& path) const;

private:
    QApplication& application_;
    // Empty until TV2's memory management and operation modules are integrated.
    // When node-producing operations are linked, call clearDictionary on shutdown.
    Dictionary dictionary_{};
    MainWindow mainWindow_;
};
