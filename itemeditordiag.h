#pragma once

#include "pasteitem.h"
#include <QDialog>

class QLineEdit;
class QPlainTextEdit;
class QKeySequenceEdit;

class ItemEditorDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ItemEditorDialog(const PasteItem &item, QWidget *parent = nullptr);

    PasteItem item() const;

private:
    QLineEdit        *m_titleEdit;
    QPlainTextEdit   *m_textEdit;
    QKeySequenceEdit *m_hotkeyEdit;
};
