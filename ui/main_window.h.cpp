#pragma once

#include <QMainWindow>
#include <QLineEdit>
#include <QListWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private:
    // Thanh đệm / Nơi gắn SearchWidget của TV3
    QLineEdit* searchField; 
    
    // Danh sách từ vựng & Chi tiết nghĩa
    QListWidget* wordList;
    QTextEdit* definitionBox;
    
    // Các nút thao tác mở WordDialog của TV4
    QPushButton* btnAddWord;
    QPushButton* btnEditWord;
    QPushButton* btnDeleteWord;

    void setupSidebar(QHBoxLayout* rootLayout);
    void setupMainContent(QHBoxLayout* rootLayout);
};