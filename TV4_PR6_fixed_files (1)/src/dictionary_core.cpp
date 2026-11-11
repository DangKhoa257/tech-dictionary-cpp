#include "dictionary.h"
#include <cctype>

std::size_t hashWord(const std::string& english) {
    if (english.empty()) return 26;
    char c = english[0];
    if (c >= 'a' && c <= 'z') return c - 'a';
    if (c >= 'A' && c <= 'Z') return c - 'A';
    return 26;
}

void clearDictionary(Dictionary& dictionary) {
    for (int i = 0; i < 26; ++i) {
        WordNode* current = dictionary.buckets[i];
        while (current) {
            WordNode* next = current->next;
            delete current;
            current = next;
        }
        dictionary.buckets[i] = nullptr;
    }
}
