#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QDesktopServices>
#include <QUrl>
#include <QMenuBar>
#include <QFileInfo>
#include "newpartdialog.h"
#include "settingsdialog.h"
#include <QPlainTextEdit>
#include <QSpinBox>
#include <QMessageBox>
#include <QFileDialog>
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>
#include <QDir>
#include "guiutils.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Settings laden
    QSettings s;
    m_showDeletedParts = s.value("ui/showDeletedParts", false).toBool();
    ui->act_ShowDeletedParts->setChecked(m_showDeletedParts); // Option setzen wie in den Einstellungen geladen


    // Repository initialisieren: Pfad aus Settings oder Vorschlag (iCloud)
    QString pathFromSettings = QSettings("Partorium","Partorium").value("databasePath").toString();
    loadOrInitRepository(pathFromSettings);

    // Repo + Models
    m_repo = new JsonPartRepository(this);
    m_repo->load();
    m_imagesModel = new QStandardItemModel(this);
    //ui->lst_Images->setModel(m_imagesModel);

    // Menüs/Aktionen
    // Anzeige gelöschter Teile umschalten
    ui->act_ShowDeletedParts->setChecked(m_showDeletedParts);
    connect(ui->act_ShowDeletedParts, &QAction::toggled, this, &MainWindow::toggleShowDeletedParts);

    // Kontektmenü für das Parts-List Widget
    ui->lst_Parts->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->lst_Parts, &QListWidget::customContextMenuRequested,this, &MainWindow::onPartsContextMenuRequested);

    // Signale (Suche / Files / Auswahl)
    connect(ui->act_NewPart, &QAction::triggered, this, &MainWindow::addNewPart);
    connect(ui->act_SelectDatabase, &QAction::triggered, this, &MainWindow::chooseDatabasePath);
    connect(ui->act_OpenDatabaseFolder, &QAction::triggered, this, &MainWindow::revealDatabaseFolder);
    connect(ui->act_OpenSettingsDialog, &QAction::triggered, this, &MainWindow::openSettingsDialog);
    connect(ui->lne_Search, &QLineEdit::textChanged, this, &MainWindow::applyFilters);
    connect(ui->lst_Files, &QListWidget::itemActivated, this, &MainWindow::onFileActivated);
    connect(ui->lst_Parts, &QListWidget::currentItemChanged, this,
            [this](QListWidgetItem* cur, QListWidgetItem*) {
                if (!cur) return;
                const int id = cur->data(Qt::UserRole).toInt();
                if (auto p = m_repo->getPart(id)) showPart(*p);
            });

    // Kategorien aus Daten ableiten
    refillCategories();

    // Liste initial füllen
    applyFilters();
}

MainWindow::~MainWindow() { delete ui; }


void MainWindow::loadOrInitRepository(const QString& path) {
    QString targetPath = path;

    if (targetPath.isEmpty()) {
        targetPath = JsonPartRepository::suggestDefaultJsonPath();
    }

    m_repo = new JsonPartRepository(this, targetPath);

    // Falls das Setzen/Laden fehlschlägt, auf Dokumente-Fallback gehen
    if (m_repo->databasePath().isEmpty()) {
        const auto fb = JsonPartRepository::suggestDefaultJsonPath();
        m_repo->setDatabasePath(fb);
    }

    // Persistieren
    QSettings("Partorium","Partorium").setValue("databasePath", m_repo->databasePath());
}

void MainWindow::chooseDatabasePath() {
    // „Speichern unter“ – Nutzer kann Ort & Name wählen. Endung .json vorschlagen.
    const QString suggested = m_repo ? m_repo->databasePath()
                                     : JsonPartRepository::suggestDefaultJsonPath();

    const QString path = QFileDialog::getSaveFileName(
        this, tr("Speicherort für Partorium-Daten wählen"),
        suggested,
        tr("JSON-Datei (*.json)"));

    if (path.isEmpty()) return;

    if (!m_repo) m_repo = new JsonPartRepository(this);

    if (!m_repo->setDatabasePath(path)) {
        QMessageBox::critical(this, tr("Fehler"),
                              tr("Der gewählte Speicherort konnte nicht gesetzt werden."));
        return;
    }

    // merken
    QSettings("Partorium","Partorium").setValue("databasePath", m_repo->databasePath());

    // UI auffrischen
    refillCategories();
    applyFilters();
}

void MainWindow::revealDatabaseFolder() {
    if (!m_repo) return;
    const QFileInfo fi(m_repo->databasePath());
    const auto folderUrl = QUrl::fromLocalFile(fi.dir().absolutePath());
    QDesktopServices::openUrl(folderUrl);
}

void MainWindow::refillCategories() {
    ui->cbb_Category->clear();
    ui->cbb_Category->addItem("Alle Kategorien");
    // Einmal alle Teile holen und Kategorien extrahieren
    QSet<QString> cats;
    for (const auto& p : m_repo->allParts()) {
        if (!p.hashtags.contains("deleted", Qt::CaseInsensitive))
            cats.insert(p.category);
    }
    QStringList list = cats.values();
    list.sort(Qt::CaseInsensitive);
    ui->cbb_Category->addItems(list);
    connect(ui->cbb_Category, &QComboBox::currentTextChanged, this, &MainWindow::applyFilters);
}

void MainWindow::applyFilters() {
    const QString term = ui->lne_Search->text();
    const QString cat  = ui->cbb_Category->currentText();
    const auto parts   = m_repo->searchParts(term, cat);

    ui->lst_Parts->clear();
    for (const auto& p : parts) {
        if (!m_showDeletedParts && p.deleted) continue;

        auto* it = new QListWidgetItem(p.name, ui->lst_Parts);
        it->setData(Qt::UserRole, p.id);

        // Optische Kennzeichnung für gelöschte Einträge (nur wenn sichtbar)
        if (p.deleted) {
            QFont f = it->font();
            f.setStrikeOut(true);
            it->setFont(f);
            it->setForeground(QBrush(Qt::gray));
            it->setToolTip(tr("Gelöscht"));
        }
    }
    // ggf. Auswahl wiederherstellen etc.
}

void MainWindow::refreshPartList(const QVector<Part>& parts) {
    ui->lst_Parts->clear();
    for (const auto& p : parts) {
        auto* it = new QListWidgetItem(p.name);
        it->setData(Qt::UserRole, p.id);
        ui->lst_Parts->addItem(it);
    }
}

void MainWindow::selectPartById(int id) {
    for (int i=0;i<ui->lst_Parts->count();++i) {
        auto* it = ui->lst_Parts->item(i);
        if (it->data(Qt::UserRole).toInt() == id) {
            ui->lst_Parts->setCurrentItem(it);
            break;
        }
    }
}

void MainWindow::showPart(const Part& p) {
    ui->lbl_IDValue->setText(QString::number(p.id));
    ui->lbl_PartNameValue->setText(p.name);
    ui->lbl_ShortDescriptionValue->setText(p.shortDescription);
    ui->lbl_CategoryValue->setText(p.category);
    ui->lbl_SubcategoryValue->setText(p.subcategory);
    ui->lbl_QuantityValue->setText(QString::number(p.quantity));
    ui->lbl_StorageValue->setText(p.storage);
    ui->lbl_StorageDetailsValue->setText(p.storageDetails);
    ui->lbl_SourceValue->setText(p.supplier);
    ui->lbl_AlternativeSourceValue->setText(p.altSupplier);
    ui->lbl_FormatValue->setText(p.format);
    ui->lbl_TypeValue->setText(p.type);
    ui->lbl_PriceValue->setText(QString::number(p.price,'f',2) + " €");

    // Texteinträge mit Links
    GuiUtils::setLabelWithOptionalLink(ui->lbl_ManufacturerValue, p.manufacturer, p.manufacturerLink);
    GuiUtils::setLabelWithOptionalLink(ui->lbl_SourceValue, p.supplier, p.supplierLink);
    GuiUtils::setLabelWithOptionalLink(ui->lbl_AlternativeSourceValue, p.altSupplier, p.altSupplierLink);

    // Hashtags (nur Text)
    ui->lbl_HashtagsValues->setText(p.hashtags.join(", "));
    ui->txt_Description->setPlainText(p.description);

    //Bild laden, auf Größe des Labels skalieren und dann dort anzeigen
    QPixmap partPixmap(p.imagePath);
    QPixmap scaledPartPixmap = partPixmap.scaled(ui->lbl_Image->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    //ui->lbl_Image->setScaledContents(true);
    ui->lbl_Image->setPixmap(scaledPartPixmap);

    // Dateienliste rechts
    ui->lst_Files->clear();
    for (const auto& path : p.localFiles) {
        auto fi = QFileInfo(path);
        auto* it = new QListWidgetItem(fi.exists() ? fi.fileName() : path);
        it->setToolTip(path);
        it->setData(Qt::UserRole, path);
        ui->lst_Files->addItem(it);
    }

    // Bilderstreifen (aktuell: Pfade -> Thumbnails)
    m_imagesModel->clear();
    const QStringList imgs = !p.images.isEmpty() ? p.images : (p.imagePath.isEmpty() ? QStringList{} : QStringList{p.imagePath});
    for (const auto& img : imgs) {
        QStandardItem* si = new QStandardItem(QIcon(img), QFileInfo(img).fileName());
        si->setToolTip(img);
        m_imagesModel->appendRow(si);
    }
}

void MainWindow::onFileActivated(QListWidgetItem* item) {
    const QString path = item->data(Qt::UserRole).toString();
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void MainWindow::openSettingsDialog() {
    SettingsDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;
}

void MainWindow::addNewPart() {
    NewPartDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;

    // --- Repository vorhanden? ---
    if (!m_repo) {
        QMessageBox::critical(this, tr("Fehler"), tr("Repository ist nicht initialisiert."));
        return;
    }

    // --- Helfer für sicheres Lesen aus dem Dialog ---
    auto getLE = [&](const char* name) -> QLineEdit* {
        auto w = dlg.findChild<QLineEdit*>(name);
        if (!w) qWarning() << "QLineEdit not found:" << name;
        return w;
    };
    auto getPTE = [&](const char* name) -> QTextEdit* {
        auto w = dlg.findChild<QTextEdit*>(name);
        if (!w) qWarning() << "QPlainTextEdit not found:" << name;
        return w;
    };
    auto getSB = [&](const char* name) -> QSpinBox* {
        auto w = dlg.findChild<QSpinBox*>(name);
        if (!w) qWarning() << "QSpinBox not found:" << name;
        return w;
    };

    // --- Felder lesen (null-sicher) ---
    Part p;
    if (auto w = getLE("edt_PartName"))                 p.name                 = w->text();
    if (auto w = getLE("edt_ShortDescription"))         p.shortDescription     = w->text();
    if (auto w = getLE("edt_Category"))                 p.category             = w->text();
    if (auto w = getLE("edt_Subcategory"))              p.subcategory          = w->text();
    if (auto w = getPTE("txt_Description"))             p.description          = w->toPlainText();
    if (auto w = getLE("edt_Source"))                   p.supplier             = w->text();
    if (auto w = getLE("edt_AlternativeSource"))        p.altSupplier          = w->text();
    if (auto w = getLE("edt_Manufacturer"))             p.manufacturer         = w->text();
    if (auto w = getLE("edt_StorageLocation"))          p.storage              = w->text();
    if (auto w = getLE("edt_StorageLocationDetails"))   p.storageDetails       = w->text();
    if (auto w = getLE("edt_Type"))                     p.type                 = w->text();
    if (auto w = getLE("edt_Format"))                   p.format               = w->text();
    if (auto w = getLE("edt_SupplierLink"))             p.supplierLink         = w->text();
    if (auto w = getLE("edt_AltSupplierLink"))          p.altSupplierLink      = w->text();
    if (auto w = getLE("edt_manufacturerLink"))         p.manufacturerLink     = w->text();


    if (auto w = getLE("edt_PartFilesFolder")) {
        const QString folderPath = w->text().trimmed();
        if (!folderPath.isEmpty()) {
            QDir d(folderPath);
            if (d.exists()) p.localFiles << folderPath;
        }
    };

    // Bild aus Dialog-Property (wird gesetzt, wenn du die Bildauswahl-Logik ergänzt)
    const QString chosenImage = dlg.property("chosenImagePath").toString();
    if (!chosenImage.isEmpty()) p.imagePath = chosenImage;

    if (auto w = getSB("spb_Quantity")) p.quantity = w->value();
    if (auto w = getLE("edt_Price")) {
        bool ok = false;
        p.price = w->text().trimmed().replace(',', '.').toDouble(&ok);
        if (!ok) p.price = 0.0;

        // Minimalvalidierung
        if (p.name.trimmed().isEmpty())
        {
            QMessageBox::warning(this, tr("Eingabe prüfen"), tr("Bitte einen Namen eingeben."));
            return;
        }
    }

    // --- Speichern ---
    const int newId = m_repo->addPart(p);

    // UI aktualisieren
    refillCategories();
    applyFilters();
    selectPartById(newId);
}

void MainWindow::onPartsContextMenuRequested(const QPoint& pos) {
    auto* item = ui->lst_Parts->itemAt(pos);
    if (!item) return;

    const int id = item->data(Qt::UserRole).toInt();
    auto pOpt = m_repo->getPart(id);
    if (!pOpt) return;
    const Part& p = *pOpt;

    QMenu m(this);
    QAction* actEdit = nullptr;
    QAction* actDelete = nullptr;
    QAction* actRestore = nullptr;

    if (p.deleted) {
        // Nur für gelöschte den Punkt "Wiederherstellen" anbieten
        actRestore = m.addAction(tr("Wiederherstellen"));

        // Optional: zur Orientierung anzeigen, aber deaktivieren
        m.addSeparator();
        QAction* a1 = m.addAction(tr("Ändern…"));   a1->setEnabled(false);
        QAction* a2 = m.addAction(tr("Löschen"));   a2->setEnabled(false);
    } else {
        // Normales Menü
        actEdit   = m.addAction(tr("Ändern…"));
        actDelete = m.addAction(tr("Löschen"));
    }

    QAction* chosen = m.exec(ui->lst_Parts->viewport()->mapToGlobal(pos));
    if (!chosen) return;

    if (chosen == actEdit)        editPart(id);
    else if (chosen == actDelete) deletePart(id);
    else if (chosen == actRestore) restorePart(id);
}

void MainWindow::deletePart(int id) {
    if (!m_repo) return;
    if (auto p = m_repo->getPart(id)) {
        Part upd = *p;
        if (upd.deleted) return;        // bereits gelöscht
        upd.deleted = true;             // <— neu, statt Hashtag
        if (!m_repo->updatePart(upd)) {
            QMessageBox::warning(this, tr("Fehler"), tr("Eintrag konnte nicht gelöscht werden."));
            return;
        }
        refillCategories();
        applyFilters();
    }
}

void MainWindow::editPart(int id) {
    if (!m_repo) return;

    auto pOpt = m_repo->getPart(id);
    if (!pOpt) return;
    Part p = *pOpt;

    NewPartDialog dlg(this);
    dlg.setWindowTitle(tr("Bauteil bearbeiten"));

    auto setLE = [&](const char* name, const QString& v){
        if (auto w = dlg.findChild<QLineEdit*>(name)) w->setText(v);
    };
    auto setPTE = [&](const char* name, const QString& v){
        if (auto w = dlg.findChild<QTextEdit*>(name)) w->setPlainText(v);
    };
    auto setSB = [&](const char* name, int v){
        if (auto w = dlg.findChild<QSpinBox*>(name)) w->setValue(v);
    };

    // Felder vorbefüllen (Namen aus newpartdialog.ui)
    setLE("edt_PartName",               p.name);
    setLE("edt_ShortDescription",       p.shortDescription);
    setLE("edt_Category",               p.category);
    setLE("edt_Subcategory",            p.subcategory);
    setPTE("txt_Description",           p.description);
    setLE("edt_Source",                 p.supplier);
    setLE("edt_SourceLink",             p.supplierLink);
    setLE("edt_AlternativeSource",      p.altSupplier);
    setLE("edt_AlternativeSourceLink",  p.altSupplierLink);
    setLE("edt_Manufacturer",           p.manufacturer);
    setLE("edt_ManufacturerLink",       p.manufacturerLink);
    setLE("edt_StorageLocation",        p.storage);
    setLE("edt_StorageLocationDetails", p.storageDetails);
    setLE("edt_Format",                 p.format);
    setLE("edt_Type",                   p.type);
    setSB("spb_Quantity",               p.quantity);
    setLE("edt_Price",                  QString::number(p.price));

    if (!p.localFiles.isEmpty()) setLE("edt_PartFilesFolder", p.localFiles.first());

    // Bild (Vorauswahl) optional anzeigen
    if (!p.imagePath.isEmpty()) {
        dlg.setProperty("chosenImagePath", p.imagePath);
        if (auto imgLbl = dlg.findChild<QLabel*>("lbl_ImagePreview")) {
            QPixmap pm(p.imagePath);
            if (!pm.isNull()) imgLbl->setPixmap(pm.scaled(128,128,Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    }

    if (dlg.exec() != QDialog::Accepted) return;

    // Werte zurücklesen (gleiches Muster wie in addNewPart)
    auto getLE = [&](const char* name) -> QLineEdit* { return dlg.findChild<QLineEdit*>(name); };
    auto getPTE= [&](const char* name) -> QTextEdit* { return dlg.findChild<QTextEdit*>(name); };
    auto getSB = [&](const char* name) -> QSpinBox*  { return dlg.findChild<QSpinBox*>(name); };

    if (auto w = getLE("edt_PartName"))               p.name = w->text().trimmed();
    if (auto w = getLE("edt_ShortDescription"))       p.shortDescription = w->text().trimmed();
    if (auto w = getLE("edt_Category"))               p.category = w->text().trimmed();
    if (auto w = getLE("edt_Subcategory"))            p.subcategory = w->text().trimmed();
    if (auto w = getPTE("txt_Description"))           p.description = w->toPlainText().trimmed();
    if (auto w = getLE("edt_Source"))                 p.supplier = w->text().trimmed();
    if (auto w = getLE("edt_SourceLink"))             p.supplierLink = w->text().trimmed();
    if (auto w = getLE("edt_AlternativeSource"))      p.altSupplier = w->text().trimmed();
    if (auto w = getLE("edt_AlternativeSourceLink"))  p.altSupplierLink = w->text().trimmed();
    if (auto w = getLE("edt_Manufacturer"))           p.manufacturer = w->text().trimmed();
    if (auto w = getLE("edt_ManufacturerLink"))       p.manufacturerLink = w->text().trimmed();
    if (auto w = getLE("edt_StorageLocation"))        p.storage = w->text().trimmed();
    if (auto w = getLE("edt_StorageLocationDetails")) p.storageDetails = w->text().trimmed();
    if (auto w = getLE("edt_Format"))                 p.format = w->text().trimmed();
    if (auto w = getLE("edt_Type"))                   p.type = w->text().trimmed();

    p.localFiles.clear();
    if (auto w = getLE("edt_PartFilesFolder")) {
        const QString filePath = w->text().trimmed();
        if (!filePath.isEmpty()) p.localFiles << filePath;
    }

    const QString chosenImage = dlg.property("chosenImagePath").toString();
    if (!chosenImage.isEmpty()) p.imagePath = chosenImage;

    if (auto w = getSB("spb_Quantity")) p.quantity = w->value();
    if (auto w = getLE("edt_Price")) {
        bool ok=false; p.price = w->text().trimmed().replace(',', '.').toDouble(&ok);
        if (!ok) p.price = 0.0;
    }

    if (p.name.trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Eingabe prüfen"), tr("Bitte einen Namen eingeben."));
        return;
    }

    if (!m_repo->updatePart(p)) {
        QMessageBox::warning(this, tr("Fehler"), tr("Bauteil konnte nicht gespeichert werden."));
        return;
    }

    // UI aktualisieren und das bearbeitete Teil wieder selektieren
    refillCategories();
    applyFilters();
    selectPartById(p.id);
}

void MainWindow::toggleShowDeletedParts(bool checked) {
    m_showDeletedParts = checked;
    QSettings s;
    s.setValue("ui/showDeletedParts", m_showDeletedParts);
    applyFilters();
}

// Wiederherstellen-Action
void MainWindow::restorePart(int id) {
    if (auto p = m_repo->getPart(id)) {
        Part u = *p; u.deleted = false;
        if (m_repo->updatePart(u)) { applyFilters(); /* selectPartById(u.id); */ }
    }
}
