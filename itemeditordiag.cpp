#include "itemeditordiag.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QKeySequenceEdit>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QGroupBox>
#include <QFormLayout>

ItemEditorDialog::ItemEditorDialog(const PasteItem &item, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Edit Paste Item");
    setMinimumWidth(460);
    setMinimumHeight(360);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(14, 14, 14, 14);

    // ── Title + Text section ──────────────────────────────────────────────
    auto *textGroup  = new QGroupBox("Item", this);
    auto *formLayout = new QFormLayout(textGroup);
    formLayout->setRowWrapPolicy(QFormLayout::DontWrapRows);
    formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    formLayout->setContentsMargins(10, 10, 10, 10);
    formLayout->setSpacing(8);

    // Title field
    m_titleEdit = new QLineEdit(textGroup);
    m_titleEdit->setPlaceholderText("Short name shown in picker menu (e.g. \"Greeting\")");
    m_titleEdit->setText(item.title);
    m_titleEdit->setMaxLength(80);
    formLayout->addRow("Title:", m_titleEdit);

    // Text field
    m_textEdit = new QPlainTextEdit(textGroup);
    m_textEdit->setPlaceholderText("Paste text — press Enter for new lines…");
    m_textEdit->setPlainText(item.text);
    m_textEdit->setMinimumHeight(140);
    m_textEdit->setTabChangesFocus(true);
    auto *textHint = new QLabel(
        "Tip: text can span multiple lines. The title is what appears in the picker menu.", textGroup);
    textHint->setStyleSheet("color: gray; font-size: 11px;");
    textHint->setWordWrap(true);
    formLayout->addRow("Text:", m_textEdit);
    formLayout->addRow("", textHint);

    mainLayout->addWidget(textGroup);

    // ── Hotkey section ────────────────────────────────────────────────────
    auto *hkGroup  = new QGroupBox("Per-Item Global Hotkey  (optional)", this);
    auto *hkLayout = new QHBoxLayout(hkGroup);
    hkLayout->setContentsMargins(10, 8, 10, 8);

    auto *hkHint = new QLabel("Shortcut:", hkGroup);
    m_hotkeyEdit = new QKeySequenceEdit(hkGroup);
    m_hotkeyEdit->setKeySequence(item.hotkey);
    m_hotkeyEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_hotkeyEdit->setToolTip("Click here and press a key combination to set a global hotkey for this item.");

    auto *clearBtn = new QPushButton("Clear", hkGroup);
    clearBtn->setFixedWidth(55);
    connect(clearBtn, &QPushButton::clicked, m_hotkeyEdit, &QKeySequenceEdit::clear);

    hkLayout->addWidget(hkHint);
    hkLayout->addWidget(m_hotkeyEdit, 1);
    hkLayout->addWidget(clearBtn);
    mainLayout->addWidget(hkGroup);

    // ── Buttons ───────────────────────────────────────────────────────────
    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttons);

    m_titleEdit->setFocus();
}

PasteItem ItemEditorDialog::item() const
{
    PasteItem p;
    p.title  = m_titleEdit->text().trimmed();
    p.text   = m_textEdit->toPlainText();
    p.hotkey = m_hotkeyEdit->keySequence();
    return p;
}
