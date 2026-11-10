#include "dictionary_storage.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <cstdio>
#include <string>
#include <system_error>
#include <unordered_set>
#include <utility>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace {

bool isValidUtf8(const std::string& text) {
    const auto* bytes = reinterpret_cast<const unsigned char*>(text.data());
    std::size_t i = 0;
    while (i < text.size()) {
        const unsigned char first = bytes[i];
        if (first <= 0x7F) {
            ++i;
            continue;
        }

        std::size_t count = 0;
        std::uint32_t codePoint = 0;
        if (first >= 0xC2 && first <= 0xDF) {
            count = 2;
            codePoint = first & 0x1F;
        } else if (first >= 0xE0 && first <= 0xEF) {
            count = 3;
            codePoint = first & 0x0F;
        } else if (first >= 0xF0 && first <= 0xF4) {
            count = 4;
            codePoint = first & 0x07;
        } else {
            return false;
        }

        if (i + count > text.size()) return false;
        for (std::size_t j = 1; j < count; ++j) {
            const unsigned char continuation = bytes[i + j];
            if ((continuation & 0xC0) != 0x80) return false;
            codePoint = (codePoint << 6) | (continuation & 0x3F);
        }

        if ((count == 3 && codePoint < 0x800) ||
            (count == 4 && codePoint < 0x10000) ||
            (codePoint >= 0xD800 && codePoint <= 0xDFFF) ||
            codePoint > 0x10FFFF) {
            return false;
        }
        i += count;
    }
    return true;
}

std::string normalizeEnglishKey(const std::string& english) {
    std::string key = english;
    for (char& character : key) {
        const unsigned char value = static_cast<unsigned char>(character);
        if (value >= 'A' && value <= 'Z') {
            character = static_cast<char>(value - 'A' + 'a');
        }
    }
    return key;
}

bool validField(const std::string& field) {
    if (field.empty() || !isValidUtf8(field)) return false;

    for (unsigned char value : field) {
        // The TSV format reserves tabs and line breaks as delimiters. Reject
        // other ASCII control bytes (including NUL) as malformed field data.
        if (value < 0x20 || value == 0x7F) return false;
    }
    return true;
}

std::string makeTemporaryPath(const std::filesystem::path& destination) {
    static std::atomic<std::uint64_t> sequence{0};
    const auto ticks = std::chrono::steady_clock::now().time_since_epoch().count();
    std::filesystem::path temporary = destination;
    temporary += ".tmp." + std::to_string(ticks) + "." +
                 std::to_string(sequence.fetch_add(1, std::memory_order_relaxed));
    return temporary.u8string();
}

bool writeTemporaryFile(const std::string& temporaryPath, const std::string& contents) {
    const std::filesystem::path nativePath = std::filesystem::u8path(temporaryPath);
#ifdef _WIN32
    HANDLE file = CreateFileW(nativePath.c_str(), GENERIC_WRITE, 0, nullptr,
                              CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;

    bool success = true;
    std::size_t offset = 0;
    while (offset < contents.size()) {
        const DWORD chunk = static_cast<DWORD>(
            (contents.size() - offset) > 0x7FFFFFFF
                ? 0x7FFFFFFF
                : (contents.size() - offset));
        DWORD written = 0;
        if (!WriteFile(file, contents.data() + offset, chunk, &written, nullptr) ||
            written == 0) {
            success = false;
            break;
        }
        offset += written;
    }
    if (success && !FlushFileBuffers(file)) success = false;
    if (!CloseHandle(file)) success = false;
    if (!success) {
        std::error_code ignored;
        std::filesystem::remove(nativePath, ignored);
    }
    return success;
#else
    const int file = ::open(nativePath.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0666);
    if (file < 0) return false;

    bool success = true;
    std::size_t offset = 0;
    while (offset < contents.size()) {
        const ssize_t written = ::write(file, contents.data() + offset,
                                        contents.size() - offset);
        if (written < 0 && errno == EINTR) continue;
        if (written <= 0) {
            success = false;
            break;
        }
        offset += static_cast<std::size_t>(written);
    }
    if (success && ::fsync(file) != 0) success = false;
    if (::close(file) != 0) success = false;
    if (!success) {
        std::error_code ignored;
        std::filesystem::remove(nativePath, ignored);
    }
    return success;
#endif
}

bool replaceDestination(const std::string& temporaryPath,
                        const std::string& destinationPath) {
#ifdef _WIN32
    const std::filesystem::path source = std::filesystem::u8path(temporaryPath);
    const std::filesystem::path destination = std::filesystem::u8path(destinationPath);
    return MoveFileExW(source.c_str(), destination.c_str(),
                       MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    return std::rename(temporaryPath.c_str(), destinationPath.c_str()) == 0;
#endif
}

void removeTemporaryFile(const std::string& temporaryPath) {
    std::error_code ignored;
    std::filesystem::remove(std::filesystem::u8path(temporaryPath), ignored);
}

class DictionaryOwner {
public:
    DictionaryOwner() = default;
    DictionaryOwner(const DictionaryOwner&) = delete;
    DictionaryOwner& operator=(const DictionaryOwner&) = delete;

    ~DictionaryOwner() { clearDictionary(dictionary_); }

    Dictionary& get() { return dictionary_; }

    void replace(Dictionary& destination) {
        clearDictionary(destination);
        for (std::size_t bucket = 0; bucket < 26; ++bucket) {
            destination.buckets[bucket] = dictionary_.buckets[bucket];
            dictionary_.buckets[bucket] = nullptr;
        }
    }

private:
    Dictionary dictionary_{};
};

} // namespace

bool loadDictionary(Dictionary& dictionary, const std::string& path) {
    std::ifstream input(std::filesystem::u8path(path), std::ios::binary);
    if (!input) return false;

    std::vector<std::pair<std::string, std::string>> entries;
    std::unordered_set<std::string> seenEnglish;
    std::string line;
    bool firstLine = true;

    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();

        if (firstLine) {
            firstLine = false;
            if (line.size() >= 3 &&
                static_cast<unsigned char>(line[0]) == 0xEF &&
                static_cast<unsigned char>(line[1]) == 0xBB &&
                static_cast<unsigned char>(line[2]) == 0xBF) {
                line.erase(0, 3);
            }
        }

        if (line.empty()) return false; // Empty physical rows are malformed.

        const std::size_t tab = line.find('\t');
        if (tab == std::string::npos || tab == 0 || tab + 1 >= line.size() ||
            line.find('\t', tab + 1) != std::string::npos) {
            return false;
        }

        std::string english = line.substr(0, tab);
        std::string vietnamese = line.substr(tab + 1);
        if (!validField(english) || !validField(vietnamese) ||
            hashWord(english) >= 26) {
            return false;
        }
        if (!seenEnglish.insert(normalizeEnglishKey(english)).second) return false;
        entries.emplace_back(std::move(english), std::move(vietnamese));
    }
    if (input.bad()) return false;

    // TV3 remains responsible for production insertion. The temporary
    // dictionary gives loading transactional behavior: the caller's current
    // nodes are untouched until every row has been accepted by addWord().
    DictionaryOwner temporary;
    for (const auto& entry : entries) {
        if (!addWord(temporary.get(), entry.first, entry.second)) return false;
    }

    temporary.replace(dictionary);
    return true;
}

bool saveDictionary(const Dictionary& dictionary, const std::string& path) {
    if (path.empty()) return false;
    const std::vector<const WordNode*> words = listAllWords(dictionary);
    std::unordered_set<std::string> seenEnglish;
    std::string contents;

    // Validate and serialize in memory before creating any temporary file.
    for (const WordNode* node : words) {
        if (node == nullptr || !validField(node->english) ||
            !validField(node->vietnamese) || hashWord(node->english) >= 26) {
            return false;
        }
        if (!seenEnglish.insert(normalizeEnglishKey(node->english)).second) return false;
        contents += node->english;
        contents.push_back('\t');
        contents += node->vietnamese;
        contents.push_back('\n');
    }

    const std::filesystem::path destination = std::filesystem::u8path(path);
    // The temporary file is a sibling of the destination, keeping replacement
    // on the same filesystem. CREATE_NEW/O_EXCL prevents clobbering a collision.
    for (int attempt = 0; attempt < 8; ++attempt) {
        const std::string temporaryPath = makeTemporaryPath(destination);
        if (!writeTemporaryFile(temporaryPath, contents)) {
            // A collision is harmless; try another unique name. Other create
            // failures will also be retried, then reported as a save failure.
            continue;
        }
        if (replaceDestination(temporaryPath, path)) return true;
        removeTemporaryFile(temporaryPath);
        return false;
    }
    return false;
}