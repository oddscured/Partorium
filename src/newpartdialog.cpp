#include "newpartdialog.h"
#include "ui_newpartdialog.h"
#include "listmanagerdialog.h"
#include "jsonpartrepository.h"
#include <QFileDialog>
#include <QLabel>
#include <QFileInfo>
#include <QSettings>
#include "guiutils.h"

NewPartDialog::NewPartDialog(JsonPartRepository* repo, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::NewPartDialog)
    , m_repo(repo)
{
    ui->setupUi(this);
    hookUpSignals();


}

NewPartDialog::~NewPartDialog() { delete ui; }

void NewPartDialog::prepareUI()
{

}

void NewPartDialog::hookUpSignals() {
    // Einstellungen laden
    QSettings s("Partorium","Partorium");
    const QString defaultImageFolder = s.value("defaultImageFolder").toString();
    const QString defaultPartsFilesFolder = s.value("defaultPartsFilesFolder").toString();

    // UI Elemente vorbereiten
    JsonPartRepository::PresetsMap presets;
    m_repo->loadPresets(presets);
    GuiUtils::applyPresetToCombo(ui->cbb_Category, presets, "Kategorie");
    GuiUtils::applyPresetToCombo(ui->cbb_Format, presets, "Format");
    GuiUtils::applyPresetToCombo(ui->cbb_Manufacturer, presets, "Hersteller");
    GuiUtils::applyPresetToCombo(ui->cbb_Source, presets, "Bezugsquelle");
    GuiUtils::applyPresetToCombo(ui->cbb_StorageLocation, presets, "Lagerort");
    GuiUtils::applyPresetToCombo(ui->cbb_SubCategory, presets, "Unterkategorie");
    GuiUtils::applyPresetToCombo(ui->cbb_Type, presets, "Typ");
    GuiUtils::applyPresetToCombo(ui->cbb_AlternativeSource, presets, "Bezugsquelle");


    /*
    QComboBox *cbb_Source = this->findChild<QComboBox*>("cbb_Source");
    cbb_Source->addItems({"", "AZ-Delivery", "Amazon", "Ali Express", "Reichelt", "Conrad"});
    QComboBox *cbb_AltSource = this->findChild<QComboBox*>("cbb_AlternativeSource");
    cbb_AltSource->addItems({"", "AZ-Delivery", "Amazon", "Ali Express", "Reichelt", "Conrad"}); //TODO: remove redundancy
    QComboBox *cbb_Manufacturer = this->findChild<QComboBox*>("cbb_Manufacturer");
    cbb_Manufacturer->addItems({"", "ELV"});
    QComboBox *cbb_Category = this->findChild<QComboBox*>("cbb_Category");
    cbb_Category->addItems({"", "Sensoren", "Aktoren", "Module", "Kabel & Stecker", "Sonstiges"});
    QComboBox *cbb_SubCategory = this->findChild<QComboBox*>("cbb_SubCategory");
    cbb_SubCategory->addItems({"", "Temperatursensoren", "Feuchtigkeitssensoren", "Bewegungssensoren", "Displays", "Motoren", "Relais", "Breadboards", "Jumper Kabel"});
    QComboBox *cbb_Storage = this->findChild<QComboBox*>("cbb_StorageLocation");
    cbb_Storage->addItems({"", "Schublade A1", "Schublade A2", "Box 1", "Box 2"});
    QComboBox *cbb_Format = this->findChild<QComboBox*>("cbb_Format");
    cbb_Format->addItems({"", "SMD", "DIP", "Sonstiges"});
    QComboBox *cbb_Type = this->findChild<QComboBox*>("cbb_Type");
    cbb_Type->addItems({"", "Widerstand", "Kondensator", "Transistor", "IC", "Sonstiges"});
*/


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

