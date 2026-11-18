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
    if (!validEnglish(english)) return invalid(QStringLiteral("Nhập thuật ngữ tiếng Anh bắt đầu bằng A–Z."));
    const WordNode* node = ::findWord(dictionary_, utf8(english));
    if (!node) return {ControllerStatus::NotFound, QStringLiteral("Không tìm thấy: %1").arg(english.trimmed()), {}};
    return {ControllerStatus::Success, QStringLiteral("Đã tìm thấy thuật ngữ."),
            QString::fromUtf8(node->vietnamese.c_str()),
            QString::fromUtf8(node->partOfSpeech.c_str()),
            QString::fromUtf8(node->example.c_str()),
            QString::fromUtf8(node->category.c_str())};
}

QStringList AppController::suggestions(const QString& prefix) const {
    QStringList result;
    for (const auto& word : suggestWords(dictionary_, utf8(prefix.trimmed())))
        result.append(QString::fromUtf8(word.c_str()));
    return result;
}

ControllerResult AppController::addWord(const QString& english, const QString& vietnamese,
                                        const QString& partOfSpeech, const QString& example,
                                        const QString& category) {
    if (!validEnglish(english) || !validField(vietnamese))
        return invalid(QStringLiteral("Nhập thuật ngữ tiếng Anh và nghĩa tiếng Việt hợp lệ."));
    if (::findWord(dictionary_, utf8(english))) return invalid(QStringLiteral("Thuật ngữ này đã tồn tại."));
    bool changed = false;
    bool historyRecorded = false;
    try {
        auto before = DictionaryHistory::capture(dictionary_);
        if (!::addEntry(dictionary_, utf8(english), utf8(vietnamese),
                        utf8(partOfSpeech), utf8(example), utf8(category)))
            return invalid(QStringLiteral("Thuật ngữ hoặc nghĩa không hợp lệ."));
        changed = true;
        history_.recordChange(std::move(before), DictionaryHistory::capture(dictionary_));
        historyRecorded = true;
    } catch (const std::exception&) {
        if (!changed) return failed(QStringLiteral("Không thể thêm thuật ngữ."));
        history_.clear();
    }
    try { dirty_ = DictionaryHistory::capture(dictionary_) != savedSnapshot_; }
    catch (const std::exception&) { dirty_ = true; }
    refreshList(english.trimmed());
    refreshActions();
    return success(historyRecorded ? QStringLiteral("Đã thêm thuật ngữ.") : QStringLiteral("Đã thêm từ; lịch sử Hoàn tác không khả dụng."));
}

ControllerResult AppController::updateWord(const QString& english, const QString& vietnamese) {
    const WordNode* original = ::findWord(dictionary_, utf8(english));
    if (!original) return updateWord(english, vietnamese, {}, {}, {});
    return updateWord(english, vietnamese,
                      QString::fromUtf8(original->partOfSpeech.c_str()),
                      QString::fromUtf8(original->example.c_str()),
                      QString::fromUtf8(original->category.c_str()));
}

ControllerResult AppController::updateWord(const QString& english, const QString& vietnamese,
                                           const QString& partOfSpeech, const QString& example,
                                           const QString& category) {
    if (!validEnglish(english) || !validField(vietnamese))
        return invalid(QStringLiteral("Nhập thuật ngữ tiếng Anh và nghĩa tiếng Việt hợp lệ."));
    if (!::findWord(dictionary_, utf8(english))) return {ControllerStatus::NotFound, QStringLiteral("Không tìm thấy thuật ngữ."), {}};
    bool historyRecorded = false;
    bool changed = false;
    try {
        auto before = DictionaryHistory::capture(dictionary_);
        if (!::updateEntry(dictionary_, utf8(english), utf8(vietnamese),
                           utf8(partOfSpeech), utf8(example), utf8(category)))
            return failed(QStringLiteral("Không thể sửa nghĩa."));
        changed = true;
        history_.recordChange(std::move(before), DictionaryHistory::capture(dictionary_));
        historyRecorded = true;
    } catch (const std::exception&) {
        if (!changed) return failed(QStringLiteral("Không thể sửa nghĩa."));
        history_.clear();
    }
    try { dirty_ = DictionaryHistory::capture(dictionary_) != savedSnapshot_; }
    catch (const std::exception&) { dirty_ = true; }
    refreshList(english.trimmed());
    refreshActions();
    return success(historyRecorded ? QStringLiteral("Đã sửa nghĩa.") :
                                     QStringLiteral("Đã sửa nghĩa; lịch sử Hoàn tác không khả dụng."));
}

ControllerResult AppController::deleteWord(const QString& english) {
    if (!validEnglish(english)) return invalid(QStringLiteral("Nhập thuật ngữ tiếng Anh hợp lệ."));
    if (!::findWord(dictionary_, utf8(english))) return {ControllerStatus::NotFound, QStringLiteral("Không tìm thấy thuật ngữ."), {}};
    bool historyRecorded = false;
    bool changed = false;
    try {
        auto before = DictionaryHistory::capture(dictionary_);
        if (!::deleteWord(dictionary_, utf8(english))) return failed(QStringLiteral("Không thể xóa thuật ngữ."));
        changed = true;
        history_.recordChange(std::move(before), DictionaryHistory::capture(dictionary_));
        historyRecorded = true;
    } catch (const std::exception&) {
        if (!changed) return failed(QStringLiteral("Không thể xóa thuật ngữ."));
        history_.clear();
    }
    try { dirty_ = DictionaryHistory::capture(dictionary_) != savedSnapshot_; }
    catch (const std::exception&) { dirty_ = true; }
    refreshList();
    refreshActions();
    return success(historyRecorded ? QStringLiteral("Đã xóa thuật ngữ.") :
                                     QStringLiteral("Đã xóa từ; lịch sử Hoàn tác không khả dụng."));
}

ControllerResult AppController::loadFromFile(const QString& path) {
    if (path.trimmed().isEmpty()) return invalid(QStringLiteral("Chọn tệp từ điển để mở."));
    DictionaryGuard incoming;
    DictionaryHistory::Snapshot incomingSnapshot;
    QString nextPath;
    bool restoresProtected = false;
    try {
        if (!::loadDictionary(incoming.value, utf8(path)))
            return failed(QStringLiteral("Không thể mở từ điển. Kiểm tra đường dẫn và định dạng TSV UTF-8."));
        incomingSnapshot = DictionaryHistory::capture(incoming.value);
        nextPath = samePath(path, seedPath())
            ? (protectedStartupPath_.isEmpty() ? editableDefaultPath() : QString{})
            : QFileInfo(path).absoluteFilePath();
        restoresProtected = !protectedStartupPath_.isEmpty() && samePath(path, protectedStartupPath_);
    } catch (const std::exception&) {
        return failed(QStringLiteral("Không thể mở từ điển. Dữ liệu hiện tại được giữ nguyên."));
    }
    for (std::size_t i = 0; i < 26; ++i) std::swap(dictionary_.buckets[i], incoming.value.buckets[i]);
    history_.clear();
    currentPath_.swap(nextPath);
    savedSnapshot_.swap(incomingSnapshot);
    if (restoresProtected) protectedStartupPath_.clear();
    dirty_ = false;
    refreshList();
    refreshActions();
    return success(QStringLiteral("Đã mở từ điển."));
}

ControllerResult AppController::saveToFile(const QString& path) {
    return saveToFileImpl(path, false);
}

ControllerResult AppController::saveToFileImpl(const QString& path, bool allowProtectedOverwrite) {
    if (path.trimmed().isEmpty()) return invalid(QStringLiteral("Chọn tệp để lưu từ điển."));
    if (samePath(path, seedPath())) return invalid(QStringLiteral("Không thể ghi đè dữ liệu mẫu. Hãy chọn tệp khác."));
    const bool replacesProtected = !protectedStartupPath_.isEmpty() && samePath(path, protectedStartupPath_);
    if (replacesProtected && !allowProtectedOverwrite)
        return invalid(QStringLiteral("Tệp cá nhân bị lỗi đang được bảo vệ. Dùng Lưu thành và xác nhận thay thế."));
    DictionaryHistory::Snapshot prepared;
    QString nextPath;
    try {
        prepared = snapshotForSave();
        nextPath = QFileInfo(path).absoluteFilePath();
    } catch (const std::exception&) {
        return failed(QStringLiteral("Không thể chuẩn bị lưu. Chưa ghi tệp."));
    }
    try {
        if (!::saveDictionary(dictionary_, utf8(path)))
            return failed(QStringLiteral("Không thể lưu từ điển. Kiểm tra quyền ghi của thư mục."));
    } catch (const std::exception&) {
        return failed(QStringLiteral("Không thể lưu từ điển. Kiểm tra quyền ghi của thư mục."));
    }
    static_assert(std::is_nothrow_swappable<DictionaryHistory::Snapshot>::value,
                  "Committing a saved snapshot must not throw after disk write");
    savedSnapshot_.swap(prepared);
    currentPath_.swap(nextPath);
    if (replacesProtected) protectedStartupPath_.clear();
    dirty_ = false;
    refreshActions();
    return success(QStringLiteral("Đã lưu từ điển."));
}

DictionaryHistory::Snapshot AppController::snapshotForSave() const {
    return DictionaryHistory::capture(dictionary_);
}

ControllerResult AppController::undo() {
    if (!history_.canUndo()) return {ControllerStatus::NotFound, QStringLiteral("Không có thao tác để hoàn tác."), {}};
    if (!history_.undo(dictionary_)) return failed(QStringLiteral("Không thể hoàn tác."));
    try { dirty_ = DictionaryHistory::capture(dictionary_) != savedSnapshot_; }
    catch (const std::exception&) { dirty_ = true; }
    refreshList();
    refreshActions();
    return success(QStringLiteral("Đã hoàn tác."));
}

ControllerResult AppController::redo() {
    if (!history_.canRedo()) return {ControllerStatus::NotFound, QStringLiteral("Không có thao tác để làm lại."), {}};
    if (!history_.redo(dictionary_)) return failed(QStringLiteral("Không thể làm lại."));
    try { dirty_ = DictionaryHistory::capture(dictionary_) != savedSnapshot_; }
    catch (const std::exception&) { dirty_ = true; }
    refreshList();
    refreshActions();
    return success(QStringLiteral("Đã làm lại."));
}

void AppController::connectUi() {
    searchWidget_ = new SearchWidget(&mainWindow_);
    mainWindow_.getSearchContainerLayout()->addWidget(searchWidget_);
    connect(searchWidget_, &SearchWidget::suggestionRequested, this,
            [this](const QString& prefix) { searchWidget_->setSuggestions(suggestions(prefix)); });
    connect(searchWidget_, &SearchWidget::searchRequested, this, [this](const QString& term) {
        const auto result = searchWord(term);
        if (result.status == ControllerStatus::Success) {
            const WordNode* found = ::findWord(dictionary_, utf8(term));
            currentFilter_ = QStringLiteral("All Words");
            mainWindow_.setActiveFilter(currentFilter_);
            refreshList(QString::fromUtf8(found->english.c_str()));
            searchWidget_->hideSuggestions();
            searchWidget_->showFoundResult(QString::fromUtf8(found->english.c_str()), result.vietnamese);
        }
        else if (result.status == ControllerStatus::NotFound) searchWidget_->showNotFound(term);
        else searchWidget_->clearResult();
        showResult(result);
    });
    connect(&mainWindow_, &MainWindow::wordSelected, this, [this](const QString& term) {
        const auto result = searchWord(term);
        mainWindow_.setWordDetail(result.status == ControllerStatus::Success ? term : QString(),
                                  result.vietnamese, result.partOfSpeech, result.example,
                                  result.category);
    });
    connect(&mainWindow_, &MainWindow::filterRequested, this, [this](const QString& filter) {
        currentFilter_ = filter;
        refreshList();
    });
    connect(&mainWindow_, &MainWindow::addWordRequested, this, [this] { openWordDialog(false); });
    connect(&mainWindow_, &MainWindow::editWordRequested, this, [this] { openWordDialog(true); });
    connect(&mainWindow_, &MainWindow::deleteWordRequested, this, [this] { deleteSelectedWord(); });

    auto* fileMenu = mainWindow_.menuBar()->addMenu(QStringLiteral("&Tệp"));
    auto* openAction = fileMenu->addAction(QStringLiteral("&Mở…"));
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, [this] { openFile(); });
    auto* saveAction = fileMenu->addAction(QStringLiteral("&Lưu"));
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, [this] { saveFile(false); });
    auto* saveAsAction = fileMenu->addAction(QStringLiteral("Lưu &thành…"));
    connect(saveAsAction, &QAction::triggered, this, [this] { saveFile(true); });
    fileMenu->addSeparator();
    auto* exitAction = fileMenu->addAction(QStringLiteral("T&hoát"));
    connect(exitAction, &QAction::triggered, &mainWindow_, &QWidget::close);

    auto* editMenu = mainWindow_.menuBar()->addMenu(QStringLiteral("&Chỉnh sửa"));
    undoAction_ = editMenu->addAction(QStringLiteral("&Hoàn tác"));
    undoAction_->setShortcut(QKeySequence::Undo);
    connect(undoAction_, &QAction::triggered, this, [this] { showResult(undo()); });
    redoAction_ = editMenu->addAction(QStringLiteral("&Làm lại"));
    redoAction_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Y));
    connect(redoAction_, &QAction::triggered, this, [this] { showResult(redo()); });
}

void AppController::refreshList(const QString& select) {
    if (searchWidget_) searchWidget_->clearResult();
    if (searchWidget_ && !searchWidget_->query().trimmed().isEmpty())
        searchWidget_->setSuggestions(suggestions(searchWidget_->query()));
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
    mainWindow_.setTotalWordCount(static_cast<int>(listAllWords(dictionary_).size()));
    mainWindow_.setWordList(words);
    if (!selected.isEmpty()) {
        auto* list = mainWindow_.findChild<QListWidget*>(QStringLiteral("wordList"));
        if (list) {
            const auto matches = list->findItems(selected, Qt::MatchExactly);
            if (!matches.isEmpty()) list->setCurrentItem(matches.front());
        }
    }
    if (mainWindow_.selectedWord().isEmpty()) mainWindow_.setWordDetail({}, {});
    mainWindow_.statusBar()->showMessage(words.isEmpty() ? QStringLiteral("Không có thuật ngữ trong mục này.") :
                                        QStringLiteral("Đang hiển thị %1 thuật ngữ.").arg(words.size()));
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
        if (result.status == ControllerStatus::Success) {
            mainWindow_.statusBar()->showMessage(listAllWords(dictionary_).empty()
                ? QStringLiteral("Tệp từ điển cá nhân đang rỗng: %1. Dữ liệu mẫu không được nạp.").arg(editable)
                : QStringLiteral("Đã nạp %1 thuật ngữ từ tệp cá nhân: %2").arg(listAllWords(dictionary_).size()).arg(editable));
            return;
        }
        protectedStartupPath_ = editable;
        currentPath_.clear();
        QMessageBox recovery(&mainWindow_);
        recovery.setIcon(QMessageBox::Warning);
        recovery.setWindowTitle(QStringLiteral("Khôi phục từ điển"));
        recovery.setText(QStringLiteral("Không thể mở từ điển cá nhân. Tệp gốc được giữ nguyên."));
        const bool hasSeed = QFileInfo::exists(seedPath());
        recovery.setInformativeText(hasSeed
            ? QStringLiteral("Mở tệp khác, nạp dữ liệu mẫu hoặc bắt đầu với từ điển rỗng. Ứng dụng sẽ hỏi trước khi thay thế tệp lỗi.")
            : QStringLiteral("Mở tệp khác hoặc bắt đầu với từ điển rỗng. Ứng dụng sẽ hỏi trước khi thay thế tệp lỗi."));
        auto* openButton = recovery.addButton(QStringLiteral("Mở tệp khác…"), QMessageBox::ActionRole);
        auto* seedButton = hasSeed
            ? recovery.addButton(QStringLiteral("Nạp dữ liệu mẫu"), QMessageBox::ActionRole) : nullptr;
        auto* emptyButton = recovery.addButton(QStringLiteral("Bắt đầu rỗng"), QMessageBox::AcceptRole);
        recovery.setDefaultButton(qobject_cast<QPushButton*>(emptyButton));
        recovery.exec();
        if (recovery.clickedButton() == openButton) {
            const QString other = QFileDialog::getOpenFileName(&mainWindow_, QStringLiteral("Mở từ điển"),
                QFileInfo(editable).absolutePath(), QStringLiteral("Dictionary TSV (*.txt *.tsv);;All files (*)"));
            if (!other.isEmpty()) showResult(loadFromFile(other));
        } else if (seedButton && recovery.clickedButton() == seedButton) {
            const auto seedResult = loadFromFile(seedPath());
            if (seedResult.status == ControllerStatus::Success) showResult(seedResult);
            else mainWindow_.statusBar()->showMessage(QStringLiteral("Không thể nạp dữ liệu mẫu. Từ điển đang rỗng."));
        }
        return;
    }
    if (QFileInfo::exists(seedPath())) {
        const auto result = loadFromFile(seedPath());
        if (result.status == ControllerStatus::Success)
            mainWindow_.statusBar()->showMessage(QStringLiteral("Đã nạp %1 thuật ngữ từ dữ liệu mẫu.").arg(listAllWords(dictionary_).size()));
        else showResult(result);
    } else mainWindow_.statusBar()->showMessage(QStringLiteral("Không tìm thấy dữ liệu mẫu. Hãy mở tệp từ điển hoặc thêm từ mới."));
    if (currentPath_.isEmpty()) currentPath_ = editable;
}

bool AppController::confirmDiscardChanges() {
    if (!dirty_) return true;
    const auto choice = QMessageBox::warning(&mainWindow_, QStringLiteral("Thay đổi chưa lưu"),
        QStringLiteral("Bạn có muốn lưu thay đổi vào từ điển không?"),
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
    const QString path = QFileDialog::getOpenFileName(&mainWindow_, QStringLiteral("Mở từ điển"),
        QFileInfo(currentPath_).absolutePath(), QStringLiteral("Dictionary TSV (*.txt *.tsv);;All files (*)"));
    if (!path.isEmpty()) showResult(loadFromFile(path));
}

bool AppController::saveFile(bool saveAs) {
    QString path = currentPath_;
    if (saveAs || path.isEmpty()) path = QFileDialog::getSaveFileName(&mainWindow_,
        QStringLiteral("Lưu từ điển"), path, QStringLiteral("Từ điển TSV (*.txt *.tsv);;Tất cả tệp (*)"));
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
    auto* dialog = edit ? new WordDialog(WordDialog::Mode::Edit, term, existing.vietnamese,
                                        existing.partOfSpeech, existing.example,
                                        existing.category, &mainWindow_)
                        : new WordDialog(WordDialog::Mode::Add, &mainWindow_);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(dialog, &WordDialog::richSaveRequested, this,
            [this, dialog, edit](const QString& english, const QString& vietnamese,
                                  const QString& partOfSpeech, const QString& example,
                                  const QString& category) {
        const auto result = edit ? updateWord(english, vietnamese, partOfSpeech, example, category)
                                 : addWord(english, vietnamese, partOfSpeech, example, category);
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
