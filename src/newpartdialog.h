#ifndef NEWPARTDIALOG_H
#define NEWPARTDIALOG_H

#include <QDialog>
#include "jsonpartrepository.h"

namespace Ui { class NewPartDialog; }

class NewPartDialog : public QDialog {
    Q_OBJECT
public:
    explicit NewPartDialog(JsonPartRepository* repo, QWidget *parent = nullptr);
    ~NewPartDialog();

signals:
    void nextPartRequested();

public:
    void resetInputs();

private:    
    Ui::NewPartDialog *ui;
    void hookUpSignals();
    void prepareUI();
    JsonPartRepository* m_repo = nullptr;  // ← neu: hier merken wir uns das Repo
    void populatePresetCombos();

};

#endif // NEWPARTDIALOG_H
