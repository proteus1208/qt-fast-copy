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

    void setData(const QList<PasteItem> &items, const QKeySequence &globalHotkey);
    void setHotkeyManager(HotkeyManager *mgr);

signals:
    void dataChanged(const QList<PasteItem> &items, const QKeySequence &globalHotkey);

private slots:
    void onAddItem();
    void onEditItem(int index);
    void onRemoveItem(int index);
    void onMoveUp(int index);
    void onMoveDown(int index);
    void onGlobalHotkeyChanged();
    void rebuildList(); // also called when hotkey registration changes

private:
    Ui::SettingsWindow *ui;
    QList<PasteItem>   m_items;
    QKeySequence       m_globalHotkey;
    QWidget           *m_listContainer = nullptr;
    QVBoxLayout       *m_listLayout    = nullptr;
    HotkeyManager     *m_hotkeyMgr    = nullptr;
};
