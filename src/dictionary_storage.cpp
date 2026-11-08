#include "dictionary_storage.h"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

bool loadDictionary(Dictionary& dictionary, const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;

    // Đọc và kiểm tra toàn bộ file trước; file lỗi không làm mất dữ liệu hiện có.
    std::vector<std::pair<std::string, std::string>> entries;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        std::size_t tab = line.find('\t');
        if (tab == std::string::npos || tab == 0 || tab == line.size() - 1)
            return false;
        if (line.find('\t', tab + 1) != std::string::npos)
            return false; // nghĩa không được chứa TAB trong định dạng này
        entries.emplace_back(line.substr(0, tab), line.substr(tab + 1));
    }
    if (in.bad()) return false;

    Dictionary temporary{};
    for (const auto& item : entries) {
        if (!addWord(temporary, item.first, item.second)) {
            clearDictionary(temporary);
            return false; // từ trùng hoặc dữ liệu không hợp lệ
        }
    }

    clearDictionary(dictionary);
    for (int i = 0; i < 26; ++i) {
        dictionary.buckets[i] = temporary.buckets[i];
        temporary.buckets[i] = nullptr; // chuyển quyền sở hữu node
    }
    return true;
}

bool saveDictionary(const Dictionary& dictionary, const std::string& path) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) return false;
    for (const WordNode* node : listAllWords(dictionary)) {
        // TAB và xuống dòng không thể xuất hiện trong một trường.
        if (node->english.find_first_of("\t\r\n") != std::string::npos ||
            node->vietnamese.find_first_of("\t\r\n") != std::string::npos)
            return false;
        out << node->english << '\t' << node->vietnamese << '\n';
        if (!out) return false;
    }
    out.flush();
    return static_cast<bool>(out);
