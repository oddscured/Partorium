#include "jsonpartrepository.h"

#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFileInfo>
#include <QJsonValue>
#include <QJsonObject>


JsonPartRepository::JsonPartRepository(QObject* parent, const QString& initialPath)
    : QObject(parent)
{
    if (!initialPath.isEmpty()) {
        setDatabasePath(initialPath);
    } else {
        setDatabasePath(suggestDefaultJsonPath());
    }
}

QString JsonPartRepository::databasePath() const { return m_path; }

bool JsonPartRepository::ensureParentDirExists(const QString& filePath) {
    QFileInfo fi(filePath);
    QDir dir = fi.dir();
    if (dir.exists()) return true;
    return dir.mkpath(".");
}

// macOS iCloud Drive: ~/Library/Mobile Documents/com~apple~CloudDocs
// Windows iCloud Drive: meist ~/iCloudDrive  oder  ~/iCloud Drive
static QStringList iCloudRootCandidates() {
    QString home = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    return {
        home + "/Library/Mobile Documents/com~apple~CloudDocs", // macOS
        home + "/iCloudDrive",                                  // Windows (häufig)
        home + "/iCloud Drive",                                 // Windows (mit Leerzeichen)
        home + "/iCloudDrive/Documents",                        // manche Konfigurationen
    };
}

QString JsonPartRepository::suggestDefaultDirectory() {
    // 1) iCloud Drive finden, sonst
    for (const auto& root : iCloudRootCandidates()) {
        if (QDir(root).exists()) {
            return root + "/Partorium";
        }
    }
    // 2) Fallback: Benutzer-Dokumente
    const auto docs = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (!docs.isEmpty()) return docs + "/Partorium";

    // 3) Fallback: AppData
    const auto base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return base; // .../Partorium (Qt hängt AppName automatisch an)
}

QString JsonPartRepository::suggestDefaultJsonPath() {
    return suggestDefaultDirectory() + "/partorium.json";
}

bool JsonPartRepository::setDatabasePath(const QString& filePath) {
    if (filePath.isEmpty()) return false;
    if (!ensureParentDirExists(filePath)) return false;

    m_path = QDir::fromNativeSeparators(filePath);

    // Datei anlegen, falls sie fehlt
    QFile f(m_path);
    if (!f.exists()) {
        if (!f.open(QIODevice::WriteOnly)) return false;
        QJsonObject root;
        root["nextId"] = 1;
        root["parts"] = QJsonArray();
        f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        f.close();
    }
    return load();
}

bool JsonPartRepository::load() {
    if (m_path.isEmpty()) return false;
    QFile f(m_path);
    if (!f.exists()) {
        // falls gelöscht wurde: neu anlegen
        return setDatabasePath(m_path);
    }
    if (!f.open(QIODevice::ReadOnly)) return false;
    const auto doc = QJsonDocument::fromJson(f.readAll());
    f.close();

    m_parts.clear();
    m_nextId = 1;
    const auto root = doc.object();
    if (root.contains("nextId")) m_nextId = root["nextId"].toInt(1);
    for (const auto& v : root["parts"].toArray()) {
        m_parts.push_back(partFromJson(v.toObject()));
        m_nextId = std::max(m_nextId, m_parts.back().id + 1);
    }
    return true;
}

bool JsonPartRepository::save() const {
    if (m_path.isEmpty()) return false;
    QJsonObject root;
    root["nextId"] = m_nextId;
    QJsonArray arr;
    for (const auto& p : m_parts) arr.push_back(toJson(p));
    root["parts"] = arr;

    QFile f(m_path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    f.close();
    return true;
}

int JsonPartRepository::addPart(Part p) {
    p.id = m_nextId++;
    p.createdAt = QDateTime::currentDateTimeUtc();
    p.updatedAt = p.createdAt;
    m_parts.push_back(p);
    save();
    return p.id;
}

bool JsonPartRepository::updatePart(const Part& p) {
    for (auto& it : m_parts) {
        if (it.id == p.id) { it = p; it.updatedAt = QDateTime::currentDateTimeUtc(); return save(); }
    }
    return false;
}

bool JsonPartRepository::removePart(int id) {
    auto before = m_parts.size();
    m_parts.erase(std::remove_if(m_parts.begin(), m_parts.end(),
                                 [=](const Part& x){ return x.id==id; }), m_parts.end());
    if (m_parts.size() != before) return save();
    return false;
}

QVector<Part> JsonPartRepository::allParts() const { return m_parts; }

std::optional<Part> JsonPartRepository::getPart(int id) const {
    for (const auto& p : m_parts) if (p.id==id) return p;
    return std::nullopt;
}

static bool containsCi(const QString& hay, const QString& needle) {
    return hay.contains(needle, Qt::CaseInsensitive);
}

QVector<Part> JsonPartRepository::searchParts(const QString& term,
                                              const QString& category,
                                              const QString& subcategory) const
{
    const QString t = term.trimmed();
    QVector<Part> out;
    for (const auto& p : m_parts) {
        if (!category.isEmpty() && category != "Alle Kategorien" && p.category != category) continue;
        if (!subcategory.isEmpty() && p.subcategory != subcategory) continue;

        if (t.isEmpty()) { out.push_back(p); continue; }

        const QStringList blob = {
            p.name, p.shortDescription, p.category, p.subcategory,
            p.description, p.supplier, p.supplierLink,
            p.altSupplier, p.altSupplierLink,
            p.manufacturer, p.manufacturerLink,
            p.storage, p.storageDetails,
            p.type, p.format,
            QString::number(p.id),
            p.hashtags.join(' '),
            p.deleted ? "deleted" : "",
            p.localFiles.join(' ')
        };
        bool hit = false;
        for (const auto& s : blob) if (containsCi(s, t)) { hit = true; break; }
        if (hit) out.push_back(p);
    }
    std::sort(out.begin(), out.end(),
              [](const Part& a, const Part& b){ return a.name.localeAwareCompare(b.name) < 0; });
    return out;
}

// ---- Vorgabelisten (Combobox-Presets) ----

// ---- Presets JSON Pfad ----
     QString JsonPartRepository::presetsJsonPath() const {
    if (m_path.isEmpty()) return QString();
    const QFileInfo fi(m_path);
    return fi.absoluteDir().absoluteFilePath("partorium_lists.json");
}

// ---- Hilfen: JSON <-> Map ----
static JsonPartRepository::PresetsMap jsonToPresetsMap(const QJsonObject& root) {
    JsonPartRepository::PresetsMap map;

    // 1) Bevorzugt "lists" lesen, wenn vorhanden:
    if (root.contains("lists") && root["lists"].isObject()) {
        const auto listsObj = root["lists"].toObject();
        for (const auto& key : listsObj.keys()) {
            QStringList arr;
            for (const auto& v : listsObj[key].toArray()) arr << v.toString();
            map.insert(key, arr);
        }
        return map;
    }

    // 2) Fallback: flaches Mapping (Top-Level keys direkt Arrays)
    for (const auto& key : root.keys()) {
        if (!root[key].isArray()) continue;
        QStringList arr;
        for (const auto& v : root[key].toArray()) arr << v.toString();
        map.insert(key, arr);
    }
    return map;
}

static QJsonObject presetsMapToJson(const JsonPartRepository::PresetsMap& map) {
    QJsonObject listsObj;
    for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
        listsObj[it.key()] = QJsonArray::fromStringList(it.value());
    }
    QJsonObject root;
    root["lists"] = listsObj;
    root["updatedAt"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    return root;
}

// ---- Laden ----
bool JsonPartRepository::loadPresets(JsonPartRepository::PresetsMap& out) const {
    const QString path = presetsJsonPath();
    if (path.isEmpty()) return false;

    // Datei ggf. anlegen
    ensureParentDirExists(path);
    if (!QFileInfo::exists(path)) {
        QFile nf(path);
        if (!nf.open(QIODevice::WriteOnly)) return false;
        // leeres Grundgerüst schreiben
        QJsonObject emptyRoot;
        emptyRoot["lists"] = QJsonObject(); // leer
        nf.write(QJsonDocument(emptyRoot).toJson(QJsonDocument::Indented));
        nf.close();
        out.clear();
        return true;
    }

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return false;
    const auto doc = QJsonDocument::fromJson(f.readAll());
    f.close();
    if (!doc.isObject()) { out.clear(); return false; }

    out = jsonToPresetsMap(doc.object());
    return true;
}

// ---- Speichern ----
bool JsonPartRepository::savePresets(const JsonPartRepository::PresetsMap& in) const {
    const QString path = presetsJsonPath();
    if (path.isEmpty()) return false;
    if (!ensureParentDirExists(path)) return false;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;

    const auto root = presetsMapToJson(in);
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    f.close();
    return true;
}
