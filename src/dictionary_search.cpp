#include "dictionary.h"

#include <cctype>
#include <string>

namespace {

// Remove whitespace only at the beginning and end. Internal characters,
// including punctuation, are preserved.
std::string trimEnglish(const std::string& value) {
    std::size_t first = 0;
    while (first < value.size() &&
           std::isspace(static_cast<unsigned char>(value[first]))) {
        ++first;
    }

    std::size_t last = value.size();
    while (last > first &&
           std::isspace(static_cast<unsigned char>(value[last - 1]))) {
        --last;
    }

    return value.substr(first, last - first);
}

bool equalsIgnoreCaseAscii(const std::string& left,
                           const std::string& right) {
    if (left.size() != right.size()) {
        return false;
    }

    for (std::size_t i = 0; i < left.size(); ++i) {
        const unsigned char leftCharacter =
            static_cast<unsigned char>(left[i]);
        const unsigned char rightCharacter =
            static_cast<unsigned char>(right[i]);

        if (std::tolower(leftCharacter) != std::tolower(rightCharacter)) {
            return false;
        }
    }

    return true;
}

}  // namespace

bool addWord(Dictionary& dictionary, const std::string& english,
             const std::string& vietnamese) {
    const std::string normalizedEnglish = trimEnglish(english);
    const std::string normalizedMeaning = trimEnglish(vietnamese);

    if (normalizedEnglish.empty() || normalizedMeaning.empty()) {
        return false;
    }

    // The shared dictionary has exactly 26 A-Z buckets. Therefore the
    // first character must be an ASCII English letter. Punctuation is
    // accepted elsewhere in the term (for example, C++, C#, O(n)).
    const std::size_t bucketIndex = hashWord(normalizedEnglish);
    if (bucketIndex >= 26) {
        return false;
    }

    WordNode* currentNode = dictionary.buckets[bucketIndex];
    while (currentNode != nullptr) {
        if (equalsIgnoreCaseAscii(currentNode->english,
                                  normalizedEnglish)) {
            // Duplicate policy: reject; do not overwrite the old meaning.
            return false;
        }
        currentNode = currentNode->next;
    }

    WordNode* newNode = new WordNode{
        normalizedEnglish, normalizedMeaning, nullptr
    };

    if (dictionary.buckets[bucketIndex] == nullptr) {
        dictionary.buckets[bucketIndex] = newNode;
        return true;
    }

    currentNode = dictionary.buckets[bucketIndex];
    while (currentNode->next != nullptr) {
        currentNode = currentNode->next;
    }
    currentNode->next = newNode;

    return true;
}

const WordNode* findWord(const Dictionary& dictionary,
                         const std::string& english) {
    const std::string normalizedEnglish = trimEnglish(english);
    if (normalizedEnglish.empty()) {
        return nullptr;
    }

    const std::size_t bucketIndex = hashWord(normalizedEnglish);
    if (bucketIndex >= 26) {
        return nullptr;
    }

    const WordNode* currentNode = dictionary.buckets[bucketIndex];
    while (currentNode != nullptr) {
        if (equalsIgnoreCaseAscii(currentNode->english,
                                  normalizedEnglish)) {
            return currentNode;
        }
        currentNode = currentNode->next;
    }

    return nullptr;
}
