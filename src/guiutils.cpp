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
