#pragma once

#include "pasteitem.h"
#include <QObject>
#include <QSystemTrayIcon>
#include <QList>
#include <QKeySequence>

class SettingsWindow;
class HotkeyManager;
class QMenu;

class TrayIcon : public QObject
{
    Q_OBJECT
public:
    explicit TrayIcon(QObject *parent = nullptr);
    ~TrayIcon() override;

public slots:
    void showPickerMenu();
    void pasteItem(int index);

private slots:
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void onSettingsTriggered();
    void onAboutTriggered();
    void onDataChanged(const QList<PasteItem> &items, const QKeySequence &globalHotkey);

private:
    void loadAndApply();
    void sendCtrlV();

    QSystemTrayIcon *m_tray       = nullptr;
    QMenu           *m_trayMenu   = nullptr;
    SettingsWindow  *m_settingsWin = nullptr;
    HotkeyManager   *m_hotkeyMgr  = nullptr;

    QList<PasteItem> m_items;
    QKeySequence     m_globalHotkey;
};
