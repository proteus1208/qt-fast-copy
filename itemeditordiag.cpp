#include "itemeditordiag.h"
#include "appstyle.h"
#include "storagemanager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QKeySequenceEdit>
#include <QPushButton>
#include <QFileInfo>
#include <QDir>

ItemEditorDialog::ItemEditorDialog(const StoreEntry &entry, QWidget *parent)
    : QDialog(parent)
    , m_kind(entry.kind)
{
    const bool isNew = entry.absolutePath.isEmpty();
    setWindowTitle(isNew ? QStringLiteral("Add text item") : QStringLiteral("Edit item"));
    setMinimumSize(440, 380);
    setModal(true);
    setStyleSheet(appStyleSheet());

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(6);
    mainLayout->setContentsMargins(14, 12, 14, 12);

    auto *title = new QLabel(windowTitle(), this);
    title->setObjectName("dialogTitle");
    QString subtitleText;
    switch (m_kind) {
    case StoreEntryKind::Text:
        subtitleText = QStringLiteral("Name is the file name (no extension). Text is pasted.");
        break;
    case StoreEntryKind::Path:
        subtitleText = QStringLiteral("Activating this item pastes the absolute file path.");
        break;
    case StoreEntryKind::Folder:
        subtitleText = QStringLiteral("Folders open as nested menus in the picker.");
        break;
    }
    auto *subtitle = new QLabel(subtitleText, this);
    subtitle->setObjectName("dialogSubtitle");
    subtitle->setWordWrap(true);
    mainLayout->addWidget(title);
    mainLayout->addWidget(subtitle);

    auto *nameLabel = new QLabel(QStringLiteral("Name"), this);
    nameLabel->setObjectName("fieldLabel");
    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setText(entry.name);
    m_nameEdit->setMaxLength(120);
    forceBlackInputText(m_nameEdit);

    if (m_kind == StoreEntryKind::Text) {
        m_nameEdit->setPlaceholderText(QStringLiteral("file name without extension"));
    } else {
        m_nameEdit->setReadOnly(true);
        m_nameEdit->setToolTip(entry.absolutePath);
    }
    mainLayout->addWidget(nameLabel);
    mainLayout->addWidget(m_nameEdit);

    if (m_kind == StoreEntryKind::Path || m_kind == StoreEntryKind::Folder) {
        auto *pathLabel = new QLabel(QStringLiteral("Path"), this);
        pathLabel->setObjectName("fieldLabel");
        auto *pathValue = new QLabel(QDir::toNativeSeparators(entry.absolutePath), this);
        pathValue->setObjectName("fieldHint");
        pathValue->setWordWrap(true);
        pathValue->setTextInteractionFlags(Qt::TextSelectableByMouse);
        mainLayout->addWidget(pathLabel);
        mainLayout->addWidget(pathValue);
    }

    if (m_kind == StoreEntryKind::Text) {
        auto *textLabel = new QLabel(QStringLiteral("Text"), this);
        textLabel->setObjectName("fieldLabel");
        m_textEdit = new QPlainTextEdit(this);
        m_textEdit->setPlaceholderText(QStringLiteral("Paste content. Use Enter for new lines."));
        if (!entry.absolutePath.isEmpty())
            m_textEdit->setPlainText(StorageManager::readTextFile(entry.absolutePath));
        m_textEdit->setMinimumHeight(110);
        m_textEdit->setTabChangesFocus(true);
        forceBlackInputText(m_textEdit);
        auto *textHint = new QLabel(QStringLiteral("This is pasted into the focused field."), this);
        textHint->setObjectName("fieldHint");
        textHint->setWordWrap(true);
        mainLayout->addWidget(textLabel);
        mainLayout->addWidget(m_textEdit, 1);
        mainLayout->addWidget(textHint);
    } else {
        mainLayout->addStretch(1);
    }

    if (m_kind != StoreEntryKind::Folder) {
        auto *hkLabel = new QLabel(QStringLiteral("Global shortcut"), this);
        hkLabel->setObjectName("fieldLabel");
        auto *hkHint = new QLabel(QStringLiteral("Optional. Works in any app without opening the picker."), this);
        hkHint->setObjectName("fieldHint");
        hkHint->setWordWrap(true);

        auto *hkRow = new QHBoxLayout;
        hkRow->setSpacing(10);
        m_hotkeyEdit = new QKeySequenceEdit(this);
        m_hotkeyEdit->setKeySequence(entry.hotkey);
        m_hotkeyEdit->setMaximumSequenceLength(1);
        forceBlackInputText(m_hotkeyEdit);
        if (auto *inner = m_hotkeyEdit->findChild<QLineEdit *>())
            forceBlackInputText(inner);
        auto *clearBtn = new QPushButton(QStringLiteral("Clear"), this);
        clearBtn->setObjectName("btnClearGlobal");
        connect(clearBtn, &QPushButton::clicked, m_hotkeyEdit, &QKeySequenceEdit::clear);
        hkRow->addWidget(m_hotkeyEdit, 1);
        hkRow->addWidget(clearBtn);

        mainLayout->addWidget(hkLabel);
        mainLayout->addWidget(hkHint);
        mainLayout->addLayout(hkRow);
    }

    auto *actions = new QHBoxLayout;
    actions->addStretch();
    auto *cancelBtn = new QPushButton(QStringLiteral("Cancel"), this);
    cancelBtn->setObjectName("btnCancel");
    auto *saveBtn = new QPushButton(isNew ? QStringLiteral("Add") : QStringLiteral("Save"), this);
    saveBtn->setObjectName("btnSave");
    saveBtn->setDefault(true);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(saveBtn, &QPushButton::clicked, this, &QDialog::accept);
    actions->addWidget(cancelBtn);
    actions->addWidget(saveBtn);
    mainLayout->addLayout(actions);

    m_nameEdit->setFocus();
}

QString ItemEditorDialog::name() const
{
    return m_nameEdit->text().trimmed();
}

QString ItemEditorDialog::text() const
{
    return m_textEdit ? m_textEdit->toPlainText() : QString();
}

QKeySequence ItemEditorDialog::hotkey() const
{
    return m_hotkeyEdit ? m_hotkeyEdit->keySequence() : QKeySequence();
}
