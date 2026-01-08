#ifndef BATCHCHANGEDIALOG_H
#define BATCHCHANGEDIALOG_H

#include <QDialog>
#include <optional>
#include <QCheckBox>

// Struktur zum Halten der Batch-Änderungen
struct BatchChangePatch
{
    std::optional<QString> type;
    std::optional<QString> category;
    std::optional<QString> subcategory;
    std::optional<QString> format;
    std::optional<QString> storage;
    std::optional<QString> storageDetails;
    // Hier weitere Felder einfügen, wenn der Dialog erweitert wurde
};

namespace Ui {
class BatchChangeDialog;
}

class BatchChangeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BatchChangeDialog(QWidget *parent = nullptr);
    ~BatchChangeDialog();

    // IDs der zu ändernden Bauteile setzen/holen
    void setPartIds(const QVector<int>& ids);
    QVector<int> partIds() const;

    // Sturktur mit den Änderungen (welche Felder auf welche Werte geändert werden sollen)
    BatchChangePatch patch() const;

    void prepareUI();

    void onCheckedChange();

private:
    Ui::BatchChangeDialog *ui;
    QVector<int> m_partIds; // IDs der zu ändernden
    //QVector<QCheckBox> myCheckboxes;
};

#endif // BATCHCHANGEDIALOG_H
