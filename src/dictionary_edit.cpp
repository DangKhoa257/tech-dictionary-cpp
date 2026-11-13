#include "dictionary.h"

#include <cctype>
#include <cstddef>
#include <string>

namespace {

bool isValidUtf8TsvField(const std::string& value) {
    for (std::size_t i = 0; i < value.size();) {
        const unsigned char lead = static_cast<unsigned char>(value[i]);
        if (lead <= 0x1F || lead == 0x7F) return false;
        if (lead <= 0x7F) {
            ++i;
            continue;
        }
        std::size_t count = 0;
        unsigned char secondMin = 0x80;
        unsigned char secondMax = 0xBF;
        if (lead >= 0xC2 && lead <= 0xDF) count = 1;
        else if (lead == 0xE0) { count = 2; secondMin = 0xA0; }
        else if (lead >= 0xE1 && lead <= 0xEC) count = 2;
        else if (lead == 0xED) { count = 2; secondMax = 0x9F; }
        else if (lead >= 0xEE && lead <= 0xEF) count = 2;
        else if (lead == 0xF0) { count = 3; secondMin = 0x90; }
        else if (lead >= 0xF1 && lead <= 0xF3) count = 3;
        else if (lead == 0xF4) { count = 3; secondMax = 0x8F; }
        else return false;
        if (i + count >= value.size()) return false;
        const unsigned char second = static_cast<unsigned char>(value[i + 1]);
        if (second < secondMin || second > secondMax) return false;
        for (std::size_t offset = 2; offset <= count; ++offset) {
            const unsigned char continuation =
                static_cast<unsigned char>(value[i + offset]);
            if (continuation < 0x80 || continuation > 0xBF) return false;
        }
        i += count + 1;
    }
    return true;
}

bool isValidEnglishKey(const std::string& english) {
    if (english.empty() || !isValidUtf8TsvField(english)) {
        return false;
    }

    const unsigned char first = static_cast<unsigned char>(english.front());
    return (first >= 'A' && first <= 'Z') || (first >= 'a' && first <= 'z');
}

std::string trimSpaces(const std::string& value) {
    const std::size_t first = value.find_first_not_of(' ');
    if (first == std::string::npos) return {};
    const std::size_t last = value.find_last_not_of(' ');
    return value.substr(first, last - first + 1);
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
    if (!isValidUtf8TsvField(vietnamese)) {
        return false;
    }
    const std::string normalizedMeaning = trimSpaces(vietnamese);
    if (normalizedMeaning.empty()) return false;

    std::size_t bucketIndex = 0;
    if (!getValidBucketIndex(english, bucketIndex)) {
        return false;
    }

    for (WordNode* current = dictionary.buckets[bucketIndex];
         current != nullptr;
         current = current->next) {
        if (equalsAsciiCaseInsensitive(current->english, english)) {
            // Assign the original UTF-8 byte sequence without modifying it.
            current->vietnamese = normalizedMeaning;
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
