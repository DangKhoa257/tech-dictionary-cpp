#include "dictionary.h"
#include "dictionary_history.h"

#include <cstddef>
#include <iostream>
#include <string>
#include <utility>

namespace {

int failures = 0;
int checks = 0;

void expect(bool condition, const std::string& description) {
    ++checks;
    if (!condition) {
        ++failures;
        std::cerr << "[FAIL] " << description << '\n';
    }
}

void appendWord(Dictionary& dictionary, const std::string& english,
                const std::string& vietnamese) {
    const std::size_t bucket = hashWord(english);
    if (bucket >= 26) {
        return;
    }

    auto* node = new WordNode{english, vietnamese, nullptr};
    WordNode** tail = &dictionary.buckets[bucket];
    while (*tail != nullptr) {
        tail = &((*tail)->next);
    }
    *tail = node;
}

const WordNode* findExact(const Dictionary& dictionary, std::size_t bucket,
                          const std::string& english) {
    if (bucket >= 26) {
        return nullptr;
    }
    for (const WordNode* node = dictionary.buckets[bucket];
         node != nullptr; node = node->next) {
        if (node->english == english) {
            return node;
        }
    }
    return nullptr;
}

std::size_t countBucket(const Dictionary& dictionary, std::size_t bucket) {
    std::size_t count = 0;
    if (bucket < 26) {
        for (const WordNode* node = dictionary.buckets[bucket];
             node != nullptr; node = node->next) {
            ++count;
        }
    }
    return count;
}

void testUpdateExistingWord() {
    Dictionary dictionary;
    appendWord(dictionary, "Algorithm", "thuat toan");
    appendWord(dictionary, "Array", "mang");

    expect(updateWordMeaning(dictionary, "Algorithm", "Thuật toán"),
           "update existing word succeeds");
    const WordNode* updated = findExact(dictionary, 0, "Algorithm");
    expect(updated != nullptr && updated->vietnamese == "Thuật toán",
           "update changes the requested Vietnamese meaning");
    expect(findExact(dictionary, 0, "Array")->vietnamese == "mang",
           "update leaves other nodes unchanged");

    clearDictionary(dictionary);
}

void testUpdateNonexistentWord() {
    Dictionary dictionary;
    appendWord(dictionary, "Apple", "tao");

    expect(!updateWordMeaning(dictionary, "Apricot", "qua mo"),
           "updating a nonexistent word returns false");
    expect(countBucket(dictionary, 0) == 1 &&
               findExact(dictionary, 0, "Apple") != nullptr,
           "nonexistent update preserves dictionary integrity");

    clearDictionary(dictionary);
}

void testUpdateEmptyMeaning() {
    Dictionary dictionary;
    appendWord(dictionary, "Book", "sach");

    expect(!updateWordMeaning(dictionary, "Book", ""),
           "empty meaning is rejected");
    expect(!updateWordMeaning(dictionary, "Book", "   "),
           "whitespace-only meaning is rejected");
    expect(findExact(dictionary, 1, "Book")->vietnamese == "sach",
           "invalid meaning does not overwrite existing meaning");

    clearDictionary(dictionary);
}

void testDeleteFirstMiddleLastAndOnlyNodes() {
    {
        Dictionary dictionary;
        appendWord(dictionary, "Apple", "tao");
        appendWord(dictionary, "Apricot", "mo");
        appendWord(dictionary, "Avocado", "bo");

        expect(deleteWord(dictionary, "Apple"), "delete first node succeeds");
        expect(dictionary.buckets[0] != nullptr &&
                   dictionary.buckets[0]->english == "Apricot",
               "deleting first node updates bucket head");
        expect(countBucket(dictionary, 0) == 2,
               "first-node deletion preserves remaining nodes");
        clearDictionary(dictionary);
    }

    {
        Dictionary dictionary;
        appendWord(dictionary, "Apple", "tao");
        appendWord(dictionary, "Apricot", "mo");
        appendWord(dictionary, "Avocado", "bo");

        expect(deleteWord(dictionary, "Apricot"), "delete middle node succeeds");
        expect(dictionary.buckets[0]->next != nullptr &&
                   dictionary.buckets[0]->next->english == "Avocado" &&
                   dictionary.buckets[0]->next->next == nullptr,
               "middle-node deletion reconnects neighbors");
        clearDictionary(dictionary);
    }

    {
        Dictionary dictionary;
        appendWord(dictionary, "Apple", "tao");
        appendWord(dictionary, "Apricot", "mo");
        appendWord(dictionary, "Avocado", "bo");

        expect(deleteWord(dictionary, "Avocado"), "delete last node succeeds");
        expect(countBucket(dictionary, 0) == 2 &&
                   dictionary.buckets[0]->next->next == nullptr,
               "last-node deletion terminates the list correctly");
        clearDictionary(dictionary);
    }

    {
        Dictionary dictionary;
        appendWord(dictionary, "Only", "duy nhat");

        expect(deleteWord(dictionary, "Only"), "delete only node succeeds");
        expect(dictionary.buckets[14] == nullptr,
               "deleting only node empties its bucket");
        clearDictionary(dictionary);
    }
}

void testDeleteMissingAndRepeatedDelete() {
    Dictionary dictionary;
    appendWord(dictionary, "Desk", "ban");
    appendWord(dictionary, "Door", "cua");

    expect(!deleteWord(dictionary, "Dog"),
           "deleting a nonexistent word returns false");
    expect(deleteWord(dictionary, "Desk"), "first delete succeeds");
    expect(!deleteWord(dictionary, "Desk"), "repeated delete returns false");
    expect(countBucket(dictionary, 3) == 1 &&
               findExact(dictionary, 3, "Door") != nullptr,
           "missing and repeated deletes preserve unrelated node");

    clearDictionary(dictionary);
}

void testCaseInsensitiveOperations() {
    Dictionary dictionary;
    appendWord(dictionary, "Computer", "may tinh");
    appendWord(dictionary, "Camera", "may anh");

    expect(updateWordMeaning(dictionary, "cOmPuTeR", "máy vi tính"),
           "update matches ASCII letters without case sensitivity");
    expect(findExact(dictionary, 2, "Computer")->vietnamese == "máy vi tính",
           "case-insensitive update preserves stored English key");
    expect(deleteWord(dictionary, "cAMERA"),
           "delete matches ASCII letters without case sensitivity");
    expect(findExact(dictionary, 2, "Camera") == nullptr,
           "case-insensitive delete removes the matching node");
    expect(countBucket(dictionary, 2) == 1,
           "case-insensitive operations leave other bucket nodes intact");

    clearDictionary(dictionary);
}

void testEmptyAndInvalidEnglishInput() {
    Dictionary dictionary;
    appendWord(dictionary, "Apple", "tao");

    expect(!updateWordMeaning(dictionary, "", "nghia"),
           "update rejects empty English key");
    expect(!deleteWord(dictionary, ""),
           "delete rejects empty English key");
    expect(!updateWordMeaning(dictionary, "1Apple", "nghia"),
           "update rejects key beginning with a digit");
    expect(!deleteWord(dictionary, "_Apple"),
           "delete rejects key beginning with punctuation");
    expect(!updateWordMeaning(dictionary, " Apple", "nghia"),
           "update rejects key beginning with whitespace");
    expect(countBucket(dictionary, 0) == 1,
           "invalid English input does not alter dictionary");

    clearDictionary(dictionary);
}

void testDictionaryIntegrityAfterOperations() {
    Dictionary dictionary;
    appendWord(dictionary, "Alpha", "a");
    appendWord(dictionary, "Atom", "b");
    appendWord(dictionary, "Beta", "c");
    appendWord(dictionary, "Bravo", "d");

    expect(updateWordMeaning(dictionary, "ATOM", "đã cập nhật"),
           "integrity scenario updates one node");
    expect(deleteWord(dictionary, "alpha"),
           "integrity scenario deletes bucket head");
    expect(deleteWord(dictionary, "Bravo"),
           "integrity scenario deletes another bucket's last node");

    expect(countBucket(dictionary, 0) == 1 &&
               findExact(dictionary, 0, "Atom") != nullptr,
           "A bucket contains only the expected remaining node");
    expect(countBucket(dictionary, 1) == 1 &&
               findExact(dictionary, 1, "Beta") != nullptr,
           "B bucket contains only the expected remaining node");
    expect(findExact(dictionary, 0, "Atom")->vietnamese == "đã cập nhật",
           "updated UTF-8 meaning remains intact");
    expect(dictionary.buckets[2] == nullptr,
           "unrelated buckets remain empty");

    clearDictionary(dictionary);
}


void testUndoRedoForAddUpdateDeleteSnapshots() {
    Dictionary dictionary;
    DictionaryHistory history;

    // Simulate a successful add operation performed by TV3/controller.
    auto beforeAdd = DictionaryHistory::capture(dictionary);
    appendWord(dictionary, "Apple", "tao");
    auto afterAdd = DictionaryHistory::capture(dictionary);
    history.recordChange(std::move(beforeAdd), std::move(afterAdd));

    expect(history.canUndo() && !history.canRedo(),
           "successful add creates one undo entry");
    expect(history.undo(dictionary), "undo add succeeds");
    expect(dictionary.buckets[0] == nullptr, "undo add restores empty bucket");
    expect(history.redo(dictionary), "redo add succeeds");
    expect(findExact(dictionary, 0, "Apple") != nullptr,
           "redo add restores the word");

    // Update is performed by TV4; capture snapshots around the successful call.
    auto beforeUpdate = DictionaryHistory::capture(dictionary);
    expect(updateWordMeaning(dictionary, "Apple", "quả táo"),
           "history scenario updates existing word");
    auto afterUpdate = DictionaryHistory::capture(dictionary);
    history.recordChange(std::move(beforeUpdate), std::move(afterUpdate));

    expect(history.undo(dictionary), "undo update succeeds");
    expect(findExact(dictionary, 0, "Apple")->vietnamese == "tao",
           "undo update restores previous meaning");
    expect(history.redo(dictionary), "redo update succeeds");
    expect(findExact(dictionary, 0, "Apple")->vietnamese == "quả táo",
           "redo update reapplies new meaning");

    // Delete is performed by TV4; record only when it succeeds.
    auto beforeDelete = DictionaryHistory::capture(dictionary);
    expect(deleteWord(dictionary, "Apple"), "history scenario deletes word");
    auto afterDelete = DictionaryHistory::capture(dictionary);
    history.recordChange(std::move(beforeDelete), std::move(afterDelete));

    expect(history.undo(dictionary), "undo delete succeeds");
    expect(findExact(dictionary, 0, "Apple") != nullptr,
           "undo delete restores deleted word");
    expect(history.redo(dictionary), "redo delete succeeds");
    expect(findExact(dictionary, 0, "Apple") == nullptr,
           "redo delete removes the word again");
    expect(!history.canRedo(), "redo stack is empty after replaying all changes");

    clearDictionary(dictionary);
}

void testUndoRedoBranchAndEmptyHistory() {
    Dictionary dictionary;
    DictionaryHistory history;

    expect(!history.undo(dictionary), "undo on empty history returns false");
    expect(!history.redo(dictionary), "redo on empty history returns false");

    auto before = DictionaryHistory::capture(dictionary);
    appendWord(dictionary, "Book", "sach");
    history.recordChange(std::move(before), DictionaryHistory::capture(dictionary));

    expect(history.undo(dictionary), "undo before branch test succeeds");
    auto beforeNewChange = DictionaryHistory::capture(dictionary);
    appendWord(dictionary, "Ball", "bong");
    history.recordChange(std::move(beforeNewChange),
                         DictionaryHistory::capture(dictionary));

    expect(!history.canRedo(), "new mutation after undo clears redo history");
    expect(findExact(dictionary, 1, "Ball") != nullptr,
           "new branch mutation remains in dictionary");

    history.clear();
    expect(!history.canUndo() && !history.canRedo(),
           "clear resets both history stacks");
    clearDictionary(dictionary);
}

}  // namespace

int main() {
    testUpdateExistingWord();
    testUpdateNonexistentWord();
    testUpdateEmptyMeaning();
    testDeleteFirstMiddleLastAndOnlyNodes();
    testDeleteMissingAndRepeatedDelete();
    testCaseInsensitiveOperations();
    testEmptyAndInvalidEnglishInput();
    testDictionaryIntegrityAfterOperations();
    testUndoRedoForAddUpdateDeleteSnapshots();
    testUndoRedoBranchAndEmptyHistory();

    if (failures != 0) {
        std::cerr << failures << " of " << checks << " checks failed.\n";
        return 1;
    }

    std::cout << "All " << checks << " checks passed.\n";
    return 0;
}
