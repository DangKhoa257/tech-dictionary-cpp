#include "dictionary_history.h"

#include <utility>

namespace {

void deleteChain(WordNode* node) noexcept {
    while (node != nullptr) {
        WordNode* next = node->next;
        delete node;
        node = next;
    }
}

void deleteBuckets(std::array<WordNode*, 26>& buckets) noexcept {
    for (WordNode*& head : buckets) {
        deleteChain(head);
        head = nullptr;
    }
}

}  // namespace

DictionaryHistory::Snapshot DictionaryHistory::capture(const Dictionary& dictionary) {
    Snapshot snapshot;

    for (std::size_t bucket = 0; bucket < snapshot.size(); ++bucket) {
        for (const WordNode* node = dictionary.buckets[bucket];
             node != nullptr; node = node->next) {
            snapshot[bucket].push_back({node->english, node->vietnamese});
        }
    }

    return snapshot;
}

void DictionaryHistory::recordChange(Snapshot before, Snapshot after) {
    if (before == after) {
        return;
    }

    undoStack_.push_back({std::move(before), std::move(after)});
    // A new successful operation after Undo creates a new history branch.
    redoStack_.clear();
}

bool DictionaryHistory::canUndo() const noexcept {
    return !undoStack_.empty();
}

bool DictionaryHistory::canRedo() const noexcept {
    return !redoStack_.empty();
}

bool DictionaryHistory::restore(Dictionary& dictionary, const Snapshot& snapshot) {
    std::array<WordNode*, 26> replacement{};

    // Build the replacement lists first. If allocation fails, the current
    // dictionary remains untouched and all partially-created nodes are freed.
    try {
        for (std::size_t bucket = 0; bucket < snapshot.size(); ++bucket) {
            WordNode** tail = &replacement[bucket];
            for (const Entry& entry : snapshot[bucket]) {
                *tail = new WordNode{entry.english, entry.vietnamese, nullptr};
                tail = &((*tail)->next);
            }
        }
    } catch (...) {
        deleteBuckets(replacement);
        return false;
    }

    for (std::size_t bucket = 0; bucket < 26; ++bucket) {
        deleteChain(dictionary.buckets[bucket]);
        dictionary.buckets[bucket] = replacement[bucket];
        replacement[bucket] = nullptr;
    }

    return true;
}

bool DictionaryHistory::undo(Dictionary& dictionary) {
    if (undoStack_.empty()) {
        return false;
    }

    Change& change = undoStack_.back();
    if (!restore(dictionary, change.before)) {
        return false;
    }

    redoStack_.push_back(std::move(change));
    undoStack_.pop_back();
    return true;
}

bool DictionaryHistory::redo(Dictionary& dictionary) {
    if (redoStack_.empty()) {
        return false;
    }

    Change& change = redoStack_.back();
    if (!restore(dictionary, change.after)) {
        return false;
    }

    undoStack_.push_back(std::move(change));
    redoStack_.pop_back();
    return true;
}

void DictionaryHistory::clear() noexcept {
    undoStack_.clear();
    redoStack_.clear();
}

std::size_t DictionaryHistory::undoCount() const noexcept {
    return undoStack_.size();
}

std::size_t DictionaryHistory::redoCount() const noexcept {
    return redoStack_.size();
}
