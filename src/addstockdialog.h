#ifndef ADDSTOCKDIALOG_H
#define ADDSTOCKDIALOG_H

#include <QDialog>
#include <QString>

class JsonPartRepository;

namespace Ui {
class AddStockDialog;
}

class AddStockDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddStockDialog(JsonPartRepository* repo, QWidget *parent = nullptr);
    ~AddStockDialog();
    int quantity() const;
    QString source() const;
    QString comment() const;

private:
    Ui::AddStockDialog *ui;
    JsonPartRepository* m_repo = nullptr;
    void hookUpSignals();
    void populateSourceCombo();
};

#endif // ADDSTOCKDIALOG_H
