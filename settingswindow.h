#pragma once

#include "pasteitem.h"
#include "hotkeymanager.h"
#include <QMainWindow>
#include <QList>
#include <QKeySequence>
#include <QVBoxLayout>
#include <QScrollArea>

QT_BEGIN_NAMESPACE
namespace Ui { class SettingsWindow; }
QT_END_NAMESPACE

class SettingsWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit SettingsWindow(QWidget *parent = nullptr);
    ~SettingsWindow() override;

    void setAppSettings(const AppSettings &settings);
    void setRootEntries(const QList<StoreEntry> &entries);
    void setHotkeyManager(HotkeyManager *mgr);

signals:
    void appSettingsChanged(const AppSettings &settings);
    void storeChanged();
    void pasteTextOnlyRequested();

private slots:
    void onAddItem();
    void onEditItem(int index);
    void onRemoveItem(int index);
    void onMoveUp(int index);
    void onMoveDown(int index);
    void onBrowseStore();
    void onOpenStore();
    void onStorePathEdited();
    void onGlobalHotkeyChanged();
    void onPasteTextHotkeyChanged();
    void rebuildList();

private:
    void emitAppSettings();
    void persistOrder();

    Ui::SettingsWindow *ui;
    AppSettings         m_app;
    QList<StoreEntry>   m_entries;
    QWidget            *m_listContainer = nullptr;
    QVBoxLayout        *m_listLayout    = nullptr;
    HotkeyManager      *m_hotkeyMgr     = nullptr;
};
