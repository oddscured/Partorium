#ifndef LISTMANAGERDIALOG_H
#define LISTMANAGERDIALOG_H

#include <QDialog>

namespace Ui {
class ListManagerDialog;
}

class ListManagerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ListManagerDialog(QWidget *parent = nullptr);
    explicit ListManagerDialog(const QMap<QString, QStringList> &presets, QWidget *parent = nullptr);
    ~ListManagerDialog() override;

    // Liefert alle Vorgabelisten (Key = Listenname/Verwendungszweck; Value = Einträge)
    QMap<QString, QStringList> presets() const;

    // Komplett ersetzen / initial setzen
    void setPresets(const QMap<QString, QStringList> &presets);

signals:
    void presetsChanged(const QMap<QString, QStringList> &presets);

private slots:

private:

    Ui::ListManagerDialog *ui = nullptr;
    QMap<QString, QStringList> m_presets;
    QString m_currentKey;
};

#endif // LISTMANAGERDIALOG_H
