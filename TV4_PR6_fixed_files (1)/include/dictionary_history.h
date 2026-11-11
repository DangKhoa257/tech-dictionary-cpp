#pragma once

#include "dictionary.h"

#include <array>
#include <cstddef>
#include <string>
#include <vector>

// A value-only snapshot of the dictionary. It owns no WordNode pointers.
class DictionaryHistory {
public:
    struct Entry {
        std::string english;
        std::string vietnamese;

        bool operator==(const Entry& other) const {
            return english == other.english && vietnamese == other.vietnamese;
        }
    };

    using Snapshot = std::array<std::vector<Entry>, 26>;

    // Capture the current dictionary before and after a successful mutation.
    // Call recordChange(before, after) only when the mutation succeeded.
    static Snapshot capture(const Dictionary& dictionary);

    void recordChange(Snapshot before, Snapshot after);

    bool canUndo() const noexcept;
    bool canRedo() const noexcept;

    // Restore the previous/next snapshot. Returns false if no operation exists.
    bool undo(Dictionary& dictionary);
    bool redo(Dictionary& dictionary);

    void clear() noexcept;
    std::size_t undoCount() const noexcept;
    std::size_t redoCount() const noexcept;

private:
    struct Change {
        Snapshot before;
        Snapshot after;
    };

    static bool restore(Dictionary& dictionary, const Snapshot& snapshot);

    std::vector<Change> undoStack_;
    std::vector<Change> redoStack_;
};
