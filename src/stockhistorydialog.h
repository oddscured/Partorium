#ifndef STOCKHISTORYDIALOG_H
#define STOCKHISTORYDIALOG_H

#include <QDialog>
#include <QVector>

class JsonPartRepository;
class StockManagementRepository;
class QStandardItemModel;
class QSortFilterProxyModel;
struct StockEntry;

namespace Ui {
class StockHistoryDialog;
}

class StockHistoryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StockHistoryDialog(JsonPartRepository* partRepo,
                                StockManagementRepository* stockRepo,
                                QWidget *parent = nullptr);
    ~StockHistoryDialog();
    void setInitialSearchText(const QString& text);

private:
    Ui::StockHistoryDialog *ui;
    JsonPartRepository* m_partRepo = nullptr;
    StockManagementRepository* m_stockRepo = nullptr;
    QStandardItemModel* m_model = nullptr;
    QSortFilterProxyModel* m_proxyModel = nullptr;
    QVector<StockEntry> m_entries;

    void hookUpSignals();
    bool loadEntries();
    void rebuildModel();
    void applySearchFilter(const QString& text);
    void handleDeleteCurrentRow();
};

#endif // STOCKHISTORYDIALOG_H
