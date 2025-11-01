#pragma once
#include <QComboBox>
#include <QMap>
#include <QStringList>
#include "guiutils.h"

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
