#include "trayicon.h"
#include "settingswindow.h"
#include "hotkeymanager.h"
#include "storagemanager.h"

#include <QApplication>
#include <QClipboard>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QCursor>
#include <QTimer>
#include <QStyle>

#ifdef Q_OS_LINUX
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <unistd.h>
#endif

TrayIcon::TrayIcon(QObject *parent)
    : QObject(parent)
{
    // --- Tray icon ---
    m_tray = new QSystemTrayIcon(this);
    // Use a built-in Qt icon as placeholder; replace with a real icon resource later
    m_tray->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogSaveButton));
    m_tray->setToolTip("Fast Paste");

    // --- Tray menu ---
    m_trayMenu = new QMenu();
    QAction *actSettings = m_trayMenu->addAction("Settings…");
    QAction *actAbout    = m_trayMenu->addAction("About");
    m_trayMenu->addSeparator();
    QAction *actQuit = m_trayMenu->addAction("Quit");

    connect(actSettings, &QAction::triggered, this, &TrayIcon::onSettingsTriggered);
    connect(actAbout,    &QAction::triggered, this, &TrayIcon::onAboutTriggered);
    connect(actQuit,     &QAction::triggered, qApp, &QApplication::quit);

    m_tray->setContextMenu(m_trayMenu);
    connect(m_tray, &QSystemTrayIcon::activated, this, &TrayIcon::onTrayActivated);
    m_tray->show();

    // --- Settings window ---
    m_settingsWin = new SettingsWindow();
    connect(m_settingsWin, &SettingsWindow::dataChanged,
            this, &TrayIcon::onDataChanged);

    // --- Hotkey manager ---
    m_hotkeyMgr = new HotkeyManager(this);
    connect(m_hotkeyMgr, &HotkeyManager::pickerRequested,
            this, &TrayIcon::showPickerMenu);
    connect(m_hotkeyMgr, &HotkeyManager::itemTriggered,
            this, &TrayIcon::pasteItem);

    loadAndApply();
}

TrayIcon::~TrayIcon()
{
    delete m_settingsWin;
    delete m_trayMenu;
}

void TrayIcon::loadAndApply()
{
    m_items        = StorageManager::loadItems();
    m_globalHotkey = StorageManager::loadGlobalHotkey();

    m_settingsWin->setData(m_items, m_globalHotkey);

    m_hotkeyMgr->setGlobalHotkey(m_globalHotkey);
    m_hotkeyMgr->setItemHotkeys(m_items);
}

void TrayIcon::onTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger) // left-click
        showPickerMenu();
}

void TrayIcon::onSettingsTriggered()
{
    m_settingsWin->show();
    m_settingsWin->raise();
    m_settingsWin->activateWindow();
}

void TrayIcon::onAboutTriggered()
{
    QMessageBox::about(nullptr, "About Fast Paste",
        "<b>Fast Paste</b><br>"
        "Quickly paste pre-configured text snippets via a global hotkey or system tray.<br><br>"
        "Version 1.0");
}

void TrayIcon::onDataChanged(const QList<PasteItem> &items, const QKeySequence &globalHotkey)
{
    m_items        = items;
    m_globalHotkey = globalHotkey;

    StorageManager::save(items, globalHotkey);

    m_hotkeyMgr->setGlobalHotkey(globalHotkey);
    m_hotkeyMgr->setItemHotkeys(items);
}

// ─── Picker menu ─────────────────────────────────────────────────────────────

void TrayIcon::showPickerMenu()
{
    if (m_items.isEmpty()) {
        m_tray->showMessage("Fast Paste",
            "No items configured. Open Settings to add some.",
            QSystemTrayIcon::Information, 2000);
        return;
    }

    QMenu menu;
    for (int i = 0; i < m_items.size(); ++i) {
        const PasteItem &item = m_items[i];

        // Label: first line of text (truncated to 60 chars)
        QString label = item.text;
        int nl = label.indexOf('\n');
        if (nl != -1) label = label.left(nl) + "…";
        if (label.length() > 60) label = label.left(60) + "…";
        if (label.trimmed().isEmpty()) label = QString("(item %1)").arg(i + 1);

        QAction *act = menu.addAction(label);
        if (!item.hotkey.isEmpty())
            act->setShortcut(item.hotkey);

        int idx = i;
        connect(act, &QAction::triggered, this, [this, idx]{ pasteItem(idx); });
    }

    menu.exec(QCursor::pos());
}

void TrayIcon::pasteItem(int index)
{
    if (index < 0 || index >= m_items.size()) return;

    // 1. Set clipboard
    QApplication::clipboard()->setText(m_items[index].text);

    // 2. Send Ctrl+V to the previously focused window.
    //    We delay slightly so the menu can close first.
    QTimer::singleShot(80, this, [this] { sendCtrlV(); });
}

void TrayIcon::sendCtrlV()
{
#ifdef Q_OS_LINUX
    Display *dpy = XOpenDisplay(nullptr);
    if (!dpy) return;

    // Find the window that currently holds focus
    Window focusWin = 0;
    int revert = 0;
    XGetInputFocus(dpy, &focusWin, &revert);

    if (focusWin == None || focusWin == PointerRoot) {
        XCloseDisplay(dpy);
        return;
    }

    KeyCode vKey     = XKeysymToKeycode(dpy, XK_v);
    KeyCode ctrlKey  = XKeysymToKeycode(dpy, XK_Control_L);

    // Press Ctrl
    XEvent ev{};
    ev.type                = KeyPress;
    ev.xkey.display        = dpy;
    ev.xkey.window         = focusWin;
    ev.xkey.root           = DefaultRootWindow(dpy);
    ev.xkey.time           = CurrentTime;
    ev.xkey.state          = ControlMask;
    ev.xkey.keycode        = ctrlKey;
    ev.xkey.same_screen    = True;
    XSendEvent(dpy, focusWin, True, KeyPressMask, &ev);

    // Press V
    ev.xkey.keycode = vKey;
    XSendEvent(dpy, focusWin, True, KeyPressMask, &ev);

    // Release V
    ev.type = KeyRelease;
    XSendEvent(dpy, focusWin, True, KeyReleaseMask, &ev);

    // Release Ctrl
    ev.xkey.keycode = ctrlKey;
    XSendEvent(dpy, focusWin, True, KeyReleaseMask, &ev);

    XFlush(dpy);
    XCloseDisplay(dpy);
#endif
}
