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
#include <QFileIconProvider>
#include <QMimeDatabase>
#include "listmanagerdialog.h"

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
    connect(ui->act_ListManager, &QAction::triggered, this, &MainWindow::openListManager);
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
    //ui->lbl_HashtagsValues->setText(p.hashtags.join(", ")); //removed, still unused
    ui->txt_Description->setPlainText(p.description);

    //Bild laden, auf Größe des Labels skalieren und dann dort anzeigen
    if(p.imagePath .isEmpty()) {
        ui->lbl_Image->setPixmap(QPixmap()); // leeren
        ui->lbl_Image->setText(tr("Kein Bild"));
        ui->lbl_Image->setAlignment(Qt::AlignCenter);
        //ui->lbl_Image->setScaledContents(false);
        //ui->lbl_Image->setStyleSheet("border: 1px solid gray;"); // optionaler Rahmen
    }
    else
    {
        QPixmap partPixmap(p.imagePath);
        QPixmap scaledPartPixmap = partPixmap.scaled(ui->lbl_Image->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        //ui->lbl_Image->setScaledContents(true);
        ui->lbl_Image->setPixmap(scaledPartPixmap);
    }


    // Dateienliste rechts (alt, zeigt nur Ordner)
    /*
    ui->lst_Files->clear();
    for (const auto& path : p.localFiles) {
        auto fi = QFileInfo(path);
        auto* it = new QListWidgetItem(fi.exists() ? fi.fileName() : path);
        it->setToolTip(path);
        it->setData(Qt::UserRole, path);
        ui->lst_Files->addItem(it);
    }
    */

    /*
    // Dateienliste rechts
    ui->lst_Files->clear();

    for (const auto& path : p.localFiles) {
        QFileInfo fi(path);

        if (fi.exists() && fi.isDir()) {
            // If this localFiles entry is a directory, list all files inside (non-recursive)
            QDir dir(fi.absoluteFilePath());
            const auto entries = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
            if (entries.isEmpty()) {
                // show an inert "empty folder" entry so the user can open the folder if desired
                auto* it = new QListWidgetItem(tr("[Ordner leer] %1").arg(fi.fileName()));
                it->setToolTip(fi.absoluteFilePath());
                it->setData(Qt::UserRole, fi.absoluteFilePath());
                it->setData(Qt::UserRole + 1, true); // mark as directory item (optional)
                ui->lst_Files->addItem(it);
            } else {
                for (const auto& efi : entries) {
                    auto* it = new QListWidgetItem(efi.fileName());
                    it->setToolTip(efi.absoluteFilePath());
                    it->setData(Qt::UserRole, efi.absoluteFilePath()); // full path for activation
                    ui->lst_Files->addItem(it);
                }
            }
        } else {
            // treat as file (existing or not) - display file name if possible, otherwise show path
            const QString label = fi.exists() ? fi.fileName() : path;
            auto* it = new QListWidgetItem(label);
            it->setToolTip(path);
            it->setData(Qt::UserRole, QFileInfo(path).absoluteFilePath());
            ui->lst_Files->addItem(it);
        }
    }
*/

    // Dateiliste rechts mit Icons
    // Dateienliste rechts (with icons)
    ui->lst_Files->clear();

    QFileIconProvider iconProvider;
    QMimeDatabase mimeDb;

    auto makeIconForFileInfo = [&](const QFileInfo &fi) -> QIcon {
        // Try mime-theme icon first (works well on Linux with icon themes)
        if (fi.exists() && fi.isFile()) {
            const QMimeType mt = mimeDb.mimeTypeForFile(fi);
            const QString iconName = mt.iconName();
            if (!iconName.isEmpty()) {
                QIcon ic = QIcon::fromTheme(iconName);
                if (!ic.isNull()) return ic;
            }
        }

        // Fall back to the platform file icon provider
        QIcon ic = iconProvider.icon(fi);
        if (!ic.isNull()) return ic;

        // Final fallback: standard file/folder icon from the widget style
        if (fi.exists() && fi.isDir())
            return this->style()->standardIcon(QStyle::SP_DirIcon);
        return this->style()->standardIcon(QStyle::SP_FileIcon);
    };

    for (const auto& path : p.localFiles) {
        QFileInfo fi(path);

        if (fi.exists() && fi.isDir()) {
            // If this localFiles entry is a directory, list all files inside (non-recursive)
            QDir dir(fi.absoluteFilePath());
            const auto entries = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
            if (entries.isEmpty()) {
                // show an inert "empty folder" entry so the user can open the folder if desired
                auto* it = new QListWidgetItem(tr("[Ordner leer] %1").arg(fi.fileName()));
                it->setToolTip(fi.absoluteFilePath());
                it->setData(Qt::UserRole, fi.absoluteFilePath());
                it->setIcon(makeIconForFileInfo(fi)); // folder icon
                it->setData(Qt::UserRole + 1, true); // mark as directory item (optional)
                ui->lst_Files->addItem(it);
            } else {
                for (const auto& efi : entries) {
                    auto* it = new QListWidgetItem(efi.fileName());
                    it->setToolTip(efi.absoluteFilePath());
                    it->setData(Qt::UserRole, efi.absoluteFilePath()); // full path for activation
                    it->setIcon(makeIconForFileInfo(efi));
                    ui->lst_Files->addItem(it);
                }
            }
        } else {
            // treat as file (existing or not) - display file name if possible, otherwise show path
            const QString label = fi.exists() ? fi.fileName() : path;
            auto* it = new QListWidgetItem(label);
            it->setToolTip(path);
            it->setData(Qt::UserRole, QFileInfo(path).absoluteFilePath());
            it->setIcon(makeIconForFileInfo(fi));
            ui->lst_Files->addItem(it);
        }
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

void MainWindow::openListManager() {

    if (!m_repo) return;

    // 1) Laden
    JsonPartRepository::PresetsMap presets;
    if (!m_repo->loadPresets(presets)) {
        QMessageBox::warning(this, tr("Hinweis"),
                             tr("Vorgabelisten konnten nicht geladen werden. Es wird leer gestartet."));
    }

    // 2) Dialog öffnen
    ListManagerDialog dlg(this);
    dlg.setWindowTitle(tr("Vorgabelisten verwalten"));
    dlg.setPresets(presets, /*currentKey=*/QString()); // optional den Start-Key setzen

    // 3) Änderungen übernehmen (bei OK)
    if (dlg.exec() == QDialog::Accepted) {
        const auto newPresets = dlg.presets();
        if (!m_repo->savePresets(newPresets)) {
            QMessageBox::critical(this, tr("Fehler"),
                                  tr("Vorgabelisten konnten nicht gespeichert werden."));
            return;
        }

        // 4) (Optional) ComboBoxen der UI aktualisieren
        //    -> siehe nächsten Schritt
        // refreshPresetBackedCombos(newPresets);
    }
}

void MainWindow::addNewPart() {
    NewPartDialog dlg(m_repo, this);
    //if (dlg.exec() != QDialog::Accepted) return;

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
    auto getCBB = [&](const char* name) -> QComboBox* {
        auto w = dlg.findChild<QComboBox*>(name);
        if (!w) qWarning() << "QComboBox not found:" << name;
        return w;
    };

    // NexPart Feature als Lambda-Funktion
    auto saveFromDialog = [&]() -> bool
    {
        // --- Felder lesen (null-sicher) ---
        Part p;
        if (auto w = getLE("edt_PartName"))                 p.name                 = w->text();
        if (auto w = getLE("edt_ShortDescription"))         p.shortDescription     = w->text();
        if (auto w = getCBB("cbb_Category"))                p.category             = w->currentText();
        if (auto w = getCBB("cbb_SubCategory"))             p.subcategory          = w->currentText();
        if (auto w = getPTE("txt_Description"))             p.description          = w->toPlainText();
        if (auto w = getCBB("cbb_Source"))                  p.supplier             = w->currentText();
        if (auto w = getCBB("cbb_AlternativeSource"))       p.altSupplier          = w->currentText();
        if (auto w = getCBB("cbb_Manufacturer"))            p.manufacturer         = w->currentText();
        if (auto w = getCBB("cbb_StorageLocation"))         p.storage              = w->currentText();
        if (auto w = getLE("edt_StorageLocationDetails"))   p.storageDetails       = w->text();
        if (auto w = getCBB("cbb_Type"))                    p.type                 = w->currentText();
        if (auto w = getCBB("cbb_Format"))                  p.format               = w->currentText();
        if (auto w = getLE("edt_SourceLink"))               p.supplierLink         = w->text();
        if (auto w = getLE("edt_AlternativeSourceLink"))    p.altSupplierLink      = w->text();
        if (auto w = getLE("edt_ManufacturerLink"))         p.manufacturerLink     = w->text();


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
                return false;
            }
        }

        // --- Speichern ---
        const int newId = m_repo->addPart(p);

        // UI aktualisieren
        refillCategories();
        applyFilters();
        selectPartById(newId);
        return true;
    };

    // --- NEU: Reaktion auf „Nächstes Teil“ ---
    connect(&dlg, &NewPartDialog::nextPartRequested, this, [&](){
        if (saveFromDialog()) {
            // Für nächsten Eintrag leeren, Dialog bleibt offen
            dlg.resetInputs();
            // Fokus wieder auf den Namen setzen
            if (auto *w = dlg.findChild<QLineEdit*>("edt_PartName")) w->setFocus();
            qDebug() << "next Part requested";
        }
    });

    // --- Wie bisher: OK/Abbrechen modal behandeln ---
    if (dlg.exec() == QDialog::Accepted) {
        // Nur wenn OK gedrückt wurde, einmal speichern und schließen:
        saveFromDialog();
    }
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
    QAction* actDeleteFinal = nullptr;
    QAction* actRestore = nullptr;

    if (p.deleted) {
        // Nur für gelöschte den Punkt "Wiederherstellen" anbieten
        actRestore = m.addAction(tr("Wiederherstellen"));
        actDeleteFinal = m.addAction(tr("Endgültig löschen"));

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
    else if (chosen == actDeleteFinal) deletePartFinal(id);
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

void MainWindow::deletePartFinal(int id) {

    if (!m_repo) return;

    // Sicherheitsabfrage
    auto btn = QMessageBox::warning(
        this,
        tr("Endgültig löschen"),
        tr("Dieses Bauteil wird dauerhaft aus der Datenbank entfernt.\n"
           "Dieser Vorgang kann nicht rückgängig gemacht werden.\n\n"
           "Möchtest du fortfahren?"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
        );
    if (btn != QMessageBox::Yes) return;

    // Bauteil wirklich löschen
    m_repo->removePart(id);

    // UI aktualisieren
    refillCategories();
    applyFilters();
}

void MainWindow::editPart(int id) {
    if (!m_repo) return;

    auto pOpt = m_repo->getPart(id);
    if (!pOpt) return;
    Part p = *pOpt;

    NewPartDialog dlg(m_repo, this);
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
    auto setCBB = [&](const char* name, const QString& v){
        if (auto w = dlg.findChild<QComboBox*>(name)) {
            int idx = w->findText(v);
            if (idx >= 0) w->setCurrentIndex(idx);
            else w->setCurrentText(v);
        }
    };

    // Felder vorbefüllen (Namen aus newpartdialog.ui)
    setLE("edt_PartName",               p.name);
    setLE("edt_ShortDescription",       p.shortDescription);
    setCBB("cbb_Category",              p.category);
    setCBB("cbb_SubCategory",           p.subcategory);
    setPTE("txt_Description",           p.description);
    setCBB("cbb_Source",                p.supplier);
    setLE("edt_SourceLink",             p.supplierLink);
    setCBB("cbb_AlternativeSource",     p.altSupplier);
    setLE("edt_AlternativeSourceLink",  p.altSupplierLink);
    setCBB("cbb_Manufacturer",          p.manufacturer);
    setLE("edt_ManufacturerLink",       p.manufacturerLink);
    setCBB("cbb_StorageLocation",       p.storage);
    setLE("edt_StorageLocationDetails", p.storageDetails);
    setCBB("cbb_Format",                p.format);
    setCBB("cbb_Type",                  p.type);
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
    auto getCBB = [&](const char* name) -> QComboBox* { return dlg.findChild<QComboBox*>(name); };

    if (auto w = getLE("edt_PartName"))               p.name = w->text().trimmed();
    if (auto w = getLE("edt_ShortDescription"))       p.shortDescription = w->text().trimmed();
    if (auto w = getCBB("cbb_Category"))              p.category = w->currentText().trimmed();
    if (auto w = getCBB("cbb_SubCategory"))           p.subcategory = w->currentText().trimmed();
    if (auto w = getPTE("txt_Description"))           p.description = w->toPlainText().trimmed();
    if (auto w = getCBB("cbb_Source"))                p.supplier = w->currentText().trimmed();
    if (auto w = getLE("edt_SourceLink"))             p.supplierLink = w->text().trimmed();
    if (auto w = getCBB("cbb_AlternativeSource"))     p.altSupplier = w->currentText().trimmed();
    if (auto w = getLE("edt_AlternativeSourceLink"))  p.altSupplierLink = w->text().trimmed();
    if (auto w = getCBB("cbb_Manufacturer"))          p.manufacturer = w->currentText().trimmed();
    if (auto w = getLE("edt_ManufacturerLink"))       p.manufacturerLink = w->text().trimmed();
    if (auto w = getCBB("cbb_StorageLocation"))       p.storage = w->currentText().trimmed();
    if (auto w = getLE("edt_StorageLocationDetails")) p.storageDetails = w->text().trimmed();
    if (auto w = getCBB("cbb_Format"))                p.format = w->currentText().trimmed();
    if (auto w = getCBB("cbb_Type"))                  p.type = w->currentText().trimmed();

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

// Beispiel: Menü-Action „Vorgabelisten…“
void MainWindow::on_act_ManagePresets_triggered()
{

}
