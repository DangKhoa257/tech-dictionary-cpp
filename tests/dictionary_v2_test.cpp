#include "dictionary.h"
#include "dictionary_history.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace {
int failures = 0;
void check(bool value, const char* label) {
    if (!value) { std::cerr << "Failed: " << label << '\n'; ++failures; }
}
void write(const std::filesystem::path& path, const std::string& text) {
    std::ofstream file(path, std::ios::binary);
    file << text;
}
std::string read(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}
} // namespace

int main() {
    const auto legacy = std::filesystem::current_path() / std::filesystem::u8path(u8"v2-đầu-vào.txt");
    const auto rich = std::filesystem::current_path() / std::filesystem::u8path(u8"v2-đầu-ra.txt");
    Dictionary dictionary{};
    write(legacy, "Compiler\tTrình biên dịch\nComputer\tMáy tính\n");
    check(loadDictionary(dictionary, legacy.u8string()), "legacy load at Unicode path");
    check(read(legacy) == "Compiler\tTrình biên dịch\nComputer\tMáy tính\n",
          "opening legacy file does not migrate on disk");
    check(findWord(dictionary, "Compiler") && findWord(dictionary, "Compiler")->example.empty(),
          "legacy metadata empty");
    check(suggestWords(dictionary, "  comp  ").size() == 2, "trimmed prefix");
    check(suggestWords(dictionary, "COMP").size() == 2, "case insensitive prefix");
    check(suggestWords(dictionary, "nomatch").empty(), "no fabricated matches");
    check(suggestWords(dictionary, "").empty(), "empty query");

    const auto before = DictionaryHistory::capture(dictionary);
    const std::string richExample = "C++ compiles to native code.\nThe path C:\\code contains\tfiles.";
    check(addEntry(dictionary, "C++", "Ngôn ngữ C++", "Noun", richExample, "Programming"),
          "add enriched term with newline");
    check(addEntry(dictionary, "C#", "Ngôn ngữ C thăng", "Noun",
                   "C# runs this application on .NET.", "Programming") &&
          suggestWords(dictionary, "C#").size() == 1,
          "C# technical term is searchable");
    check(updateEntry(dictionary, "Compiler", "Bộ biên dịch", "Noun",
                      "The compiler checks syntax before generating code.", "Programming"),
          "update legacy term with metadata");
    DictionaryHistory history;
    history.recordChange(before, DictionaryHistory::capture(dictionary));
    check(history.undo(dictionary), "metadata undo");
    check(findWord(dictionary, "Compiler") && findWord(dictionary, "Compiler")->example.empty(),
          "undo restores legacy metadata");
    check(history.redo(dictionary), "metadata redo");
    check(findWord(dictionary, "Compiler") && findWord(dictionary, "Compiler")->category == "Programming",
          "redo restores category");
    check(suggestWords(dictionary, "C+").size() == 1, "punctuation prefix");
    check(saveDictionary(dictionary, rich.u8string()), "save version 2");
    check(read(legacy).rfind("Compiler\t", 0) == 0,
          "saving elsewhere preserves original legacy file");
    check(read(rich).rfind("#TECHDICTIONARY\t2\n", 0) == 0, "explicit version header");
    Dictionary roundTrip{};
    check(loadDictionary(roundTrip, rich.u8string()), "reload version 2");
    const auto* cxx = findWord(roundTrip, "C++");
    check(cxx && cxx->example == richExample &&
          cxx->partOfSpeech == "Noun" && cxx->category == "Programming", "round-trip all metadata");
    const auto* stable = findWord(roundTrip, "Compiler");
    write(legacy, "#TECHDICTIONARY\t2\nBroken\tvalue\tNoun\tbad\\x\tProgramming\n");
    check(!loadDictionary(roundTrip, legacy.u8string()) && findWord(roundTrip, "Compiler") == stable,
          "invalid escape preserves dictionary");
    write(legacy, "#TECHDICTIONARY\t3\n");
    check(!loadDictionary(roundTrip, legacy.u8string()), "unknown version rejected");
    write(legacy, "Broken\tvalue\tthird\n");
    check(!loadDictionary(roundTrip, legacy.u8string()), "legacy malformed row rejected");
    const std::string previous = read(rich);
    roundTrip.buckets[0] = new WordNode{"Bad\tKey", "value", roundTrip.buckets[0]};
    check(!saveDictionary(roundTrip, rich.u8string()) && read(rich) == previous,
          "failed save retains previous file");
    WordNode* bad = roundTrip.buckets[0];
    roundTrip.buckets[0] = bad->next;
    delete bad;
    clearDictionary(roundTrip);

    Dictionary seed{};
    check(loadDictionary(seed, TECH_DICTIONARY_SEED_PATH), "real version 2 seed loads");
    check(listAllWords(seed).size() >= 550, "seed has at least 550 terms");
    for (const char* sample : {"ALU", "Constexpr", "Constructor", "Trie", "Dijkstra's algorithm",
                               "ACID", "BGP", "Context switch", "Ransomware", "Embedding",
                               "Gradient descent", "Confidence interval", "Code review",
                               "Autoscaling", "Service worker"}) {
        check(findWord(seed, sample) != nullptr, "category sample searchable");
    }
    const auto* algorithm = findWord(seed, "Algorithm");
    check(algorithm && !algorithm->example.empty(), "legacy term enriched in actual seed");
    clearDictionary(seed);
    clearDictionary(dictionary);
    std::error_code ignored;
    std::filesystem::remove(legacy, ignored);
    std::filesystem::remove(rich, ignored);
    return failures ? 1 : 0;
}
