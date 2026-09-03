#include "settingswindow.h"
#include "ui_settingswindow.h"
#include "itemeditordiag.h"
#include "appstyle.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFontMetrics>
#include <QStyle>

SettingsWindow::SettingsWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::SettingsWindow)
{
    ui->setupUi(this);
    setStyleSheet(appStyleSheet());

    ui->labelPageTitle->setObjectName("labelPageTitle");
    ui->labelPageSubtitle->setObjectName("labelPageSubtitle");
    ui->labelGlobalTitle->setObjectName("labelGlobalTitle");
    ui->labelGlobalHint->setObjectName("labelGlobalHint");
    ui->labelItems->setObjectName("labelItems");
    ui->labelHint->setObjectName("labelHint");
    ui->btnAddItem->setObjectName("btnAddItem");
    ui->btnClearGlobal->setObjectName("btnClearGlobal");
    ui->scrollArea->setWidgetResizable(true);

    m_listContainer = ui->scrollAreaWidgetContents;
    m_listLayout    = qobject_cast<QVBoxLayout *>(m_listContainer->layout());
    m_listLayout->setAlignment(Qt::AlignTop);

    connect(ui->btnAddItem, &QPushButton::clicked, this, &SettingsWindow::onAddItem);
    connect(ui->btnClearGlobal, &QPushButton::clicked, ui->keySeqEdit, &QKeySequenceEdit::clear);
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

void SettingsWindow::rebuildList()
{
    QLayoutItem *child;
    while ((child = m_listLayout->takeAt(0)) != nullptr) {
        if (child->widget())
            child->widget()->deleteLater();
        delete child;
    }

    const bool pickerOk = m_hotkeyMgr ? m_hotkeyMgr->isPickerRegistered() : false;
    const QSet<int> itemOkSet = m_hotkeyMgr ? m_hotkeyMgr->registeredItemIndices() : QSet<int>{};

    if (!m_globalHotkey.isEmpty()) {
        ui->labelGlobalStatus->setVisible(true);
        ui->labelGlobalStatus->setObjectName(pickerOk ? "statusOk" : "statusFail");
        ui->labelGlobalStatus->setText(pickerOk ? "Active" : "Failed");
        ui->labelGlobalStatus->setToolTip(pickerOk
            ? "Picker shortcut is registered."
            : "Could not register this shortcut. It may already be in use.");
        ui->labelGlobalStatus->style()->unpolish(ui->labelGlobalStatus);
        ui->labelGlobalStatus->style()->polish(ui->labelGlobalStatus);
    } else {
        ui->labelGlobalStatus->setVisible(false);
    }

    for (int i = 0; i < m_items.size(); ++i) {
        const PasteItem &item = m_items[i];

        auto *row = new QWidget(m_listContainer);
        row->setObjectName("itemCard");
        row->setMinimumHeight(64);

        auto *hl = new QHBoxLayout(row);
        hl->setContentsMargins(16, 12, 12, 12);
        hl->setSpacing(12);

        auto *numLabel = new QLabel(QString::number(i + 1), row);
        numLabel->setObjectName("itemIndex");
        numLabel->setFixedSize(22, 22);
        numLabel->setAlignment(Qt::AlignCenter);
        hl->addWidget(numLabel);

        auto *textBlock = new QWidget(row);
        auto *vl = new QVBoxLayout(textBlock);
        vl->setContentsMargins(0, 0, 0, 0);
        vl->setSpacing(2);

        const QString titleText = item.title.trimmed().isEmpty()
            ? QString("Untitled item %1").arg(i + 1)
            : item.title;
        auto *titleLabel = new QLabel(titleText, textBlock);
        titleLabel->setObjectName("itemTitle");
        titleLabel->setTextFormat(Qt::PlainText);
        vl->addWidget(titleLabel);

        QString preview = item.text;
        preview.replace(QLatin1Char('\n'), QLatin1Char(' '));
        const QString elided = QFontMetrics(titleLabel->font()).elidedText(preview, Qt::ElideRight, 320);
        if (!elided.trimmed().isEmpty()) {
            auto *previewLabel = new QLabel(elided, textBlock);
            previewLabel->setObjectName("itemPreview");
            previewLabel->setTextFormat(Qt::PlainText);
            vl->addWidget(previewLabel);
        }
        hl->addWidget(textBlock, 1);

        if (!item.hotkey.isEmpty()) {
            const bool hkOk = itemOkSet.contains(i);
            auto *status = new QLabel(hkOk ? item.hotkey.toString() + "  Active"
                                           : item.hotkey.toString() + "  Failed", row);
            status->setObjectName(hkOk ? "statusOk" : "statusFail");
            status->setToolTip(hkOk
                ? "Global shortcut is registered."
                : "Could not register this shortcut.");
            hl->addWidget(status);
        }

        auto makeGhost = [&](const QString &text, const QString &tip, const char *name) {
            auto *b = new QPushButton(text, row);
            b->setObjectName(name);
            b->setToolTip(tip);
            b->setCursor(Qt::PointingHandCursor);
            b->setFixedHeight(32);
            return b;
        };

        auto *btnUp   = makeGhost("Up", "Move up", "btnGhost");
        auto *btnDown = makeGhost("Down", "Move down", "btnGhost");
        auto *btnEdit = makeGhost("Edit", "Edit item", "btnGhost");
        auto *btnDel  = makeGhost("Remove", "Remove item", "btnDanger");

        hl->addWidget(btnUp);
        hl->addWidget(btnDown);
        hl->addWidget(btnEdit);
        hl->addWidget(btnDel);

        const int idx = i;
        connect(btnUp,   &QPushButton::clicked, this, [this, idx]{ onMoveUp(idx); });
        connect(btnDown, &QPushButton::clicked, this, [this, idx]{ onMoveDown(idx); });
        connect(btnEdit, &QPushButton::clicked, this, [this, idx]{ onEditItem(idx); });
        connect(btnDel,  &QPushButton::clicked, this, [this, idx]{ onRemoveItem(idx); });

        m_listLayout->addWidget(row);
    }

    if (m_items.isEmpty()) {
        auto *empty = new QLabel("No items yet. Add one to start pasting instantly.", m_listContainer);
        empty->setObjectName("labelPageSubtitle");
        empty->setAlignment(Qt::AlignCenter);
        empty->setMinimumHeight(80);
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
