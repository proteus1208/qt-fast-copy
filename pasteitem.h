#pragma once

#include <QString>
#include <QKeySequence>
#include <QIcon>
#include <QList>

// How a store entry behaves when activated in the picker.
enum class StoreEntryKind {
    Text,   // extensionless file → paste UTF-8 file contents
    Path,   // normal file (has extension) → paste absolute path
    Folder  // directory → nested submenu of children
};

struct StoreEntry {
    QString        name;         // file/folder name = context menu label
    QString        absolutePath;
    StoreEntryKind kind = StoreEntryKind::Text;
    QKeySequence   hotkey;       // optional per-entry global hotkey (leaves only)
    QIcon          icon;         // native explorer icon (cached at load)
    QList<StoreEntry> children;  // populated for folders (may be lazy-empty)
};

// App-level settings that stay in QSettings (not in the store folder).
struct AppSettings {
    QString      storePath;
    QKeySequence globalHotkey;
    QKeySequence pasteTextOnlyHotkey;
};
