#include "settingswindow.h"
#include "ui_settingswindow.h"
#include "itemeditordiag.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QFontMetrics>

SettingsWindow::SettingsWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::SettingsWindow)
{
    ui->setupUi(this);

    m_listContainer = ui->scrollAreaWidgetContents;
    m_listLayout    = qobject_cast<QVBoxLayout *>(m_listContainer->layout());
    m_listLayout->setAlignment(Qt::AlignTop);

    connect(ui->btnAddItem,    &QPushButton::clicked,
            this, &SettingsWindow::onAddItem);
    connect(ui->btnClearGlobal, &QPushButton::clicked,
            ui->keySeqEdit, &QKeySequenceEdit::clear);
    connect(ui->keySeqEdit, &QKeySequenceEdit::keySequenceChanged,
            this, &SettingsWindow::onGlobalHotkeyChanged);
}

SettingsWindow::~SettingsWindow()
{
    delete ui;
}

void SettingsWindow::setHotkeyManager(HotkeyManager *mgr)
{
    if (m_hotkeyMgr)
        disconnect(m_hotkeyMgr, &HotkeyManager::registrationChanged,
                   this, &SettingsWindow::rebuildList);
    m_hotkeyMgr = mgr;
    if (m_hotkeyMgr)
        connect(m_hotkeyMgr, &HotkeyManager::registrationChanged,
                this, &SettingsWindow::rebuildList);
}

void SettingsWindow::setData(const QList<PasteItem> &items, const QKeySequence &globalHotkey)
{
    m_items        = items;
    m_globalHotkey = globalHotkey;

    ui->keySeqEdit->blockSignals(true);
    ui->keySeqEdit->setKeySequence(globalHotkey);
    ui->keySeqEdit->blockSignals(false);

    rebuildList();
}

// ── Helper: small colored status dot ──────────────────────────────────────────
static QLabel *makeStatusDot(QWidget *parent, bool ok, const QString &tip)
{
    QLabel *dot = new QLabel(ok ? "●" : "●", parent);
    dot->setFixedSize(16, 16);
    dot->setAlignment(Qt::AlignCenter);
    dot->setToolTip(tip);
    if (ok) {
        dot->setStyleSheet("color: #27ae60; font-size: 14px;"); // green
    } else {
        dot->setStyleSheet("color: #e74c3c; font-size: 14px;"); // red
    }
    return dot;
}

void SettingsWindow::rebuildList()
{
    // Remove all existing rows
    QLayoutItem *child;
    while ((child = m_listLayout->takeAt(0)) != nullptr) {
        if (child->widget()) child->widget()->deleteLater();
        delete child;
    }

    // Get current registration state
    bool      pickerOk   = m_hotkeyMgr ? m_hotkeyMgr->isPickerRegistered() : false;
    QSet<int> itemOkSet  = m_hotkeyMgr ? m_hotkeyMgr->registeredItemIndices() : QSet<int>{};

    // Update global hotkey status label
    if (!m_globalHotkey.isEmpty()) {
        ui->labelGlobalStatus->setVisible(true);
        if (pickerOk) {
            ui->labelGlobalStatus->setText("● Active");
            ui->labelGlobalStatus->setStyleSheet("color: #27ae60; font-weight: bold;");
            ui->labelGlobalStatus->setToolTip("Global picker hotkey registered successfully.");
        } else {
            ui->labelGlobalStatus->setText("● Failed");
            ui->labelGlobalStatus->setStyleSheet("color: #e74c3c; font-weight: bold;");
            ui->labelGlobalStatus->setToolTip(
                "Failed to register the hotkey — it may already be in use by another app.");
        }
    } else {
        ui->labelGlobalStatus->setVisible(false);
    }

    // Build item rows
    for (int i = 0; i < m_items.size(); ++i) {
        const PasteItem &item = m_items[i];

        QFrame *row = new QFrame(m_listContainer);
        row->setFrameShape(QFrame::StyledPanel);
        row->setStyleSheet("QFrame { background: palette(base); border-radius: 4px; }");

        QHBoxLayout *hl = new QHBoxLayout(row);
        hl->setContentsMargins(8, 5, 8, 5);
        hl->setSpacing(6);

        // ── Index badge ───────────────────────────────────────────────────
        QLabel *numLabel = new QLabel(QString::number(i + 1), row);
        numLabel->setFixedSize(22, 22);
        numLabel->setAlignment(Qt::AlignCenter);
        numLabel->setStyleSheet(
            "background: #0078d4; color: white; border-radius: 11px;"
            "font-weight: bold; font-size: 11px;");
        hl->addWidget(numLabel);

        // ── Title + text preview (stacked vertically) ─────────────────────
        QWidget *textBlock = new QWidget(row);
        QVBoxLayout *vl = new QVBoxLayout(textBlock);
        vl->setContentsMargins(0, 0, 0, 0);
        vl->setSpacing(1);

        // Title label (bold)
        QString titleText = item.title.trimmed().isEmpty()
                            ? QString("(item %1)").arg(i + 1)
                            : item.title;
        QLabel *titleLabel = new QLabel(titleText, textBlock);
        titleLabel->setStyleSheet("font-weight: bold; font-size: 12px;");
        titleLabel->setTextFormat(Qt::PlainText);
        vl->addWidget(titleLabel);

        // Text preview (gray, single line)
        QString preview = item.text;
        preview.replace('\n', ' ');
        QFontMetrics fm(titleLabel->font());
        QString elided = fm.elidedText(preview, Qt::ElideRight, 300);
        if (!elided.isEmpty()) {
            QLabel *previewLabel = new QLabel(elided, textBlock);
            previewLabel->setStyleSheet("color: gray; font-size: 11px;");
            previewLabel->setTextFormat(Qt::PlainText);
            vl->addWidget(previewLabel);
        }

        hl->addWidget(textBlock, 1);

        // ── Hotkey badge + status dot ─────────────────────────────────────
        if (!item.hotkey.isEmpty()) {
            bool hkOk = itemOkSet.contains(i);

            // Dot indicator
            QString tip = hkOk
                ? QString("Hotkey %1 registered OK").arg(item.hotkey.toString())
                : QString("Hotkey %1 FAILED — already in use or unsupported")
                          .arg(item.hotkey.toString());
            QLabel *dot = makeStatusDot(row, hkOk, tip);
            hl->addWidget(dot);

            // Hotkey text badge
            QLabel *hkLabel = new QLabel(
                QString("  %1  ").arg(item.hotkey.toString()), row);
            hkLabel->setStyleSheet(
                hkOk
                ? "color: #1a7a1a; background: #d4edda; border: 1px solid #b0d9b8;"
                  "border-radius: 3px; font-size: 10px; padding: 1px 4px;"
                : "color: #7a1a1a; background: #fde8e8; border: 1px solid #e0b0b0;"
                  "border-radius: 3px; font-size: 10px; padding: 1px 4px;");
            hkLabel->setToolTip(tip);
            hl->addWidget(hkLabel);
        }

        // ── Separator ─────────────────────────────────────────────────────
        QFrame *sep = new QFrame(row);
        sep->setFrameShape(QFrame::VLine);
        sep->setStyleSheet("color: #ccc;");
        hl->addWidget(sep);

        // ── Action buttons ────────────────────────────────────────────────
        auto makeIconBtn = [&](const QString &text, const QString &tip) {
            QPushButton *b = new QPushButton(text, row);
            b->setFixedSize(26, 26);
            b->setToolTip(tip);
            b->setStyleSheet(
                "QPushButton { border: none; border-radius: 4px; }"
                "QPushButton:hover { background: #e0e0e0; }");
            return b;
        };

        QPushButton *btnUp   = makeIconBtn("▲", "Move up");
        QPushButton *btnDown = makeIconBtn("▼", "Move down");
        QPushButton *btnEdit = new QPushButton("Edit", row);
        btnEdit->setFixedHeight(26);
        btnEdit->setToolTip("Edit title, text and hotkey");
        QPushButton *btnDel  = makeIconBtn("✕", "Remove item");
        btnDel->setStyleSheet(
            "QPushButton { border: none; border-radius: 4px; color: #c0392b; }"
            "QPushButton:hover { background: #fde8e8; }");

        hl->addWidget(btnUp);
        hl->addWidget(btnDown);
        hl->addWidget(btnEdit);
        hl->addWidget(btnDel);

        int idx = i;
        connect(btnUp,   &QPushButton::clicked, this, [this, idx]{ onMoveUp(idx); });
        connect(btnDown, &QPushButton::clicked, this, [this, idx]{ onMoveDown(idx); });
        connect(btnEdit, &QPushButton::clicked, this, [this, idx]{ onEditItem(idx); });
        connect(btnDel,  &QPushButton::clicked, this, [this, idx]{ onRemoveItem(idx); });

        m_listLayout->addWidget(row);
    }

    if (m_items.isEmpty()) {
        QLabel *empty = new QLabel(
            "No items yet. Click \"＋ Add Item\" to get started.", m_listContainer);
        empty->setAlignment(Qt::AlignCenter);
        empty->setStyleSheet("color: gray; padding: 20px;");
        m_listLayout->addWidget(empty);
    }
}

void SettingsWindow::onAddItem()
{
    PasteItem item;
    ItemEditorDialog dlg(item, this);
    if (dlg.exec() == QDialog::Accepted) {
        m_items.append(dlg.item());
        rebuildList();
        emit dataChanged(m_items, m_globalHotkey);
    }
}

void SettingsWindow::onEditItem(int index)
{
    if (index < 0 || index >= m_items.size()) return;
    ItemEditorDialog dlg(m_items[index], this);
    if (dlg.exec() == QDialog::Accepted) {
        m_items[index] = dlg.item();
        rebuildList();
        emit dataChanged(m_items, m_globalHotkey);
    }
}

void SettingsWindow::onRemoveItem(int index)
{
    if (index < 0 || index >= m_items.size()) return;
    m_items.removeAt(index);
    rebuildList();
    emit dataChanged(m_items, m_globalHotkey);
}

void SettingsWindow::onMoveUp(int index)
{
    if (index <= 0 || index >= m_items.size()) return;
    m_items.swapItemsAt(index - 1, index);
    rebuildList();
    emit dataChanged(m_items, m_globalHotkey);
}

void SettingsWindow::onMoveDown(int index)
{
    if (index < 0 || index >= m_items.size() - 1) return;
    m_items.swapItemsAt(index, index + 1);
    rebuildList();
    emit dataChanged(m_items, m_globalHotkey);
}

void SettingsWindow::onGlobalHotkeyChanged()
{
    m_globalHotkey = ui->keySeqEdit->keySequence();
    emit dataChanged(m_items, m_globalHotkey);
}
