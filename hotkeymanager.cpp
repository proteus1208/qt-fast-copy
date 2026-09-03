#include "hotkeymanager.h"

#include <QApplication>
#include <QDebug>

#ifdef Q_OS_WIN
#include <windows.h>

static UINT qtModsToWin(Qt::KeyboardModifiers mods)
{
    UINT w = 0;
    if (mods & Qt::AltModifier)     w |= MOD_ALT;
    if (mods & Qt::ControlModifier) w |= MOD_CONTROL;
    if (mods & Qt::ShiftModifier)   w |= MOD_SHIFT;
    if (mods & Qt::MetaModifier)    w |= MOD_WIN;
    return w;
}

static UINT qtKeyToVK(Qt::Key key)
{
    if (key >= Qt::Key_A && key <= Qt::Key_Z)
        return 'A' + (key - Qt::Key_A);
    if (key >= Qt::Key_0 && key <= Qt::Key_9)
        return '0' + (key - Qt::Key_0);
    if (key >= Qt::Key_F1 && key <= Qt::Key_F24)
        return VK_F1 + (key - Qt::Key_F1);

    switch (key) {
    case Qt::Key_Space:        return VK_SPACE;
    case Qt::Key_Return:       return VK_RETURN;
    case Qt::Key_Enter:        return VK_RETURN;
    case Qt::Key_Escape:       return VK_ESCAPE;
    case Qt::Key_Tab:          return VK_TAB;
    case Qt::Key_Backspace:    return VK_BACK;
    case Qt::Key_Delete:       return VK_DELETE;
    case Qt::Key_Insert:       return VK_INSERT;
    case Qt::Key_Home:         return VK_HOME;
    case Qt::Key_End:          return VK_END;
    case Qt::Key_PageUp:       return VK_PRIOR;
    case Qt::Key_PageDown:     return VK_NEXT;
    case Qt::Key_Left:         return VK_LEFT;
    case Qt::Key_Right:        return VK_RIGHT;
    case Qt::Key_Up:           return VK_UP;
    case Qt::Key_Down:         return VK_DOWN;
    case Qt::Key_Print:        return VK_SNAPSHOT;
    case Qt::Key_Pause:        return VK_PAUSE;
    case Qt::Key_CapsLock:     return VK_CAPITAL;
    case Qt::Key_NumLock:      return VK_NUMLOCK;
    case Qt::Key_ScrollLock:   return VK_SCROLL;
    case Qt::Key_Semicolon:    return VK_OEM_1;
    case Qt::Key_Equal:        return VK_OEM_PLUS;
    case Qt::Key_Comma:        return VK_OEM_COMMA;
    case Qt::Key_Minus:        return VK_OEM_MINUS;
    case Qt::Key_Period:       return VK_OEM_PERIOD;
    case Qt::Key_Slash:        return VK_OEM_2;
    case Qt::Key_QuoteLeft:    return VK_OEM_3;
    case Qt::Key_BracketLeft:  return VK_OEM_4;
    case Qt::Key_Backslash:    return VK_OEM_5;
    case Qt::Key_BracketRight: return VK_OEM_6;
    case Qt::Key_Apostrophe:   return VK_OEM_7;
    default: return 0;
    }
}

static bool decodeSequence(const QKeySequence &seq,
                            Qt::Key &outKey,
                            Qt::KeyboardModifiers &outMods)
{
    if (seq.isEmpty()) return false;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    outKey  = seq[0].key();
    outMods = seq[0].keyboardModifiers();
#else
    int combo = seq[0];
    outKey  = static_cast<Qt::Key>(combo & ~Qt::KeyboardModifierMask);
    outMods = static_cast<Qt::KeyboardModifiers>(combo & Qt::KeyboardModifierMask);
#endif
    return true;
}
#endif // Q_OS_WIN

// ─────────────────────────────────────────────────────────────────────────────

HotkeyManager::HotkeyManager(QObject *parent)
    : QObject(parent)
{
    qApp->installNativeEventFilter(this);
}

HotkeyManager::~HotkeyManager()
{
    unregisterAll();
    qApp->removeNativeEventFilter(this);
}

bool HotkeyManager::isPickerRegistered() const
{
    return m_pickerOk;
}

QSet<int> HotkeyManager::registeredItemIndices() const
{
    return m_itemOkIndices;
}

void HotkeyManager::setGlobalHotkey(const QKeySequence &seq)
{
    // Remove old picker registrations
    QList<int> toRemove;
    for (auto it = m_idToIndex.begin(); it != m_idToIndex.end(); ++it)
        if (it.value() == -1) toRemove.append(it.key());

#ifdef Q_OS_WIN
    for (int id : toRemove) {
        UnregisterHotKey(nullptr, id);
        m_idToIndex.remove(id);
    }
#else
    for (int id : toRemove)
        m_idToIndex.remove(id);
#endif

    m_pickerOk = false;

    if (!seq.isEmpty())
        m_pickerOk = registerKey(seq, -1);

    emit registrationChanged();
}

void HotkeyManager::setItemHotkeys(const QList<PasteItem> &items)
{
    unregisterItemHotkeys();
    m_itemOkIndices.clear();

    for (int i = 0; i < items.size(); ++i) {
        if (!items[i].hotkey.isEmpty()) {
            if (registerKey(items[i].hotkey, i))
                m_itemOkIndices.insert(i);
        }
    }

    emit registrationChanged();
}

void HotkeyManager::unregisterItemHotkeys()
{
    QList<int> toRemove;
    for (auto it = m_idToIndex.begin(); it != m_idToIndex.end(); ++it)
        if (it.value() >= 0) toRemove.append(it.key());

#ifdef Q_OS_WIN
    for (int id : toRemove) {
        UnregisterHotKey(nullptr, id);
        m_idToIndex.remove(id);
    }
#else
    for (int id : toRemove)
        m_idToIndex.remove(id);
#endif

    m_itemOkIndices.clear();
}

void HotkeyManager::unregisterAll()
{
#ifdef Q_OS_WIN
    for (int id : m_idToIndex.keys())
        UnregisterHotKey(nullptr, id);
#endif
    m_idToIndex.clear();
    m_pickerOk = false;
    m_itemOkIndices.clear();
}

bool HotkeyManager::registerKey(const QKeySequence &seq, int itemIndex)
{
    if (seq.isEmpty()) return false;

#ifdef Q_OS_WIN
    Qt::Key key;
    Qt::KeyboardModifiers mods;
    if (!decodeSequence(seq, key, mods)) return false;

    UINT vk    = qtKeyToVK(key);
    UINT wmods = qtModsToWin(mods);

    if (vk == 0) {
        qWarning() << "HotkeyManager: cannot map key" << key;
        return false;
    }

    int id = m_nextId++;
    if (!RegisterHotKey(nullptr, id, wmods | MOD_NOREPEAT, vk)) {
        qWarning() << "HotkeyManager: RegisterHotKey failed for"
                   << seq.toString() << "error:" << GetLastError();
        return false;
    }

    m_idToIndex[id] = itemIndex;
    return true;
#else
    Q_UNUSED(seq); Q_UNUSED(itemIndex);
    return false;
#endif
}

bool HotkeyManager::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result)
{
    Q_UNUSED(result);

#ifdef Q_OS_WIN
    if (eventType != "windows_generic_MSG" && eventType != "windows_dispatcher_MSG")
        return false;

    MSG *msg = static_cast<MSG *>(message);
    if (!msg || msg->message != WM_HOTKEY)
        return false;

    int id = static_cast<int>(msg->wParam);
    auto it = m_idToIndex.find(id);
    if (it == m_idToIndex.end()) return false;

    if (it.value() == -1)
        emit pickerRequested();
    else
        emit itemTriggered(it.value());

    return true;
#else
    Q_UNUSED(eventType); Q_UNUSED(message);
    return false;
#endif
}
