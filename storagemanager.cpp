#include "storagemanager.h"

#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileIconProvider>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSaveFile>
#include <QSet>
#include <QRegularExpression>
#include <QPair>

static const char *ORG = "qt-fast-copy";
static const char *APP = "qt-fast-copy";

static QFileIconProvider &iconProvider()
{
    static QFileIconProvider provider;
    return provider;
}

// ─── App settings ────────────────────────────────────────────────────────────

QString StorageManager::defaultStorePath()
{
    return QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
        .filePath(QStringLiteral("FastPaste"));
}

AppSettings StorageManager::loadAppSettings()
{
    QSettings s(QSettings::IniFormat, QSettings::UserScope, ORG, APP);
    AppSettings a;
    a.storePath = s.value(QStringLiteral("General/storePath"), defaultStorePath()).toString();
    if (a.storePath.trimmed().isEmpty())
        a.storePath = defaultStorePath();
    a.globalHotkey = QKeySequence(
        s.value(QStringLiteral("General/globalHotkey"), QStringLiteral("Ctrl+Shift+V")).toString());
    a.pasteTextOnlyHotkey = QKeySequence(
        s.value(QStringLiteral("General/pasteTextOnlyHotkey"), QStringLiteral("Ctrl+Shift+T")).toString());
    return a;
}

void StorageManager::saveAppSettings(const AppSettings &settings)
{
    QSettings s(QSettings::IniFormat, QSettings::UserScope, ORG, APP);
    s.setValue(QStringLiteral("General/storePath"), settings.storePath);
    s.setValue(QStringLiteral("General/globalHotkey"), settings.globalHotkey.toString());
    s.setValue(QStringLiteral("General/pasteTextOnlyHotkey"), settings.pasteTextOnlyHotkey.toString());
}

void StorageManager::ensureStoreFolder(const QString &storePath)
{
    QDir().mkpath(storePath);
}

// ─── Meta helpers ────────────────────────────────────────────────────────────

bool StorageManager::isMetaFile(const QString &name)
{
    return name.compare(QLatin1String(kMetaFileName), Qt::CaseInsensitive) == 0;
}

bool StorageManager::isHiddenName(const QString &name)
{
    return name.startsWith(QLatin1Char('.'));
}

QStringList StorageManager::readMetaOrder(const QString &dirPath)
{
    const QString metaPath = QDir(dirPath).filePath(QLatin1String(kMetaFileName));
    QFile f(metaPath);
    if (!f.open(QIODevice::ReadOnly))
        return {};

    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();
    if (!doc.isObject())
        return {};

    const QJsonArray order = doc.object().value(QStringLiteral("order")).toArray();
    QStringList out;
    out.reserve(order.size());
    for (const QJsonValue &v : order)
        out.append(v.toString());
    return out;
}

QHash<QString, QKeySequence> StorageManager::readMetaHotkeys(const QString &dirPath)
{
    const QString metaPath = QDir(dirPath).filePath(QLatin1String(kMetaFileName));
    QFile f(metaPath);
    if (!f.open(QIODevice::ReadOnly))
        return {};

    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();
    if (!doc.isObject())
        return {};

    QHash<QString, QKeySequence> out;
    const QJsonObject hotkeys = doc.object().value(QStringLiteral("hotkeys")).toObject();
    for (auto it = hotkeys.begin(); it != hotkeys.end(); ++it)
        out.insert(it.key(), QKeySequence(it.value().toString()));
    return out;
}

bool StorageManager::writeMeta(const QString &dirPath,
                               const QStringList &order,
                               const QHash<QString, QKeySequence> &hotkeys)
{
    ensureStoreFolder(dirPath);

    QJsonArray orderArr;
    for (const QString &name : order)
        orderArr.append(name);

    QJsonObject hotkeysObj;
    for (auto it = hotkeys.begin(); it != hotkeys.end(); ++it) {
        if (!it.value().isEmpty())
            hotkeysObj.insert(it.key(), it.value().toString());
    }

    QJsonObject root;
    root.insert(QStringLiteral("order"), orderArr);
    root.insert(QStringLiteral("hotkeys"), hotkeysObj);

    QSaveFile f(QDir(dirPath).filePath(QLatin1String(kMetaFileName)));
    if (!f.open(QIODevice::WriteOnly))
        return false;
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return f.commit();
}

StoreEntry StorageManager::makeEntry(const QString &dirPath, const QString &name,
                                     const QHash<QString, QKeySequence> &hotkeys)
{
    StoreEntry e;
    e.name = name;
    e.absolutePath = QDir(dirPath).filePath(name);

    const QFileInfo info(e.absolutePath);
    e.icon = iconProvider().icon(info);

    if (info.isDir()) {
        e.kind = StoreEntryKind::Folder;
    } else if (info.suffix().isEmpty()) {
        e.kind = StoreEntryKind::Text;
        e.hotkey = hotkeys.value(name);
    } else {
        e.kind = StoreEntryKind::Path;
        e.hotkey = hotkeys.value(name);
    }
    return e;
}

// ─── Load / save directory ───────────────────────────────────────────────────

QList<StoreEntry> StorageManager::loadDirectory(const QString &dirPath)
{
    QList<StoreEntry> entries;
    QDir dir(dirPath);
    if (!dir.exists())
        return entries;

    const QHash<QString, QKeySequence> hotkeys = readMetaHotkeys(dirPath);
    const QStringList preferred = readMetaOrder(dirPath);

    const QFileInfoList infos = dir.entryInfoList(
        QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot,
        QDir::Name | QDir::IgnoreCase);

    QHash<QString, QFileInfo> byName;
    for (const QFileInfo &info : infos) {
        const QString name = info.fileName();
        if (isMetaFile(name) || isHiddenName(name))
            continue;
        byName.insert(name, info);
    }

    QSet<QString> used;
    for (const QString &name : preferred) {
        if (!byName.contains(name) || used.contains(name))
            continue;
        entries.append(makeEntry(dirPath, name, hotkeys));
        used.insert(name);
    }

    QStringList remaining = byName.keys();
    remaining.sort(Qt::CaseInsensitive);
    for (const QString &name : remaining) {
        if (used.contains(name))
            continue;
        entries.append(makeEntry(dirPath, name, hotkeys));
    }

    return entries;
}

QList<StoreEntry> StorageManager::loadTree(const QString &dirPath)
{
    QList<StoreEntry> entries = loadDirectory(dirPath);
    for (StoreEntry &e : entries) {
        if (e.kind == StoreEntryKind::Folder)
            e.children = loadTree(e.absolutePath);
    }
    return entries;
}

bool StorageManager::saveDirectoryMeta(const QString &dirPath,
                                       const QList<StoreEntry> &entries)
{
    QStringList order;
    QHash<QString, QKeySequence> hotkeys;
    order.reserve(entries.size());
    for (const StoreEntry &e : entries) {
        order.append(e.name);
        if (!e.hotkey.isEmpty() && e.kind != StoreEntryKind::Folder)
            hotkeys.insert(e.name, e.hotkey);
    }
    return writeMeta(dirPath, order, hotkeys);
}

// ─── File operations ─────────────────────────────────────────────────────────

QString StorageManager::readTextFile(const QString &absolutePath)
{
    QFile f(absolutePath);
    if (!f.open(QIODevice::ReadOnly))
        return {};
    return QString::fromUtf8(f.readAll());
}

bool StorageManager::writeTextFile(const QString &absolutePath, const QString &text)
{
    QSaveFile f(absolutePath);
    if (!f.open(QIODevice::WriteOnly))
        return false;
    f.write(text.toUtf8());
    return f.commit();
}

bool StorageManager::createTextItem(const QString &dirPath,
                                    const QString &name,
                                    const QString &text,
                                    const QKeySequence &hotkey,
                                    QString *errorOut)
{
    const QString safeName = name.trimmed();
    if (safeName.isEmpty()) {
        if (errorOut) *errorOut = QStringLiteral("Name is required.");
        return false;
    }
    if (safeName.contains(QLatin1Char('/')) || safeName.contains(QLatin1Char('\\'))
        || safeName.contains(QLatin1Char('.')) || isMetaFile(safeName) || isHiddenName(safeName)) {
        if (errorOut)
            *errorOut = QStringLiteral("Use a name without extension or path separators.");
        return false;
    }

    ensureStoreFolder(dirPath);
    const QString path = QDir(dirPath).filePath(safeName);
    if (QFileInfo::exists(path)) {
        if (errorOut) *errorOut = QStringLiteral("An item with that name already exists.");
        return false;
    }

    if (!writeTextFile(path, text)) {
        if (errorOut) *errorOut = QStringLiteral("Could not write the file.");
        return false;
    }

    QList<StoreEntry> entries = loadDirectory(dirPath);
    // New item already appears (alphabetically if not in meta). Pin it at end with hotkey.
    for (StoreEntry &e : entries) {
        if (e.name == safeName) {
            e.hotkey = hotkey;
            break;
        }
    }
    saveDirectoryMeta(dirPath, entries);
    return true;
}

bool StorageManager::updateEntryHotkey(const QString &dirPath,
                                       const QString &name,
                                       const QKeySequence &hotkey)
{
    QList<StoreEntry> entries = loadDirectory(dirPath);
    bool found = false;
    for (StoreEntry &e : entries) {
        if (e.name == name) {
            e.hotkey = hotkey;
            found = true;
            break;
        }
    }
    if (!found)
        return false;
    return saveDirectoryMeta(dirPath, entries);
}

bool StorageManager::removeEntry(const QString &absolutePath, QString *errorOut)
{
    QFileInfo info(absolutePath);
    if (!info.exists()) {
        if (errorOut) *errorOut = QStringLiteral("Item not found.");
        return false;
    }

    const QString dirPath = info.absolutePath();
    const QString name = info.fileName();

    bool ok = false;
    if (info.isDir())
        ok = QDir(absolutePath).removeRecursively();
    else
        ok = QFile::remove(absolutePath);

    if (!ok) {
        if (errorOut) *errorOut = QStringLiteral("Could not delete the item.");
        return false;
    }

    // Drop from meta order/hotkeys if present.
    QList<StoreEntry> entries = loadDirectory(dirPath);
    saveDirectoryMeta(dirPath, entries);
    Q_UNUSED(name);
    return true;
}

QList<QPair<QString, QKeySequence>> StorageManager::collectHotkeys(const QList<StoreEntry> &tree)
{
    QList<QPair<QString, QKeySequence>> out;
    for (const StoreEntry &e : tree) {
        if (e.kind == StoreEntryKind::Folder) {
            out += collectHotkeys(e.children);
        } else if (!e.hotkey.isEmpty()) {
            out.append({e.absolutePath, e.hotkey});
        }
    }
    return out;
}

// ─── Legacy migration ────────────────────────────────────────────────────────

void StorageManager::migrateLegacyItemsIfNeeded(const QString &storePath)
{
    QSettings s(QSettings::IniFormat, QSettings::UserScope, ORG, APP);
    if (s.value(QStringLiteral("General/migratedToFolderStore"), false).toBool())
        return;

    const int count = s.value(QStringLiteral("General/itemCount"), 0).toInt();
    if (count <= 0) {
        s.setValue(QStringLiteral("General/migratedToFolderStore"), true);
        return;
    }

    ensureStoreFolder(storePath);
    QList<StoreEntry> created;

    for (int i = 0; i < count; ++i) {
        s.beginGroup(QStringLiteral("Item%1").arg(i + 1));
        const QString title = s.value(QStringLiteral("title")).toString().trimmed();
        const QString text  = s.value(QStringLiteral("text")).toString();
        const QKeySequence hotkey(s.value(QStringLiteral("hotkey")).toString());
        s.endGroup();

        QString name = title;
        name.replace(QRegularExpression(QStringLiteral(R"([\\/:*?"<>|])")), QStringLiteral("_"));
        name = name.trimmed();
        if (name.isEmpty())
            name = QStringLiteral("item%1").arg(i + 1);
        // Extensionless text file
        if (name.contains(QLatin1Char('.')))
            name.replace(QLatin1Char('.'), QLatin1Char('_'));

        QString unique = name;
        int n = 2;
        while (QFileInfo::exists(QDir(storePath).filePath(unique)))
            unique = QStringLiteral("%1_%2").arg(name).arg(n++);

        const QString path = QDir(storePath).filePath(unique);
        if (!writeTextFile(path, text))
            continue;

        StoreEntry e;
        e.name = unique;
        e.absolutePath = path;
        e.kind = StoreEntryKind::Text;
        e.hotkey = hotkey;
        created.append(e);
    }

    if (!created.isEmpty())
        saveDirectoryMeta(storePath, created);

    // Clear legacy item groups; keep General hotkeys / storePath.
    for (int i = 0; i < count; ++i)
        s.remove(QStringLiteral("Item%1").arg(i + 1));
    s.remove(QStringLiteral("General/itemCount"));
    s.setValue(QStringLiteral("General/migratedToFolderStore"), true);
}
