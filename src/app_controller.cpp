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
#include <QPushButton>
#include <QStandardPaths>
#include <QStatusBar>
#include <QVBoxLayout>

#include <exception>
#include <type_traits>
#include <utility>
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

struct DictionaryGuard {
    DictionaryGuard() = default;
    Dictionary value{};
    ~DictionaryGuard() { clearDictionary(value); }
    DictionaryGuard(const DictionaryGuard&) = delete;
    DictionaryGuard& operator=(const DictionaryGuard&) = delete;
};

}  // namespace

AppController::AppController(QApplication& application, QString userDictionaryPath,
                             QString bundledDictionaryPath)
    : application_(application),
      userDictionaryPathOverride_(std::move(userDictionaryPath)),
      bundledDictionaryPathOverride_(std::move(bundledDictionaryPath)) {
    mainWindow_.setWindowTitle(QStringLiteral("Tech Dictionary[*]"));
    mainWindow_.installEventFilter(this);
    connectUi();
    mainWindow_.setInteractionEnabled(true);
    refreshList();
    refreshActions();
}

AppController::~AppController() {
    mainWindow_.removeEventFilter(this);
    disconnect(&mainWindow_, nullptr, this, nullptr);
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
    DictionaryGuard incoming;
    DictionaryHistory::Snapshot incomingSnapshot;
    QString nextPath;
    bool restoresProtected = false;
    try {
        if (!::loadDictionary(incoming.value, utf8(path)))
            return failed(QStringLiteral("Could not open dictionary. Check the path and UTF-8 TSV format."));
        incomingSnapshot = DictionaryHistory::capture(incoming.value);
        nextPath = samePath(path, seedPath())
            ? (protectedStartupPath_.isEmpty() ? editableDefaultPath() : QString{})
            : QFileInfo(path).absoluteFilePath();
        restoresProtected = !protectedStartupPath_.isEmpty() && samePath(path, protectedStartupPath_);
    } catch (const std::exception&) {
        return failed(QStringLiteral("Could not open dictionary. Existing data was kept."));
    }
    for (std::size_t i = 0; i < 26; ++i) std::swap(dictionary_.buckets[i], incoming.value.buckets[i]);
    history_.clear();
    currentPath_.swap(nextPath);
    savedSnapshot_.swap(incomingSnapshot);
    if (restoresProtected) protectedStartupPath_.clear();
    dirty_ = false;
    refreshList();
    refreshActions();
    return success(QStringLiteral("Dictionary opened."));
}

ControllerResult AppController::saveToFile(const QString& path) {
    return saveToFileImpl(path, false);
}

ControllerResult AppController::saveToFileImpl(const QString& path, bool allowProtectedOverwrite) {
    if (path.trimmed().isEmpty()) return invalid(QStringLiteral("Choose a dictionary file to save."));
    if (samePath(path, seedPath())) return invalid(QStringLiteral("The shipped dictionary is a read-only seed. Choose another file."));
    const bool replacesProtected = !protectedStartupPath_.isEmpty() && samePath(path, protectedStartupPath_);
    if (replacesProtected && !allowProtectedOverwrite)
        return invalid(QStringLiteral("This unreadable startup file is protected. Use Save As and explicitly confirm replacement."));
    DictionaryHistory::Snapshot prepared;
    QString nextPath;
    try {
        prepared = snapshotForSave();
        nextPath = QFileInfo(path).absoluteFilePath();
    } catch (const std::exception&) {
        return failed(QStringLiteral("Could not prepare the save. No file was written."));
    }
    try {
        if (!::saveDictionary(dictionary_, utf8(path)))
            return failed(QStringLiteral("Could not save dictionary. Check the destination permissions."));
    } catch (const std::exception&) {
        return failed(QStringLiteral("Could not save dictionary. Check the destination permissions."));
    }
    static_assert(std::is_nothrow_swappable<DictionaryHistory::Snapshot>::value,
                  "Committing a saved snapshot must not throw after disk write");
    savedSnapshot_.swap(prepared);
    currentPath_.swap(nextPath);
    if (replacesProtected) protectedStartupPath_.clear();
    dirty_ = false;
    refreshActions();
    return success(QStringLiteral("Dictionary saved."));
}

DictionaryHistory::Snapshot AppController::snapshotForSave() const {
    return DictionaryHistory::capture(dictionary_);
}

ControllerResult AppController::undo() {
    if (!history_.canUndo()) return {ControllerStatus::NotFound, QStringLiteral("Nothing to undo."), {}};
    if (!history_.undo(dictionary_)) return failed(QStringLiteral("Could not undo change."));
    try { dirty_ = DictionaryHistory::capture(dictionary_) != savedSnapshot_; }
    catch (const std::exception&) { dirty_ = true; }
    refreshList();
    refreshActions();
    return success(QStringLiteral("Change undone."));
}

ControllerResult AppController::redo() {
    if (!history_.canRedo()) return {ControllerStatus::NotFound, QStringLiteral("Nothing to redo."), {}};
    if (!history_.redo(dictionary_)) return failed(QStringLiteral("Could not redo change."));
    try { dirty_ = DictionaryHistory::capture(dictionary_) != savedSnapshot_; }
    catch (const std::exception&) { dirty_ = true; }
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
    const QString selected = select.isEmpty() ? mainWindow_.selectedWord() : select;
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
    if (!selected.isEmpty()) {
        auto* list = mainWindow_.findChild<QListWidget*>(QStringLiteral("wordList"));
        if (list) {
            const auto matches = list->findItems(selected, Qt::MatchExactly);
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
    if (!userDictionaryPathOverride_.isEmpty()) return userDictionaryPathOverride_;
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).filePath(QStringLiteral("dictionary.txt"));
}

QString AppController::seedPath() const {
    if (!bundledDictionaryPathOverride_.isEmpty()) return bundledDictionaryPathOverride_;
    return QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("data/dictionary.txt"));
}

void AppController::loadStartupDictionary() {
    const QString editable = editableDefaultPath();
    if (QFileInfo::exists(editable)) {
        const auto result = loadFromFile(editable);
        if (result.status == ControllerStatus::Success) { showResult(result); return; }
        protectedStartupPath_ = editable;
        currentPath_.clear();
        QMessageBox recovery(&mainWindow_);
        recovery.setIcon(QMessageBox::Warning);
        recovery.setWindowTitle(QStringLiteral("Dictionary recovery"));
        recovery.setText(QStringLiteral("Your dictionary file could not be loaded. It has been left unchanged."));
        const bool hasSeed = QFileInfo::exists(seedPath());
        recovery.setInformativeText(hasSeed
            ? QStringLiteral("Open another file, load the bundled dictionary, or start empty. Saving will not overwrite the unreadable file without your confirmation.")
            : QStringLiteral("Open another file or start empty. Saving will not overwrite the unreadable file without your confirmation."));
        auto* openButton = recovery.addButton(QStringLiteral("Open another file…"), QMessageBox::ActionRole);
        auto* seedButton = hasSeed
            ? recovery.addButton(QStringLiteral("Load bundled dictionary"), QMessageBox::ActionRole) : nullptr;
        auto* emptyButton = recovery.addButton(QStringLiteral("Start empty"), QMessageBox::AcceptRole);
        recovery.setDefaultButton(qobject_cast<QPushButton*>(emptyButton));
        recovery.exec();
        if (recovery.clickedButton() == openButton) {
            const QString other = QFileDialog::getOpenFileName(&mainWindow_, QStringLiteral("Open dictionary"),
                QFileInfo(editable).absolutePath(), QStringLiteral("Dictionary TSV (*.txt *.tsv);;All files (*)"));
            if (!other.isEmpty()) showResult(loadFromFile(other));
        } else if (seedButton && recovery.clickedButton() == seedButton) {
            const auto seedResult = loadFromFile(seedPath());
            if (seedResult.status == ControllerStatus::Success) showResult(seedResult);
            else mainWindow_.statusBar()->showMessage(QStringLiteral("Bundled dictionary could not be loaded. Started empty."));
        }
        return;
    }
    if (QFileInfo::exists(seedPath())) showResult(loadFromFile(seedPath()));
    else mainWindow_.statusBar()->showMessage(QStringLiteral("No dictionary seed found. You can add terms or open a file."));
    if (currentPath_.isEmpty()) currentPath_ = editable;
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
    const bool replacingProtected = !protectedStartupPath_.isEmpty() && samePath(path, protectedStartupPath_);
    if (replacingProtected) {
        const auto answer = QMessageBox::warning(&mainWindow_, QStringLiteral("Replace unreadable dictionary?"),
            QStringLiteral("This will replace the unreadable original file. Continue?"),
            QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
        if (answer != QMessageBox::Yes) return false;
    }
    if (samePath(path, editableDefaultPath())) QDir().mkpath(QFileInfo(path).absolutePath());
    const auto result = saveToFileImpl(path, replacingProtected);
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
