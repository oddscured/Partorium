#ifndef IMPORTDATADIALOG_H
#define IMPORTDATADIALOG_H

#include <QDialog>

namespace Ui {
class ImportDataDialog;
}

class ImportDataDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ImportDataDialog(QWidget *parent = nullptr);
    ~ImportDataDialog();

private:
    Ui::ImportDataDialog *ui;
};

#endif // IMPORTDATADIALOG_H
