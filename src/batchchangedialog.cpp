#include "batchchangedialog.h"
#include "ui_batchchangedialog.h"
#include "guiutils.h"
#include "jsonpartrepository.h"
#include <optional>

struct PartBatchPatch
{
    std::optional<QString> category;
    std::optional<QString> subcategory;
    std::optional<QString> format;
    std::optional<QString> type;
    // usw.
};

// Liste der Checkboxes im Dialog
 QVector<QCheckBox*> myCheckboxes;

// Konstruktor
BatchChangeDialog::BatchChangeDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::BatchChangeDialog)
{
    ui->setupUi(this);
    prepareUI();

    // Buttons verbinden, OK und Abbrechen
    connect(ui->btn_Cancel, &QPushButton::clicked, this, &BatchChangeDialog::reject);
    connect(ui->btn_BatchUpdate, &QPushButton::clicked, this, &BatchChangeDialog::accept);

    // Alle Auswahlboxen leer setzen
    ui->cbb_Category->setCurrentIndex(-1);
    ui->cbb_SubCategory->setCurrentIndex(-1);
    ui->cbb_Format->setCurrentIndex(-1);
    ui->cbb_Type->setCurrentIndex(-1);
    ui->cbb_Storage->setCurrentIndex(-1);

    // Alle Checkboxes in eine Liste packen und mit onCheckedChange verbinden
    myCheckboxes.append(ui->chb_Category);
    myCheckboxes.append(ui->chb_SubCategory);
    myCheckboxes.append(ui->chb_Format);
    myCheckboxes.append(ui->chb_Type);
    myCheckboxes.append(ui->chb_Storage);
    myCheckboxes.append(ui->chb_StorageDetails);
    // Alle Checkboxes mit dem Slot verbinden
    for (QCheckBox* checkbox : std::as_const(myCheckboxes)) {   // as_const ist wichtig, damit ein const Iterator zurückgegeben wird
        connect(checkbox, &QCheckBox::checkStateChanged, this, &BatchChangeDialog::onCheckedChange);
    }
}

// Füllt die Comboboxen mit den Presets aus der JSON-Datei
void BatchChangeDialog::prepareUI()
{
    JsonPartRepository* m_repo = new JsonPartRepository(this); // Temporäres Repo nur zum Laden der Presets
    JsonPartRepository::PresetsMap presets;
    m_repo->loadPresets(presets);

    GuiUtils::applyPresetToCombo(ui->cbb_Category, presets, "Kategorie");
    GuiUtils::applyPresetToCombo(ui->cbb_Format, presets, "Format");
    GuiUtils::applyPresetToCombo(ui->cbb_Storage, presets, "Lagerort");
    GuiUtils::applyPresetToCombo(ui->cbb_SubCategory, presets, "Unterkategorie");
    GuiUtils::applyPresetToCombo(ui->cbb_Type, presets, "Typ");
}

// Setzt die IDs der zu ändernden Bauteile
void BatchChangeDialog::setPartIds(const QVector<int>& ids)
{
    m_partIds = ids;
    this->setWindowTitle(tr("%1 Bauteile ändern").arg(m_partIds.size())); // Anzahl zu ändernder Parts im Window-Title anzeigen
}

// Holt die IDs der zu ändernden Bauteile
QVector<int> BatchChangeDialog::partIds() const
{
    return m_partIds;
}

// Erstellt die BatchChangePatch-Struktur mit den zu ändernden Feldern
BatchChangePatch BatchChangeDialog::patch() const
{
    BatchChangePatch p;

    // Kein trimmed(), damit leer "" wirklich gesetzt werden kann
    if (ui->chb_Category->isChecked()) {
        p.category = ui->cbb_Category->currentText();
    }
    if (ui->chb_SubCategory->isChecked()) {
        p.subcategory = ui->cbb_SubCategory->currentText();
    }
    if (ui->chb_Format->isChecked()) {
        p.format = ui->cbb_Format->currentText();
    }
    if (ui->chb_Type->isChecked()) {
        p.type = ui->cbb_Type->currentText();
    }
    if (ui->chb_Storage->isChecked()) {
        p.storage = ui->cbb_Storage->currentText();
    }
    if (ui->chb_StorageDetails->isChecked()) {
        p.storageDetails = ui->edt_StorageDetails->text();
    }

    return p;
}

// wenn keine Box aktiviert ist, soll der OK-Button deaktiviert werden, sonst aktiviert
void BatchChangeDialog::onCheckedChange()
{
    bool anyChecked = std::any_of(myCheckboxes.begin(),
                                  myCheckboxes.end(),
                                  [](QCheckBox* cb){ return cb->isChecked(); });

    ui->btn_BatchUpdate->setEnabled(anyChecked);
    return;
}

// Destructor
BatchChangeDialog::~BatchChangeDialog()
{
    delete ui;
}
