#ifndef NEWPARTDIALOG_H
#define NEWPARTDIALOG_H

#include <QDialog>

namespace Ui { class NewPartDialog; }

class NewPartDialog : public QDialog {
    Q_OBJECT
public:
    explicit NewPartDialog(QWidget *parent = nullptr);
    ~NewPartDialog();

private:
    Ui::NewPartDialog *ui;
    void hookUpSignals();
};

#endif // NEWPARTDIALOG_H
