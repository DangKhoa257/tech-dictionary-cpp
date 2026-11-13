#include "dictionary.h"
#include "dictionary_history.h"

#include <cstdio>
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
    expect(!updateWordMeaning(dictionary, "Book", "bad\tmeaning"),
           "tab in meaning is rejected");
    expect(!updateWordMeaning(dictionary, "Book", "bad\nmeaning"),
           "newline in meaning is rejected");
    expect(!updateWordMeaning(dictionary, "Book", std::string("bad\xFF", 4)),
           "invalid UTF-8 meaning is rejected");
    expect(updateWordMeaning(dictionary, "Book", "  Sách  "),
           "valid padded meaning is normalized");
    expect(findExact(dictionary, 1, "Book")->vietnamese == "Sách",
           "updated meaning is savable without outer spaces");
    const std::string path = "tv4_roundtrip_test.txt";
    expect(saveDictionary(dictionary, path), "updated dictionary saves through TV6");
    Dictionary loaded{};
    expect(loadDictionary(loaded, path), "updated dictionary reloads through TV6");
    const WordNode* reloaded = findWord(loaded, "book");
    expect(reloaded != nullptr && reloaded->vietnamese == "Sách",
           "TV3 search finds reloaded update");
    clearDictionary(loaded);
    std::remove(path.c_str());

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

void testUndoRedoAddWord() {
    Dictionary dictionary;
    DictionaryHistory history;

    auto beforeAdd = DictionaryHistory::capture(dictionary);
    appendWord(dictionary, "Algorithm", "Thuật toán");
    auto afterAdd = DictionaryHistory::capture(dictionary);
    history.recordChange(beforeAdd, afterAdd);

    expect(findExact(dictionary, 0, "Algorithm") != nullptr, "Dictionary contains Algorithm after Add");
    expect(history.undo(dictionary), "Undo add succeeds");
    expect(findExact(dictionary, 0, "Algorithm") == nullptr, "Undo removes Algorithm");
    expect(history.redo(dictionary), "Redo add succeeds");
    const WordNode* node = findExact(dictionary, 0, "Algorithm");
    expect(node != nullptr && node->vietnamese == "Thuật toán", "Redo restores Algorithm with original meaning");

    clearDictionary(dictionary);
}

void testUndoRedoUpdateWord() {
    Dictionary dictionary;
    DictionaryHistory history;
    appendWord(dictionary, "Algorithm", "Thuật toán");

    auto beforeUpdate = DictionaryHistory::capture(dictionary);
    expect(updateWordMeaning(dictionary, "Algorithm", "Thuật toán mới"), "Update succeeds");
    auto afterUpdate = DictionaryHistory::capture(dictionary);
    history.recordChange(beforeUpdate, afterUpdate);

    expect(history.undo(dictionary), "Undo update succeeds");
    const WordNode* node1 = findExact(dictionary, 0, "Algorithm");
    expect(node1 != nullptr && node1->vietnamese == "Thuật toán", "Undo restores original meaning");
    expect(history.redo(dictionary), "Redo update succeeds");
    const WordNode* node2 = findExact(dictionary, 0, "Algorithm");
    expect(node2 != nullptr && node2->vietnamese == "Thuật toán mới", "Redo reapplies new meaning");

    clearDictionary(dictionary);
}

void testUndoRedoDeleteWord() {
    Dictionary dictionary;
    DictionaryHistory history;
    appendWord(dictionary, "Algorithm", "Thuật toán");

    auto beforeDelete = DictionaryHistory::capture(dictionary);
    expect(deleteWord(dictionary, "Algorithm"), "Delete succeeds");
    auto afterDelete = DictionaryHistory::capture(dictionary);
    history.recordChange(beforeDelete, afterDelete);

    expect(findExact(dictionary, 0, "Algorithm") == nullptr, "Algorithm disappears after delete");
    expect(history.undo(dictionary), "Undo delete succeeds");
    const WordNode* node1 = findExact(dictionary, 0, "Algorithm");
    expect(node1 != nullptr && node1->vietnamese == "Thuật toán", "Undo restores deleted word with previous meaning");
    expect(history.redo(dictionary), "Redo delete succeeds");
    expect(findExact(dictionary, 0, "Algorithm") == nullptr, "Redo deletes word again");

    clearDictionary(dictionary);
}

void testUndoRedoMultipleOperations() {
    Dictionary dictionary;
    DictionaryHistory history;

    auto b1 = DictionaryHistory::capture(dictionary);
    appendWord(dictionary, "Apple", "Quả táo");
    history.recordChange(b1, DictionaryHistory::capture(dictionary));

    auto b2 = DictionaryHistory::capture(dictionary);
    appendWord(dictionary, "Banana", "Quả chuối");
    history.recordChange(b2, DictionaryHistory::capture(dictionary));

    auto b3 = DictionaryHistory::capture(dictionary);
    expect(updateWordMeaning(dictionary, "Apple", "Táo tây"), "Update Apple succeeds");
    history.recordChange(b3, DictionaryHistory::capture(dictionary));

    expect(history.undo(dictionary), "Undo Update A succeeds");
    expect(findExact(dictionary, 0, "Apple")->vietnamese == "Quả táo", "Revert Update A");
    expect(history.undo(dictionary), "Undo Add B succeeds");
    expect(findExact(dictionary, 1, "Banana") == nullptr, "Remove B");
    expect(history.redo(dictionary), "Redo Add B succeeds");
    expect(findExact(dictionary, 1, "Banana")->vietnamese == "Quả chuối", "Restore B");
    expect(history.redo(dictionary), "Redo Update A succeeds");
    expect(findExact(dictionary, 0, "Apple")->vietnamese == "Táo tây", "Reapply Update A");

    clearDictionary(dictionary);
}

void testUndoRedoBranchInvalidation() {
    Dictionary dictionary;
    DictionaryHistory history;

    auto b1 = DictionaryHistory::capture(dictionary);
    appendWord(dictionary, "Apple", "Táo");
    history.recordChange(b1, DictionaryHistory::capture(dictionary));

    auto b2 = DictionaryHistory::capture(dictionary);
    appendWord(dictionary, "Banana", "Chuối");
    history.recordChange(b2, DictionaryHistory::capture(dictionary));

    expect(history.undo(dictionary), "Undo B succeeds");
    expect(findExact(dictionary, 1, "Banana") == nullptr, "B is removed");
    expect(history.canRedo(), "Redo is available before new action");

    auto b3 = DictionaryHistory::capture(dictionary);
    appendWord(dictionary, "Cherry", "Anh đào");
    history.recordChange(b3, DictionaryHistory::capture(dictionary));

    expect(!history.canRedo(), "Redo is unavailable after new action on branched history");
    expect(!history.redo(dictionary), "Redo returns false when redo stack is invalidated");

    clearDictionary(dictionary);
}

void testUndoRedoEmptyHistory() {
    Dictionary dictionary;
    DictionaryHistory history;

    expect(!history.canUndo(), "canUndo is false for empty history");
    expect(!history.canRedo(), "canRedo is false for empty history");
    expect(!history.undo(dictionary), "Undo returns false on empty history");
    expect(!history.redo(dictionary), "Redo returns false on empty history");
}

void testUndoRedoPreservation() {
    Dictionary dictionary;
    DictionaryHistory history;

    auto b1 = DictionaryHistory::capture(dictionary);
    appendWord(dictionary, "Alpha", "Số một - α");
    appendWord(dictionary, "Atom", "Hạt nhân - Atom");
    appendWord(dictionary, "Beta", "Số hai - β");
    appendWord(dictionary, "Zebra", "Ngựa vằn - UTF8");
    history.recordChange(b1, DictionaryHistory::capture(dictionary));

    const WordNode* head = dictionary.buckets[0];
    expect(head != nullptr && head->english == "Alpha", "Bucket A first node is Alpha");
    expect(head->next != nullptr && head->next->english == "Atom", "Bucket A second node is Atom");

    auto b2 = DictionaryHistory::capture(dictionary);
    updateWordMeaning(dictionary, "Atom", "Nguyên tử");
    history.recordChange(b2, DictionaryHistory::capture(dictionary));

    history.undo(dictionary);
    const WordNode* restoredHead = dictionary.buckets[0];
    expect(restoredHead != nullptr && restoredHead->english == "Alpha", "Preserved order: Alpha");
    expect(restoredHead->next != nullptr && restoredHead->next->english == "Atom" && restoredHead->next->vietnamese == "Hạt nhân - Atom", "Preserved order & UTF-8: Atom");
    expect(dictionary.buckets[25] != nullptr && dictionary.buckets[25]->english == "Zebra" && dictionary.buckets[25]->vietnamese == "Ngựa vằn - UTF8", "Bucket 25 preserved");

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

    testUndoRedoAddWord();
    testUndoRedoUpdateWord();
    testUndoRedoDeleteWord();
    testUndoRedoMultipleOperations();
    testUndoRedoBranchInvalidation();
    testUndoRedoEmptyHistory();
    testUndoRedoPreservation();

    if (failures != 0) {
        std::cerr << failures << " of " << checks << " checks failed.\n";
        return 1;
    }

    std::cout << "All " << checks << " checks passed.\n";
    return 0;
}
