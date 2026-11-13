#pragma once

#include "dictionary.h"

#include <string>

// Read UTF-8 TSV rows in the form English<TAB>Vietnamese.
// On failure, leave the existing dictionary unchanged.
bool loadDictionary(Dictionary& dictionary, const std::string& path);

// Save UTF-8 TSV rows through a validated temporary file before replacement.
bool saveDictionary(const Dictionary& dictionary, const std::string& path);