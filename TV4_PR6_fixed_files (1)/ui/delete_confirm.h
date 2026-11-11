#pragma once

#include <QString>

class QWidget;

// Returns true only when the user explicitly confirms deletion.
bool confirmDeleteWord(QWidget* parent, const QString& englishWord);
