#include "addstockdialog.h"
#include "ui_addstockdialog.h"
#include "guiutils.h"
#include "jsonpartrepository.h"
#include <QLineEdit>
#include <QMessageBox>

AddStockDialog::AddStockDialog(JsonPartRepository* repo, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AddStockDialog)
    , m_repo(repo)
{
    ui->setupUi(this);
    populateSourceCombo();
    hookUpSignals();
}

AddStockDialog::~AddStockDialog()
{
    delete ui;
}

int AddStockDialog::quantity() const {
    return ui->nud_Qty->value();
}

QString AddStockDialog::source() const {
    return ui->cbb_Source->currentText().trimmed();
}

QString AddStockDialog::comment() const {
    return ui->txt_Comment->toPlainText().trimmed();
}

void AddStockDialog::hookUpSignals() {
    ui->nud_Qty->setMinimum(1);
    ui->cbb_Source->setEditable(true);
    ui->cbb_Source->setInsertPolicy(QComboBox::NoInsert);
    if (auto* le = ui->cbb_Source->lineEdit()) {
        le->setPlaceholderText(tr("Source"));
    }

    ui->btn_OK->setDefault(true);
    ui->btn_OK->setAutoDefault(true);
    ui->cbb_Source->setFocus();

    connect(ui->btn_OK, &QPushButton::clicked, this, [this]() {
        if (ui->nud_Qty->value() <= 0) {
            QMessageBox::warning(this, tr("Input Error"), tr("Quantity must be greater than 0."));
            return;
        }
        if (ui->cbb_Source->currentText().trimmed().isEmpty()) {
            QMessageBox::warning(this, tr("Input Error"), tr("Source is required."));
            return;
        }
        accept();
    });

    connect(ui->btn_Cancel, &QPushButton::clicked, this, &QDialog::reject);
}

void AddStockDialog::populateSourceCombo() {
    if (!m_repo) return;

    JsonPartRepository::PresetsMap presets;
    if (!m_repo->loadPresets(presets)) return;
    GuiUtils::applyPresetToCombo(ui->cbb_Source, presets, "Bezugsquelle");
}
