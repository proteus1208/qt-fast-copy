#include "hotkeymanager.h"

#include <QApplication>
#include <QDebug>
#include <functional>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace {

constexpr int kActionPicker = -1;
constexpr int kActionPasteTextOnly = -2;

class HotkeySinkWidget : public QWidget
{
public:
    using Handler = std::function<bool(int)>;

    explicit HotkeySinkWidget(Handler handler)
        : QWidget(nullptr)
        , m_handler(std::move(handler))
    {
        setObjectName(QStringLiteral("FastPasteHotkeySink"));
        setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus);
        setAttribute(Qt::WA_DontShowOnScreen, true);
        setAttribute(Qt::WA_NativeWindow, true);
        resize(1, 1);
    }

protected:
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override
    {
#ifdef Q_OS_WIN
        if (eventType == "windows_generic_MSG" || eventType == "windows_dispatcher_MSG") {
            auto *msg = static_cast<MSG *>(message);
            if (msg && msg->message == WM_HOTKEY && m_handler(static_cast<int>(msg->wParam))) {
                if (result)
                    *result = 0;
                return true;
            }
        }
#else
        Q_UNUSED(eventType);
        Q_UNUSED(message);
        Q_UNUSED(result);
#endif
        return QWidget::nativeEvent(eventType, message, result);
    }

private:
    Handler m_handler;
};

} // namespace

#ifdef Q_OS_WIN
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
        return static_cast<UINT>('A' + (key - Qt::Key_A));
    if (key >= Qt::Key_0 && key <= Qt::Key_9)
        return static_cast<UINT>('0' + (key - Qt::Key_0));
    if (key >= Qt::Key_F1 && key <= Qt::Key_F24)
        return static_cast<UINT>(VK_F1 + (key - Qt::Key_F1));

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
    if (seq.isEmpty())
        return false;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    const QKeyCombination combo = seq[0];
    outKey  = combo.key();
    outMods = combo.keyboardModifiers();
#else
    const int combo = seq[0];
    outKey  = static_cast<Qt::Key>(combo & ~Qt::KeyboardModifierMask);
    outMods = static_cast<Qt::KeyboardModifiers>(combo & Qt::KeyboardModifierMask);
#endif
    outMods &= (Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier);
    return outKey != Qt::Key_unknown && outKey != 0;
}
#endif // Q_OS_WIN

HotkeyManager::HotkeyManager(QObject *parent)
    : QObject(parent)
{
    m_sink = new HotkeySinkWidget([this](int id) { return handleHotkeyId(id); });
    m_sink->winId();

    qApp->installNativeEventFilter(this);
}

HotkeyManager::~HotkeyManager()
{
    unregisterAll();
    qApp->removeNativeEventFilter(this);
    delete m_sink;
    m_sink = nullptr;
}

bool HotkeyManager::isPickerRegistered() const
{
    return m_pickerOk;
}

bool HotkeyManager::isPasteTextOnlyRegistered() const
{
    return m_pasteTextOk;
}

QSet<QString> HotkeyManager::registeredItemPaths() const
{
    return m_itemOkPaths;
}

#ifdef Q_OS_WIN
void *HotkeyManager::nativeHwnd() const
{
    if (!m_sink)
        return nullptr;
    return reinterpret_cast<void *>(m_sink->winId());
}
#endif

void HotkeyManager::unregisterByAction(int action)
{
    QList<int> toRemove;
    for (auto it = m_idToAction.begin(); it != m_idToAction.end(); ++it) {
        if (action >= 0) {
            if (it.value() >= 0)
                toRemove.append(it.key());
        } else if (it.value() == action) {
            toRemove.append(it.key());
        }
    }

#ifdef Q_OS_WIN
    HWND hwnd = static_cast<HWND>(nativeHwnd());
    for (int id : toRemove) {
        if (hwnd)
            UnregisterHotKey(hwnd, id);
        m_idToAction.remove(id);
    }
#else
    for (int id : toRemove)
        m_idToAction.remove(id);
#endif
}

void HotkeyManager::setGlobalHotkey(const QKeySequence &seq)
{
    unregisterByAction(kActionPicker);
    m_pickerOk = false;
    if (!seq.isEmpty())
        m_pickerOk = registerKey(seq, kActionPicker);
    emit registrationChanged();
}

void HotkeyManager::setPasteTextOnlyHotkey(const QKeySequence &seq)
{
    unregisterByAction(kActionPasteTextOnly);
    m_pasteTextOk = false;
    if (!seq.isEmpty())
        m_pasteTextOk = registerKey(seq, kActionPasteTextOnly);
    emit registrationChanged();
}

void HotkeyManager::setItemHotkeys(const QList<QPair<QString, QKeySequence>> &pathAndHotkey)
{
    unregisterItemHotkeys();
    m_paths.clear();
    m_paths.reserve(pathAndHotkey.size());

    for (const auto &pair : pathAndHotkey) {
        if (pair.second.isEmpty() || pair.first.isEmpty())
            continue;
        const int action = m_paths.size();
        m_paths.append(pair.first);
        if (registerKey(pair.second, action))
            m_itemOkPaths.insert(pair.first);
    }

    emit registrationChanged();
}

void HotkeyManager::unregisterItemHotkeys()
{
    unregisterByAction(0); // clears all action >= 0
    m_itemOkPaths.clear();
}

void HotkeyManager::unregisterAll()
{
#ifdef Q_OS_WIN
    HWND hwnd = static_cast<HWND>(nativeHwnd());
    if (hwnd) {
        for (int id : m_idToAction.keys())
            UnregisterHotKey(hwnd, id);
    }
#endif
    m_idToAction.clear();
    m_paths.clear();
    m_pickerOk = false;
    m_pasteTextOk = false;
    m_itemOkPaths.clear();
}

bool HotkeyManager::registerKey(const QKeySequence &seq, int action)
{
    if (seq.isEmpty())
        return false;

#ifdef Q_OS_WIN
    Qt::Key key = Qt::Key_unknown;
    Qt::KeyboardModifiers mods;
    if (!decodeSequence(seq, key, mods))
        return false;

    const UINT vk    = qtKeyToVK(key);
    const UINT wmods = qtModsToWin(mods);
    HWND hwnd        = static_cast<HWND>(nativeHwnd());

    if (!hwnd || vk == 0) {
        qWarning() << "HotkeyManager: cannot register" << seq.toString();
        return false;
    }

    const int id = m_nextId++;
    if (!RegisterHotKey(hwnd, id, wmods | MOD_NOREPEAT, vk)) {
        qWarning() << "HotkeyManager: RegisterHotKey failed for"
                   << seq.toString() << "error:" << GetLastError();
        return false;
    }

    m_idToAction[id] = action;
    return true;
#else
    Q_UNUSED(seq);
    Q_UNUSED(action);
    return false;
#endif
}

bool HotkeyManager::handleHotkeyId(int id)
{
    const auto it = m_idToAction.find(id);
    if (it == m_idToAction.end())
        return false;

    const int action = it.value();
    if (action == kActionPicker) {
        emit pickerRequested();
    } else if (action == kActionPasteTextOnly) {
        emit pasteTextOnlyRequested();
    } else if (action >= 0 && action < m_paths.size()) {
        emit itemTriggered(m_paths[action]);
    } else {
        return false;
    }
    return true;
}

bool HotkeyManager::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result)
{
    Q_UNUSED(result);

#ifdef Q_OS_WIN
    if (eventType != "windows_generic_MSG" && eventType != "windows_dispatcher_MSG")
        return false;

    auto *msg = static_cast<MSG *>(message);
    if (!msg || msg->message != WM_HOTKEY)
        return false;

    return handleHotkeyId(static_cast<int>(msg->wParam));
#else
    Q_UNUSED(eventType);
    Q_UNUSED(message);
    return false;
#endif
}
