/*
#include "listmanagerdialog.h"
#include "ui_listmanagerdialog.h"

ListManagerDialog::ListManagerDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ListManagerDialog)
{
    ui->setupUi(this);

    QComboBox *cbb_Lists = this->findChild<QComboBox*>("cbb_Lists");
    cbb_Lists->addItems({"Typ", "Kategorie", "Unterkategorie", "Format", "Bezugsquelle", "Hersteller", "Lagerort"});

    //ui->cbb_Lists->addItems({"Typ", "Kategorie", "Unterkategorie", "Format", "Bezugsquelle", "Hersteller", "Lagerort"});
    //ui->cbb_Lists->update();

}

ListManagerDialog::~ListManagerDialog()
{
    delete ui;
}

QMap<QString, QStringList> ListManagerDialog::presets() const
{
    return m_presets;
}
*/

#include "listmanagerdialog.h"
#include "ui_listmanagerdialog.h"  // wird aus deiner .ui generiert

#include <QInputDialog>
#include <QMessageBox>

ListManagerDialog::ListManagerDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ListManagerDialog)
{
    ui->setupUi(this);

    //Standard-Listen einfügen
    //QComboBox *cbb_Lists = this->findChild<QComboBox*>("cbb_Lists");
    //->addItems({"Typ", "Kategorie", "Unterkategorie", "Format", "Bezugsquelle", "Hersteller", "Lagerort"});


    // sinnvolle Defaults
    ui->lst_Entries->setSelectionMode(QAbstractItemView::ExtendedSelection);

    // Signals verdrahten
    connect(ui->cbb_Lists, &QComboBox::currentIndexChanged,this, &ListManagerDialog::onListChanged);
    connect(ui->btn_Add, &QPushButton::clicked, this, &ListManagerDialog::onAdd);
    connect(ui->btn_Remove, &QPushButton::clicked, this, &ListManagerDialog::onRemove);
    connect(ui->btn_MoveUp, &QPushButton::clicked, this, &ListManagerDialog::onMoveUp);
    connect(ui->btn_MoveDown, &QPushButton::clicked, this, &ListManagerDialog::onMoveDown);
    connect(this, &QDialog::accepted, this, &ListManagerDialog::onAccept);
    setDirty(false);
    refreshListKeys();
    refreshEntries();
}

ListManagerDialog::~ListManagerDialog()
{
    delete ui;
}

void ListManagerDialog::setPresets(const QMap<QString, QStringList> &presets,
                                   const QString &currentKey)
{
    m_presets = presets;
    if (!currentKey.isEmpty() && m_presets.contains(currentKey))
        m_currentKey = currentKey;
    else if (!m_presets.isEmpty())
        m_currentKey = m_presets.firstKey();
    else
        m_currentKey.clear();

    refreshListKeys();
    refreshEntries();
    setDirty(false);
}

void ListManagerDialog::setCurrentListKey(const QString &key)
{
    if (!m_presets.contains(key)) return;
    m_currentKey = key;
    refreshListKeys();
    refreshEntries();
}

void ListManagerDialog::refreshListKeys()
{
    ui->cbb_Lists->blockSignals(true);
    ui->cbb_Lists->clear();
    for (const auto &k : m_presets.keys())
        ui->cbb_Lists->addItem(k);

    int idx = m_currentKey.isEmpty() ? -1 : ui->cbb_Lists->findText(m_currentKey);
    ui->cbb_Lists->setCurrentIndex(idx);
    ui->cbb_Lists->blockSignals(false);

    const bool haveList = (idx >= 0);
    ui->lst_Entries->setEnabled(haveList);
    ui->btn_Add->setEnabled(haveList);
    ui->btn_Remove->setEnabled(haveList);
    ui->btn_MoveUp->setEnabled(haveList);
    ui->btn_MoveDown->setEnabled(haveList);
}

void ListManagerDialog::refreshEntries()
{
    ui->lst_Entries->clear();
    if (!m_presets.contains(m_currentKey)) return;

    const auto items = m_presets.value(m_currentKey);
    ui->lst_Entries->addItems(items);
}

void ListManagerDialog::onListChanged(int index)
{
    if (index < 0) {
        m_currentKey.clear();
        ui->lst_Entries->clear();
        return;
    }
    m_currentKey = ui->cbb_Lists->itemText(index);
    refreshEntries();
}

bool ListManagerDialog::containsCaseInsensitive(const QStringList &list, const QString &value) const
{
    for (const auto &v : list)
        if (v.compare(value, Qt::CaseInsensitive) == 0)
            return true;
    return false;
}

void ListManagerDialog::onAdd()
{
    if (m_currentKey.isEmpty()) {
        QMessageBox::information(this, tr("Keine Liste"),
                                 tr("Bitte zuerst eine Liste in der Auswahl wählen."));
        return;
    }

    bool ok = false;
    const QString text = QInputDialog::getText(this, tr("Eintrag hinzufügen"),
                                               tr("Neuer Eintrag:"), QLineEdit::Normal,
                                               QString(), &ok).trimmed();
    if (!ok || text.isEmpty()) return;

    auto &lst = m_presets[m_currentKey];
    if (containsCaseInsensitive(lst, text)) {
        QMessageBox::information(this, tr("Duplikat"),
                                 tr("Der Eintrag existiert bereits in dieser Liste."));
        return;
    }

    lst.append(text);
    setDirty(true);
    refreshEntries();
}

void ListManagerDialog::onRemove()
{
    if (m_currentKey.isEmpty()) return;

    const auto selected = ui->lst_Entries->selectedItems();
    if (selected.isEmpty()) return;

    auto &lst = m_presets[m_currentKey];
    for (auto *it : selected) {
        lst.removeAll(it->text());
        delete it;
    }
    setDirty(true);
}

void ListManagerDialog::onMoveUp()
{
    int row = ui->lst_Entries->currentRow();
    if (row <= 0) return;

    auto &lst = m_presets[m_currentKey];
    if (row >= lst.size()) return;

    lst.swapItemsAt(row, row - 1);
    setDirty(true);
    refreshEntries();
    ui->lst_Entries->setCurrentRow(row - 1);
}

void ListManagerDialog::onMoveDown()
{
    int row = ui->lst_Entries->currentRow();
    auto &lst = m_presets[m_currentKey];
    if (row < 0 || row >= lst.size() - 1) return;

    lst.swapItemsAt(row, row + 1);
    setDirty(true);
    refreshEntries();
    ui->lst_Entries->setCurrentRow(row + 1);
}

void ListManagerDialog::onAccept()
{
    if (m_dirty)
        emit presetsChanged(m_presets);
}

void ListManagerDialog::setDirty(bool dirty)
{
    m_dirty = dirty;
    setWindowModified(dirty);
}
