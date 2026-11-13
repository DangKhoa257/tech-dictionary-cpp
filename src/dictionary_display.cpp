#include "dictionary.h"

#include <cctype>
#include <ostream>
#include <vector>

std::vector<const WordNode*> listBucket(const Dictionary& dictionary,
                                        std::size_t bucketIndex) {
    std::vector<const WordNode*> words;
    if (bucketIndex >= 26) return words;

    for (const WordNode* node = dictionary.buckets[bucketIndex];
         node != nullptr; node = node->next) {
        words.push_back(node);
    }
    return words;
}

std::vector<const WordNode*> listAllWords(const Dictionary& dictionary) {
    std::vector<const WordNode*> words;
    for (std::size_t bucketIndex = 0; bucketIndex < 26; ++bucketIndex) {
        for (const WordNode* node = dictionary.buckets[bucketIndex];
             node != nullptr; node = node->next) {
            words.push_back(node);
        }
    }
    return words;
}

void displayAllWords(const Dictionary& dictionary, std::ostream& out) {
    out << "=== TU DIEN ANH - VIET ===\n";
    for (const WordNode* node : listAllWords(dictionary)) {
        out << node->english << " : " << node->vietnamese << '\n';
    }
}

void displayWordsByLetter(const Dictionary& dictionary, char letter,
                          std::ostream& out) {
    const unsigned char character = static_cast<unsigned char>(letter);
    const unsigned char upper = static_cast<unsigned char>(std::toupper(character));
    if (upper < 'A' || upper > 'Z') {
        out << "Ky tu khong hop le. Hay chon A-Z.\n";
        return;
    }

    const std::size_t bucket = static_cast<std::size_t>(upper - 'A');
    const std::vector<const WordNode*> words = listBucket(dictionary, bucket);
    out << "=== TU BAT DAU BANG " << static_cast<char>(upper) << " ===\n";
    if (words.empty()) {
        out << "Chua co tu nao.\n";
        return;
    }
    for (const WordNode* node : words) {
        out << node->english << " : " << node->vietnamese << '\n';
    }
}