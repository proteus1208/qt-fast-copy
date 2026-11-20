#pragma once

#include <QObject>
#include <QKeySequence>
#include <QList>
#include <QPair>
#include <QAbstractNativeEventFilter>
#include <QHash>
#include <QSet>
#include <QWidget>
#include <QString>

// Special action ids stored in m_idToAction:
//   -1 → global picker hotkey
//   -2 → paste-text-only hotkey
//   >=0 → index into m_paths (absolute path of store entry)
class HotkeyManager : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT
public:
    explicit HotkeyManager(QObject *parent = nullptr);
    ~HotkeyManager() override;

    void setGlobalHotkey(const QKeySequence &seq);
    void setPasteTextOnlyHotkey(const QKeySequence &seq);
    void setItemHotkeys(const QList<QPair<QString, QKeySequence>> &pathAndHotkey);

    bool isPickerRegistered() const;
    bool isPasteTextOnlyRegistered() const;
    QSet<QString> registeredItemPaths() const;

signals:
    void pickerRequested();
    void pasteTextOnlyRequested();
    void itemTriggered(const QString &absolutePath);
    void registrationChanged();

protected:
    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;

private:
    void unregisterAll();
    void unregisterByAction(int action); // -1, -2, or clear all path actions (>=0)
    void unregisterItemHotkeys();
    bool registerKey(const QKeySequence &seq, int action);
    bool handleHotkeyId(int id);

#ifdef Q_OS_WIN
    void *nativeHwnd() const; // HWND
#endif

    QWidget *m_sink = nullptr;
    int m_nextId = 1;
    QHash<int, int> m_idToAction; // hotkey id → action
    QList<QString> m_paths;       // action index → absolute path
    bool m_pickerOk = false;
    bool m_pasteTextOk = false;
    QSet<QString> m_itemOkPaths;
};
