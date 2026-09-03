#pragma once

#include "pasteitem.h"
#include <QDialog>

class QPlainTextEdit;
class QKeySequenceEdit;

class ItemEditorDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ItemEditorDialog(const PasteItem &item, QWidget *parent = nullptr);

    PasteItem item() const;

private:
    QPlainTextEdit   *m_textEdit;
    QKeySequenceEdit *m_hotkeyEdit;
};
