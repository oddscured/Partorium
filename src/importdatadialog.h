#ifndef IMPORTDATADIALOG_H
#define IMPORTDATADIALOG_H

#include <QDialog>
#include <QStringList>
#include <QVector>
#include "jsonpartrepository.h"

namespace Ui {
class ImportDataDialog;
}

class ImportDataDialog : public QDialog
{
    Q_OBJECT

signals:
    void importFinished(int importedCount, int duplicateCount, int errorCount);

public:
    //explicit ImportDataDialog(QWidget *parent = nullptr);
    ~ImportDataDialog();
    explicit ImportDataDialog(JsonPartRepository* repo, QWidget* parent = nullptr);


private:
    Ui::ImportDataDialog *ui;

    // Für das importieren/anlegen des part
    void startImport();
    JsonPartRepository* m_repo = nullptr;

    // zur Steuerung des Verhaltens
    bool stopOnError() const;    // Anhalten bei Fehlern im Import?
    bool skipEmptyRows() const;  // Leere Zeilen überspringen?
    bool trimWhitespace() const; // Whitespace an Werten entfernen?

    // Der Importieren-Button wird nur freigegeben, wenn alle Vorbedingungen erfüllt sind
    // CSV-Vorschau geladen, mindestens eine Zeile in der Mapping-Tabelle
    void updateImportButtonState();
    void hookUpMappingTableSignals(); // Hilfsfunktion um festzustellen, ob die Mapping-Tabelle geändert wurde

    // wichtig für den Abbrechen/Schließen-Button: darf nicht betätigt werden während der Import läuft
    bool m_importRunning = false;
    bool m_importFinished = false;
    void updateCloseButtonState();

    void prepareUI();
    void hookUpSignals();
    void browsCsvFile();

    void loadPreview();

    QString chosenCsvPath() const;
    QChar currentDelimiter() const;
    QString currentEncodingName() const;
    bool hasHeader() const;
    int skipRows() const;

    static QStringList parseCsvLine(const QString& line, QChar delimiter);

    // Für das Mapping der CSV-Spalten:
    QHash<QString, QString> m_csvToPart; // key: CSV column name, value: Part field name
    void hookUpMappingSignals();
    void mapSelected();
    void unmapSelected();
    void clearMapping();
    bool mappingExistsCsv(const QString& csvCol) const;
    bool mappingExistsPart(const QString& partField) const;
    void refreshMappingTable();

    // Für den Import
    //bool applyField(Part& p, const QString& field, const QString& value, QString& errorOut) const;
    bool applyField(Part& p, const QString& field, const QString& value, QString& errorOut, bool doTrim) const;

    // Zähler für die Progress-Bar
    int countImportableRecords(const QString& path) const;

};

#endif // IMPORTDATADIALOG_H
