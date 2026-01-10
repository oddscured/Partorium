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

// Laden der Einstellungen in die Dialog-Elemente
void SettingsDialog::loadSettings() {
    // Einstellungen der Anwendung laden
    QSettings s("Partorium","Partorium");
    // Standardordner für Bilder laden
    auto edtImagesFolder = this->findChild<QLineEdit*>("edt_DefaultImageFolder");
    if (edtImagesFolder) {
        const QString v = s.value("defaultImageFolder").toString();
        edtImagesFolder->setText(v);
    }  
    // Standardordner für Bauteildateien laden
    auto edtFilesFolder = this->findChild<QLineEdit*>("edt_DefaultPartsFilesFolder");
    if (edtFilesFolder) {
        const QString v = s.value("defaultPartsFilesFolder").toString();
        edtFilesFolder->setText(v);
    }    
    // Währungssymbol laden
    if (auto cbb = findChild<QComboBox*>("cbb_Currency")) {
        const QString cur = s.value("currencySymbol", "€").toString();
        int idx = cbb->findText(cur);
        if (idx >= 0) cbb->setCurrentIndex(idx);
        else cbb->setCurrentText(cur);
    }
}

// Speichern der Einstellungen aus den Dialog-Elementen
void SettingsDialog::saveSettings() {
    // Einstellungen der Anwendung speichern
    QSettings s("Partorium","Partorium");
    // Standardordner für Bilder speichern
    auto edtImagesFolder = this->findChild<QLineEdit*>("edt_DefaultImageFolder");
    if (edtImagesFolder) {
        s.setValue("defaultImageFolder", edtImagesFolder->text());
    }
    // Standardordner für Bauteildateien speichern
    auto edtFilesFolder = this->findChild<QLineEdit*>("edt_DefaultPartsFilesFolder");
    if (edtFilesFolder) {
        s.setValue("defaultPartsFilesFolder", edtFilesFolder->text());
    }
    // Währungssymbol speichern
    if (auto cbb = findChild<QComboBox*>("cbb_Currency")) {
        s.setValue("currencySymbol", cbb->currentText());
    }
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}
