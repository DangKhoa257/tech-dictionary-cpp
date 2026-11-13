#include "dictionary.h"

#include <cctype>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
bool isValidEnglishKey(const std::string& english) {
    if (english.empty()) {
        return false;
    }

    const unsigned char first = static_cast<unsigned char>(english.front());
    return (first >= 'A' && first <= 'Z') || (first >= 'a' && first <= 'z');
}
}

std::size_t hashWord(const std::string& english) {
    if (!isValidEnglishKey(english)) {
        throw std::invalid_argument("English word must start with an ASCII letter.");
    }

    const unsigned char first = static_cast<unsigned char>(english.front());
    if (first >= 'a' && first <= 'z') {
        return static_cast<std::size_t>(first - 'a');
    }

    return static_cast<std::size_t>(first - 'A');
}

void clearDictionary(Dictionary& dictionary) {
    for (std::size_t i = 0; i < 26; ++i) {
        WordNode* current = dictionary.buckets[i];
        dictionary.buckets[i] = nullptr;

        while (current != nullptr) {
            WordNode* next = current->next;
            delete current;
            current = next;
        }
    }
}

bool addWord(Dictionary& dictionary, const std::string& english, const std::string& vietnamese) {
    if (!isValidEnglishKey(english) || vietnamese.empty()) {
        return false;
    }

    const std::size_t bucketIndex = hashWord(english);
    WordNode* current = dictionary.buckets[bucketIndex];

    while (current != nullptr) {
        if (current->english == english) {
            return false;
        }
        current = current->next;
    }

    WordNode* newNode = new WordNode{english, vietnamese, nullptr};

    if (dictionary.buckets[bucketIndex] == nullptr) {
        dictionary.buckets[bucketIndex] = newNode;
        return true;
    }

    current = dictionary.buckets[bucketIndex];
    while (current->next != nullptr) {
        current = current->next;
    }
    current->next = newNode;

    return true;
}

const WordNode* findWord(const Dictionary& dictionary, const std::string& english) {
    if (!isValidEnglishKey(english)) {
        return nullptr;
    }

    const std::size_t bucketIndex = hashWord(english);
    const WordNode* current = dictionary.buckets[bucketIndex];

    while (current != nullptr) {
        if (current->english == english) {
            return current;
        }
        current = current->next;
    }

    return nullptr;
}

bool updateWordMeaning(Dictionary& dictionary, const std::string& english,
                       const std::string& vietnamese) {
    if (!isValidEnglishKey(english) || vietnamese.empty()) {
        return false;
    }

    const std::size_t bucketIndex = hashWord(english);
    WordNode* current = dictionary.buckets[bucketIndex];

    while (current != nullptr) {
        if (current->english == english) {
            current->vietnamese = vietnamese;
            return true;
        }
        current = current->next;
    }

    return false;
}

bool deleteWord(Dictionary& dictionary, const std::string& english) {
    if (!isValidEnglishKey(english)) {
        return false;
    }

    const std::size_t bucketIndex = hashWord(english);
    WordNode* current = dictionary.buckets[bucketIndex];
    WordNode* previous = nullptr;

    while (current != nullptr) {
        if (current->english == english) {
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

std::vector<const WordNode*> listBucket(const Dictionary& dictionary, std::size_t bucketIndex) {
    std::vector<const WordNode*> result;

    if (bucketIndex >= 26) {
        return result;
    }

    const WordNode* current = dictionary.buckets[bucketIndex];
    while (current != nullptr) {
        result.push_back(current);
        current = current->next;
    }

    return result;
}

std::vector<const WordNode*> listAllWords(const Dictionary& dictionary) {
    std::vector<const WordNode*> result;

    for (std::size_t i = 0; i < 26; ++i) {
        const WordNode* current = dictionary.buckets[i];
        while (current != nullptr) {
            result.push_back(current);
            current = current->next;
        }
    }

    return result;
}


