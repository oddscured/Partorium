#include "newpartdialog.h"
#include "ui_newpartdialog.h"
#include <QFileDialog>
#include <QLabel>
#include <QFileInfo>

NewPartDialog::NewPartDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::NewPartDialog)
{
    ui->setupUi(this);
    hookUpSignals();
}

NewPartDialog::~NewPartDialog() { delete ui; }

void NewPartDialog::hookUpSignals() {
    auto btn = this->findChild<QPushButton*>("btn_ChooseImage");
    auto lbl = this->findChild<QLabel*>("lbl_ChooseImage");
    auto imglbl = this->findChild<QLabel*>("lbl_ImagePreview");
    if (btn) {
        connect(btn, &QPushButton::clicked, this, [this, lbl, imglbl]{
            const QString file = QFileDialog::getOpenFileName(this,
                                                              tr("Anzeigebild wählen"), QString(),
                                                              tr("Bilder (*.png *.jpg *.jpeg *.bmp *.gif);;Alle Dateien (*)"));
            if (!file.isEmpty()) {
                // Im Dialog merken (für MainWindow)
                this->setProperty("chosenImagePath", file);
                if (lbl) lbl->setText(QFileInfo(file).fileName());
                if (imglbl) imglbl->setPixmap(QPixmap(file).scaled(128,128,Qt::KeepAspectRatio, Qt::SmoothTransformation));
            }
        });
    }
}
