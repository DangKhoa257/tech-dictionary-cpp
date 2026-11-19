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

class QGraphicsOpacityEffect;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    void setWordList(const QStringList &words);
    void setTotalWordCount(int count);
    void setWordDetail(const QString &title, const QString &definition,
                       const QString &partOfSpeech = {}, const QString &example = {},
                       const QString &category = {}, const QStringList &relatedWords = {});
    QString selectedWord() const;
    void setInteractionEnabled(bool enabled);
    void setActiveFilter(const QString& filter);

    QVBoxLayout* getSearchContainerLayout() const;

signals:
    void filterRequested(const QString &range);
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
    void setupSidebar(QHBoxLayout* mainLayout);
    void setupHeader(QVBoxLayout* contentLayout);
    void setupSearchArea(QVBoxLayout* contentLayout);
    void setupStatsCards(QVBoxLayout* contentLayout);
    void setupContentSplitter(QVBoxLayout* contentLayout);
    void animateDetailTransition();

    QWidget *centralWidget = nullptr;
    QFrame *sidebarFrame = nullptr;
    QWidget *contentWidget = nullptr;
    QVBoxLayout *searchContainerLayout = nullptr;

    QLabel *brandLabel = nullptr;
    QLabel *eyebrowLabel = nullptr;
    QLabel *headingLabel = nullptr;
    QLabel *descriptionLabel = nullptr;
    QLabel *sidebarFooterLabel = nullptr;
    QLabel *totalWordCountLabel_ = nullptr;
    QLabel *visibleCountLabel_ = nullptr;
    QLabel *emptyStateLabel_ = nullptr;
    QLabel *statsTermsCountLabel_ = nullptr;

    QListWidget *wordListWidget = nullptr;
    QTextEdit *wordDetailTextEdit = nullptr;
    QGraphicsOpacityEffect *detailOpacityEffect_ = nullptr;

    QPushButton *btnAdd = nullptr;
    QPushButton *btnEdit = nullptr;
    QPushButton *btnDelete = nullptr;
    QList<QPushButton*> filterButtons_;
    bool interactionEnabled_ = false;
};
