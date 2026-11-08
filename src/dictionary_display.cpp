#include "dictionary.h"
#include <cctype>
#include <iostream>
#include <ostream>
void displayAllWords(const Dictionary& dictionary, std::ostream& out) {
    out << "=== TU DIEN ANH - VIET ===\n";
    for (const WordNode* node : listAllWords(dictionary))
        out << node->english << " : " << node->vietnamese << '\n';
}
void displayWordsByLetter(const Dictionary& dictionary, char letter, std::ostream& out) {
    unsigned char c = static_cast<unsigned char>(letter);
    c = static_cast<unsigned char>(std::toupper(c));
    if (c < 'A' || c > 'Z') {
        out << "Ky tu khong hop le. Hay chon A-Z.\n";
        return;
    }
    std::size_t bucket = c - 'A';
    auto words = listBucket(dictionary, bucket);
    out << "=== TU BAT DAU BANG " << static_cast<char>(c) << " ===\n";
    if (words.empty()) { out << "Chua co tu nao.\n"; return; }
    for (const WordNode* node : words)
        out << node->english << " : " << node->vietnamese << '\n';
}
