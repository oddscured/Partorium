#pragma once
#include <QObject>
#include <QVector>
#include <optional>
#include <QString>

#include "part.h"

class JsonPartRepository : public QObject {
    Q_OBJECT
public:
    explicit JsonPartRepository(QObject* parent = nullptr,
                                const QString& initialPath = QString());

    // Speicherort setzen/abfragen
    bool setDatabasePath(const QString& filePath);  // legt Ordner an, erstellt Datei falls nötig, lädt
    QString databasePath() const;

    // Hilfen: sinnvollen Standard vorschlagen (z.B. iCloud)
    static QString suggestDefaultJsonPath();        // .../Partorium/partorium.json
    static QString suggestDefaultDirectory();       // .../Partorium/

    // Daten laden/speichern
    bool load();                  // lädt Datei, erstellt sie falls nicht vorhanden
    bool save() const;

    // CRUD
    int addPart(Part p);
    bool updatePart(const Part& p);
    bool removePart(int id);

    QVector<Part> allParts() const;
    std::optional<Part> getPart(int id) const;

    QVector<Part> searchParts(const QString& term,
                              const QString& category = QString(),
                              const QString& subcategory = QString()) const;

    // ---- Vorgabelisten (Combobox-Presets) ----
    using PresetsMap = QMap<QString, QStringList>;

    // Pfad zur Listen-JSON: <Ordner von partorium.json>/partorium_lists.json
    QString presetsJsonPath() const;

    // Laden/Speichern der Listen; legt Datei bei Bedarf an (leer).
    bool loadPresets(PresetsMap& out) const;
    bool savePresets(const PresetsMap& in) const;

    // Überprüfen, ob ein part schon existiert - wichtig beim Import von Dateien
    bool existsExactPart(const Part& p) const;
    bool existsDuplicateOf(const Part& p) const;

private:
    QString m_path;               // kompletter Pfad zur JSON-Datei
    QVector<Part> m_parts;
    int m_nextId = 1;

    static bool ensureParentDirExists(const QString& filePath);
};
