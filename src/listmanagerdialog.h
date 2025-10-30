/*
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
*/

#pragma once
#include <QDialog>
#include <QMap>
#include <QStringList>

namespace Ui { class ListManagerDialog; }

class ListManagerDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ListManagerDialog(QWidget *parent = nullptr);
    ~ListManagerDialog() override;

    // Daten setzen/holen
    void setPresets(const QMap<QString, QStringList> &presets,
                    const QString &currentKey = QString());
    QMap<QString, QStringList> presets() const { return m_presets; }

    // aktive Liste setzen/abfragen
    void setCurrentListKey(const QString &key);
    QString currentListKey() const { return m_currentKey; }

signals:
    // wird bei OK gesendet (nur wenn Änderungen passiert sind)
    void presetsChanged(const QMap<QString, QStringList> &presets);

private slots:
    void onListChanged(int index);
    void onAdd();
    void onRemove();
    void onMoveUp();
    void onMoveDown();
    void onAccept();

private:
    void refreshListKeys();
    void refreshEntries();
    void setDirty(bool dirty);
    bool containsCaseInsensitive(const QStringList &list, const QString &value) const;

private:
    Ui::ListManagerDialog *ui = nullptr;
    QMap<QString, QStringList> m_presets;
    QString m_currentKey;
    bool m_dirty = false;
};
