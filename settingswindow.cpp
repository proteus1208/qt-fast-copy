#include "settingswindow.h"
#include "ui_settingswindow.h"
#include "itemeditordiag.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QFrame>

SettingsWindow::SettingsWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::SettingsWindow)
{
    ui->setupUi(this);

    // The scroll area's inner widget and layout are the item list
    m_listContainer = ui->scrollAreaWidgetContents;
    m_listLayout    = qobject_cast<QVBoxLayout *>(m_listContainer->layout());
    m_listLayout->setAlignment(Qt::AlignTop);

    connect(ui->btnAddItem, &QPushButton::clicked, this, &SettingsWindow::onAddItem);
    connect(ui->keySeqEdit, &QKeySequenceEdit::keySequenceChanged,
            this, &SettingsWindow::onGlobalHotkeyChanged);
}

SettingsWindow::~SettingsWindow()
{
    delete ui;
}

void SettingsWindow::setData(const QList<PasteItem> &items, const QKeySequence &globalHotkey)
{
    m_items       = items;
    m_globalHotkey = globalHotkey;

    // Block signal to avoid emitting dataChanged during load
    ui->keySeqEdit->blockSignals(true);
    ui->keySeqEdit->setKeySequence(globalHotkey);
    ui->keySeqEdit->blockSignals(false);

    rebuildList();
}

void SettingsWindow::rebuildList()
{
    // Remove all existing rows
    QLayoutItem *item;
    while ((item = m_listLayout->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    for (int i = 0; i < m_items.size(); ++i) {
        // Row frame
        QFrame *row = new QFrame(m_listContainer);
        row->setFrameShape(QFrame::StyledPanel);
        QHBoxLayout *hl = new QHBoxLayout(row);
        hl->setContentsMargins(4, 2, 4, 2);

        // Index label
        QLabel *numLabel = new QLabel(QString("%1.").arg(i + 1), row);
        numLabel->setFixedWidth(28);
        hl->addWidget(numLabel);

        // Text preview (single line, elided)
        QString preview = m_items[i].text;
        preview.replace('\n', ' ');
        QLabel *textLabel = new QLabel(preview, row);
        textLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        textLabel->setTextFormat(Qt::PlainText);
        // Elide is handled by Qt when window resizes; set minimum width
        textLabel->setMinimumWidth(0);
        hl->addWidget(textLabel, 1);

        // Hotkey badge
        if (!m_items[i].hotkey.isEmpty()) {
            QLabel *hkLabel = new QLabel(QString("[%1]").arg(m_items[i].hotkey.toString()), row);
            hkLabel->setStyleSheet("color: gray; font-size: 10px;");
            hl->addWidget(hkLabel);
        }

        // Buttons
        auto makeBtn = [&](const QString &text) {
            QPushButton *b = new QPushButton(text, row);
            b->setFixedWidth(28);
            return b;
        };

        QPushButton *btnUp   = makeBtn("▲");
        QPushButton *btnDown = makeBtn("▼");
        QPushButton *btnEdit = new QPushButton("Edit", row);
        QPushButton *btnDel  = makeBtn("✕");

        hl->addWidget(btnUp);
        hl->addWidget(btnDown);
        hl->addWidget(btnEdit);
        hl->addWidget(btnDel);

        int idx = i; // capture for lambdas
        connect(btnUp,   &QPushButton::clicked, this, [this, idx]{ onMoveUp(idx); });
        connect(btnDown, &QPushButton::clicked, this, [this, idx]{ onMoveDown(idx); });
        connect(btnEdit, &QPushButton::clicked, this, [this, idx]{ onEditItem(idx); });
        connect(btnDel,  &QPushButton::clicked, this, [this, idx]{ onRemoveItem(idx); });

        m_listLayout->addWidget(row);
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
