#pragma once

#include <QApplication>
#include <QObject>
#include <QString>

#include "dictionary.h"
#include "dictionary_history.h"
#include "main_window.h"

class QAction;
class SearchWidget;
class QEvent;

enum class ControllerStatus { InvalidInput, Unavailable, NotFound, Failed, Success };

struct ControllerResult {
    ControllerStatus status;
    QString message;
    QString vietnamese;
};

class AppController : public QObject {
public:
    explicit AppController(QApplication& application, QString userDictionaryPath = {},
                           QString bundledDictionaryPath = {});
    ~AppController() override;
    AppController(const AppController&) = delete;
    AppController& operator=(const AppController&) = delete;

    int run();
    ControllerResult searchWord(const QString& english) const;
    ControllerResult addWord(const QString& english, const QString& vietnamese);
    ControllerResult updateWord(const QString& english, const QString& vietnamese);
    ControllerResult deleteWord(const QString& english);
    ControllerResult loadFromFile(const QString& path);
    ControllerResult saveToFile(const QString& path);
    ControllerResult undo();
    ControllerResult redo();
    bool isDirty() const noexcept { return dirty_; }
    QString currentPath() const { return currentPath_; }
    MainWindow& window() noexcept { return mainWindow_; }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    // Kept overridable so integration tests can inject a snapshot failure before a save.
    virtual DictionaryHistory::Snapshot snapshotForSave() const;

private:
    void connectUi();
    void refreshList(const QString& select = {});
    void refreshActions();
    void showResult(const ControllerResult& result);
    void loadStartupDictionary();
    bool confirmDiscardChanges();
    QString editableDefaultPath() const;
    QString seedPath() const;
    void openFile();
    bool saveFile(bool saveAs);
    ControllerResult saveToFileImpl(const QString& path, bool allowProtectedOverwrite);
    void openWordDialog(bool edit);
    void deleteSelectedWord();

    QApplication& application_;
    Dictionary dictionary_{};
    DictionaryHistory history_;
    MainWindow mainWindow_;
    SearchWidget* searchWidget_ = nullptr;
    QAction* undoAction_ = nullptr;
    QAction* redoAction_ = nullptr;
    QString currentPath_;
    QString currentFilter_ = QStringLiteral("All Words");
    DictionaryHistory::Snapshot savedSnapshot_{};
    QString userDictionaryPathOverride_;
    QString bundledDictionaryPathOverride_;
    QString protectedStartupPath_;
    bool dirty_ = false;
};
