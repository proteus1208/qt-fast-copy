#pragma once

#include <QString>
#include <QKeySequence>

struct PasteItem {
    QString text;        // multi-line text (contains real newlines)
    QKeySequence hotkey; // optional per-item global hotkey
};
