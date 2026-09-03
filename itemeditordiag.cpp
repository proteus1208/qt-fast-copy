#include "itemeditordiag.h"
#include "appstyle.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QKeySequenceEdit>
#include <QPushButton>

ItemEditorDialog::ItemEditorDialog(const PasteItem &item, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Edit item");
    setMinimumSize(480, 420);
    setModal(true);
    setStyleSheet(appStyleSheet());

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(28, 24, 28, 24);

    auto *title = new QLabel("Edit item", this);
    title->setObjectName("dialogTitle");
    auto *subtitle = new QLabel("Title appears in the picker. Text is what gets pasted.", this);
    subtitle->setObjectName("dialogSubtitle");
    subtitle->setWordWrap(true);
    mainLayout->addWidget(title);
    mainLayout->addWidget(subtitle);

    auto *titleLabel = new QLabel("Title", this);
    titleLabel->setObjectName("fieldLabel");
    m_titleEdit = new QLineEdit(this);
    m_titleEdit->setPlaceholderText("Shown in the picker menu");
    m_titleEdit->setText(item.title);
    m_titleEdit->setMaxLength(80);
    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(m_titleEdit);

    auto *textLabel = new QLabel("Text", this);
    textLabel->setObjectName("fieldLabel");
    m_textEdit = new QPlainTextEdit(this);
    m_textEdit->setPlaceholderText("Paste content. Use Enter for new lines.");
    m_textEdit->setPlainText(item.text);
    m_textEdit->setMinimumHeight(140);
    m_textEdit->setTabChangesFocus(true);
    auto *textHint = new QLabel("This is pasted into the focused field.", this);
    textHint->setObjectName("fieldHint");
    textHint->setWordWrap(true);
    mainLayout->addWidget(textLabel);
    mainLayout->addWidget(m_textEdit, 1);
    mainLayout->addWidget(textHint);

    auto *hkLabel = new QLabel("Global shortcut", this);
    hkLabel->setObjectName("fieldLabel");
    auto *hkHint = new QLabel("Optional. Works in any app without opening this window.", this);
    hkHint->setObjectName("fieldHint");
    hkHint->setWordWrap(true);

    auto *hkRow = new QHBoxLayout;
    hkRow->setSpacing(10);
    m_hotkeyEdit = new QKeySequenceEdit(this);
    m_hotkeyEdit->setKeySequence(item.hotkey);
    m_hotkeyEdit->setMaximumSequenceLength(1);
    auto *clearBtn = new QPushButton("Clear", this);
    clearBtn->setObjectName("btnClearGlobal");
    connect(clearBtn, &QPushButton::clicked, m_hotkeyEdit, &QKeySequenceEdit::clear);
    hkRow->addWidget(m_hotkeyEdit, 1);
    hkRow->addWidget(clearBtn);

    mainLayout->addWidget(hkLabel);
    mainLayout->addWidget(hkHint);
    mainLayout->addLayout(hkRow);

    auto *actions = new QHBoxLayout;
    actions->addStretch();
    auto *cancelBtn = new QPushButton("Cancel", this);
    cancelBtn->setObjectName("btnCancel");
    auto *saveBtn = new QPushButton("Save", this);
    saveBtn->setObjectName("btnSave");
    saveBtn->setDefault(true);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(saveBtn, &QPushButton::clicked, this, &QDialog::accept);
    actions->addWidget(cancelBtn);
    actions->addWidget(saveBtn);
    mainLayout->addLayout(actions);

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
