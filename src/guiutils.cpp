#pragma once
#include <QComboBox>
#include <QFileDialog>
#include <QMap>
#include <QStringList>
#include "guiutils.h"
#include <QDialog>
#include <QTranslator>

//GuiUtils::GuiUtils() {}


void GuiUtils::setLabelWithOptionalLink(QLabel* label, const QString& text, const QString& link)
{
    if (!link.isEmpty())
    {
        QString html = QString("<a href=\"%1\">%2</a>").arg(link, text);
        label->setText(html);
        label->setTextFormat(Qt::RichText);
        label->setTextInteractionFlags(Qt::TextBrowserInteraction);
        label->setOpenExternalLinks(true);
    }
    else
    {
        label->setText(text);
    }
}

void GuiUtils::applyPresetToCombo(QComboBox *combo,
                        const QMap<QString, QStringList> &presets,
                        const QString &key,
                        bool editable)
{
    if (!combo) return;

    combo->blockSignals(true);
    combo->clear();
    combo->addItems(presets.value(key));
    combo->setEditable(editable);
    combo->setInsertPolicy(QComboBox::InsertAtTop);
    combo->blockSignals(false);
}

QString GuiUtils::getImageFileNameWithSearchString(QWidget* parent, QString searchName, QString dir)
{

    QString file = "";
    QFileDialog dialog(parent);
    dialog.setWindowTitle(("Bild auswählen"));
    dialog.setDirectory(dir);

    // Erstes und letztes Zeichen von search Name entfernen damit die Suche funktioniert, Leerzeichen ebenfalls entfernen
    searchName = searchName.removeFirst();
    searchName = searchName.removeLast();
    searchName = searchName.trimmed();
    searchName = searchName.replace("-", "*");
    searchName = searchName.replace(" ", "*");

    // Filter: nur Bilder, die den Bauteilnamen enthalten
    dialog.setNameFilters({
        QString(("Bilder (*%1*.png *%1*.jpg *%1*.jpeg *%1*.webp)")).arg(searchName),
        ("Alle Bilder (*.png *.jpg *.jpeg *.webp)"),
        ("Alle Dateien (*)")
    });

    dialog.setFileMode(QFileDialog::ExistingFile);

    if (dialog.exec() == QDialog::Accepted) {
        return dialog.selectedFiles().at(0);
    }
    return file;
}
