#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include <QFileDialog>
#include <QSettings>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
    hookUpSignals();
    loadSettings();
}

void SettingsDialog::hookUpSignals() {
    connect(this, &QDialog::accepted, this, &SettingsDialog::saveSettings);
    auto edtImagesFolder = this->findChild<QLineEdit*>("edt_DefaultImageFolder");
    auto btnImagesFolder = this->findChild<QPushButton*>("btn_ChooseDefaultImageFolder");
    if (btnImagesFolder) {
        connect(btnImagesFolder, &QPushButton::clicked, this, [this, edtImagesFolder]{
            const QString folder = QFileDialog::getExistingDirectory(this,
                                                              tr("Standard-Bilderordner wählen"), QString(),
                                                              QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
            if (!folder.isEmpty()) {
                // Im Dialog merken (für MainWindow)
                this->setProperty("chosenImagePath", folder);
                if (edtImagesFolder) edtImagesFolder->setText(QFileInfo(folder).filePath());
            }
        });
    }
    auto edtFilesFolder = this->findChild<QLineEdit*>("edt_DefaultPartsFilesFolder");
    auto btnFilesFolder = this->findChild<QPushButton*>("btn_ChooseDefaultPartsFilesFolder");
    if (btnFilesFolder) {
        connect(btnFilesFolder, &QPushButton::clicked, this, [this, edtFilesFolder]{
            const QString folder = QFileDialog::getExistingDirectory(this,
                                                              tr("Standard-Bauteilordner wählen"), QString(),
                                                              QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
            if (!folder.isEmpty()) {
                // Im Dialog merken (für MainWindow)
                this->setProperty("chosenPartFilesPath", folder);
                if (edtFilesFolder) edtFilesFolder->setText(QFileInfo(folder).filePath());
            }
        });
    }
}

void SettingsDialog::loadSettings() {
    QSettings s("Partorium","Partorium");
    auto edtImagesFolder = this->findChild<QLineEdit*>("edt_DefaultImageFolder");
    if (edtImagesFolder) {
        const QString v = s.value("defaultImageFolder").toString();
        edtImagesFolder->setText(v);
    }
    auto edtFilesFolder = this->findChild<QLineEdit*>("edt_DefaultPartsFilesFolder");
    if (edtFilesFolder) {
        const QString v = s.value("defaultPartsFilesFolder").toString();
        edtFilesFolder->setText(v);
    }
}

void SettingsDialog::saveSettings() {
    QSettings s("Partorium","Partorium");
    auto edtImagesFolder = this->findChild<QLineEdit*>("edt_DefaultImageFolder");
    if (edtImagesFolder) {
        s.setValue("defaultImageFolder", edtImagesFolder->text());
    }
    auto edtFilesFolder = this->findChild<QLineEdit*>("edt_DefaultPartsFilesFolder");
    if (edtFilesFolder) {
        s.setValue("defaultPartsFilesFolder", edtFilesFolder->text());
    }
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}
