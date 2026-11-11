#pragma once
#include <string>
#include <cstddef>

struct WordNode {
    std::string english;
    std::string vietnamese;
    WordNode* next;
};

struct Dictionary {
    WordNode* buckets[26] = {nullptr};
};

std::size_t hashWord(const std::string& english);
void clearDictionary(Dictionary& dictionary);

bool updateWordMeaning(Dictionary& dictionary, const std::string& english, const std::string& vietnamese);
bool deleteWord(Dictionary& dictionary, const std::string& english);
