#pragma once
#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>

struct Part {
    int id = 0;                             // automatisch vergeben
    QString name;                           // Name
    QString shortDescription;               // Kurzbeschreibung
    QString category;                       // Kategorie
    QString subcategory;                    // Unterkategorie
    QString type;                           // Typ (Modul, Bauteil, Mechanik, etc.)
    QString format;                         // Format (z.B. SMD, THT, 1206, ...)
    QString description;                    // Beschreibung (Langtext)
    QString supplier;                       // Lieferant
    QString supplierLink;                   // Link zum Lieferant
    QString altSupplier;                    // Alternativlieferant
    QString altSupplierLink;                // Link zum Alternativlieferant
    QString manufacturer;                   // Hersteller
    QString manufacturerLink;               // Link zum Hersteller
    QStringList localFiles;                 // Dateien rechts
    int quantity = 0;                       // Anzahl
    double price = 0.0;                     // Preis
    QString storage;                        // Lagerort
    QString storageDetails;                 // Lagerort Details
    QString imagePath;                      // Anzeigebild
    QStringList images;                     // Weitere Bilder
    QStringList hashtags;                   // Schlagwörter
    bool deleted = false;                   // gelöscht (wird nicht mehr angezeigt)
    QDateTime createdAt;                    // optional
    QDateTime updatedAt;                    // optional
};

// ---- JSON Serialisierung ----
inline QJsonObject toJson(const Part& p) {
    QJsonObject o;
    o["id"] = p.id;
    o["name"] = p.name;
    o["shortDescription"] = p.shortDescription;
    o["category"] = p.category;
    o["subcategory"] = p.subcategory;
    o["description"] = p.description;
    o["format"] = p.format;
    o["type"] = p.type;
    o["supplier"] = p.supplier;
    o["supplierLink"] = p.supplierLink;
    o["altSupplier"] = p.altSupplier;
    o["altSupplierLink"] = p.altSupplierLink;
    o["manufacturer"] = p.manufacturer;
    o["manufacturerLink"] = p.manufacturerLink;
    o["localFiles"] = QJsonArray::fromStringList(p.localFiles);
    o["quantity"] = p.quantity;
    o["price"] = p.price;
    o["storage"] = p.storage;
    o["storageDetails"] = p.storageDetails;
    o["imagePath"] = p.imagePath;
    o["images"] = QJsonArray::fromStringList(p.images);
    o["hashtags"] = QJsonArray::fromStringList(p.hashtags);
    o["deleted"] = p.deleted;
    o["createdAt"] = p.createdAt.toString(Qt::ISODate);
    o["updatedAt"] = p.updatedAt.toString(Qt::ISODate);
    return o;
}

inline Part partFromJson(const QJsonObject& o) {
    Part p;
    p.id = o["id"].toInt();
    p.name = o["name"].toString();
    p.shortDescription = o["shortDescription"].toString();
    p.category = o["category"].toString();
    p.subcategory = o["subcategory"].toString();
    p.type = o["type"].toString();
    p.format = o["format"].toString();
    p.description = o["description"].toString();
    p.supplier = o["supplier"].toString();
    p.supplierLink = o["supplierLink"].toString();
    p.altSupplier = o["altSupplier"].toString();
    p.altSupplierLink = o["altSupplierLink"].toString();
    p.manufacturer = o["manufacturer"].toString();
    p.manufacturerLink = o["manufacturerLink"].toString();
    for (auto v : o["localFiles"].toArray()) p.localFiles << v.toString();
    p.quantity = o["quantity"].toInt();
    p.price = o["price"].toDouble();
    p.storage = o["storage"].toString();
    p.storageDetails = o["storageDetails"].toString();
    p.imagePath = o["imagePath"].toString();
    for (auto v : o["images"].toArray()) p.images << v.toString();
    for (auto v : o["hashtags"].toArray()) p.hashtags << v.toString();
    p.deleted = o["deleted"].toBool();
    p.createdAt = QDateTime::fromString(o["createdAt"].toString(), Qt::ISODate);
    p.updatedAt = QDateTime::fromString(o["updatedAt"].toString(), Qt::ISODate);
    return p;
}
