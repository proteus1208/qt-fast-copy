#pragma once

#include "pasteitem.h"
#include <QDialog>

class QLineEdit;
class QPlainTextEdit;
class QKeySequenceEdit;
class QLabel;

class ItemEditorDialog : public QDialog
{
    Q_OBJECT
public:
    // kind controls which fields are editable.
    // Text: name + content + hotkey
    // Path: name (read-only path shown) + hotkey
    // Folder: name only (informational)
    explicit ItemEditorDialog(const StoreEntry &entry, QWidget *parent = nullptr);

    QString name() const;
    QString text() const;
    QKeySequence hotkey() const;

private:
    StoreEntryKind    m_kind;
    QLineEdit        *m_nameEdit;
    QPlainTextEdit   *m_textEdit = nullptr;
    QKeySequenceEdit *m_hotkeyEdit = nullptr;
};
