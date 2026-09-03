#include "storagemanager.h"
#include <QSettings>

static const char *ORG = "qt-fast-copy";
static const char *APP = "qt-fast-copy";

void StorageManager::save(const QList<PasteItem> &items, const QKeySequence &globalHotkey)
{
    QSettings s(QSettings::IniFormat, QSettings::UserScope, ORG, APP);
    s.clear();

    s.setValue("General/globalHotkey", globalHotkey.toString());
    s.setValue("General/itemCount", items.size());

    for (int i = 0; i < items.size(); ++i) {
        s.beginGroup(QString("Item%1").arg(i + 1));
        s.setValue("title", items[i].title);
        s.setValue("text", items[i].text);
        s.setValue("hotkey", items[i].hotkey.toString());
        s.endGroup();
    }
}

QList<PasteItem> StorageManager::loadItems()
{
    QSettings s(QSettings::IniFormat, QSettings::UserScope, ORG, APP);
    int count = s.value("General/itemCount", 0).toInt();
    QList<PasteItem> items;
    for (int i = 0; i < count; ++i) {
        s.beginGroup(QString("Item%1").arg(i + 1));
        PasteItem item;
        item.title  = s.value("title").toString();
        item.text   = s.value("text").toString();
        item.hotkey = QKeySequence(s.value("hotkey").toString());
        s.endGroup();
        items.append(item);
    }
    return items;
}

QKeySequence StorageManager::loadGlobalHotkey()
{
    QSettings s(QSettings::IniFormat, QSettings::UserScope, ORG, APP);
    QString hk = s.value("General/globalHotkey", "Ctrl+Shift+V").toString();
    return QKeySequence(hk);
}
