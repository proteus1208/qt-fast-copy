#include "settingswindow.h"
#include "ui_settingswindow.h"
#include "itemeditordiag.h"
#include "storagemanager.h"
#include "appstyle.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QResizeEvent>
#include <QSizePolicy>
#include <QStyle>
#include <QIcon>
#include <QKeySequenceEdit>
#include <QFileDialog>
#include <QDesktopServices>
#include <QUrl>
#include <QMessageBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <QIODevice>

namespace {

class ElidedLabel : public QLabel
{
public:
    explicit ElidedLabel(QWidget *parent = nullptr) : QLabel(parent)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        setMinimumWidth(0);
        setWordWrap(false);
        setTextFormat(Qt::PlainText);
    }

    void setFullText(const QString &text)
    {
        m_fullText = text;
        setToolTip(text);
        updateElide();
    }

protected:
    void resizeEvent(QResizeEvent *event) override
    {
        QLabel::resizeEvent(event);
        updateElide();
    }

private:
    void updateElide()
    {
        const int w = qMax(0, width());
        setText(fontMetrics().elidedText(m_fullText, Qt::ElideRight, w));
    }

    QString m_fullText;
};

QString kindLabel(StoreEntryKind kind)
{
    switch (kind) {
    case StoreEntryKind::Text:   return QStringLiteral("Text");
    case StoreEntryKind::Path:   return QStringLiteral("Path");
    case StoreEntryKind::Folder: return QStringLiteral("Folder");
    }
    return {};
}

} // namespace

SettingsWindow::SettingsWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::SettingsWindow)
{
    ui->setupUi(this);
    setStyleSheet(appStyleSheet());

    ui->labelPageTitle->setObjectName("labelPageTitle");
    ui->labelPageSubtitle->setObjectName("labelPageSubtitle");
    ui->labelStoreTitle->setObjectName("labelGlobalTitle");
    ui->labelStoreHint->setObjectName("labelGlobalHint");
    ui->labelGlobalTitle->setObjectName("labelGlobalTitle");
    ui->labelGlobalHint->setObjectName("labelGlobalHint");
    ui->labelPasteTextTitle->setObjectName("labelGlobalTitle");
    ui->labelPasteTextHint->setObjectName("labelGlobalHint");
    ui->labelItems->setObjectName("labelItems");
    ui->labelHint->setObjectName("labelHint");
    ui->btnAddItem->setObjectName("btnAddItem");
    ui->btnClearGlobal->setObjectName("btnClearGlobal");
    ui->btnClearPasteText->setObjectName("btnClearGlobal");
    ui->btnBrowseStore->setObjectName("btnGhost");
    ui->btnOpenStore->setObjectName("btnLink");
    ui->btnOpenStore->setCursor(Qt::PointingHandCursor);
    ui->scrollArea->setWidgetResizable(true);

    forceBlackInputText(ui->editStorePath);
    forceBlackInputText(ui->keySeqEdit);
    forceBlackInputText(ui->keySeqPasteText);
    if (auto *inner = ui->keySeqEdit->findChild<QLineEdit *>())
        forceBlackInputText(inner);
    if (auto *inner = ui->keySeqPasteText->findChild<QLineEdit *>())
        forceBlackInputText(inner);

    ui->keySeqEdit->setMaximumSequenceLength(1);
    ui->keySeqPasteText->setMaximumSequenceLength(1);

    m_listContainer = ui->scrollAreaWidgetContents;
    m_listLayout    = qobject_cast<QVBoxLayout *>(m_listContainer->layout());
    m_listLayout->setAlignment(Qt::AlignTop);

    connect(ui->btnAddItem, &QPushButton::clicked, this, &SettingsWindow::onAddItem);
    connect(ui->btnBrowseStore, &QPushButton::clicked, this, &SettingsWindow::onBrowseStore);
    connect(ui->btnOpenStore, &QPushButton::clicked, this, &SettingsWindow::onOpenStore);
    connect(ui->editStorePath, &QLineEdit::editingFinished, this, &SettingsWindow::onStorePathEdited);
    connect(ui->btnClearGlobal, &QPushButton::clicked, ui->keySeqEdit, &QKeySequenceEdit::clear);
    connect(ui->btnClearPasteText, &QPushButton::clicked, ui->keySeqPasteText, &QKeySequenceEdit::clear);
    connect(ui->btnPasteTextOnly, &QPushButton::clicked, this, &SettingsWindow::pasteTextOnlyRequested);
    connect(ui->keySeqEdit, &QKeySequenceEdit::keySequenceChanged,
            this, &SettingsWindow::onGlobalHotkeyChanged);
    connect(ui->keySeqPasteText, &QKeySequenceEdit::keySequenceChanged,
            this, &SettingsWindow::onPasteTextHotkeyChanged);
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

void SettingsWindow::setAppSettings(const AppSettings &settings)
{
    m_app = settings;

    ui->editStorePath->blockSignals(true);
    ui->editStorePath->setText(QDir::toNativeSeparators(m_app.storePath));
    ui->editStorePath->blockSignals(false);

    ui->keySeqEdit->blockSignals(true);
    ui->keySeqEdit->setKeySequence(m_app.globalHotkey);
    ui->keySeqEdit->blockSignals(false);

    ui->keySeqPasteText->blockSignals(true);
    ui->keySeqPasteText->setKeySequence(m_app.pasteTextOnlyHotkey);
    ui->keySeqPasteText->blockSignals(false);

    rebuildList();
}

void SettingsWindow::setRootEntries(const QList<StoreEntry> &entries)
{
    m_entries = entries;
    rebuildList();
}

void SettingsWindow::emitAppSettings()
{
    emit appSettingsChanged(m_app);
}

void SettingsWindow::persistOrder()
{
    StorageManager::saveDirectoryMeta(m_app.storePath, m_entries);
    rebuildList();
    emit storeChanged();
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
    const bool pasteTextOk = m_hotkeyMgr ? m_hotkeyMgr->isPasteTextOnlyRegistered() : false;
    const QSet<QString> itemOkSet = m_hotkeyMgr ? m_hotkeyMgr->registeredItemPaths() : QSet<QString>{};

    if (!m_app.globalHotkey.isEmpty()) {
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

    if (!m_app.pasteTextOnlyHotkey.isEmpty()) {
        ui->labelPasteTextStatus->setVisible(true);
        ui->labelPasteTextStatus->setObjectName(pasteTextOk ? "statusOk" : "statusFail");
        ui->labelPasteTextStatus->setText(pasteTextOk ? "Active" : "Failed");
        ui->labelPasteTextStatus->setToolTip(pasteTextOk
            ? "Paste-text-only shortcut is registered."
            : "Could not register this shortcut. It may already be in use.");
        ui->labelPasteTextStatus->style()->unpolish(ui->labelPasteTextStatus);
        ui->labelPasteTextStatus->style()->polish(ui->labelPasteTextStatus);
    } else {
        ui->labelPasteTextStatus->setVisible(false);
    }

    for (int i = 0; i < m_entries.size(); ++i) {
        const StoreEntry &item = m_entries[i];

        auto *row = new QWidget(m_listContainer);
        row->setObjectName("itemCard");

        auto *hl = new QHBoxLayout(row);
        hl->setContentsMargins(8, 6, 6, 6);
        hl->setSpacing(6);

        auto *numLabel = new QLabel(QString::number(i + 1), row);
        numLabel->setObjectName("itemIndex");
        numLabel->setFixedSize(22, 22);
        numLabel->setAlignment(Qt::AlignCenter);
        hl->addWidget(numLabel);

        auto *iconLabel = new QLabel(row);
        iconLabel->setPixmap(item.icon.pixmap(18, 18));
        iconLabel->setFixedSize(20, 20);
        hl->addWidget(iconLabel);

        auto *textBlock = new QWidget(row);
        textBlock->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        textBlock->setMinimumWidth(0);
        auto *vl = new QVBoxLayout(textBlock);
        vl->setContentsMargins(0, 0, 0, 0);
        vl->setSpacing(2);

        auto *titleLabel = new ElidedLabel(textBlock);
        titleLabel->setObjectName("itemTitle");
        titleLabel->setFullText(item.name);
        vl->addWidget(titleLabel);

        QString preview = kindLabel(item.kind);
        if (item.kind == StoreEntryKind::Text) {
            // Preview only — avoid loading huge files into the settings list.
            QFile f(item.absolutePath);
            QString body;
            if (f.open(QIODevice::ReadOnly))
                body = QString::fromUtf8(f.read(240)).trimmed();
            body.replace(QLatin1Char('\n'), QLatin1Char(' '));
            body = body.simplified();
            if (!body.isEmpty())
                preview += QStringLiteral(" · ") + body;
        } else if (item.kind == StoreEntryKind::Path) {
            preview += QStringLiteral(" · ") + QDir::toNativeSeparators(item.absolutePath);
        }

        auto *previewLabel = new ElidedLabel(textBlock);
        previewLabel->setObjectName("itemPreview");
        previewLabel->setFullText(preview);
        vl->addWidget(previewLabel);
        hl->addWidget(textBlock, 1);

        if (!item.hotkey.isEmpty() && item.kind != StoreEntryKind::Folder) {
            const bool hkOk = itemOkSet.contains(item.absolutePath);
            auto *status = new QLabel(hkOk ? item.hotkey.toString() + "  Active"
                                           : item.hotkey.toString() + "  Failed", row);
            status->setObjectName(hkOk ? "statusOk" : "statusFail");
            status->setToolTip(hkOk
                ? "Global shortcut is registered."
                : "Could not register this shortcut.");
            status->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
            hl->addWidget(status);
        }

        auto makeIconBtn = [&](const QIcon &icon, const QString &tip, const char *name) {
            auto *b = new QPushButton(row);
            b->setObjectName(name);
            b->setIcon(icon);
            b->setIconSize(QSize(14, 14));
            b->setToolTip(tip);
            b->setCursor(Qt::PointingHandCursor);
            b->setFixedSize(28, 26);
            b->setFlat(true);
            return b;
        };

        const QIcon editIcon = QIcon::fromTheme(
            QStringLiteral("document-edit"),
            style()->standardIcon(QStyle::SP_FileDialogDetailedView));
        const QIcon delIcon = QIcon::fromTheme(
            QStringLiteral("edit-delete"),
            style()->standardIcon(QStyle::SP_TrashIcon));

        auto *btnUp   = makeIconBtn(style()->standardIcon(QStyle::SP_ArrowUp), "Move up", "btnIcon");
        auto *btnDown = makeIconBtn(style()->standardIcon(QStyle::SP_ArrowDown), "Move down", "btnIcon");
        auto *btnEdit = makeIconBtn(editIcon, "Edit item", "btnIcon");
        auto *btnDel  = makeIconBtn(delIcon, "Remove item", "btnDanger");

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

    if (m_entries.isEmpty()) {
        auto *empty = new QLabel(
            QStringLiteral("Store folder is empty. Add a text item, or drop files/folders in Explorer."),
            m_listContainer);
        empty->setObjectName("labelPageSubtitle");
        empty->setAlignment(Qt::AlignCenter);
        empty->setMinimumHeight(80);
        empty->setWordWrap(true);
        m_listLayout->addWidget(empty);
    }
}

void SettingsWindow::onAddItem()
{
    StoreEntry blank;
    blank.kind = StoreEntryKind::Text;
    ItemEditorDialog dlg(blank, this);
    if (dlg.exec() != QDialog::Accepted)
        return;

    QString error;
    if (!StorageManager::createTextItem(m_app.storePath, dlg.name(), dlg.text(), dlg.hotkey(), &error)) {
        QMessageBox::warning(this, QStringLiteral("Add item"), error);
        return;
    }
    m_entries = StorageManager::loadDirectory(m_app.storePath);
    rebuildList();
    emit storeChanged();
}

void SettingsWindow::onEditItem(int index)
{
    if (index < 0 || index >= m_entries.size()) return;
    StoreEntry entry = m_entries[index];
    ItemEditorDialog dlg(entry, this);
    if (dlg.exec() != QDialog::Accepted)
        return;

    if (entry.kind == StoreEntryKind::Text) {
        const QString newName = dlg.name();
        if (newName.isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("Edit item"),
                                 QStringLiteral("Name is required."));
            return;
        }

        // Rename if needed (still extensionless).
        if (newName != entry.name) {
            if (newName.contains(QLatin1Char('.')) || newName.contains(QLatin1Char('/'))
                || newName.contains(QLatin1Char('\\'))) {
                QMessageBox::warning(this, QStringLiteral("Edit item"),
                                     QStringLiteral("Use a name without extension or path separators."));
                return;
            }
            const QString newPath = QDir(m_app.storePath).filePath(newName);
            if (QFileInfo::exists(newPath)) {
                QMessageBox::warning(this, QStringLiteral("Edit item"),
                                     QStringLiteral("An item with that name already exists."));
                return;
            }
            if (!QFile::rename(entry.absolutePath, newPath)) {
                QMessageBox::warning(this, QStringLiteral("Edit item"),
                                     QStringLiteral("Could not rename the file."));
                return;
            }
            entry.absolutePath = newPath;
            entry.name = newName;
        }

        if (!StorageManager::writeTextFile(entry.absolutePath, dlg.text())) {
            QMessageBox::warning(this, QStringLiteral("Edit item"),
                                 QStringLiteral("Could not save the text file."));
            return;
        }
        entry.hotkey = dlg.hotkey();
        m_entries[index] = entry;
        persistOrder();
        return;
    }

    if (entry.kind == StoreEntryKind::Path) {
        entry.hotkey = dlg.hotkey();
        m_entries[index] = entry;
        persistOrder();
    }
    // Folder: nothing editable beyond order.
}

void SettingsWindow::onRemoveItem(int index)
{
    if (index < 0 || index >= m_entries.size()) return;
    const StoreEntry entry = m_entries[index];

    const auto reply = QMessageBox::question(
        this,
        QStringLiteral("Remove item"),
        QStringLiteral("Delete \"%1\" from the store folder?").arg(entry.name),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (reply != QMessageBox::Yes)
        return;

    QString error;
    if (!StorageManager::removeEntry(entry.absolutePath, &error)) {
        QMessageBox::warning(this, QStringLiteral("Remove item"), error);
        return;
    }
    m_entries = StorageManager::loadDirectory(m_app.storePath);
    rebuildList();
    emit storeChanged();
}

void SettingsWindow::onMoveUp(int index)
{
    if (index <= 0 || index >= m_entries.size()) return;
    m_entries.swapItemsAt(index - 1, index);
    persistOrder();
}

void SettingsWindow::onMoveDown(int index)
{
    if (index < 0 || index >= m_entries.size() - 1) return;
    m_entries.swapItemsAt(index, index + 1);
    persistOrder();
}

void SettingsWindow::onBrowseStore()
{
    const QString dir = QFileDialog::getExistingDirectory(
        this,
        QStringLiteral("Choose store folder"),
        m_app.storePath);
    if (dir.isEmpty())
        return;
    m_app.storePath = dir;
    ui->editStorePath->setText(QDir::toNativeSeparators(dir));
    emitAppSettings();
}

void SettingsWindow::onOpenStore()
{
    StorageManager::ensureStoreFolder(m_app.storePath);
    QDesktopServices::openUrl(QUrl::fromLocalFile(m_app.storePath));
}

void SettingsWindow::onStorePathEdited()
{
    const QString path = QDir::fromNativeSeparators(ui->editStorePath->text().trimmed());
    if (path.isEmpty() || path == m_app.storePath)
        return;
    m_app.storePath = path;
    emitAppSettings();
}

void SettingsWindow::onGlobalHotkeyChanged()
{
    m_app.globalHotkey = ui->keySeqEdit->keySequence();
    emitAppSettings();
}

void SettingsWindow::onPasteTextHotkeyChanged()
{
    m_app.pasteTextOnlyHotkey = ui->keySeqPasteText->keySequence();
    emitAppSettings();
}
