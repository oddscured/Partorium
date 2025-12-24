#include "importdatadialog.h"
#include "ui_importdatadialog.h"
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


ImportDataDialog::ImportDataDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ImportDataDialog)
{
    ui->setupUi(this);
    hookUpSignals();
}

// Destruktor
ImportDataDialog::~ImportDataDialog()
{
    delete ui;
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
        QString line = in.readLine();
        ++lineNo;

        // SkipRows
        if (lineNo <= skip)
            continue;

        // Leere Zeilen ignorieren in Preview
        if (line.trimmed().isEmpty())
            continue;

        const QStringList cols = parseCsvLine(line, delim);

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
