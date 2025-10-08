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
    auto edt = this->findChild<QLineEdit*>("edt_DefaultImageFolder");
    auto btn = this->findChild<QPushButton*>("btn_ChooseDefaultImageFolder");
    if (btn) {
        connect(btn, &QPushButton::clicked, this, [this, edt]{
            const QString folder = QFileDialog::getExistingDirectory(this,
                                                              tr("Standard-Bilderordner wählen"), QString(),
                                                              QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
            if (!folder.isEmpty()) {
                // Im Dialog merken (für MainWindow)
                this->setProperty("chosenImagePath", folder);
                if (edt) edt->setText(QFileInfo(folder).fileName());
            }
        });
    }
}

void SettingsDialog::loadSettings() {
    QSettings s("Partorium","Partorium");
    auto edt = this->findChild<QLineEdit*>("edt_DefaultImageFolder");
    if (edt) {
        const QString v = s.value("defaultImageFolder").toString();
        edt->setText(v);
    }
}

void SettingsDialog::saveSettings() {
    QSettings s("Partorium","Partorium");
    auto edt = this->findChild<QLineEdit*>("edt_DefaultImageFolder");
    if (edt) {
        s.setValue("defaultImageFolder", edt->text());
    }
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}
