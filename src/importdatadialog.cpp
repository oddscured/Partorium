#include "importdatadialog.h"
#include "ui_importdatadialog.h"

ImportDataDialog::ImportDataDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ImportDataDialog)
{
    ui->setupUi(this);
}

ImportDataDialog::~ImportDataDialog()
{
    delete ui;
}
