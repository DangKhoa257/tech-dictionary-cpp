#include "dictionary.h"

#include <cctype>
#include <cstddef>
#include <string>

namespace {

bool isValidEnglishKey(const std::string& english) {
    if (english.empty()) {
        return false;
    }

    const unsigned char first = static_cast<unsigned char>(english.front());
    return (first >= 'A' && first <= 'Z') || (first >= 'a' && first <= 'z');
}

bool isBlank(const std::string& value) {
    for (unsigned char character : value) {
        if (!std::isspace(character)) {
            return false;
        }
    }
    return true;
}

bool equalsAsciiCaseInsensitive(const std::string& left, const std::string& right) {
    if (left.size() != right.size()) {
        return false;
    }

    for (std::size_t i = 0; i < left.size(); ++i) {
        unsigned char leftCharacter = static_cast<unsigned char>(left[i]);
        unsigned char rightCharacter = static_cast<unsigned char>(right[i]);

        if (leftCharacter >= 'A' && leftCharacter <= 'Z') {
            leftCharacter = static_cast<unsigned char>(leftCharacter - 'A' + 'a');
        }
        if (rightCharacter >= 'A' && rightCharacter <= 'Z') {
            rightCharacter = static_cast<unsigned char>(rightCharacter - 'A' + 'a');
        }

        if (leftCharacter != rightCharacter) {
            return false;
        }
    }

    return true;
}

bool getValidBucketIndex(const std::string& english, std::size_t& bucketIndex) {
    if (!isValidEnglishKey(english)) {
        return false;
    }

    // TV2 owns hashWord(). Its documented invalid-input sentinel is 26.
    bucketIndex = hashWord(english);
    return bucketIndex < 26;
}

}  // namespace

bool updateWordMeaning(Dictionary& dictionary, const std::string& english,
                       const std::string& vietnamese) {
    if (isBlank(vietnamese)) {
        return false;
    }

    std::size_t bucketIndex = 0;
    if (!getValidBucketIndex(english, bucketIndex)) {
        return false;
    }

    for (WordNode* current = dictionary.buckets[bucketIndex];
         current != nullptr;
         current = current->next) {
        if (equalsAsciiCaseInsensitive(current->english, english)) {
            // Assign the original UTF-8 byte sequence without modifying it.
            current->vietnamese = vietnamese;
            return true;
        }
    }

    return false;
}

bool deleteWord(Dictionary& dictionary, const std::string& english) {
    std::size_t bucketIndex = 0;
    if (!getValidBucketIndex(english, bucketIndex)) {
        return false;
    }

    WordNode* previous = nullptr;
    WordNode* current = dictionary.buckets[bucketIndex];

    while (current != nullptr) {
        if (equalsAsciiCaseInsensitive(current->english, english)) {
            if (previous == nullptr) {
                dictionary.buckets[bucketIndex] = current->next;
            } else {
                previous->next = current->next;
            }

            delete current;
            return true;
        }

        previous = current;
        current = current->next;
    }

    return false;
}


