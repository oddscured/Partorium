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

BatchChangeDialog::BatchChangeDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::BatchChangeDialog)
{
    ui->setupUi(this);
    prepareUI();
}

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

BatchChangeDialog::~BatchChangeDialog()
{
    delete ui;
}
