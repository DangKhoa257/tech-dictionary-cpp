#pragma once

#include <QMainWindow>
#include <QPushButton>
#include <QListWidget>
#include <QTextEdit>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QStringList>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // Các hàm cho TV1 kết nối danh sách từ và hiển thị chi tiết
    void setWordList(const QStringList &words);
    void setWordDetail(const QString &title, const QString &definition);
    // Empty when no word is selected. Controller enables requests after wiring.
    QString selectedWord() const;
    void setInteractionEnabled(bool enabled);

    // Hàm lấy layout để TV1 nhúng SearchWidget của TV3
    QVBoxLayout* getSearchContainerLayout() const;

signals:
    // Values: "All Words", "A - E", "F - J", "K - O", "P - T", "U - Z".
    void filterRequested(const QString &range);

    // Tín hiệu Thêm / Sửa / Xóa cho TV1 nối với TV4
    void addWordRequested();
    void editWordRequested();
    void deleteWordRequested();

    void wordSelected(const QString &word);

private slots:
    void onNavButtonClicked();
    void onAddClicked();
    void onEditClicked();
    void onDeleteClicked();
    void onItemSelectionChanged();

private:
    void setupUi();

    QWidget *centralWidget;
    QFrame *sidebarFrame;
    QWidget *contentWidget;
    QVBoxLayout *searchContainerLayout;

    QLabel *brandLabel;
    QLabel *eyebrowLabel;
    QLabel *headingLabel;
    QLabel *descriptionLabel;
    QLabel *sidebarFooterLabel;

    QListWidget *wordListWidget;
    QTextEdit *wordDetailTextEdit;

    QPushButton *btnAdd;
    QPushButton *btnEdit;
    QPushButton *btnDelete;
    QList<QPushButton*> filterButtons_;
    bool interactionEnabled_ = false;
};
