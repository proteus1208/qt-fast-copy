#pragma once

#include "pasteitem.h"
#include <QObject>
#include <QKeySequence>
#include <QList>
#include <QAbstractNativeEventFilter>
#include <QHash>
#include <QSet>
#include <QWidget>

// index == -1  -> global picker hotkey
// index >= 0   -> per-item global hotkey (works without any window open)
class HotkeyManager : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT
public:
    explicit HotkeyManager(QObject *parent = nullptr);
    ~HotkeyManager() override;

    void setGlobalHotkey(const QKeySequence &seq);
    void setItemHotkeys(const QList<PasteItem> &items);

    bool isPickerRegistered() const;
    QSet<int> registeredItemIndices() const;

signals:
    void pickerRequested();
    void itemTriggered(int index);
    void registrationChanged();

protected:
    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;

private:
    void unregisterAll();
    void unregisterItemHotkeys();
    bool registerKey(const QKeySequence &seq, int itemIndex);
    bool handleHotkeyId(int id);

#ifdef Q_OS_WIN
    void *nativeHwnd() const; // HWND
#endif

    QWidget *m_sink = nullptr; // hidden window that receives WM_HOTKEY
    int m_nextId = 1;
    QHash<int, int> m_idToIndex;
    bool m_pickerOk = false;
    QSet<int> m_itemOkIndices;
};
