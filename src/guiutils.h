#ifndef GUIUTILS_H
#define GUIUTILS_H
#include <QLabel>
#include <QString>

class GuiUtils
{
public:
    //GuiUtils();
    // Setzt einen QLabel mit optionalem Link
    static void setLabelWithOptionalLink(QLabel* label, const QString& text, const QString& link);

};

#endif // GUIUTILS_H
