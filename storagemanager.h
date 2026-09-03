#pragma once

#include "pasteitem.h"
#include <QList>
#include <QKeySequence>

class StorageManager {
public:
    static void save(const QList<PasteItem> &items, const QKeySequence &globalHotkey);
    static QList<PasteItem> loadItems();
    static QKeySequence loadGlobalHotkey();
};
