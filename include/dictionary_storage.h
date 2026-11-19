#pragma once

#include "dictionary.h"

#include <string>

// Read legacy two-column UTF-8 TSV or #TECHDICTIONARY<TAB>2 five-column TSV.
// On failure, leave the existing dictionary unchanged.
bool loadDictionary(Dictionary& dictionary, const std::string& path);

// Save version 2 UTF-8 TSV through a validated temporary file before replacement.
bool saveDictionary(const Dictionary& dictionary, const std::string& path);
