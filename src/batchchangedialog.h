#ifndef BATCHCHANGEDIALOG_H
#define BATCHCHANGEDIALOG_H

#include <QDialog>

namespace Ui {
class BatchChangeDialog;
}

class BatchChangeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BatchChangeDialog(QWidget *parent = nullptr);
    ~BatchChangeDialog();

    void prepareUI();
private:
    Ui::BatchChangeDialog *ui;
};

#endif // BATCHCHANGEDIALOG_H
