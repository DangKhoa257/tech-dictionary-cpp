#include "dictionary.h"

#include <cctype>

std::size_t hashWord(const std::string& english) {
    if (english.empty()) {
        return 26;
    }

    unsigned char firstCharacter =
        static_cast<unsigned char>(english[0]);

    if (firstCharacter < 'A' || firstCharacter > 'Z') {
        if (firstCharacter < 'a' || firstCharacter > 'z') {
            return 26;
        }
    }

    firstCharacter = static_cast<unsigned char>(
        std::toupper(firstCharacter)
    );

    return static_cast<std::size_t>(firstCharacter - 'A');
}

void clearDictionary(Dictionary& dictionary) {
    for (std::size_t bucketIndex = 0; bucketIndex < 26; ++bucketIndex) {
        WordNode* currentNode = dictionary.buckets[bucketIndex];

        while (currentNode != nullptr) {
            WordNode* nextNode = currentNode->next;

            delete currentNode;

            currentNode = nextNode;
        }

        dictionary.buckets[bucketIndex] = nullptr;
    }
}