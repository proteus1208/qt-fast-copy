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
#include <QIcon>
#include <QPixmap>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

TrayIcon::TrayIcon(QObject *parent)
    : QObject(parent)
{
    // --- Tray icon ---
    m_tray = new QSystemTrayIcon(this);
    QIcon appIcon(":/assets/icon.ico");
    if (appIcon.isNull())
        appIcon = QApplication::style()->standardIcon(QStyle::SP_ComputerIcon);
    m_tray->setIcon(appIcon);
    qApp->setWindowIcon(appIcon); // also sets taskbar / window title-bar icon
    m_tray->setToolTip("Fast Paste\nLeft-click to open picker\nRight-click for menu");

    // --- Tray menu ---
    m_trayMenu = new QMenu();
    QAction *actSettings = m_trayMenu->addAction("⚙  Settings…");
    QAction *actAbout    = m_trayMenu->addAction("ℹ  About");
    m_trayMenu->addSeparator();
    QAction *actQuit = m_trayMenu->addAction("✕  Quit");

    connect(actSettings, &QAction::triggered, this, &TrayIcon::onSettingsTriggered);
    connect(actAbout,    &QAction::triggered, this, &TrayIcon::onAboutTriggered);
    connect(actQuit,     &QAction::triggered, qApp, &QApplication::quit);

    m_tray->setContextMenu(m_trayMenu);
    connect(m_tray, &QSystemTrayIcon::activated, this, &TrayIcon::onTrayActivated);
    m_tray->show();

    // --- Hotkey manager ---
    m_hotkeyMgr = new HotkeyManager(this);
    connect(m_hotkeyMgr, &HotkeyManager::pickerRequested,
            this, &TrayIcon::showPickerMenu);
    connect(m_hotkeyMgr, &HotkeyManager::itemTriggered,
            this, &TrayIcon::pasteItem);

    // --- Settings window ---
    m_settingsWin = new SettingsWindow();
    m_settingsWin->setHotkeyManager(m_hotkeyMgr);
    connect(m_settingsWin, &SettingsWindow::dataChanged,
            this, &TrayIcon::onDataChanged);

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
        "<b>Fast Paste</b><br><br>"
        "Quickly paste pre-configured text snippets via a global hotkey or system tray.<br><br>"
        "• Press the configured global hotkey to open the picker menu<br>"
        "• Configure per-item hotkeys to paste directly<br>"
        "• Text can contain multiple lines<br><br>"
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
            "No items configured.\nRight-click the tray icon → Settings to add some.",
            QSystemTrayIcon::Information, 3000);
        return;
    }

    QMenu menu;
    menu.setTitle("Fast Paste");

    for (int i = 0; i < m_items.size(); ++i) {
        const PasteItem &item = m_items[i];

        // Use title if set, otherwise fall back to first line of text
        QString label = item.title.trimmed();
        if (label.isEmpty()) {
            label = item.text;
            int nl = label.indexOf('\n');
            if (nl != -1) label = label.left(nl) + "…";
            if (label.length() > 60) label = label.left(60) + "…";
        }
        if (label.trimmed().isEmpty()) label = QString("(item %1)").arg(i + 1);

        QString displayLabel = QString("%1. %2").arg(i + 1).arg(label);

        QAction *act = menu.addAction(displayLabel);
        if (!item.hotkey.isEmpty()) {
            act->setShortcut(item.hotkey);
            act->setShortcutVisibleInContextMenu(true);
        }

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

    // Give the user time to release the item hotkey, then paste into the
    // currently focused app. Item hotkeys are global — no picker needed.
    QTimer::singleShot(250, this, [this] { sendCtrlV(); });
}

#ifdef Q_OS_WIN
static void releaseIfHeld(WORD vk)
{
    if (GetAsyncKeyState(vk) & 0x8000) {
        INPUT up{};
        up.type       = INPUT_KEYBOARD;
        up.ki.wVk     = vk;
        up.ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(1, &up, sizeof(INPUT));
    }
}
#endif

void TrayIcon::sendCtrlV()
{
#ifdef Q_OS_WIN
    // If the item hotkey used Ctrl/Shift/Alt/Win, those keys may still be down.
    releaseIfHeld(VK_CONTROL);
    releaseIfHeld(VK_LCONTROL);
    releaseIfHeld(VK_RCONTROL);
    releaseIfHeld(VK_SHIFT);
    releaseIfHeld(VK_LSHIFT);
    releaseIfHeld(VK_RSHIFT);
    releaseIfHeld(VK_MENU);
    releaseIfHeld(VK_LMENU);
    releaseIfHeld(VK_RMENU);
    releaseIfHeld(VK_LWIN);
    releaseIfHeld(VK_RWIN);

    INPUT inputs[4] = {};
    inputs[0].type       = INPUT_KEYBOARD;
    inputs[0].ki.wVk     = VK_CONTROL;
    inputs[1].type       = INPUT_KEYBOARD;
    inputs[1].ki.wVk     = 'V';
    inputs[2].type       = INPUT_KEYBOARD;
    inputs[2].ki.wVk     = 'V';
    inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;
    inputs[3].type       = INPUT_KEYBOARD;
    inputs[3].ki.wVk     = VK_CONTROL;
    inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(4, inputs, sizeof(INPUT));
#endif
}
