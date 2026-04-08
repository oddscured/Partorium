#include "removestockdialog.h"
#include "ui_removestockdialog.h"
#include <QComboBox>
#include <QLineEdit>
#include <QMessageBox>

RemoveStockDialog::RemoveStockDialog(const QStringList& previousProjectTitles, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::RemoveStockDialog)
{
    ui->setupUi(this);
    ui->cbb_ProjectTitle->setEditable(true);
    ui->cbb_ProjectTitle->setInsertPolicy(QComboBox::NoInsert);
    ui->cbb_ProjectTitle->addItems(previousProjectTitles);
    if (auto* le = ui->cbb_ProjectTitle->lineEdit()) {
        le->setPlaceholderText(tr("Project title"));
        le->setClearButtonEnabled(true);
    }


    hookUpSignals();
}

RemoveStockDialog::~RemoveStockDialog()
{
    delete ui;
}

int RemoveStockDialog::quantity() const {
    return ui->nud_Qty->value();
}

QString RemoveStockDialog::projectTitle() const {
    return ui->cbb_ProjectTitle->currentText().trimmed();
}

QString RemoveStockDialog::comment() const {
    return ui->txt_Comment->toPlainText().trimmed();
}

void RemoveStockDialog::hookUpSignals() {
    ui->nud_Qty->setMinimum(1);
    ui->btn_OK->setDefault(true);
    ui->btn_OK->setAutoDefault(true);

    connect(ui->btn_OK, &QPushButton::clicked, this, [this]() {
        if (ui->nud_Qty->value() <= 0) {
            QMessageBox::warning(this, tr("Input Error"), tr("Quantity must be greater than 0."));
            return;
        }
        if (ui->cbb_ProjectTitle->currentText().trimmed().isEmpty()) {
            QMessageBox::warning(this, tr("Input Error"), tr("Project title is required."));
            return;
        }
        accept();
    });

    connect(ui->btn_Cancel, &QPushButton::clicked, this, &QDialog::reject);
}
