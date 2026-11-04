#pragma once

#include <cstddef>
#include <string>
#include <vector>

// UTF-8 text is stored in std::string. Ownership of nodes belongs to Dictionary.
struct WordNode {
    std::string english;
    std::string vietnamese;
    WordNode* next = nullptr;
};

// Bucket 0 holds A/a, bucket 25 holds Z/z. Each bucket is a singly linked list.
struct Dictionary {
    WordNode* buckets[26]{};
};

// Return the 0-based bucket index for a word's first ASCII English letter.
// The implementation must specify how it rejects empty/non-letter input.
std::size_t hashWord(const std::string& english);

// Delete every node owned by dictionary and reset all bucket heads to nullptr.
void clearDictionary(Dictionary& dictionary);

// Insert one English word and Vietnamese meaning; return false for invalid or duplicate input.
bool addWord(Dictionary& dictionary, const std::string& english, const std::string& vietnamese);

// Return a non-owning pointer to a matching node, or nullptr when absent.
const WordNode* findWord(const Dictionary& dictionary, const std::string& english);

// Change an existing word's meaning; return false when the word is absent.
bool updateWordMeaning(Dictionary& dictionary, const std::string& english,
                       const std::string& vietnamese);

// Remove a matching node and free its memory; return false when absent.
bool deleteWord(Dictionary& dictionary, const std::string& english);

// Return non-owning pointers to nodes in the requested bucket (0 to 25).
std::vector<const WordNode*> listBucket(const Dictionary& dictionary, std::size_t bucketIndex);

// Return non-owning pointers to every node in bucket order.
std::vector<const WordNode*> listAllWords(const Dictionary& dictionary);

// Load UTF-8 entries from path; return false on I/O or format failure.
// The storage module will define the exact file format and failure behavior.
bool loadDictionary(Dictionary& dictionary, const std::string& path);

// Write UTF-8 entries to path; return false on I/O failure.
bool saveDictionary(const Dictionary& dictionary, const std::string& path);
