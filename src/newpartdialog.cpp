#include "newpartdialog.h"
#include "ui_newpartdialog.h"
#include <QFileDialog>
#include <QLabel>
#include <QFileInfo>
#include <QSettings>

NewPartDialog::NewPartDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::NewPartDialog)
{
    ui->setupUi(this);
    hookUpSignals();
}

NewPartDialog::~NewPartDialog() { delete ui; }

void NewPartDialog::hookUpSignals() {
    // Einstellungen laden
    QSettings s("Partorium","Partorium");
    const QString defaultImageFolder = s.value("defaultImageFolder").toString();
    const QString defaultPartsFilesFolder = s.value("defaultPartsFilesFolder").toString();

    // UI Elemente suchen und zuordnen
    auto btnFolder = this->findChild<QPushButton*>("btn_SelectPartFilesFolder");
    auto btnImg = this->findChild<QPushButton*>("btn_ChooseImage");
    auto lblImg = this->findChild<QLabel*>("lbl_ChooseImage");
    auto imglbl = this->findChild<QLabel*>("lbl_ImagePreview");
    auto edtFolder = this->findChild<QLineEdit*>("edt_PartFilesFolder");

    if (btnImg) {
        connect(btnImg, &QPushButton::clicked, this, [this, lblImg, imglbl, defaultImageFolder]{
            const QString file = QFileDialog::getOpenFileName(this,
                                                              tr("Anzeigebild wählen"),
                                                              defaultImageFolder,//QString(),
                                                              tr("Bilder (*.png *.jpg *.jpeg *.bmp *.gif);;Alle Dateien (*)")
                                                              );
            if (!file.isEmpty()) {
                // Im Dialog merken (für MainWindow)
                this->setProperty("chosenImagePath", file);
                if (lblImg) lblImg->setText(QFileInfo(file).fileName());
                if (imglbl) imglbl->setPixmap(QPixmap(file).scaled(128,128,Qt::KeepAspectRatio, Qt::SmoothTransformation));
            }
        });
    }

    if (btnFolder) {
        connect(btnFolder, &QPushButton::clicked, this, [this, edtFolder, defaultPartsFilesFolder]{
            const QString dir = QFileDialog::getExistingDirectory(this,
                                                                  tr("Ordner wählen"),
                                                                  defaultPartsFilesFolder, //QString(),
                                                                  QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
            if (!dir.isEmpty()) {
                // Im Dialog merken (für MainWindow)
                this->setProperty("chosenFolderPath", dir);
                if (edtFolder) edtFolder->setText(dir);
            }
        });
    }


}
