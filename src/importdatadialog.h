#ifndef IMPORTDATADIALOG_H
#define IMPORTDATADIALOG_H

#include <QDialog>
#include <QStringList>
#include <QVector>

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

    void hookUpSignals();
    void browsCsvFile();

    void loadPreview();

    QString chosenCsvPath() const;
    QChar currentDelimiter() const;
    QString currentEncodingName() const;
    bool hasHeader() const;
    int skipRows() const;

    static QStringList parseCsvLine(const QString& line, QChar delimiter);
};

#endif // IMPORTDATADIALOG_H
