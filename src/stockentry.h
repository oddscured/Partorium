#pragma once

#include <QDateTime>
#include <QJsonObject>
#include <QString>

struct StockEntry {
    QDateTime date;
    QString type;      // "add" or "remove"
    int partId = 0;
    QString partName;
    QString title;     // project title for remove, empty for add
    QString source;    // source for add, empty for remove
    int quantity = 0;  // changed quantity
    bool deleted = false;
    QString comment;
};

inline QJsonObject toJson(const StockEntry& entry) {
    QJsonObject o;
    o["date"] = entry.date.toUTC().toString(Qt::ISODate);
    o["type"] = entry.type;
    o["partId"] = entry.partId;
    o["partName"] = entry.partName;
    o["title"] = entry.title;
    o["source"] = entry.source;
    o["quantity"] = entry.quantity;
    o["deleted"] = entry.deleted;
    o["comment"] = entry.comment;
    return o;
}

inline StockEntry stockEntryFromJson(const QJsonObject& o) {
    StockEntry entry;
    entry.date = QDateTime::fromString(o["date"].toString(), Qt::ISODate);
    entry.type = o["type"].toString();
    entry.partId = o["partId"].toInt();
    entry.partName = o["partName"].toString();
    entry.title = o["title"].toString();
    entry.source = o["source"].toString();
    entry.quantity = o["quantity"].toInt();
    entry.deleted = o["deleted"].toBool(false);
    entry.comment = o["comment"].toString();
    return entry;
}
