#include "dictionary.h"

#include <cassert>

int main() {
    // Test hashWord
    assert(hashWord("A") == 0);
    assert(hashWord("a") == 0);
    assert(hashWord("Apple") == 0);
    assert(hashWord("banana") == 1);
    assert(hashWord("Computer") == 2);
    assert(hashWord("Zebra") == 25);
    assert(hashWord("z") == 25);

    // Test invalid input
    assert(hashWord("") == 26);
    assert(hashWord("123abc") == 26);
    assert(hashWord("@hello") == 26);

    // Test empty dictionary
    Dictionary dictionary{};

    clearDictionary(dictionary);

    for (std::size_t i = 0; i < 26; ++i) {
        assert(dictionary.buckets[i] == nullptr);
    }

    // Test dictionary with multiple nodes
    dictionary.buckets[0] = new WordNode{
        "Apple", "Quả táo", nullptr
    };

    dictionary.buckets[0]->next = new WordNode{
        "Algorithm", "Thuật toán", nullptr
    };

    dictionary.buckets[1] = new WordNode{
        "Binary", "Nhị phân", nullptr
    };

    clearDictionary(dictionary);

    for (std::size_t i = 0; i < 26; ++i) {
        assert(dictionary.buckets[i] == nullptr);
    }

    // Test clearing twice
    clearDictionary(dictionary);

    return 0;
}