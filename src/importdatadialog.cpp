#include "importdatadialog.h"
#include "ui_importdatadialog.h"
#include "part.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QFile>
#include <QStandardItemModel>
#include <QTableView>
#include <QListWidget>
#include <QCheckBox>
#include <QSpinBox>
#include <QComboBox>
#include <QLineEdit>
#include <QMessageBox>
#include <QTextStream>
#include <QTableWidget>
#include <QMessageBox>
#include <QRegularExpression>
#include <QPlainTextEdit>
#include <QProgressBar>



/*
ImportDataDialog::ImportDataDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ImportDataDialog)
 */
    ImportDataDialog::ImportDataDialog(JsonPartRepository* repo, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ImportDataDialog)
    , m_repo(repo)
{
    ui->setupUi(this);
    this->setAttribute(Qt::WA_DeleteOnClose, false); // Behebt ein Problem mit dem Crash der Anwendung aufgrund von Double-Delete

    prepareUI();                 // UI-Elemente einrichten
    hookUpSignals();             // Signale und Slots verbinden
    hookUpMappingTableSignals(); // Sonderfunktion für das QTableWidget da dies selbst kein onChange-Event hat
}

// Destruktor
ImportDataDialog::~ImportDataDialog()
{
    qDebug() << "ImportDataDialog destructor";

    // onChange-Events des Mapping-TableWidgets deaktivieren, damit der Destruktor arbeiten kann
    auto tbl = findChild<QTableWidget*>("tbl_Mapping");
    if (tbl) {
        auto m = tbl->model();
        if (m) {
            m->blockSignals(true);
        }
    }

    delete ui;
}

void ImportDataDialog::prepareUI()
{
    auto lstPart = findChild<QListWidget*>("lst_PartFields");
    if (lstPart) {
        lstPart->clear();
        lstPart->addItems(partFieldNames());
    }
}

// Alle Signale und Slots verbinden
void ImportDataDialog::hookUpSignals()
{
    auto btnBrowse = findChild<QPushButton*>("btn_BrowseCsv");
    if (btnBrowse) {
        connect(btnBrowse, &QPushButton::clicked, this, &ImportDataDialog::browsCsvFile);
    }

    // Preview-Button erstmal deaktivieren, bis eine Datei gewählt wurde
    if (auto btnPreview = findChild<QPushButton*>("btn_LoadPreview")) {
        btnPreview->setEnabled(false);
    }

    // Vorschau-Button
    if (auto btnPreview = findChild<QPushButton*>("btn_LoadPreview")) {
        connect(btnPreview, &QPushButton::clicked, this, &ImportDataDialog::loadPreview);
    }

    // Für das Mapping

    // Feld zuornen
    auto btnMap = findChild<QPushButton*>("btn_MapToField");
    if (btnMap) {
        connect(btnMap, &QPushButton::clicked, this, &ImportDataDialog::mapSelected);
    }

    // Zuordnung aufheben
    auto btnUnmap = findChild<QPushButton*>("btn_Unmap");
    if (btnUnmap) {
        connect(btnUnmap, &QPushButton::clicked, this, &ImportDataDialog::unmapSelected);
    }

    // Zuordnungen löschen
    auto btnClear = findChild<QPushButton*>("btn_ClearMapping");
    if (btnClear) {
        connect(btnClear, &QPushButton::clicked, this, &ImportDataDialog::clearMapping);
    }

    // Doppelklick als "zuweisen" verwenden
    auto lstCsv = findChild<QListWidget*>("lst_CsvColumns");
    if (lstCsv) {
        connect(lstCsv, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem*) {
            this->mapSelected();
        });
    }
    auto lstPart = findChild<QListWidget*>("lst_PartFields");
    if (lstPart) {
        connect(lstPart, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem*) {

            auto lstCsv = this->findChild<QListWidget*>("lst_CsvColumns");
            if (!lstCsv) {
                return;
            }

            if (!lstCsv->currentItem()) {
                return;
            }

            this->mapSelected();
        });
    }

    // Importvorgng starten
    auto btnStart = findChild<QPushButton*>("btn_StartImport");
    if (btnStart) {
        connect(btnStart, &QPushButton::clicked, this, &ImportDataDialog::startImport);
    }

    // Dialog schließen
    auto btnClose = findChild<QPushButton*>("btn_Close");
    if (!btnClose) {
        return;
    }

    connect(btnClose, &QPushButton::clicked, this, [this]() {
        if (m_importRunning) {
            // während Import nichts tun
            return;
        }

        this->accept(); // Dialog schließen
    });

}

// Status eines CSV-Datensatzes
enum class CsvRecordStatus
{
    Ok,
    EndOfFile,
    IncompleteAtEof
};

// Prüft, ob ein CSV-Datensatz komplett ist (Quotes geschlossen)
static bool isCsvRecordComplete(const QString& text)
{
    bool inQuotes = false;

    for (int i = 0; i < text.size(); ++i) {
        const QChar c = text.at(i);

        if (c == '"') {
            if (inQuotes) {
                if (i + 1 < text.size() && text.at(i + 1) == '"') {
                    ++i;
                } else {
                    inQuotes = false;
                }
            } else {
                inQuotes = true;
            }
        }
    }

    return !inQuotes;
}

// Liest so lange weiter, bis Quotes wieder geschlossen sind.
// Liefert IncompleteAtEof, wenn EOF erreicht wird, aber Quotes noch offen sind.
static CsvRecordStatus readCsvRecordChecked(QTextStream& in, QString& outRecord, int& physicalLineNo)
{
    outRecord.clear();

    if (in.atEnd()) {
        return CsvRecordStatus::EndOfFile;
    }

    outRecord = in.readLine();
    ++physicalLineNo;

    while (!isCsvRecordComplete(outRecord)) {
        if (in.atEnd()) {
            return CsvRecordStatus::IncompleteAtEof;
        }

        outRecord += "\n";
        outRecord += in.readLine();
        ++physicalLineNo;
    }

    return CsvRecordStatus::Ok;
}


// Datei-öffnen Dialog zeigen um eine CSV-Datei auszuwählen
void ImportDataDialog::browsCsvFile()
{
    // Settings laden, um letzten Ordner zu merken
    QSettings s("Partorium", "Partorium");

    // Startordner: zuletzt verwendeter Ordner, sonst defaultPartsFilesFolder, sonst Home
    QString startDir = s.value("import/lastCsvDir").toString();
    if (startDir.isEmpty())
        startDir = s.value("defaultPartsFilesFolder").toString();
    if (startDir.isEmpty())
        startDir = QDir::homePath();

    const QString filter = tr("CSV-Dateien (*.csv *.CSV *.txt);;Alle Dateien (*.*)");
    const QString file = QFileDialog::getOpenFileName(this, tr("CSV-Datei auswählen"), startDir, filter);

    // Bei Fehler abbrechen
    if (file.isEmpty())
        return;

    // UI aktualisieren
    if (auto edt = findChild<QLineEdit*>("edt_ImportFileName")) {
        edt->setText(QFileInfo(file).filePath());
    }

    // Delimiter auf Standardwert setzen
    // if (auto cmb = findChild<QComboBox*>("cmb_Delimiter")) {
    //     if (cmb->currentText().isEmpty())
    //         cmb->setCurrentText(";");
    // }

    // Dialog-intern merken (für späteren Import/Preview)
    setProperty("chosenCsvPath", file);

    // Letzten Ordner speichern
    s.setValue("import/lastCsvDir", QFileInfo(file).absolutePath());

    // Preview-Button aktivieren
    if (auto btnPreview = findChild<QPushButton*>("btn_LoadPreview")) {
        btnPreview->setEnabled(true);
    }
}

// Lädt eine Vorschau der CSV-Datei und zeigt sie im Dialog an
QString ImportDataDialog::chosenCsvPath() const
{
    const QString p = property("chosenCsvPath").toString();
    if (!p.isEmpty()) return p;

    if (auto edt = findChild<QLineEdit*>("edt_ImportFileName"))
        return edt->text().trimmed();

    return {};
}

// Aktueller Delimiter aus UI bestimmen
QChar ImportDataDialog::currentDelimiter() const
{
    if (auto cmb = findChild<QComboBox*>("cbb_Delimiter")) {
        QString t = cmb->currentText().trimmed();

        if (t == "\\t" || t.contains("Tab", Qt::CaseInsensitive))
        {
            qDebug() << "Tab was selected as Delimiter, returning special char for Tab";
            return '\t';
        }

        if (!t.isEmpty())
        {
            qDebug() << "Returning delimiter:" << t.at(0);
            return t.at(0);
        }
    }

    // Fallback
    qDebug() << "No delimiter selected, returning default ;";
    return ';';
}

// Aktuelles Encoding aus UI bestimmen
QString ImportDataDialog::currentEncodingName() const
{
    if (auto cmb = findChild<QComboBox*>("cbb_Encoding")) {
        const QString t = cmb->currentText().trimmed();
        if (!t.isEmpty()) return t;
    }
    return "UTF-8";
}

// Header vorhanden?
bool ImportDataDialog::hasHeader() const
{
    if (auto chb = findChild<QCheckBox*>("chb_HasHeader"))
        return chb->isChecked();
    return true;
}

// Anzahl zu überspringender Zeilen
int ImportDataDialog::skipRows() const
{
    if (auto spn = findChild<QSpinBox*>("spn_SkipRows"))
        return spn->value();
    return 0;
}

// Einfacher CSV-Parser: unterstützt Quotes "..." und escaped Quotes "" innerhalb von Quotes.
QStringList ImportDataDialog::parseCsvLine(const QString& line, QChar delimiter)
{
    QStringList out;
    QString cur;
    bool inQuotes = false;

    for (int i = 0; i < line.size(); ++i) {
        const QChar c = line.at(i);

        if (inQuotes) {
            if (c == '"') {
                // "" -> "
                if (i + 1 < line.size() && line.at(i + 1) == '"') {
                    cur += '"';
                    ++i;
                } else {
                    inQuotes = false;
                }
            } else {
                cur += c;
            }
        } else {
            if (c == '"') {
                inQuotes = true;
            } else if (c == delimiter) {
                out << cur;
                cur.clear();
            } else {
                cur += c;
            }
        }
    }
    out << cur;
    return out;
}

// Lädt eine Vorschau der CSV-Datei und zeigt sie im Dialog an
void ImportDataDialog::loadPreview()
{
    const QString path = chosenCsvPath();
    if (path.isEmpty()) {
        QMessageBox::warning(this, tr("Keine Datei"), tr("Bitte zuerst eine CSV-Datei auswählen."));
        return;
    }

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this,
                              tr("Datei kann nicht geöffnet werden"),
                              tr("Die Datei konnte nicht geöffnet werden:\n%1").arg(path));
        return;
    }

    QTextStream in(&f);
/*
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    in.setCodec(currentEncodingName().toUtf8().constData());
#else
    in.setEncoding(QStringConverter::encodingForName(currentEncodingName().toUtf8()));
#endif
*/
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    in.setCodec(currentEncodingName().toUtf8().constData());
#else
    auto optEnc = QStringConverter::encodingForName(currentEncodingName().toUtf8());
    if (optEnc.has_value()) {
        in.setEncoding(*optEnc);
    } else {
        // Fallback, z. B. UTF-8
        in.setEncoding(QStringConverter::Utf8);
    }
#endif

    const QChar delim = currentDelimiter();
    const int skip = qMax(0, skipRows());
    const bool header = hasHeader();
    const bool skipEmpty = skipEmptyRows();

    auto lstCols = findChild<QListWidget*>("lst_CsvColumns");
    auto view = findChild<QTableView*>("tbl_Preview");
    auto lblInfo = findChild<QLabel*>("lbl_PreviewInfo");
    auto log = findChild<QPlainTextEdit*>("txt_Log");

    if (lstCols) {
        lstCols->clear();
    }

    if (view) {
        view->setModel(nullptr);
    }

    if (lblInfo) {
        lblInfo->setText(tr("Vorschau wird geladen..."));
    }

    if (log) {
        log->appendPlainText(tr("Vorschau laden: %1").arg(path));
    }

    QStringList headerCols;
    QVector<QStringList> rows;

    int physicalLineNo = 0;     // zählt physische Textzeilen
    int recordNo = 0;           // zählt Records (für skip/header)
    bool headerConsumed = false;

    while (!in.atEnd()) {

        QString record;
        const CsvRecordStatus st = readCsvRecordChecked(in, record, physicalLineNo);

        if (st == CsvRecordStatus::EndOfFile) {
            break;
        }

        if (st == CsvRecordStatus::IncompleteAtEof) {
            if (log) {
                log->appendPlainText(
                    tr("CSV inkompatibel: Offene Anführungszeichen bis zum Dateiende (EOF). Vorschau abgebrochen.")
                    );
            }
            QMessageBox::warning(this,
                                 tr("CSV inkompatibel"),
                                 tr("Die CSV-Datei scheint nicht korrekt formatiert zu sein.\n"
                                    "Ein Feld enthält vermutlich Zeilenumbrüche, aber die Anführungszeichen sind nicht korrekt gesetzt.\n"
                                    "Vorschau wurde abgebrochen."));
            break;
        }

        if (skipEmpty) {
            if (record.trimmed().isEmpty()) {
                continue;
            }
        }

        ++recordNo;

        if (recordNo <= skip) {
            continue;
        }

        if (header && !headerConsumed) {
            headerCols = parseCsvLine(record, delim);
            headerConsumed = true;
            continue;
        }

        const QStringList cols = parseCsvLine(record, delim);
        rows.push_back(cols);

        if (rows.size() >= 25) {
            break;
        }
    }

    // Wenn kein Header vorhanden oder Header leer: Fallback "Spalte 1..n"
    int maxCols = headerCols.size();
    for (const auto& r : rows) {
        if (r.size() > maxCols) {
            maxCols = r.size();
        }
    }

    if (headerCols.isEmpty()) {
        for (int i = 0; i < maxCols; ++i) {
            headerCols << tr("Spalte %1").arg(i + 1);
        }
    }

    // 1) CSV-Spaltenliste füllen
    if (lstCols) {
        lstCols->clear();
        lstCols->addItems(headerCols);
    }

    // 2) Preview-Table füllen (QTableView + Model)
    if (view) {
        auto* model = new QStandardItemModel(view);

        model->setColumnCount(headerCols.size());
        model->setHorizontalHeaderLabels(headerCols);

        for (int r = 0; r < rows.size(); ++r) {
            model->insertRow(r);

            const auto& cols = rows[r];
            for (int c = 0; c < headerCols.size(); ++c) {
                const QString value = (c >= 0 && c < cols.size()) ? cols[c] : QString();
                model->setItem(r, c, new QStandardItem(value));
            }
        }

        view->setModel(model);
        view->resizeColumnsToContents();
    }

    // Info-Label aktualisieren
    if (lblInfo) {
        const QString delimText = (delim == '\t') ? "\\t" : QString(delim);
        lblInfo->setText(tr("Delimiter: '%1' | Vorschau: %2 Zeilen, %3 Spalten | Header: %4 | SkipRows: %5")
                             .arg(delimText)
                             .arg(rows.size())
                             .arg(headerCols.size())
                             .arg(header ? tr("Ja") : tr("Nein"))
                             .arg(skip));
    }

    if (log) {
        log->appendPlainText(tr("Vorschau geladen. Spalten: %1, Zeilen: %2")
                                 .arg(headerCols.size())
                                 .arg(rows.size()));
    }
}
/*
void ImportDataDialog::loadPreview()
{
    const QString path = chosenCsvPath();
    if (path.isEmpty()) {
        QMessageBox::warning(this, tr("Keine Datei"), tr("Bitte zuerst eine CSV-Datei auswählen."));
        return;
    }

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Datei kann nicht geöffnet werden"),
                              tr("Die Datei konnte nicht geöffnet werden:\n%1").arg(path));
        return;
    }

    QTextStream in(&f);

    // Encoding setzen
    // QStringConverter::encodingForName() hat sich in Qt 6 geändert, QStringConverter::encodingForName()
    // liefert in Qt 6 ein std::optional<QStringConverter::Encoding> zurück.
    // QTextStream::setEncoding() erwartet aber einen reinen QStringConverter::Encoding, nicht ein Optional
    #if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        in.setCodec(currentEncodingName().toUtf8().constData());
    #else
        auto optEnc = QStringConverter::encodingForName(currentEncodingName().toUtf8());
        if (optEnc.has_value()) {
            in.setEncoding(*optEnc);
        } else {
            // Fallback, z. B. UTF-8
            in.setEncoding(QStringConverter::Utf8);
        }
    #endif

    const QChar delim = currentDelimiter();
    const int skip = qMax(0, skipRows());
    const bool header = hasHeader();

    // Zeilen lesen
    QStringList headerCols;
    QVector<QStringList> rows;

    int lineNo = 0;
    while (!in.atEnd()) {
        //QString line = in.readLine();
        QString record;

        // leere Zeilen überspringen
        const bool skipEmpty = skipEmptyRows();
        if (skipEmpty) {
            if (record.trimmed().isEmpty()) {
                continue;
            }
        }


        CsvRecordStatus st = readCsvRecordChecked(in, record, lineNo);

        if (st == CsvRecordStatus::EndOfFile) {
            break;
        }

        if (st == CsvRecordStatus::IncompleteAtEof) {
            // Log/Info (Preview): inkompatible CSV
            break;
        }

        const QStringList cols = parseCsvLine(record, delim);
        ++lineNo;

        // SkipRows
        if (lineNo <= skip)
            continue;

        // Leere Zeilen ignorieren in Preview
        //if (line.trimmed().isEmpty())
        //    continue;

        //const QStringList cols = parseCsvLine(line, delim);

        if (headerCols.isEmpty() && header) {
            headerCols = cols;
            continue;
        }

        rows.push_back(cols);
        if (rows.size() >= 25) // Preview-Limit
            break;
    }

    // Fallback, wenn kein Header vorhanden
    int maxCols = headerCols.size();
    for (const auto& r : rows) maxCols = qMax(maxCols, r.size());
    if (headerCols.isEmpty()) {
        for (int i = 0; i < maxCols; ++i)
            headerCols << tr("Spalte %1").arg(i + 1);
    }

    // 1) CSV-Spaltenliste füllen
    if (auto lst = findChild<QListWidget*>("lst_CsvColumns")) {
        lst->clear();
        lst->addItems(headerCols);
    }

    // 2) Preview-Table füllen (QTableView + Model)
    if (auto view = findChild<QTableView*>("tbl_Preview")) {
        auto *model = new QStandardItemModel(this);
        model->setColumnCount(headerCols.size());
        model->setHorizontalHeaderLabels(headerCols);

        for (int r = 0; r < rows.size(); ++r) {
            model->insertRow(r);
            const auto& cols = rows[r];
            for (int c = 0; c < headerCols.size(); ++c) {
                const QString value = (c < cols.size()) ? cols[c] : QString();
                model->setItem(r, c, new QStandardItem(value));
            }
        }

        view->setModel(model);
        view->resizeColumnsToContents();
    }

    // Optional: Info-Label aktualisieren
    if (auto lbl = findChild<QLabel*>("lbl_PreviewInfo")) {
        lbl->setText(tr("Delimiter: '%1' | Vorschau: %2 Zeilen, %3 Spalten")
                         .arg(delim == '\t' ? "\\t" : QString(delim))
                         .arg(rows.size())
                         .arg(headerCols.size()));
    }
}
*/

bool ImportDataDialog::mappingExistsCsv(const QString& csvCol) const
{
    return m_csvToPart.contains(csvCol);
}

bool ImportDataDialog::mappingExistsPart(const QString& partField) const
{
    return m_csvToPart.values().contains(partField);
}

//
void ImportDataDialog::mapSelected()
{
    auto lstCsv = findChild<QListWidget*>("lst_CsvColumns");
    auto lstPart = findChild<QListWidget*>("lst_PartFields");
    auto tbl = findChild<QTableWidget*>("tbl_Mapping");

    if (!lstCsv || !lstPart || !tbl) {
        return;
    }

    const auto csvItem = lstCsv->currentItem();
    const auto partItem = lstPart->currentItem();

    if (!csvItem || !partItem) {
        QMessageBox::information(this, tr("Zuordnung"), tr("Bitte eine CSV-Spalte und ein Part-Feld auswählen."));
        return;
    }

    const QString csvCol = csvItem->text().trimmed();
    const QString partField = partItem->text().trimmed();

    if (csvCol.isEmpty() || partField.isEmpty()) {
        return;
    }

    // CSV-Spalte darf nur einmal gemappt werden
    if (mappingExistsCsv(csvCol)) {
        QMessageBox::warning(this, tr("Zuordnung"),
                             tr("Die CSV-Spalte \"%1\" ist bereits zugeordnet.").arg(csvCol));
        return;
    }

    // Part-Feld darf nur einmal Ziel sein (sonst Konflikt)
    if (mappingExistsPart(partField)) {
        QMessageBox::warning(this, tr("Zuordnung"),
                             tr("Das Part-Feld \"%1\" ist bereits einer anderen CSV-Spalte zugeordnet.").arg(partField));
        return;
    }

    m_csvToPart.insert(csvCol, partField);
    refreshMappingTable();
}

// Zuordnung der selektierten CSV-Spalte aufheben
void ImportDataDialog::unmapSelected()
{
    auto tbl = findChild<QTableWidget*>("tbl_Mapping");
    if (!tbl) {
        return;
    }

    const int row = tbl->currentRow();
    if (row < 0) {
        return;
    }

    auto csvItem = tbl->item(row, 0);
    if (!csvItem) {
        return;
    }

    const QString csvCol = csvItem->text().trimmed();
    if (csvCol.isEmpty()) {
        return;
    }

    m_csvToPart.remove(csvCol);
    refreshMappingTable();
}

// Alle Zuordnungen löschen
void ImportDataDialog::clearMapping()
{
    m_csvToPart.clear();
    refreshMappingTable();
}

// Mapping-Tabelle aktualisieren
void ImportDataDialog::refreshMappingTable()
{
    auto tbl = findChild<QTableWidget*>("tbl_Mapping");
    if (!tbl) {
        return;
    }

    tbl->setRowCount(0); //

    int row = 0;
    for (auto it = m_csvToPart.constBegin(); it != m_csvToPart.constEnd(); ++it) {
        tbl->insertRow(row);

        auto *csvItem = new QTableWidgetItem(it.key());
        auto *partItem = new QTableWidgetItem(it.value());

        // optional: nicht editierbar (zusätzlich zu editTriggers)
        csvItem->setFlags(csvItem->flags() & ~Qt::ItemIsEditable);
        partItem->setFlags(partItem->flags() & ~Qt::ItemIsEditable);

        tbl->setItem(row, 0, csvItem);
        tbl->setItem(row, 1, partItem);

        ++row;
    }

    tbl->resizeColumnsToContents();
}

// Hilfsfunktion: Normalisiert einen String für den Vergleich (Kleinbuchstaben, ohne Leerzeichen, etc.)
static QString normKey(QString s)
{
    s = s.trimmed();
    s = s.toLower();
    s.replace(' ', "");
    s.replace('_', "");
    s.replace('-', "");
    return s;
}

// Hilfsfunktion: List-Wert aus CSV in QStringList umwandeln (Trennzeichen , oder ;)
static QStringList splitListValue(const QString& v, bool doTrim)
{
    QString t = v;

    if (doTrim) {
        t = t.trimmed();
    }

    if (t.isEmpty()) {
        return {};
    }

    QStringList parts = t.split(QRegularExpression("[,;]"), Qt::SkipEmptyParts);

    if (doTrim) {
        for (QString& s : parts) {
            s = s.trimmed();
        }
    }

    return parts;
}

// Einen CSV-Wert in das entsprechende Part-Feld übernehmen
bool ImportDataDialog::applyField(Part& p, const QString& field, const QString& value, QString& errorOut, bool doTrim) const
{
    const QString f = field.trimmed();           // Feldname darf immer getrimmt werden
    QString v = value;

    if (doTrim) {
        v = v.trimmed();
    }

    // Strings
    if (f == "name") { p.name = v; return true; }
    if (f == "shortDescription") { p.shortDescription = v; return true; }
    if (f == "category") { p.category = v; return true; }
    if (f == "subcategory") { p.subcategory = v; return true; }
    if (f == "type") { p.type = v; return true; }
    if (f == "format") { p.format = v; return true; }
    if (f == "description") { p.description = v; return true; }

    if (f == "supplier") { p.supplier = v; return true; }
    if (f == "supplierLink") { p.supplierLink = v; return true; }
    if (f == "altSupplier") { p.altSupplier = v; return true; }
    if (f == "altSupplierLink") { p.altSupplierLink = v; return true; }

    if (f == "manufacturer") { p.manufacturer = v; return true; }
    if (f == "manufacturerLink") { p.manufacturerLink = v; return true; }

    if (f == "storage") { p.storage = v; return true; }
    if (f == "storageDetails") { p.storageDetails = v; return true; }
    if (f == "imagePath") { p.imagePath = v; return true; }

    // Listen
    if (f == "localFiles") { p.localFiles = splitListValue(v, doTrim); return true; }
    if (f == "images") { p.images = splitListValue(v, doTrim); return true; }
    if (f == "hashtags") { p.hashtags = splitListValue(v, doTrim); return true; }

    // Bool
    if (f == "deleted") {
        QString t = v.toLower();
        if (doTrim) { t = t.trimmed(); }
        p.deleted = (t == "1" || t == "true" || t == "yes" || t == "ja");
        return true;
    }

    // Zahlen: fürs Parsen immer trimmen (sonst unnötig fehleranfällig)
    if (f == "quantity") {
        QString t = value.trimmed();
        bool ok = false;
        const int q = t.toInt(&ok);
        if (!ok) {
            errorOut = QString("quantity ist keine Zahl: \"%1\"").arg(value);
            return false;
        }
        p.quantity = q;
        return true;
    }

    if (f == "price") {
        QString t = value.trimmed();
        t.replace(',', '.');
        bool ok = false;
        const double d = t.toDouble(&ok);
        if (!ok) {
            errorOut = QString("price ist keine Zahl: \"%1\"").arg(value);
            return false;
        }
        p.price = d;
        return true;
    }

    errorOut = QString("Unbekanntes Feld: %1").arg(field);
    return false;
}

// Importvorgang durchführen
void ImportDataDialog::startImport()
{
    // Schließen-Button und Progress-Label vorbereiten
    m_importRunning = true;
    m_importFinished = false;
    updateCloseButtonState();

    // Label neben dem Progress-Bar aktualisieren
    auto progressLabel = findChild<QLabel*>("lbl_ImportStatus");
    if(progressLabel)
    {
        progressLabel->setText(tr("Importiere..."));
    }

    if (!m_repo) {
        QMessageBox::critical(this, tr("Import"), tr("Repository ist nicht verfügbar."));
        return;
    }

    const QString path = chosenCsvPath();
    if (path.isEmpty()) {
        QMessageBox::warning(this, tr("Import"), tr("Bitte zuerst eine CSV-Datei auswählen."));
        return;
    }

    // Header aus lst_CsvColumns (das sind die Spaltennamen nach loadPreview)
    auto lstCsv = findChild<QListWidget*>("lst_CsvColumns");
    if (!lstCsv || lstCsv->count() == 0) {
        QMessageBox::warning(this, tr("Import"), tr("Bitte zuerst die Vorschau laden (CSV-Spalten fehlen)."));
        return;
    }

    QHash<QString, int> headerIndex;
    for (int i = 0; i < lstCsv->count(); ++i) {
        const QString col = lstCsv->item(i)->text().trimmed();
        if (!col.isEmpty()) {
            headerIndex.insert(col, i);
        }
    }

    if (m_csvToPart.isEmpty()) {
        QMessageBox::information(this, tr("Import"), tr("Es gibt keine Zuordnungen. Bitte Mapping erstellen."));
        return;
    }

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Import"), tr("Datei kann nicht geöffnet werden:\n%1").arg(path));
        return;
    }

    QTextStream in(&f);
// Encoding setzen
// QStringConverter::encodingForName() hat sich in Qt 6 geändert, QStringConverter::encodingForName()
// liefert in Qt 6 ein std::optional<QStringConverter::Encoding> zurück.
// QTextStream::setEncoding() erwartet aber einen reinen QStringConverter::Encoding, nicht ein Optional
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    in.setCodec(currentEncodingName().toUtf8().constData());
#else
    auto optEnc = QStringConverter::encodingForName(currentEncodingName().toUtf8());
    if (optEnc.has_value()) {
        in.setEncoding(*optEnc);
    } else {
        // Fallback, z. B. UTF-8
        in.setEncoding(QStringConverter::Utf8);
    }
#endif


    // Progressbar korrekt setzen
    auto prg = findChild<QProgressBar*>("prg_Import");
    const int totalRecords = countImportableRecords(path);
    if (prg) {
        prg->setRange(0, 100);
        prg->setValue(0);
    }

    const QChar delim = currentDelimiter();
    const int skip = qMax(0, skipRows());
    const bool header = hasHeader();

    auto log = findChild<QPlainTextEdit*>("txt_Log");
    //auto prg = findChild<QProgressBar*>("prg_Import");

    if (log) { log->appendPlainText(tr("Starte Import: %1").arg(path)); }

    int lineNo = 0;
    bool headerConsumed = false;

    int okCount = 0;
    int errCount = 0;
    int dupCount = 0;
    int processedCount = 0;

    // Abbruch bei Fehlern?
     const bool stop = stopOnError();

    // leere Zeilen überspringen?
    const bool skipEmpty = skipEmptyRows();

    // Whitespace trimmen?
    const bool doTrim = trimWhitespace();

    bool aborted = false;

    if (prg) {
        prg->setRange(0, 0); // indeterminate
        prg->setValue(0);
    }

    int physicalLineNo = 0;

    while (!in.atEnd()) {
        QString record;
        const int recordStartLine = physicalLineNo + 1;

        CsvRecordStatus st = readCsvRecordChecked(in, record, physicalLineNo);

        if (st == CsvRecordStatus::EndOfFile) {
            break;
        }

        if (st == CsvRecordStatus::IncompleteAtEof) {
            ++errCount;

            if (log) {
                log->appendPlainText(tr("CSV inkompatibel: Record ab Zeile %1 hat offene Anführungszeichen (EOF erreicht).")
                                         .arg(recordStartLine));
            }

            aborted = true;

            if (stop) {
                if (log) {
                    log->appendPlainText(tr("Import abgebrochen: inkompatibler CSV-Record (Stop on error aktiv)."));
                }
            }

            break;
        }

        // skipRows als physische Zeilen behandeln:
        if (recordStartLine <= skip) {
            continue;
        }

        // Header-Zeile überspringen (wenn aktiv)
        if (header && !headerConsumed) {
            headerConsumed = true;
            continue;
        }

        const QStringList cols = parseCsvLine(record, delim);
        ++lineNo;

        // Wenn die Zeile bezogen auf das Mapping "leer" sein sollte, dann überspringen (wenn aktiv)
        if (skipEmpty) {
            bool anyMappedValue = false;

            for (auto it = m_csvToPart.constBegin(); it != m_csvToPart.constEnd(); ++it) {
                const QString csvColName = it.key();

                if (!headerIndex.contains(csvColName)) {
                    continue;
                }

                const int idx = headerIndex.value(csvColName);
                const QString v = (idx >= 0 && idx < cols.size()) ? cols[idx] : QString();

                if (!v.trimmed().isEmpty()) {
                    anyMappedValue = true;
                    break;
                }
            }

            if (!anyMappedValue) {
                if (log) {
                    log->appendPlainText(tr("Zeile %1: übersprungen (leerer Datensatz bezogen auf Mapping).")
                                             .arg(recordStartLine)); // falls du recordStartLine nutzt
                    ++dupCount;
                }
                continue;
            }
        }

        // Zähler für Progressbar aktualisieren
        ++processedCount;
        if (prg) {
            int percent = 0;
            if (totalRecords > 0) {
                percent = (processedCount * 100) / totalRecords;
            }
            prg->setValue(percent);
        }


        if (lineNo <= skip) {
            continue;
        }

        if (record.trimmed().isEmpty()) {
            continue;
        }

        if (header && !headerConsumed) {
            headerConsumed = true;
            continue; // Headerzeile überspringen
        }

        //const QStringList cols = parseCsvLine(line, delim);

        Part p;
        bool rowOk = true;

        for (auto it = m_csvToPart.constBegin(); it != m_csvToPart.constEnd(); ++it) {
            const QString csvColName = it.key();
            const QString partField = it.value();

            if (!headerIndex.contains(csvColName)) {
                // Spalte existiert nicht -> ignorieren
                continue;
            }

            const int idx = headerIndex.value(csvColName);
            const QString v = (idx >= 0 && idx < cols.size()) ? cols[idx] : QString();

            QString err;
            if (!applyField(p, partField, v, err, doTrim)) {
                rowOk = false;
                if (log) {
                    log->appendPlainText(tr("CSV-Zeile %1: %2").arg(lineNo).arg(err));
                }
            }
        }

        if (rowOk) {
            if (m_repo->existsDuplicateOf(p)) {
                if (log) {
                    log->appendPlainText(tr("Duplikat übersprungen (Record ab Zeile %1): \"%2\"")
                                             .arg(lineNo)
                                             .arg(p.name));
                ++dupCount;
                }
                continue;
            }

            m_repo->addPart(p);
            ++okCount;
        } else {
            ++errCount;

            if (stop) {
                aborted = true;

                if (log) {
                    log->appendPlainText(tr("Import abgebrochen: Fehler in Zeile %1 (Stop on error aktiv).").arg(lineNo));
                }

                break;
            }
        }
    }

    if (prg) {
        prg->setRange(0, 1);
        prg->setValue(1);
    }

    // Log aktualisieren
    if (log) {
        if (aborted) {
            log->appendPlainText(tr("Status: ABGEBROCHEN. Erfolgreich: %1 | Fehler: %2").arg(okCount).arg(errCount));
        } else {
            log->appendPlainText(tr("Status: FERTIG. Erfolgreich: %1 | Fehler: %2").arg(okCount).arg(errCount));
        }
    }

    // Progressbar aktualisieren
    if (prg) {
        prg->setValue(100);
    }

    // Zusammenfassung am Ende anzeigen
    if (aborted) {
        QMessageBox::warning(this, tr("Import"),
                             tr("Import abgebrochen (Stop on error aktiv).\nErfolgreich: %1\nFehler: %2 \nDuplikate übersprungen: %3")
                                 .arg(okCount).arg(errCount).arg(dupCount));
        // Label neben dem Progress-Bar auf "Abgebrochen" setzen
        auto progressLabel = findChild<QLabel*>("lbl_ImportStatus");
        if(progressLabel)
        {
            progressLabel->setText(tr("Abgebrochen"));
        }
    } else {
        QMessageBox::information(this, tr("Import"),
                                 tr("Import abgeschlossen.\nErfolgreich: %1\nFehler: %2 \nDuplikate übersprungen: %3")
                                     .arg(okCount).arg(errCount).arg(dupCount));
        // Label neben dem Progress-Bar auf "Fertig" setzen
        auto progressLabel = findChild<QLabel*>("lbl_ImportStatus");
        if(progressLabel)
        {
            progressLabel->setText(tr("Fertig"));
        }
    }

    // Abbrechen/Schließen-Button aktualisieren
    m_importRunning = false;
    m_importFinished = true;
    updateCloseButtonState();

    // Signal an MainWindow schicken, damit dort die Part-Liste aktualisiert wird
    emit importFinished(okCount, dupCount, errCount);
}

// Sollen bei Fehlern im Importvorgang abgebrochen werden?
bool ImportDataDialog::stopOnError() const
{
    if (auto chb = findChild<QCheckBox*>("chb_StopOnError")) {
        return chb->isChecked();
    }
    return false;
}

// Sollen leere Zeilen übersprungen werden?
bool ImportDataDialog::skipEmptyRows() const
{
    auto chb = findChild<QCheckBox*>("chb_SkipEmptyRows");
    if (chb) {
        return chb->isChecked();
    }
    return true; // Default: leere Zeilen überspringen
}

// Sollen führende und nachfolgende Leerzeichen entfernt (getrimmt) werden?
bool ImportDataDialog::trimWhitespace() const
{
    auto chb = findChild<QCheckBox*>("chb_TrimWhitespace");
    if (chb) {
        return chb->isChecked();
    }
    return true; // Default sinnvoll
}

// Hilfsfunktion für die Anzahl importierbarer Datensätze in der CSV-Datei
// wird für die Progressbar gebraucht (kostet allerdings Performance)
int ImportDataDialog::countImportableRecords(const QString& path) const
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return 0;
    }

    QTextStream in(&f);
    /*
    #if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        in.setCodec(currentEncodingName().toUtf8().constData());
    #else
        in.setEncoding(QStringConverter::encodingForName(currentEncodingName().toUtf8()));
    #endif
    */
    #if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        in.setCodec(currentEncodingName().toUtf8().constData());
    #else
        auto optEnc = QStringConverter::encodingForName(currentEncodingName().toUtf8());
        if (optEnc.has_value()) {
            in.setEncoding(*optEnc);
        } else {
            // Fallback, z. B. UTF-8
            in.setEncoding(QStringConverter::Utf8);
        }
    #endif

    const int skip = qMax(0, skipRows());
    const bool header = hasHeader();

    int physicalLineNo = 0;
    int total = 0;
    bool headerConsumed = false;

    while (!in.atEnd()) {
        QString record;
        const int recordStartLine = physicalLineNo + 1;
        const CsvRecordStatus st = readCsvRecordChecked(in, record, physicalLineNo);

        if (st == CsvRecordStatus::EndOfFile) {
            break;
        }

        if (st == CsvRecordStatus::IncompleteAtEof) {
            break; // inkompatibel -> Import wird später eh abbrechen
        }

        if (recordStartLine <= skip) {
            continue;
        }

        if (header && !headerConsumed) {
            headerConsumed = true;
            continue;
        }

        if (skipEmptyRows()) {
            if (record.trimmed().isEmpty()) {
                continue;
            }
        }

        ++total;
    }

    return total;
}

// Aktualisiert den Zustand des Import-Buttons basierend auf dem Mapping
void ImportDataDialog::updateImportButtonState()
{
    // UI-Elemente holen
    auto tbl = findChild<QTableWidget*>("tbl_Mapping");
    auto btn = findChild<QPushButton*>("btn_StartImport");

    // Sicherstellen, dass die Elemente existieren
    if (!tbl || !btn) {
        return;
    }

    // Der Button wird nur Enabled, wenn tbl_Mapping mindestens eine Zeile hat
    btn->setEnabled(tbl->rowCount() > 0);
    qDebug() << "Import button state updated. Enabled:" << btn->isEnabled();
}

// Hilfsfunktion, die bei jeder Änderung der Mapping-Tabelle prüft, ob der Import-Button
// freigegeben werden kann oder nicht
void ImportDataDialog::hookUpMappingTableSignals()
{
    auto tbl = findChild<QTableWidget*>("tbl_Mapping");
    if (!tbl) {
        return;
    }

    auto* m = tbl->model();
    if (!m) {
        return;
    }

    connect(m, &QAbstractItemModel::rowsInserted, this, [this]() {
        this->updateImportButtonState();
        qDebug() << "Mapping table rows inserted.";
    });

    connect(m, &QAbstractItemModel::rowsRemoved, this, [this]() {
        this->updateImportButtonState();
        qDebug() << "Mapping table rows removed.";
    });

    // Mapping-Zellen editiert oder per Code geändert
    connect(m, &QAbstractItemModel::modelReset, this, [this]() {
        this->updateImportButtonState();
        qDebug() << "Mapping table model reset.";
    });
}

// Aktualisiert den Zustand des Schließen-Buttons basierend auf dem Importstatus
// eigetlich obsolet, muss keine extra-Funktion sein und kann
void ImportDataDialog::updateCloseButtonState()
{
    auto btnClose = findChild<QPushButton*>("btn_Close");
    if (!btnClose) {
        return;
    }

    btnClose->setEnabled(!m_importRunning);

    if (m_importFinished) {
        btnClose->setText(tr("Schließen"));
    } else {
        btnClose->setText(tr("Schließen")); // oder "Schließen", wie du es möchtest
    }
}
