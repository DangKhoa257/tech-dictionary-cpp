#include "dictionary.h"

#include <cassert>
#include <string>

int main() {
    Dictionary dictionary{};

    // Add a valid word and search it.
    assert(addWord(dictionary, "Computer", "Máy tính"));
    const WordNode* computer = findWord(dictionary, "Computer");
    assert(computer != nullptr);
    assert(computer->english == "Computer");
    assert(computer->vietnamese == "Máy tính");

    // Search is case-insensitive.
    assert(findWord(dictionary, "computer") != nullptr);
    assert(findWord(dictionary, "COMPUTER") != nullptr);

    // Leading/trailing whitespace is trimmed.
    assert(findWord(dictionary, "  Computer  ") != nullptr);
    assert(addWord(dictionary, "  Compiler  ", "Trình biên dịch"));
    assert(findWord(dictionary, "Compiler") != nullptr);

    // Duplicate insertion is rejected and the original meaning is kept.
    assert(!addWord(dictionary, "computer", "Nghĩa mới"));
    computer = findWord(dictionary, "COMPUTER");
    assert(computer != nullptr);
    assert(computer->vietnamese == "Máy tính");

    // Punctuation is accepted inside a term when it starts with A-Z.
    assert(addWord(dictionary, "C++", "Ngôn ngữ lập trình"));
    assert(addWord(dictionary, "C#", "Ngôn ngữ lập trình"));
    assert(findWord(dictionary, "c++") != nullptr);
    assert(findWord(dictionary, "C#") != nullptr);

    // Invalid empty input and a term without an A-Z first character.
    assert(!addWord(dictionary, "", "Nghĩa"));
    assert(!addWord(dictionary, "   ", "Nghĩa"));
    assert(!addWord(dictionary, "123abc", "Nghĩa"));
    assert(!addWord(dictionary, "@term", "Nghĩa"));
    assert(findWord(dictionary, "") == nullptr);
    assert(findWord(dictionary, "@term") == nullptr);

    // Empty meaning is rejected.
    assert(!addWord(dictionary, "Keyboard", "   "));

    clearDictionary(dictionary);
    for (std::size_t i = 0; i < 26; ++i) {
        assert(dictionary.buckets[i] == nullptr);
    }

    return 0;
}
