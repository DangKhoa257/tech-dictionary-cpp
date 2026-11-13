#include "dictionary.h"

#include <cstddef>
#include <iostream>
#include <string>

namespace {

int failures = 0;

void check(bool condition, const char* description) {
    if (!condition) {
        std::cerr << "FAIL: " << description << '\n';
        ++failures;
    }
}

std::size_t countWords(const Dictionary& dictionary) {
    std::size_t count = 0;
    for (std::size_t bucket = 0; bucket < 26; ++bucket) {
        for (const WordNode* node = dictionary.buckets[bucket];
             node != nullptr; node = node->next) {
            ++count;
        }
    }
    return count;
}

}  // namespace

int main() {
    Dictionary dictionary{};

    check(addWord(dictionary, "Computer", "Máy tính"),
          "valid add succeeds");
    const WordNode* computer = findWord(dictionary, "Computer");
    check(computer != nullptr, "valid search finds inserted word");
    check(computer != nullptr && computer->english == "Computer",
          "English spelling is preserved");
    check(computer != nullptr && computer->vietnamese == "Máy tính",
          "Vietnamese UTF-8 meaning is preserved");

    check(findWord(dictionary, "computer") != nullptr,
          "lowercase search is case-insensitive");
    check(findWord(dictionary, "COMPUTER") != nullptr,
          "uppercase search is case-insensitive");
    check(findWord(dictionary, "  Computer  ") != nullptr,
          "search trims leading and trailing spaces");

    check(addWord(dictionary, "  Compiler  ", "  Trình biên dịch  "),
          "insertion trims leading and trailing spaces");
    const WordNode* compiler = findWord(dictionary, "Compiler");
    check(compiler != nullptr && compiler->english == "Compiler",
          "trimmed English term is stored");
    check(compiler != nullptr &&
              compiler->vietnamese == "Trình biên dịch",
          "trimmed Vietnamese meaning is stored");

    const std::size_t countBeforeDuplicate = countWords(dictionary);
    check(!addWord(dictionary, "computer", "Nghĩa mới"),
          "case-insensitive duplicate is rejected");
    computer = findWord(dictionary, "COMPUTER");
    check(computer != nullptr && computer->vietnamese == "Máy tính",
          "duplicate rejection preserves original meaning");
    check(countWords(dictionary) == countBeforeDuplicate,
          "duplicate rejection does not change dictionary size");

    check(addWord(dictionary, "C++", "Ngôn ngữ lập trình"),
          "C++ is accepted");
    check(addWord(dictionary, "C#", "Ngôn ngữ lập trình"),
          "C# is accepted");
    check(addWord(dictionary, "O(n)", "Độ phức tạp thuật toán"),
          "O(n) is accepted");
    check(addWord(dictionary, "Machine Learning", "Học máy"),
          "multi-word technical term is accepted");
    check(findWord(dictionary, "c++") != nullptr,
          "punctuated term is found case-insensitively");
    check(findWord(dictionary, "o(N)") != nullptr,
          "ASCII case-insensitive search preserves punctuation");

    const std::size_t countBeforeInvalid = countWords(dictionary);
    check(!addWord(dictionary, "", "Nghĩa"), "empty English is rejected");
    check(!addWord(dictionary, "   ", "Nghĩa"),
          "whitespace-only English is rejected");
    check(!addWord(dictionary, "Keyboard", ""),
          "empty Vietnamese meaning is rejected");
    check(!addWord(dictionary, "Keyboard", "   "),
          "whitespace-only Vietnamese meaning is rejected");
    check(!addWord(dictionary, "123abc", "Nghĩa"),
          "digit as initial character is rejected");
    check(!addWord(dictionary, "@term", "Nghĩa"),
          "punctuation as initial character is rejected");
    check(!addWord(dictionary, "éclair", "Nghĩa"),
          "non-ASCII initial character is rejected");

    check(!addWord(dictionary, "Bad\tTerm", "Nghĩa"),
          "tab in English field is rejected");
    check(!addWord(dictionary, "BadMeaning", "Nghĩa\tphụ"),
          "tab in Vietnamese field is rejected");
    check(!addWord(dictionary, "Bad\nTerm", "Nghĩa"),
          "newline in English field is rejected");
    check(!addWord(dictionary, "BadTerm", "Nghĩa\nphụ"),
          "newline in Vietnamese field is rejected");
    check(!addWord(dictionary, "Bad\rTerm", "Nghĩa"),
          "carriage return in English field is rejected");
    check(!addWord(dictionary, "BadTerm", "Nghĩa\rphụ"),
          "carriage return in Vietnamese field is rejected");
    check(!addWord(dictionary, std::string("Bad") + char(0x01) + "Term",
                   "Nghĩa"),
          "other ASCII control byte in English field is rejected");
    check(!addWord(dictionary, "BadTerm",
                   std::string("Nghĩa") + char(0x7F)),
          "DEL in Vietnamese field is rejected");

    check(!addWord(dictionary, std::string("Bad") + char(0xC3), "Nghĩa"),
          "truncated UTF-8 sequence is rejected");
    check(!addWord(dictionary, "BadTerm", std::string("x") + char(0x80)),
          "isolated UTF-8 continuation byte is rejected");
    check(!addWord(dictionary, std::string("Bad") + char(0xC0) +
                   char(0xAF), "Nghĩa"),
          "overlong UTF-8 encoding is rejected");
    check(!addWord(dictionary, std::string("Bad") + char(0xED) +
                   char(0xA0) + char(0x80), "Nghĩa"),
          "UTF-8 surrogate encoding is rejected");
    check(!addWord(dictionary, "BadTerm",
                   std::string("x") + char(0xF4) + char(0x90) +
                   char(0x80) + char(0x80)),
          "UTF-8 code point above U+10FFFF is rejected");

    check(countWords(dictionary) == countBeforeInvalid,
          "all rejected insertions preserve dictionary size");
    check(findWord(dictionary, "Computer") == computer,
          "rejected insertions preserve existing node");
    check(findWord(dictionary, "Compiler") != nullptr,
          "rejected insertions preserve other existing entries");
    check(findWord(dictionary, "BadTerm") == nullptr,
          "rejected invalid entry is not searchable");
    check(findWord(dictionary, std::string("Bad") + char(0xFF)) == nullptr,
          "search rejects invalid UTF-8 input");

    // Verify 26-bucket placement and insertion-order singly linked lists.
    check(hashWord("Apple") == 0, "A maps to bucket 0");
    check(hashWord("zebra") == 25, "Z maps to bucket 25");
    check(hashWord("1abc") == 26, "invalid initial maps outside buckets");

    check(addWord(dictionary, "Alpha", "Một"),
          "first A-bucket entry inserts");
    check(addWord(dictionary, "Another", "Hai"),
          "second A-bucket entry inserts");
    const WordNode* alpha = dictionary.buckets[0];
    check(alpha != nullptr && alpha->english == "Alpha",
          "A bucket starts with first inserted node");
    check(alpha != nullptr && alpha->next != nullptr &&
              alpha->next->english == "Another",
          "collision chain preserves insertion order");
    check(alpha != nullptr && alpha->next != nullptr &&
              alpha->next->next == nullptr,
          "collision chain terminates correctly");

    check(addWord(dictionary, "Zulu", "Cuối"),
          "Z-bucket entry inserts");
    check(dictionary.buckets[25] != nullptr &&
              dictionary.buckets[25]->english == "Zulu",
          "Z term is placed in bucket 25");

    clearDictionary(dictionary);
    for (std::size_t bucket = 0; bucket < 26; ++bucket) {
        check(dictionary.buckets[bucket] == nullptr,
              "clearDictionary resets every bucket");
    }

    if (failures != 0) {
        std::cerr << failures << " test(s) failed.\n";
        return 1;
    }

    std::cout << "All DictionarySearchTests passed.\n";
    return 0;
}
