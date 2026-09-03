#pragma once

#include "pasteitem.h"
#include <QObject>
#include <QKeySequence>
#include <QList>
#include <QAbstractNativeEventFilter>
#include <QHash>
#include <QSet>

// index == -1  -> global picker hotkey
// index >= 0   -> per-item hotkey
class HotkeyManager : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT
public:
    explicit HotkeyManager(QObject *parent = nullptr);
    ~HotkeyManager() override;

    void setGlobalHotkey(const QKeySequence &seq);
    void setItemHotkeys(const QList<PasteItem> &items);

    // Returns true if the picker global hotkey is currently registered OK
    bool isPickerRegistered() const;

    // Returns the set of item indices whose hotkey is registered successfully
    QSet<int> registeredItemIndices() const;

signals:
    void pickerRequested();
    void itemTriggered(int index);

    // Emitted whenever registration state changes (e.g. after setGlobalHotkey / setItemHotkeys)
    void registrationChanged();

protected:
    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;

private:
    void unregisterAll();
    void unregisterItemHotkeys();
    bool registerKey(const QKeySequence &seq, int itemIndex);

    int m_nextId = 1;

    // Maps Windows hotkey ID -> item index (-1 = picker)
    QHash<int, int> m_idToIndex;

    // Tracks which registrations succeeded
    bool     m_pickerOk = false;
    QSet<int> m_itemOkIndices; // item indices that registered successfully
};
