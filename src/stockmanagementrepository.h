#pragma once

#include <QVector>
#include <QString>

#include "stockentry.h"

class StockManagementRepository {
public:
    // Expects path to partorium.json and places stock_management.json in the same directory.
    bool setBaseDatabasePath(const QString& partoriumDatabasePath);
    QString stockManagementPath() const;

    bool appendEntry(const StockEntry& entry) const;
    bool loadEntries(QVector<StockEntry>& outEntries) const;
    bool saveEntries(const QVector<StockEntry>& entries) const;

private:
    QString m_path;

    static bool ensureParentDirExists(const QString& filePath);
    static bool readEntriesFromFile(const QString& filePath, QVector<StockEntry>& outEntries);
};
