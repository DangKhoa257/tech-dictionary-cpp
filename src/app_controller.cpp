#include "app_controller.h"

#include "delete_confirm.h"
#include "search_widget.h"
#include "word_dialog.h"

#include <QAction>
#include <QCoreApplication>
#include <QDir>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QStandardPaths>
#include <QStatusBar>
#include <QVBoxLayout>

#include <exception>
#include <vector>

namespace {

ControllerResult invalid(const QString& message) {
    return {ControllerStatus::InvalidInput, message, {}};
}
ControllerResult failed(const QString& message) {
    return {ControllerStatus::Failed, message, {}};
}
ControllerResult success(const QString& message) {
    return {ControllerStatus::Success, message, {}};
}

bool validField(const QString& text) {
    if (text.trimmed().isEmpty()) return false;
    for (qsizetype i = 0; i < text.size(); ++i) {
        const ushort c = text.at(i).unicode();
        if (c < 0x20 || c == 0x7f) return false;
        if (QChar::isHighSurrogate(c)) {
            if (++i >= text.size() || !QChar::isLowSurrogate(text.at(i).unicode())) return false;
        } else if (QChar::isLowSurrogate(c)) {
            return false;
        }
    }
    return true;
}

bool validEnglish(const QString& text) {
    if (!validField(text)) return false;
    const QChar first = text.trimmed().front();
    return (first >= QLatin1Char('A') && first <= QLatin1Char('Z')) ||
           (first >= QLatin1Char('a') && first <= QLatin1Char('z'));
}

std::string utf8(const QString& text) {
    const QByteArray bytes = text.toUtf8();
    return std::string(bytes.constData(), static_cast<std::size_t>(bytes.size()));
}

bool samePath(const QString& a, const QString& b) {
#ifdef Q_OS_WIN
    constexpr auto sensitivity = Qt::CaseInsensitive;
#else
    constexpr auto sensitivity = Qt::CaseSensitive;
#endif
    return QDir::cleanPath(QFileInfo(a).absoluteFilePath()).compare(
               QDir::cleanPath(QFileInfo(b).absoluteFilePath()), sensitivity) == 0;
}

}  // namespace

AppController::AppController(QApplication& application) : application_(application) {
    mainWindow_.setWindowTitle(QStringLiteral("Tech Dictionary[*]"));
    mainWindow_.installEventFilter(this);
    connectUi();
    mainWindow_.setInteractionEnabled(true);
    refreshList();
    refreshActions();
}

AppController::~AppController() {
    clearDictionary(dictionary_);
}

int AppController::run() {
    loadStartupDictionary();
    mainWindow_.show();
    return application_.exec();
}

ControllerResult AppController::searchWord(const QString& english) const {
    if (!validEnglish(english)) return invalid(QStringLiteral("Enter an English term beginning with A-Z."));
    const WordNode* node = ::findWord(dictionary_, utf8(english));
    if (!node) return {ControllerStatus::NotFound, QStringLiteral("Term not found: %1").arg(english.trimmed()), {}};
    return {ControllerStatus::Success, QStringLiteral("Term found."),
            QString::fromUtf8(node->vietnamese.c_str())};
}

ControllerResult AppController::addWord(const QString& english, const QString& vietnamese) {
    if (!validEnglish(english) || !validField(vietnamese))
        return invalid(QStringLiteral("Enter a valid English term and Vietnamese meaning."));
    if (::findWord(dictionary_, utf8(english))) return invalid(QStringLiteral("Term already exists."));
    bool changed = false;
    bool historyRecorded = false;
    try {
        auto before = DictionaryHistory::capture(dictionary_);
        if (!::addWord(dictionary_, utf8(english), utf8(vietnamese)))
            return invalid(QStringLiteral("Invalid term or meaning."));
        changed = true;
        history_.recordChange(std::move(before), DictionaryHistory::capture(dictionary_));
        historyRecorded = true;
    } catch (const std::exception&) {
        if (!changed) return failed(QStringLiteral("Could not add term."));
        history_.clear();
    }
    try { dirty_ = DictionaryHistory::capture(dictionary_) != savedSnapshot_; }
    catch (const std::exception&) { dirty_ = true; }
    refreshList(english.trimmed());
    refreshActions();
    return success(historyRecorded ? QStringLiteral("Term added.") : QStringLiteral("Term added; Undo history unavailable."));
}

ControllerResult AppController::updateWord(const QString& english, const QString& vietnamese) {
    if (!validEnglish(english) || !validField(vietnamese))
        return invalid(QStringLiteral("Enter a valid English term and Vietnamese meaning."));
    if (!::findWord(dictionary_, utf8(english))) return {ControllerStatus::NotFound, QStringLiteral("Term not found."), {}};
    bool historyRecorded = false;
    bool changed = false;
    try {
        auto before = DictionaryHistory::capture(dictionary_);
        if (!::updateWordMeaning(dictionary_, utf8(english), utf8(vietnamese)))
            return failed(QStringLiteral("Could not update term."));
        changed = true;
        history_.recordChange(std::move(before), DictionaryHistory::capture(dictionary_));
        historyRecorded = true;
    } catch (const std::exception&) {
        if (!changed) return failed(QStringLiteral("Could not update term."));
        history_.clear();
    }
    try { dirty_ = DictionaryHistory::capture(dictionary_) != savedSnapshot_; }
    catch (const std::exception&) { dirty_ = true; }
    refreshList(english.trimmed());
    refreshActions();
    return success(historyRecorded ? QStringLiteral("Meaning updated.") :
                                     QStringLiteral("Meaning updated; Undo history unavailable."));
}

ControllerResult AppController::deleteWord(const QString& english) {
    if (!validEnglish(english)) return invalid(QStringLiteral("Enter a valid English term."));
    if (!::findWord(dictionary_, utf8(english))) return {ControllerStatus::NotFound, QStringLiteral("Term not found."), {}};
    bool historyRecorded = false;
    bool changed = false;
    try {
        auto before = DictionaryHistory::capture(dictionary_);
        if (!::deleteWord(dictionary_, utf8(english))) return failed(QStringLiteral("Could not delete term."));
        changed = true;
        history_.recordChange(std::move(before), DictionaryHistory::capture(dictionary_));
        historyRecorded = true;
    } catch (const std::exception&) {
        if (!changed) return failed(QStringLiteral("Could not delete term."));
        history_.clear();
    }
    try { dirty_ = DictionaryHistory::capture(dictionary_) != savedSnapshot_; }
    catch (const std::exception&) { dirty_ = true; }
    refreshList();
    refreshActions();
    return success(historyRecorded ? QStringLiteral("Term deleted.") :
                                     QStringLiteral("Term deleted; Undo history unavailable."));
}

ControllerResult AppController::loadFromFile(const QString& path) {
    if (path.trimmed().isEmpty()) return invalid(QStringLiteral("Choose a dictionary file to open."));
    if (!::loadDictionary(dictionary_, utf8(path))) return failed(QStringLiteral("Could not open dictionary. Check the path and UTF-8 TSV format."));
    history_.clear();
    currentPath_ = samePath(path, seedPath()) ? editableDefaultPath() : QFileInfo(path).absoluteFilePath();
    savedSnapshot_ = DictionaryHistory::capture(dictionary_);
    dirty_ = false;
    refreshList();
    refreshActions();
    return success(QStringLiteral("Dictionary opened."));
}

ControllerResult AppController::saveToFile(const QString& path) {
    if (path.trimmed().isEmpty()) return invalid(QStringLiteral("Choose a dictionary file to save."));
    if (samePath(path, seedPath())) return invalid(QStringLiteral("The shipped dictionary is a read-only seed. Choose another file."));
    if (!::saveDictionary(dictionary_, utf8(path))) return failed(QStringLiteral("Could not save dictionary. Check the destination permissions."));
    currentPath_ = QFileInfo(path).absoluteFilePath();
    try {
        savedSnapshot_ = DictionaryHistory::capture(dictionary_);
    } catch (const std::exception&) {
        history_.clear();
    }
    dirty_ = false;
    refreshActions();
    return success(QStringLiteral("Dictionary saved."));
}

ControllerResult AppController::undo() {
    if (!history_.canUndo()) return {ControllerStatus::Unavailable, QStringLiteral("Nothing to undo."), {}};
    if (!history_.undo(dictionary_)) return failed(QStringLiteral("Could not undo change."));
    dirty_ = DictionaryHistory::capture(dictionary_) != savedSnapshot_;
    refreshList();
    refreshActions();
    return success(QStringLiteral("Change undone."));
}

ControllerResult AppController::redo() {
    if (!history_.canRedo()) return {ControllerStatus::Unavailable, QStringLiteral("Nothing to redo."), {}};
    if (!history_.redo(dictionary_)) return failed(QStringLiteral("Could not redo change."));
    dirty_ = DictionaryHistory::capture(dictionary_) != savedSnapshot_;
    refreshList();
    refreshActions();
    return success(QStringLiteral("Change redone."));
}

void AppController::connectUi() {
    searchWidget_ = new SearchWidget(&mainWindow_);
    mainWindow_.getSearchContainerLayout()->addWidget(searchWidget_);
    connect(searchWidget_, &SearchWidget::searchRequested, this, [this](const QString& term) {
        const auto result = searchWord(term);
        if (result.status == ControllerStatus::Success) searchWidget_->showFoundResult(term, result.vietnamese);
        else if (result.status == ControllerStatus::NotFound) searchWidget_->showNotFound(term);
        else searchWidget_->clearResult();
        showResult(result);
    });
    connect(&mainWindow_, &MainWindow::wordSelected, this, [this](const QString& term) {
        const auto result = searchWord(term);
        mainWindow_.setWordDetail(result.status == ControllerStatus::Success ? term : QString(), result.vietnamese);
    });
    connect(&mainWindow_, &MainWindow::filterRequested, this, [this](const QString& filter) {
        currentFilter_ = filter;
        refreshList();
    });
    connect(&mainWindow_, &MainWindow::addWordRequested, this, [this] { openWordDialog(false); });
    connect(&mainWindow_, &MainWindow::editWordRequested, this, [this] { openWordDialog(true); });
    connect(&mainWindow_, &MainWindow::deleteWordRequested, this, [this] { deleteSelectedWord(); });

    auto* fileMenu = mainWindow_.menuBar()->addMenu(QStringLiteral("&File"));
    auto* openAction = fileMenu->addAction(QStringLiteral("&Open…"));
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, [this] { openFile(); });
    auto* saveAction = fileMenu->addAction(QStringLiteral("&Save"));
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, [this] { saveFile(false); });
    auto* saveAsAction = fileMenu->addAction(QStringLiteral("Save &As…"));
    connect(saveAsAction, &QAction::triggered, this, [this] { saveFile(true); });
    fileMenu->addSeparator();
    auto* exitAction = fileMenu->addAction(QStringLiteral("E&xit"));
    connect(exitAction, &QAction::triggered, &mainWindow_, &QWidget::close);

    auto* editMenu = mainWindow_.menuBar()->addMenu(QStringLiteral("&Edit"));
    undoAction_ = editMenu->addAction(QStringLiteral("&Undo"));
    undoAction_->setShortcut(QKeySequence::Undo);
    connect(undoAction_, &QAction::triggered, this, [this] { showResult(undo()); });
    redoAction_ = editMenu->addAction(QStringLiteral("&Redo"));
    redoAction_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Y));
    connect(redoAction_, &QAction::triggered, this, [this] { showResult(redo()); });
}

void AppController::refreshList(const QString& select) {
    if (searchWidget_) searchWidget_->clearResult();
    QStringList words;
    std::vector<const WordNode*> nodes;
    if (currentFilter_ == QLatin1String("All Words")) {
        nodes = listAllWords(dictionary_);
    } else {
        int first = -1, last = -1;
        if (currentFilter_ == QLatin1String("A - E")) { first = 0; last = 4; }
        else if (currentFilter_ == QLatin1String("F - J")) { first = 5; last = 9; }
        else if (currentFilter_ == QLatin1String("K - O")) { first = 10; last = 14; }
        else if (currentFilter_ == QLatin1String("P - T")) { first = 15; last = 19; }
        else if (currentFilter_ == QLatin1String("U - Z")) { first = 20; last = 25; }
        for (int i = first; i <= last; ++i) {
            const auto bucket = listBucket(dictionary_, static_cast<std::size_t>(i));
            nodes.insert(nodes.end(), bucket.begin(), bucket.end());
        }
    }
    for (const auto* node : nodes) words.push_back(QString::fromUtf8(node->english.c_str()));
    mainWindow_.setWordList(words);
    if (!select.isEmpty()) {
        auto* list = mainWindow_.findChild<QListWidget*>(QStringLiteral("wordList"));
        if (list) {
            const auto matches = list->findItems(select, Qt::MatchExactly);
            if (!matches.isEmpty()) list->setCurrentItem(matches.front());
        }
    }
    if (mainWindow_.selectedWord().isEmpty()) mainWindow_.setWordDetail({}, {});
    mainWindow_.statusBar()->showMessage(words.isEmpty() ? QStringLiteral("No terms in this range.") :
                                        QStringLiteral("%1 terms").arg(words.size()));
}

void AppController::refreshActions() {
    if (undoAction_) undoAction_->setEnabled(history_.canUndo());
    if (redoAction_) redoAction_->setEnabled(history_.canRedo());
    mainWindow_.setWindowModified(dirty_);
}

void AppController::showResult(const ControllerResult& result) {
    mainWindow_.statusBar()->showMessage(result.message, 7000);
    if (result.status == ControllerStatus::Failed) QMessageBox::warning(&mainWindow_, QStringLiteral("Tech Dictionary"), result.message);
}

QString AppController::editableDefaultPath() const {
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).filePath(QStringLiteral("dictionary.txt"));
}

QString AppController::seedPath() const {
    return QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("data/dictionary.txt"));
}

void AppController::loadStartupDictionary() {
    const QString editable = editableDefaultPath();
    const bool hasEditable = QFileInfo::exists(editable);
    const QString path = hasEditable ? editable : seedPath();
    if (QFileInfo::exists(path)) {
        showResult(loadFromFile(path));
    } else {
        mainWindow_.statusBar()->showMessage(QStringLiteral("No dictionary seed found. You can add terms or open a file."));
        currentPath_ = editable;
    }
}

bool AppController::confirmDiscardChanges() {
    if (!dirty_) return true;
    const auto choice = QMessageBox::warning(&mainWindow_, QStringLiteral("Unsaved changes"),
        QStringLiteral("Save changes to the dictionary?"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
    if (choice == QMessageBox::Cancel) return false;
    if (choice == QMessageBox::Discard) return true;
    return saveFile(false);
}

bool AppController::eventFilter(QObject* watched, QEvent* event) {
    if (watched == &mainWindow_ && event->type() == QEvent::Close && !confirmDiscardChanges()) {
        event->ignore();
        return true;
    }
    return QObject::eventFilter(watched, event);
}

void AppController::openFile() {
    if (!confirmDiscardChanges()) return;
    const QString path = QFileDialog::getOpenFileName(&mainWindow_, QStringLiteral("Open dictionary"),
        QFileInfo(currentPath_).absolutePath(), QStringLiteral("Dictionary TSV (*.txt *.tsv);;All files (*)"));
    if (!path.isEmpty()) showResult(loadFromFile(path));
}

bool AppController::saveFile(bool saveAs) {
    QString path = currentPath_;
    if (saveAs || path.isEmpty()) path = QFileDialog::getSaveFileName(&mainWindow_,
        QStringLiteral("Save dictionary"), path, QStringLiteral("Dictionary TSV (*.txt *.tsv);;All files (*)"));
    if (path.isEmpty()) return false;
    if (samePath(path, editableDefaultPath())) QDir().mkpath(QFileInfo(path).absolutePath());
    const auto result = saveToFile(path);
    showResult(result);
    return result.status == ControllerStatus::Success;
}

void AppController::openWordDialog(bool edit) {
    const QString term = mainWindow_.selectedWord();
    if (edit && term.isEmpty()) return;
    const auto existing = edit ? searchWord(term) : ControllerResult{};
    auto* dialog = edit ? new WordDialog(WordDialog::Mode::Edit, term, existing.vietnamese, &mainWindow_)
                        : new WordDialog(WordDialog::Mode::Add, &mainWindow_);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(dialog, &WordDialog::saveRequested, this, [this, dialog, edit](const QString& english, const QString& vietnamese) {
        const auto result = edit ? updateWord(english, vietnamese) : addWord(english, vietnamese);
        if (result.status == ControllerStatus::Success) {
            showResult(result);
            dialog->accept();
        } else dialog->showSaveError(result.message);
    });
    dialog->show();
}

void AppController::deleteSelectedWord() {
    const QString term = mainWindow_.selectedWord();
    if (!term.isEmpty() && confirmDeleteWord(&mainWindow_, term)) showResult(deleteWord(term));
}
