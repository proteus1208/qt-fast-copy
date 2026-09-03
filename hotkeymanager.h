#pragma once

#include "pasteitem.h"
#include <QObject>
#include <QKeySequence>
#include <QList>
#include <QAbstractNativeEventFilter>

// Maps a registered keycode+modifiers back to an index:
//   index == -1  -> global picker hotkey
//   index >= 0   -> per-item hotkey
struct HotkeyRegistration {
    int      keycode;
    unsigned modifiers;
    int      itemIndex; // -1 = picker
};

class HotkeyManager : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT
public:
    explicit HotkeyManager(QObject *parent = nullptr);
    ~HotkeyManager() override;

    void setGlobalHotkey(const QKeySequence &seq);
    void setItemHotkeys(const QList<PasteItem> &items);

signals:
    void pickerRequested();
    void itemTriggered(int index);

protected:
    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;

private:
    void unregisterAll();
    void registerKey(const QKeySequence &seq, int itemIndex);

    QList<HotkeyRegistration> m_regs;
    void *m_display = nullptr; // Display*
    unsigned long m_rootWindow = 0;
};
