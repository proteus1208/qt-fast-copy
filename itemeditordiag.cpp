#include "itemeditordiag.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QKeySequenceEdit>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QGroupBox>

ItemEditorDialog::ItemEditorDialog(const PasteItem &item, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Edit Paste Item");
    setMinimumWidth(420);

    auto *mainLayout = new QVBoxLayout(this);

    // Text section
    auto *textGroup = new QGroupBox("Text (supports multiple lines)", this);
    auto *textLayout = new QVBoxLayout(textGroup);
    m_textEdit = new QPlainTextEdit(textGroup);
    m_textEdit->setPlaceholderText("Enter the text to paste…");
    m_textEdit->setPlainText(item.text);
    m_textEdit->setMinimumHeight(120);
    textLayout->addWidget(m_textEdit);
    mainLayout->addWidget(textGroup);

    // Hotkey section
    auto *hkGroup  = new QGroupBox("Item Hotkey (optional — works globally)", this);
    auto *hkLayout = new QHBoxLayout(hkGroup);
    auto *hkLabel  = new QLabel("Hotkey:", hkGroup);
    m_hotkeyEdit   = new QKeySequenceEdit(hkGroup);
    m_hotkeyEdit->setKeySequence(item.hotkey);
    auto *clearBtn = new QPushButton("Clear", hkGroup);
    connect(clearBtn, &QPushButton::clicked, m_hotkeyEdit, &QKeySequenceEdit::clear);
    hkLayout->addWidget(hkLabel);
    hkLayout->addWidget(m_hotkeyEdit, 1);
    hkLayout->addWidget(clearBtn);
    mainLayout->addWidget(hkGroup);

    // OK / Cancel
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttons);
}

PasteItem ItemEditorDialog::item() const
{
    PasteItem p;
    p.text   = m_textEdit->toPlainText();
    p.hotkey = m_hotkeyEdit->keySequence();
    return p;
}
