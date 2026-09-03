#pragma once

#include <QString>
#include <QKeySequence>

struct PasteItem {
    QString      title;  // display name shown in picker menu and settings list
    QString      text;   // multi-line paste text (contains real newlines)
    QKeySequence hotkey; // optional per-item global hotkey
};
