#pragma once

#include "pasteitem.h"
#include <QList>
#include <QString>
#include <QKeySequence>
#include <QHash>
#include <QPair>

class StorageManager {
public:
    static constexpr const char *kMetaFileName = "meta.json";

    // --- App settings (QSettings) ---
    static AppSettings loadAppSettings();
    static void saveAppSettings(const AppSettings &settings);
    static QString defaultStorePath();

    // --- Store folder ---
    static void ensureStoreFolder(const QString &storePath);

    // Load one directory level. Folders get kind=Folder; children stay empty
    // (caller / tray loads on demand for performance).
    static QList<StoreEntry> loadDirectory(const QString &dirPath);

    // Recursively load the whole tree (settings list uses root only).
    static QList<StoreEntry> loadTree(const QString &dirPath);

    // Persist display order (+ hotkeys) for one directory.
    static bool saveDirectoryMeta(const QString &dirPath,
                                  const QList<StoreEntry> &entries);

    // Create / update / delete entries on disk.
    static bool createTextItem(const QString &dirPath,
                               const QString &name,
                               const QString &text,
                               const QKeySequence &hotkey,
                               QString *errorOut = nullptr);
    static bool writeTextFile(const QString &absolutePath, const QString &text);
    static QString readTextFile(const QString &absolutePath);
    static bool updateEntryHotkey(const QString &dirPath,
                                  const QString &name,
                                  const QKeySequence &hotkey);
    static bool removeEntry(const QString &absolutePath, QString *errorOut = nullptr);

    // Collect leaf path→hotkey pairs for HotkeyManager registration.
    static QList<QPair<QString, QKeySequence>> collectHotkeys(const QList<StoreEntry> &tree);

    // One-time migration from legacy Ini ItemN groups into the store folder.
    static void migrateLegacyItemsIfNeeded(const QString &storePath);

private:
    static QHash<QString, QKeySequence> readMetaHotkeys(const QString &dirPath);
    static QStringList readMetaOrder(const QString &dirPath);
    static bool writeMeta(const QString &dirPath,
                          const QStringList &order,
                          const QHash<QString, QKeySequence> &hotkeys);
    static StoreEntry makeEntry(const QString &dirPath, const QString &name,
                                const QHash<QString, QKeySequence> &hotkeys);
    static bool isMetaFile(const QString &name);
    static bool isHiddenName(const QString &name);
};
