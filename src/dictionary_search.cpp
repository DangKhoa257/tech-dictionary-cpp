#include "dictionary.h"

#include <string>
#include <algorithm>

namespace {

// The dictionary file is tab-separated. Reject ASCII control bytes before
// trimming so no accepted value can create an extra field or record.
bool isValidUtf8TsvField(const std::string& value) {
    for (std::size_t i = 0; i < value.size();) {
        const unsigned char lead = static_cast<unsigned char>(value[i]);

        if (lead <= 0x1F || lead == 0x7F) {
            return false;
        }

        if (lead <= 0x7F) {
            ++i;
            continue;
        }

        std::size_t continuationCount = 0;
        unsigned char secondMin = 0x80;
        unsigned char secondMax = 0xBF;

        if (lead >= 0xC2 && lead <= 0xDF) {
            continuationCount = 1;
        } else if (lead == 0xE0) {
            continuationCount = 2;
            secondMin = 0xA0;  // Reject overlong encodings.
        } else if (lead >= 0xE1 && lead <= 0xEC) {
            continuationCount = 2;
        } else if (lead == 0xED) {
            continuationCount = 2;
            secondMax = 0x9F;  // Reject UTF-16 surrogate code points.
        } else if (lead >= 0xEE && lead <= 0xEF) {
            continuationCount = 2;
        } else if (lead == 0xF0) {
            continuationCount = 3;
            secondMin = 0x90;  // Reject overlong encodings.
        } else if (lead >= 0xF1 && lead <= 0xF3) {
            continuationCount = 3;
        } else if (lead == 0xF4) {
            continuationCount = 3;
            secondMax = 0x8F;  // U+10FFFF is the Unicode maximum.
        } else {
            return false;
        }

        if (i + continuationCount >= value.size()) {
            return false;
        }

        const unsigned char second =
            static_cast<unsigned char>(value[i + 1]);
        if (second < secondMin || second > secondMax) {
            return false;
        }

        for (std::size_t offset = 2; offset <= continuationCount; ++offset) {
            const unsigned char continuation =
                static_cast<unsigned char>(value[i + offset]);
            if (continuation < 0x80 || continuation > 0xBF) {
                return false;
            }
        }

        i += continuationCount + 1;
    }

    return true;
}

// Match the project's existing trim behavior for ordinary ASCII whitespace.
// Control bytes are rejected before this function is used for insertion.
std::string trimWhitespace(const std::string& value) {
    std::size_t first = 0;
    while (first < value.size() &&
           (value[first] == ' ' || value[first] == '\f' ||
            value[first] == '\v')) {
        ++first;
    }

    std::size_t last = value.size();
    while (last > first &&
           (value[last - 1] == ' ' || value[last - 1] == '\f' ||
            value[last - 1] == '\v')) {
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
        unsigned char leftCharacter = static_cast<unsigned char>(left[i]);
        unsigned char rightCharacter = static_cast<unsigned char>(right[i]);

        if (leftCharacter >= 'A' && leftCharacter <= 'Z') {
            leftCharacter = static_cast<unsigned char>(
                leftCharacter - 'A' + 'a');
        }
        if (rightCharacter >= 'A' && rightCharacter <= 'Z') {
            rightCharacter = static_cast<unsigned char>(
                rightCharacter - 'A' + 'a');
        }

        if (leftCharacter != rightCharacter) {
            return false;
        }
    }

    return true;
}

std::string lowerAscii(std::string value) {
    for (char& c : value) if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    return value;
}

}  // namespace

bool isValidEntryMetadata(const std::string& value) {
    std::string flattened = value;
    for (char& c : flattened) if (c == '\t' || c == '\n' || c == '\r') c = ' ';
    return isValidUtf8TsvField(flattened);
}

bool addWord(Dictionary& dictionary, const std::string& english,
             const std::string& vietnamese) {
    if (!isValidUtf8TsvField(english) ||
        !isValidUtf8TsvField(vietnamese)) {
        return false;
    }

    const std::string normalizedEnglish = trimWhitespace(english);
    const std::string normalizedMeaning = trimWhitespace(vietnamese);

    if (normalizedEnglish.empty() || normalizedMeaning.empty()) {
        return false;
    }

    // The shared dictionary has exactly 26 A-Z buckets. Punctuation is
    // allowed after the initial ASCII letter (C++, C#, O(n), etc.).
    const std::size_t bucketIndex = hashWord(normalizedEnglish);
    if (bucketIndex >= 26) {
        return false;
    }

    WordNode* currentNode = dictionary.buckets[bucketIndex];
    while (currentNode != nullptr) {
        if (equalsIgnoreCaseAscii(currentNode->english,
                                  normalizedEnglish)) {
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
    if (!isValidUtf8TsvField(english)) {
        return nullptr;
    }

    const std::string normalizedEnglish = trimWhitespace(english);
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

bool addEntry(Dictionary& dictionary, const std::string& english,
              const std::string& vietnamese, const std::string& partOfSpeech,
              const std::string& example, const std::string& category) {
    if (!isValidEntryMetadata(vietnamese) || vietnamese.empty() ||
        !isValidEntryMetadata(partOfSpeech) || !isValidEntryMetadata(example) ||
        !isValidEntryMetadata(category)) return false;
    std::string meaning = vietnamese;
    std::string part = partOfSpeech;
    std::string sentence = example;
    std::string topic = category;
    if (!addWord(dictionary, english, isValidUtf8TsvField(vietnamese) ? vietnamese : "-")) return false;
    WordNode* node = const_cast<WordNode*>(findWord(dictionary, english));
    node->vietnamese.swap(meaning);
    node->partOfSpeech.swap(part);
    node->example.swap(sentence);
    node->category.swap(topic);
    return true;
}

std::vector<std::string> suggestWords(const Dictionary& dictionary,
                                      const std::string& prefix, std::size_t limit) {
    const std::string query = lowerAscii(trimWhitespace(prefix));
    if (query.empty() || limit == 0 || hashWord(query) >= 26) return {};
    std::vector<std::string> matches;
    for (const WordNode* node = dictionary.buckets[hashWord(query)];
         node != nullptr; node = node->next) {
        if (lowerAscii(node->english).rfind(query, 0) == 0) matches.push_back(node->english);
    }
    std::sort(matches.begin(), matches.end(), [&](const auto& a, const auto& b) {
        const bool aExact = lowerAscii(a) == query;
        const bool bExact = lowerAscii(b) == query;
        if (aExact != bExact) return aExact;
        return lowerAscii(a) < lowerAscii(b);
    });
    if (matches.size() > limit) matches.resize(limit);
    return matches;
}
