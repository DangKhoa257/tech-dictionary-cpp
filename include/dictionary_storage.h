#pragma once
#include "dictionary.h"
#include <string>

//  đọc/ghi file UTF-8, mỗi dòng English<TAB>Vietnamese.
bool loadDictionary(Dictionary& dictionary, const std::string& path);
bool saveDictionary(const Dictionary& dictionary, const std::string& path);