#pragma once

#include <QWidget>
#include <QString>

class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;

// A self-contained search panel. It emits a request and does not access
// Dictionary directly; the controller/owner supplies the result.
class SearchWidget : public QWidget {
    Q_OBJECT

public:
    explicit SearchWidget(QWidget* parent = nullptr);

    void showFoundResult(const QString& english, const QString& vietnamese);
    void showNotFound(const QString& english);
    void showDisconnected();
    void clearResult();

signals:
    void searchRequested(const QString& english);

private slots:
    void requestSearch();

private:
    QLineEdit* searchInput_;
    QPushButton* searchButton_;
    QLabel* statusLabel_;
    QListWidget* resultList_;
};
