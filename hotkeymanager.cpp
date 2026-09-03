#include "hotkeymanager.h"
#include <QApplication>
#include <QDebug>

#ifdef Q_OS_LINUX
#include <X11/Xlib.h>
#include <X11/keysym.h>

// Convert Qt modifier flags -> X11 modifier mask
static unsigned qtModsToX11(Qt::KeyboardModifiers mods)
{
    unsigned x = 0;
    if (mods & Qt::ShiftModifier)   x |= ShiftMask;
    if (mods & Qt::ControlModifier) x |= ControlMask;
    if (mods & Qt::AltModifier)     x |= Mod1Mask;
    if (mods & Qt::MetaModifier)    x |= Mod4Mask;
    return x;
}

// Convert Qt key -> X11 KeySym
static KeySym qtKeyToKeySym(int qtKey)
{
    // Letters/digits map directly
    if (qtKey >= Qt::Key_A && qtKey <= Qt::Key_Z)
        return XK_a + (qtKey - Qt::Key_A);
    if (qtKey >= Qt::Key_0 && qtKey <= Qt::Key_9)
        return XK_0 + (qtKey - Qt::Key_0);

    switch (qtKey) {
    case Qt::Key_F1:  return XK_F1;
    case Qt::Key_F2:  return XK_F2;
    case Qt::Key_F3:  return XK_F3;
    case Qt::Key_F4:  return XK_F4;
    case Qt::Key_F5:  return XK_F5;
    case Qt::Key_F6:  return XK_F6;
    case Qt::Key_F7:  return XK_F7;
    case Qt::Key_F8:  return XK_F8;
    case Qt::Key_F9:  return XK_F9;
    case Qt::Key_F10: return XK_F10;
    case Qt::Key_F11: return XK_F11;
    case Qt::Key_F12: return XK_F12;
    case Qt::Key_Space: return XK_space;
    case Qt::Key_Return: return XK_Return;
    case Qt::Key_Escape: return XK_Escape;
    default: return 0;
    }
}
#endif // Q_OS_LINUX

HotkeyManager::HotkeyManager(QObject *parent)
    : QObject(parent)
{
#ifdef Q_OS_LINUX
    m_display = XOpenDisplay(nullptr);
    if (m_display)
        m_rootWindow = DefaultRootWindow(static_cast<Display *>(m_display));
    qApp->installNativeEventFilter(this);
#endif
}

HotkeyManager::~HotkeyManager()
{
    unregisterAll();
#ifdef Q_OS_LINUX
    if (m_display) {
        qApp->removeNativeEventFilter(this);
        XCloseDisplay(static_cast<Display *>(m_display));
    }
#endif
}

void HotkeyManager::setGlobalHotkey(const QKeySequence &seq)
{
    unregisterAll();
    if (!seq.isEmpty())
        registerKey(seq, -1);
}

void HotkeyManager::setItemHotkeys(const QList<PasteItem> &items)
{
    // Keep picker registration (index -1), remove item registrations
    QList<HotkeyRegistration> pickerRegs;
    for (auto &r : m_regs) {
        if (r.itemIndex == -1) pickerRegs.append(r);
    }

#ifdef Q_OS_LINUX
    Display *dpy = static_cast<Display *>(m_display);
    if (dpy) {
        for (auto &r : m_regs) {
            if (r.itemIndex >= 0) {
                XUngrabKey(dpy, r.keycode, r.modifiers, m_rootWindow);
                // Also ungrab with NumLock (Mod2Mask) and CapsLock (LockMask)
                XUngrabKey(dpy, r.keycode, r.modifiers | Mod2Mask, m_rootWindow);
                XUngrabKey(dpy, r.keycode, r.modifiers | LockMask, m_rootWindow);
                XUngrabKey(dpy, r.keycode, r.modifiers | Mod2Mask | LockMask, m_rootWindow);
            }
        }
    }
#endif
    m_regs = pickerRegs;

    for (int i = 0; i < items.size(); ++i) {
        if (!items[i].hotkey.isEmpty())
            registerKey(items[i].hotkey, i);
    }
}

void HotkeyManager::unregisterAll()
{
#ifdef Q_OS_LINUX
    Display *dpy = static_cast<Display *>(m_display);
    if (dpy) {
        for (auto &r : m_regs) {
            XUngrabKey(dpy, r.keycode, r.modifiers, m_rootWindow);
            XUngrabKey(dpy, r.keycode, r.modifiers | Mod2Mask, m_rootWindow);
            XUngrabKey(dpy, r.keycode, r.modifiers | LockMask, m_rootWindow);
            XUngrabKey(dpy, r.keycode, r.modifiers | Mod2Mask | LockMask, m_rootWindow);
        }
    }
#endif
    m_regs.clear();
}

void HotkeyManager::registerKey(const QKeySequence &seq, int itemIndex)
{
    if (seq.isEmpty()) return;

#ifdef Q_OS_LINUX
    Display *dpy = static_cast<Display *>(m_display);
    if (!dpy) return;

    // Qt6: seq[0] is Qt::Key combo; Qt5: seq[0] is int
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    Qt::Key key = seq[0].key();
    Qt::KeyboardModifiers mods = seq[0].keyboardModifiers();
#else
    int combo = seq[0];
    Qt::Key key = static_cast<Qt::Key>(combo & ~Qt::KeyboardModifierMask);
    Qt::KeyboardModifiers mods = static_cast<Qt::KeyboardModifiers>(combo & Qt::KeyboardModifierMask);
#endif

    KeySym sym = qtKeyToKeySym(static_cast<int>(key));
    if (sym == 0) {
        qWarning() << "HotkeyManager: cannot map key" << key;
        return;
    }

    KeyCode kc = XKeysymToKeycode(dpy, sym);
    unsigned xmods = qtModsToX11(mods);

    // Grab with common lock-key combinations
    unsigned lockVariants[] = {0, Mod2Mask, LockMask, Mod2Mask | LockMask};
    for (unsigned lv : lockVariants)
        XGrabKey(dpy, kc, xmods | lv, m_rootWindow, True, GrabModeAsync, GrabModeAsync);

    HotkeyRegistration reg;
    reg.keycode   = kc;
    reg.modifiers = xmods;
    reg.itemIndex = itemIndex;
    m_regs.append(reg);
#else
    Q_UNUSED(seq); Q_UNUSED(itemIndex);
#endif
}

bool HotkeyManager::nativeEventFilter(const QByteArray &eventType, void *message, qintptr * /*result*/)
{
#ifdef Q_OS_LINUX
    if (eventType != "xcb_generic_event_t") return false;

    // We receive raw XEvent through Qt's XCB connection
    // Qt on X11 wraps xcb events; we can also use XNextEvent approach but
    // it's simpler to check via xcb structure. Use XEvent cast via XCB.
    // Actually with Qt6/xcb we get xcb_generic_event_t*; use response_type.
    auto *ev = static_cast<xcb_generic_event_t *>(message);
    if (!ev) return false;

    // XCB key press event type = 2
    if ((ev->response_type & ~0x80) != 2 /*XCB_KEY_PRESS*/) return false;

    auto *kev = reinterpret_cast<xcb_key_press_event_t *>(ev);
    unsigned mods = kev->state & ~(Mod2Mask | LockMask); // strip numlock/capslock

    for (auto &r : m_regs) {
        if (r.keycode == kev->detail && r.modifiers == mods) {
            if (r.itemIndex == -1)
                emit pickerRequested();
            else
                emit itemTriggered(r.itemIndex);
            return true; // consumed
        }
    }
#else
    Q_UNUSED(eventType); Q_UNUSED(message);
#endif
    return false;
}
