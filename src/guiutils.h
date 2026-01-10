#pragma once
#ifndef GUIUTILS_H
#define GUIUTILS_H
#include <QLabel>
#include <QString>
#include <QComboBox>
#include <QMap>
#include <QStringList>

class GuiUtils
{
    // Typalias für die Preset-Map
    using PresetsMap = QMap<QString, QStringList>;

public:
    //GuiUtils();
    // Setzt einen QLabel mit optionalem Link
    static void setLabelWithOptionalLink(QLabel* label, const QString& text, const QString& link);

    // Combobox mit Werten aus Preset-Map füllen
    static void applyPresetToCombo(QComboBox *combo,
                            const PresetsMap &presets,
                            const QString &key,
                            bool editable = true);

    // Suchdialog für Bilder, mit Vorgabe eines Such-Strings
    static QString getImageFileNameWithSearchString(QWidget* parent, QString searchName, QString dir);

    // Standard-Währungssymbol
    static QString getCurrencySymbol();
};

#endif // GUIUTILS_H
