#include "trayicon.h"
#include "settingswindow.h"
#include "hotkeymanager.h"
#include "storagemanager.h"

#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QCursor>
#include <QTimer>
#include <QStyle>
#include <QIcon>
#include <QFileInfo>
#include <QDir>
#include <QDesktopServices>
#include <QUrl>
#include <QFileSystemWatcher>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

TrayIcon::TrayIcon(QObject *parent)
    : QObject(parent)
{
    m_tray = new QSystemTrayIcon(this);
    QIcon appIcon(":/assets/icon.ico");
    if (appIcon.isNull())
        appIcon = QApplication::style()->standardIcon(QStyle::SP_ComputerIcon);
    m_tray->setIcon(appIcon);
    qApp->setWindowIcon(appIcon);
    m_tray->setToolTip("Fast Paste\nRight-click for Settings, About, Quit");

    m_trayMenu = new QMenu();
    QAction *actSettings = m_trayMenu->addAction("Settings");
    QAction *actOpenStore = m_trayMenu->addAction("Open store folder");
    QAction *actAbout    = m_trayMenu->addAction("About");
    m_trayMenu->addSeparator();
    QAction *actQuit = m_trayMenu->addAction("Quit");

    connect(actSettings, &QAction::triggered, this, &TrayIcon::onSettingsTriggered);
    connect(actOpenStore, &QAction::triggered, this, &TrayIcon::onOpenStoreFolder);
    connect(actAbout,    &QAction::triggered, this, &TrayIcon::onAboutTriggered);
    connect(actQuit,     &QAction::triggered, qApp, &QApplication::quit);

    m_tray->setContextMenu(m_trayMenu);
    m_tray->show();

    m_hotkeyMgr = new HotkeyManager(this);
    connect(m_hotkeyMgr, &HotkeyManager::pickerRequested,
            this, &TrayIcon::showPickerMenu);
    connect(m_hotkeyMgr, &HotkeyManager::pasteTextOnlyRequested,
            this, &TrayIcon::pasteTextOnly);
    connect(m_hotkeyMgr, &HotkeyManager::itemTriggered,
            this, &TrayIcon::pasteStorePath);

    m_watcher = new QFileSystemWatcher(this);
    connect(m_watcher, &QFileSystemWatcher::directoryChanged,
            this, &TrayIcon::onStoreChanged);
    connect(m_watcher, &QFileSystemWatcher::fileChanged,
            this, &TrayIcon::onStoreChanged);

    m_settingsWin = new SettingsWindow();
    m_settingsWin->setHotkeyManager(m_hotkeyMgr);
    connect(m_settingsWin, &SettingsWindow::appSettingsChanged,
            this, &TrayIcon::onAppSettingsChanged);
    connect(m_settingsWin, &SettingsWindow::storeChanged,
            this, &TrayIcon::onStoreChanged);
    connect(m_settingsWin, &SettingsWindow::pasteTextOnlyRequested,
            this, &TrayIcon::pasteTextOnly);

    loadAndApply();
}

TrayIcon::~TrayIcon()
{
    delete m_settingsWin;
    delete m_trayMenu;
}

void TrayIcon::loadAndApply()
{
    m_app = StorageManager::loadAppSettings();
    StorageManager::ensureStoreFolder(m_app.storePath);
    StorageManager::migrateLegacyItemsIfNeeded(m_app.storePath);

    reloadStore();

    m_settingsWin->setAppSettings(m_app);
    m_settingsWin->setRootEntries(m_rootEntries);

    m_hotkeyMgr->setGlobalHotkey(m_app.globalHotkey);
    m_hotkeyMgr->setPasteTextOnlyHotkey(m_app.pasteTextOnlyHotkey);
}

void TrayIcon::clearStoreWatches()
{
    const QStringList watchedDirs = m_watcher->directories();
    if (!watchedDirs.isEmpty())
        m_watcher->removePaths(watchedDirs);
    const QStringList watchedFiles = m_watcher->files();
    if (!watchedFiles.isEmpty())
        m_watcher->removePaths(watchedFiles);
}

void TrayIcon::watchTree(const QString &dirPath, const QList<StoreEntry> &tree)
{
    if (QDir(dirPath).exists())
        m_watcher->addPath(dirPath);
    const QString metaPath = QDir(dirPath).filePath(QLatin1String(StorageManager::kMetaFileName));
    if (QFileInfo::exists(metaPath))
        m_watcher->addPath(metaPath);

    for (const StoreEntry &e : tree) {
        if (e.kind == StoreEntryKind::Folder)
            watchTree(e.absolutePath, e.children);
    }
}

void TrayIcon::reloadStore()
{
    clearStoreWatches();

    // One tree walk: settings list, FS watches, and hotkey registration.
    const QList<StoreEntry> tree = StorageManager::loadTree(m_app.storePath);
    m_rootEntries.clear();
    m_rootEntries.reserve(tree.size());
    for (const StoreEntry &e : tree) {
        StoreEntry root = e;
        root.children.clear();
        m_rootEntries.append(root);
    }

    watchTree(m_app.storePath, tree);
    m_hotkeyMgr->setItemHotkeys(StorageManager::collectHotkeys(tree));

    if (m_settingsWin)
        m_settingsWin->setRootEntries(m_rootEntries);
}

void TrayIcon::onStoreChanged()
{
    // Debounce bursty FS + settings saves (meta.json + file writes).
    if (m_reloadPending)
        return;
    m_reloadPending = true;
    QTimer::singleShot(200, this, [this] {
        m_reloadPending = false;
        reloadStore();
    });
}

void TrayIcon::onSettingsTriggered()
{
    m_settingsWin->show();
    m_settingsWin->raise();
    m_settingsWin->activateWindow();
}

void TrayIcon::onOpenStoreFolder()
{
    StorageManager::ensureStoreFolder(m_app.storePath);
    QDesktopServices::openUrl(QUrl::fromLocalFile(m_app.storePath));
}

void TrayIcon::onAboutTriggered()
{
    QMessageBox::about(nullptr, "About Fast Paste",
        "<b>Fast Paste</b><br><br>"
        "Paste items from a store folder that mirrors File Explorer.<br><br>"
        "• Extensionless files paste their text content<br>"
        "• Normal files paste their absolute path<br>"
        "• Folders open as nested menus<br>"
        "• <b>Paste text only</b> strips images/HTML from the clipboard<br><br>"
        "Version 1.1");
}

void TrayIcon::onAppSettingsChanged(const AppSettings &settings)
{
    const bool pathChanged = settings.storePath != m_app.storePath;
    m_app = settings;
    StorageManager::saveAppSettings(m_app);
    StorageManager::ensureStoreFolder(m_app.storePath);

    m_hotkeyMgr->setGlobalHotkey(m_app.globalHotkey);
    m_hotkeyMgr->setPasteTextOnlyHotkey(m_app.pasteTextOnlyHotkey);

    if (pathChanged)
        reloadStore();
}

// ─── Picker menu ─────────────────────────────────────────────────────────────

void TrayIcon::populateMenu(QMenu *menu, const QString &dirPath)
{
    const QList<StoreEntry> entries = StorageManager::loadDirectory(dirPath);

    if (entries.isEmpty()) {
        QAction *empty = menu->addAction(QStringLiteral("(empty)"));
        empty->setEnabled(false);
        return;
    }

    for (const StoreEntry &entry : entries) {
        if (entry.kind == StoreEntryKind::Folder) {
            QMenu *sub = menu->addMenu(entry.icon, entry.name);
            // Lazy-load children when the submenu opens (performance).
            const QString childPath = entry.absolutePath;
            connect(sub, &QMenu::aboutToShow, this, [this, sub, childPath] {
                if (!sub->actions().isEmpty())
                    return;
                populateMenu(sub, childPath);
            });
            continue;
        }

        QAction *act = menu->addAction(entry.icon, entry.name);
        if (!entry.hotkey.isEmpty()) {
            act->setShortcut(entry.hotkey);
            act->setShortcutVisibleInContextMenu(true);
        }
        const QString path = entry.absolutePath;
        connect(act, &QAction::triggered, this, [this, path] { pasteStorePath(path); });
    }
}

void TrayIcon::showPickerMenu()
{
    QMenu menu;
    menu.setTitle(QStringLiteral("Fast Paste"));

    // First line: paste text only (strips OneNote images / rich clipboard data).
    QAction *pasteText = menu.addAction(
        QApplication::style()->standardIcon(QStyle::SP_FileDialogContentsView),
        QStringLiteral("Paste text only"));
    if (!m_app.pasteTextOnlyHotkey.isEmpty()) {
        pasteText->setShortcut(m_app.pasteTextOnlyHotkey);
        pasteText->setShortcutVisibleInContextMenu(true);
    }
    connect(pasteText, &QAction::triggered, this, &TrayIcon::pasteTextOnly);
    menu.addSeparator();

    // Live directory read keeps the picker fresh; folders lazy-load on open.
    populateMenu(&menu, m_app.storePath);

    menu.exec(QCursor::pos());
}

void TrayIcon::clipboardPaste(const QString &text)
{
    QApplication::clipboard()->setText(text);
    QTimer::singleShot(250, this, [this] { sendCtrlV(); });
}

void TrayIcon::pasteStorePath(const QString &absolutePath)
{
    const QFileInfo info(absolutePath);
    if (!info.exists()) {
        m_tray->showMessage(QStringLiteral("Fast Paste"),
                            QStringLiteral("Item no longer exists:\n%1").arg(absolutePath),
                            QSystemTrayIcon::Warning, 3000);
        reloadStore();
        return;
    }

    if (info.isDir()) {
        // Folder activation via hotkey: paste the folder path.
        clipboardPaste(QDir::toNativeSeparators(info.absoluteFilePath()));
        return;
    }

    if (info.suffix().isEmpty()) {
        clipboardPaste(StorageManager::readTextFile(absolutePath));
        return;
    }

    clipboardPaste(QDir::toNativeSeparators(info.absoluteFilePath()));
}

void TrayIcon::pasteTextOnly()
{
    const QClipboard *cb = QApplication::clipboard();
    const QMimeData *mime = cb->mimeData();
    const QString text = mime ? mime->text() : QString();
    // Replace complex clipboard (images / HTML) with plain text only, then paste.
    clipboardPaste(text);
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
