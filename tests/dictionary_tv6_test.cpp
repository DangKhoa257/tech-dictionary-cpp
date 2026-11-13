#include "dictionary.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

void displayAllWords(const Dictionary& dictionary, std::ostream& out);
void displayWordsByLetter(const Dictionary& dictionary, char letter, std::ostream& out);

namespace {
int checks = 0;

void check(bool condition, const char* expression, int line) {
    ++checks;
    if (!condition) {
        std::cerr << "CHECK failed at line " << line << ": " << expression << '\n';
        std::exit(1);
    }
}

#define CHECK(expression) check((expression), #expression, __LINE__)

// These tests link TV3's production addWord and findWord.

bool writeFile(const std::string& path, const std::string& data) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) return false;
    output.write(data.data(), static_cast<std::streamsize>(data.size()));
    output.close();
    return static_cast<bool>(output);
}

std::string readFile(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(input),
                        std::istreambuf_iterator<char>());
}

void checkLoadFailurePreservesDictionary(Dictionary& dictionary,
                                         const std::string& path,
                                         const std::string& invalidData) {
    const std::size_t oldSize = listAllWords(dictionary).size();
    const WordNode* oldAlgorithm = findWord(dictionary, "ALGORITHM");
    CHECK(writeFile(path, invalidData));
    CHECK(!loadDictionary(dictionary, path));
    CHECK(listAllWords(dictionary).size() == oldSize);
    CHECK(findWord(dictionary, "algorithm") == oldAlgorithm);
}

} // namespace

int main() {
    const std::string path = "tv6_test_dictionary.txt";
    Dictionary dictionary{};

    // Empty dictionary and invalid bucket boundaries.
    CHECK(listAllWords(dictionary).empty());
    CHECK(listBucket(dictionary, 0).empty());
    CHECK(listBucket(dictionary, 25).empty());
    CHECK(listBucket(dictionary, 26).empty());
    CHECK(listBucket(dictionary, static_cast<std::size_t>(-1)).empty());
    CHECK(saveDictionary(dictionary, path));
    CHECK(readFile(path).empty());

    // UTF-8, BOM, CRLF and multiple entries sharing one bucket.
    const std::string bom = "\xEF\xBB\xBF";
    CHECK(writeFile(path,
        bom + "Algorithm\tThuật toán\r\n"
             "Artificial Intelligence\tTrí tuệ nhân tạo\r\n"
             "Array\tMảng\n"
             "Database\tCơ sở dữ liệu\n"));
    CHECK(loadDictionary(dictionary, path));
    CHECK(listAllWords(dictionary).size() == 4);
    const auto aWords = listBucket(dictionary, 0);
    CHECK(aWords.size() == 3);
    CHECK(aWords[0]->english == "Algorithm");
    CHECK(aWords[1]->english == "Artificial Intelligence");
    CHECK(aWords[2]->english == "Array");
    CHECK(listBucket(dictionary, 3).size() == 1);
    CHECK(listAllWords(dictionary)[3]->english == "Database");
    CHECK(findWord(dictionary, "algorithm") != nullptr);
    CHECK(findWord(dictionary, "ALGORITHM")->vietnamese == "Thuật toán");

    std::ostringstream allDisplay;
    displayAllWords(dictionary, allDisplay);
    CHECK(allDisplay.str().find("Algorithm : Thuật toán") != std::string::npos);
    std::ostringstream letterDisplay;
    displayWordsByLetter(dictionary, 'a', letterDisplay);
    CHECK(letterDisplay.str().find("Artificial Intelligence") != std::string::npos);
    std::ostringstream invalidDisplay;
    displayWordsByLetter(dictionary, '1', invalidDisplay);
    CHECK(invalidDisplay.str().find("khong hop le") != std::string::npos);

    // Save round trip retains UTF-8 meanings.
    CHECK(saveDictionary(dictionary, path));
    CHECK(readFile(path).find("Trí tuệ nhân tạo") != std::string::npos);
    Dictionary roundTrip{};
    CHECK(loadDictionary(roundTrip, path));
    CHECK(listAllWords(roundTrip).size() == 4);
    CHECK(findWord(roundTrip, "DATABASE")->vietnamese == "Cơ sở dữ liệu");
    CHECK(addWord(roundTrip, " Router ", " Bộ định tuyến "));
    CHECK(!addWord(roundTrip, "router", "duplicate"));
    CHECK(saveDictionary(roundTrip, path));
    Dictionary integrated{};
    CHECK(loadDictionary(integrated, path));
    CHECK(findWord(integrated, "ROUTER") != nullptr);
    CHECK(findWord(integrated, "ROUTER")->vietnamese == "Bộ định tuyến");
    clearDictionary(integrated);
    clearDictionary(roundTrip);

    // Exact duplicate and ASCII case-insensitive duplicate must fail transactionally.
    checkLoadFailurePreservesDictionary(dictionary, path,
        "Algorithm\tA\nAlgorithm\tB\n");
    checkLoadFailurePreservesDictionary(dictionary, path,
        "Algorithm\tA\nalgorithm\tB\n");
    checkLoadFailurePreservesDictionary(dictionary, path,
        "DATABASE\tA\ndatabase\tB\n");
    checkLoadFailurePreservesDictionary(dictionary, path,
        "Algorithm \tA\nalgorithm\tB\n");

    // Malformed rows and invalid fields.
    checkLoadFailurePreservesDictionary(dictionary, path, "Broken row without tab\n");
    checkLoadFailurePreservesDictionary(dictionary, path, "Algorithm\tMeaning\n\nArray\tMảng\n");
    checkLoadFailurePreservesDictionary(dictionary, path, "\tMeaning\n");
    checkLoadFailurePreservesDictionary(dictionary, path, "English\t\n");
    checkLoadFailurePreservesDictionary(dictionary, path, "English\tMeaning\tExtra\n");
    checkLoadFailurePreservesDictionary(dictionary, path, "1Algorithm\tMeaning\n");
    checkLoadFailurePreservesDictionary(dictionary, path, "_Algorithm\tMeaning\n");

    std::string invalidUtf8 = "Algorithm\t";
    invalidUtf8.push_back(static_cast<char>(0xC3)); // truncated two-byte sequence
    invalidUtf8.push_back('\n');
    checkLoadFailurePreservesDictionary(dictionary, path, invalidUtf8);
    std::string overlongUtf8 = "Algorithm\t";
    overlongUtf8.push_back(static_cast<char>(0xC0));
    overlongUtf8.push_back(static_cast<char>(0xAF));
    overlongUtf8.push_back('\n');
    checkLoadFailurePreservesDictionary(dictionary, path, overlongUtf8);

    // Missing file must not clear existing dictionary.
    const std::size_t beforeMissing = listAllWords(dictionary).size();
    CHECK(!loadDictionary(dictionary, "missing_tv6_file_987654321.txt"));
    CHECK(listAllWords(dictionary).size() == beforeMissing);

    // Saving invalid dictionary data must preserve pre-existing destination bytes.
    const std::string oldFile = "KEEP OLD DATA\n";
    CHECK(writeFile(path, oldFile));
    WordNode* duplicate = new WordNode{"algorithm", "meaning 2", nullptr};
    WordNode* aTail = dictionary.buckets[0];
    while (aTail->next != nullptr) aTail = aTail->next;
    aTail->next = duplicate;
    CHECK(!saveDictionary(dictionary, path));
    CHECK(readFile(path) == oldFile);
    aTail->next = nullptr;
    delete duplicate;

    WordNode* badEnglish = new WordNode{"Bad\tEnglish", "meaning", nullptr};
    dictionary.buckets[1] = badEnglish;
    CHECK(!saveDictionary(dictionary, path));
    CHECK(readFile(path) == oldFile);
    dictionary.buckets[1] = nullptr;
    delete badEnglish;

    WordNode* invalidMeaning = new WordNode{"Zebra", std::string("bad\xFF", 4), nullptr};
    dictionary.buckets[25] = invalidMeaning;
    CHECK(!saveDictionary(dictionary, path));
    CHECK(readFile(path) == oldFile);
    dictionary.buckets[25] = nullptr;
    delete invalidMeaning;

    WordNode* padded = new WordNode{"Algorithm ", "meaning", nullptr};
    aTail->next = padded;
    CHECK(!saveDictionary(dictionary, path));
    CHECK(readFile(path) == oldFile);
    aTail->next = nullptr;
    delete padded;

    WordNode* wrongBucket = new WordNode{"Zebra", "meaning", nullptr};
    dictionary.buckets[1] = wrongBucket;
    CHECK(!saveDictionary(dictionary, path));
    CHECK(readFile(path) == oldFile);
    dictionary.buckets[1] = nullptr;
    delete wrongBucket;

    // Empty file is a valid dictionary and replaces the old contents only on success.
    CHECK(writeFile(path, ""));
    CHECK(loadDictionary(dictionary, path));
    CHECK(listAllWords(dictionary).empty());

    // Repeated load/clear exercises temporary and dictionary ownership cleanup.
    for (int iteration = 0; iteration < 40; ++iteration) {
        CHECK(writeFile(path, "Algorithm\tThuật toán\nArray\tMảng\n"));
        CHECK(loadDictionary(dictionary, path));
        CHECK(listAllWords(dictionary).size() == 2);
        clearDictionary(dictionary);
        CHECK(listAllWords(dictionary).empty());
    }

    clearDictionary(dictionary);
    std::remove(path.c_str());
    std::cout << "TV6 tests passed: " << checks << " checks\n";
    return 0;
}
