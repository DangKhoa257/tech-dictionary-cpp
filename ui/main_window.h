#pragma once

#include <QMainWindow>

// Initial presentation shell. Feature controls remain disabled until connected.
class MainWindow : public QMainWindow {
public:
    explicit MainWindow(QWidget* parent = nullptr);
};
