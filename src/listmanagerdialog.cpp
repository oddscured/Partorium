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


