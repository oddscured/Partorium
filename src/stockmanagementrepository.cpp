#include "stockmanagementrepository.h"

#include <QDir>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

bool StockManagementRepository::setBaseDatabasePath(const QString& partoriumDatabasePath) {
    if (partoriumDatabasePath.isEmpty()) return false;

    const QFileInfo fi(QDir::fromNativeSeparators(partoriumDatabasePath));
    const QString filePath = fi.absoluteDir().absoluteFilePath("stock_management.json");

    if (!ensureParentDirExists(filePath)) return false;

    if (!QFileInfo::exists(filePath)) {
        QFile f(filePath);
        if (!f.open(QIODevice::WriteOnly)) return false;
        QJsonObject root;
        root["entries"] = QJsonArray();
        root["updatedAt"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
        f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        f.close();
    }

    m_path = filePath;
    return true;
}

QString StockManagementRepository::stockManagementPath() const {
    return m_path;
}

bool StockManagementRepository::appendEntry(const StockEntry& entry) const {
    if (m_path.isEmpty()) return false;

    QVector<StockEntry> entries;
    if (!readEntriesFromFile(m_path, entries)) return false;

    entries.push_back(entry);

    QJsonArray arr;
    for (const auto& it : entries) {
        arr.push_back(toJson(it));
    }

    QJsonObject root;
    root["entries"] = arr;
    root["updatedAt"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    QFile f(m_path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    f.close();
    return true;
}

bool StockManagementRepository::loadEntries(QVector<StockEntry>& outEntries) const {
    outEntries.clear();
    if (m_path.isEmpty()) return false;
    return readEntriesFromFile(m_path, outEntries);
}

bool StockManagementRepository::saveEntries(const QVector<StockEntry>& entries) const {
    if (m_path.isEmpty()) return false;

    QJsonArray arr;
    for (const auto& entry : entries) {
        arr.push_back(toJson(entry));
    }

    QJsonObject root;
    root["entries"] = arr;
    root["updatedAt"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    QFile f(m_path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    f.close();
    return true;
}

bool StockManagementRepository::ensureParentDirExists(const QString& filePath) {
    const QFileInfo fi(filePath);
    QDir dir = fi.absoluteDir();
    if (dir.exists()) return true;
    return dir.mkpath(".");
}

bool StockManagementRepository::readEntriesFromFile(const QString& filePath, QVector<StockEntry>& outEntries) {
    outEntries.clear();

    QFile f(filePath);
    if (!QFileInfo::exists(filePath)) {
        return false;
    }
    if (!f.open(QIODevice::ReadOnly)) return false;

    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();

    QJsonArray arr;
    if (doc.isObject()) {
        const QJsonObject root = doc.object();
        if (root["entries"].isArray()) {
            arr = root["entries"].toArray();
        } else {
            arr = QJsonArray();
        }
    } else if (doc.isArray()) {
        // Backward-compatible fallback if the file accidentally contains a raw array.
        arr = doc.array();
    } else {
        return false;
    }

    outEntries.reserve(arr.size());
    for (const auto& v : arr) {
        if (!v.isObject()) continue;
        outEntries.push_back(stockEntryFromJson(v.toObject()));
    }
    return true;
}
