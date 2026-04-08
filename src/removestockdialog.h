#ifndef REMOVESTOCKDIALOG_H
#define REMOVESTOCKDIALOG_H

#include <QDialog>
#include <QString>
#include <QStringList>

namespace Ui {
class RemoveStockDialog;
}

class RemoveStockDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RemoveStockDialog(const QStringList& previousProjectTitles, QWidget *parent = nullptr);
    ~RemoveStockDialog();
    int quantity() const;
    QString projectTitle() const;
    QString comment() const;

private:
    Ui::RemoveStockDialog *ui;
    void hookUpSignals();
};

#endif // REMOVESTOCKDIALOG_H
