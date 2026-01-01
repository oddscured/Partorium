#include "newpartdialog.h"
#include "ui_newpartdialog.h"
//#include "listmanagerdialog.h"
#include "jsonpartrepository.h"
#include <QFileDialog>
#include <QLabel>
#include <QFileInfo>
#include <QSettings>
#include "guiutils.h"
#include <QMessageBox>

NewPartDialog::NewPartDialog(JsonPartRepository* repo, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::NewPartDialog)
    , m_repo(repo)
{
    ui->setupUi(this);
    hookUpSignals();

    //connect(ui->btn_NextPart, &QPushButton::clicked, this, &NewPartDialog::nextPartRequested);

}

NewPartDialog::~NewPartDialog() { delete ui; }

void NewPartDialog::prepareUI()
{

}

void NewPartDialog::hookUpSignals() {
    // Einstellungen laden und in Strings schreiben
    QSettings s("Partorium","Partorium");
    const QString defaultImageFolder = s.value("defaultImageFolder").toString();
    const QString defaultPartsFilesFolder = s.value("defaultPartsFilesFolder").toString();

    // UI Elemente vorbereiten. Die ComboBoxes mit Presets füllen
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


    // connect(ui->btn_NextPart, &QPushButton::clicked, this, [this]{
    //     //qDebug() << "btn_NextPart clicked in dialog" << this;
    //     emit nextPartRequested();
    //     //qDebug() << "nextPartRequested emitted" << this;
    // });

    // NextPart button aus der UI nehmen
    if (auto nextBtn = this->findChild<QPushButton*>("btn_NextPart")) {
        nextBtn->setAutoDefault(false); // Enter soll weiter OK drücken
        nextBtn->setDefault(false);
        connect(nextBtn, &QPushButton::clicked, this, [this](){
            // Zum Debuggen:
            qDebug() << "newpartdialog.cpp: btn_NextPart clicked -> emit nextPartRequested()";
            emit nextPartRequested();
        });
    } else {
        qWarning() << "btn_NextPart nicht gefunden – stimmt der ObjectName im .ui?";
    }


    // Ok-Button -> prüfen ob ein Name vergeben wurde, wenn nicht Meldung anzeigen. Ansonsten Dialog akzeptieren
    // TODO: hier ggf. validieren/speichern
    connect(ui->btn_Ok, &QPushButton::clicked, this, [this]{
        if(ui->edt_PartName->text() == "")
        {
            QMessageBox::critical(this, "Fehler", "Es muss mindestens ein Bauteilname vergeben werden!");
            return;
        }
        this->accept();
    });

    // Abbrechen -> Dialog verwerfen
    connect(ui->btn_Cancel, &QPushButton::clicked, this, &QDialog::reject);


    // UI Elemente suchen und zuordnen
    auto btnFolder = this->findChild<QPushButton*>("btn_SelectPartFilesFolder");
    auto btnImg = this->findChild<QPushButton*>("btn_ChooseImage");
    auto lblImg = this->findChild<QLabel*>("lbl_ChooseImage");
    auto imglbl = this->findChild<QLabel*>("lbl_ImagePreview");
    auto edtFolder = this->findChild<QLineEdit*>("edt_PartFilesFolder");
    auto btnDeleteImage = this->findChild<QPushButton*>("btn_DeleteImage");

    if (btnImg) {
        connect(btnImg, &QPushButton::clicked, this, [this, lblImg, imglbl, defaultImageFolder, btnDeleteImage]{
            const QString file = GuiUtils::getImageFileNameWithSearchString(this,
                                                                 ui->edt_PartName->text().trimmed(),
                                                                 defaultImageFolder);

            if (!file.isEmpty()) {
                // Im Dialog merken (für MainWindow)
                this->setProperty("chosenImagePath", file);
                if (lblImg) lblImg->setText(QFileInfo(file).fileName());
                if (imglbl) imglbl->setPixmap(QPixmap(file).scaled(128,128,Qt::KeepAspectRatio, Qt::SmoothTransformation));
                if (btnDeleteImage) btnDeleteImage->setEnabled(true);
            } else qDebug() << "file was empty!";
        });
    }

    // Löschen Botton der ein existierendes Bild entfernt
    if (btnDeleteImage) {
        connect(btnDeleteImage, &QPushButton::clicked, this,
                [this, lblImg, imglbl, btnDeleteImage] {
                    this->setProperty("chosenImagePath", QVariant());
                    if (lblImg) { lblImg->setText(tr("IMAGE")); }
                    if (imglbl) {
                        imglbl->clear();
                        imglbl->setText(tr("IMAGE"));
                        imglbl->setAlignment(Qt::AlignCenter);
                    }
                    this->setProperty("chosenImagePath", "");


                    // Button wieder deaktivieren
                    btnDeleteImage->setEnabled(false);
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

// Abfragen, ob Felder initialisiert werden sollen nach Hinzufügen eines neuen Bauteils
bool NewPartDialog::initializeAfterAddEnabled() const {
    //return QSettings("Partorium","Partorium")
    return QSettings()
    .value("ui/initializeNewPartFields", true).toBool();
}

void NewPartDialog::resetInputs()
{
    // TODO: überlegen welche Felder stehen bleiben können/müssen und diese dann ausnehmen

    // Immer das Namensfeld leeren
    ui->edt_PartName->clear();

    // Wenn Option deaktiviert ist, nichts tun
    if(!initializeAfterAddEnabled()) return;

    // Textfelder leeren
    for (auto *le : findChildren<QLineEdit*>())    le->clear();
    for (auto *te : findChildren<QTextEdit*>())    te->clear();

    // Zahlenfelder zurücksetzen
    for (auto *sb : findChildren<QSpinBox*>())     sb->setValue(0);

    // Vom Dialog gemerkte Pfade/Bilder zurücksetzen
    this->setProperty("chosenImagePath", QVariant());

    // Bild-Preview zurücksetzen:
    if (auto *lbl = findChild<QLabel*>("lbl_Image")) {
        lbl->clear();
        lbl->setText(tr("IMAGE"));
        lbl->setAlignment(Qt::AlignCenter);
    }

}

