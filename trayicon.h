#pragma once

#include "pasteitem.h"
#include <QObject>
#include <QSystemTrayIcon>
#include <QList>
#include <QKeySequence>
#include <QString>

class SettingsWindow;
class HotkeyManager;
class QMenu;
class QFileSystemWatcher;

class TrayIcon : public QObject
{
    Q_OBJECT
public:
    explicit TrayIcon(QObject *parent = nullptr);
    ~TrayIcon() override;

public slots:
    void showPickerMenu();
    void pasteStorePath(const QString &absolutePath);
    void pasteTextOnly();

private slots:
    void onSettingsTriggered();
    void onAboutTriggered();
    void onOpenStoreFolder();
    void onAppSettingsChanged(const AppSettings &settings);
    void onStoreChanged();
    void reloadStore();

private:
    void loadAndApply();
    void sendCtrlV();
    void populateMenu(QMenu *menu, const QString &dirPath);
    void clipboardPaste(const QString &text);
    void clearStoreWatches();
    void watchTree(const QString &dirPath, const QList<StoreEntry> &tree);

    QSystemTrayIcon *m_tray        = nullptr;
    QMenu           *m_trayMenu    = nullptr;
    SettingsWindow  *m_settingsWin = nullptr;
    HotkeyManager   *m_hotkeyMgr   = nullptr;
    QFileSystemWatcher *m_watcher  = nullptr;

    AppSettings       m_app;
    QList<StoreEntry> m_rootEntries; // root level (folders lazy-load children)
    bool              m_reloadPending = false;
};
