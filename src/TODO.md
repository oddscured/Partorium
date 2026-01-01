## Add features
[ ] add selectable currency symbol to settings dialog and new part dialog + main window 

[ ] add language selection to settings symbol and/or main menu bar 

[ ] show additional images when clicking on main part image

[ ] use new search textbox: https://doc.qt.io/qt-6/qml-qtquick-controls-searchfield.html?utm_source=installer&utm_medium=banner&utm_campaign=installer4Qt610

[ ] add comment function to add individual comments to each item (i.e. used in robot car project, etc.)

[ ] add batch-update function (select multiple parts in the list -> context-click -> "batch update" -> change property for all selected parts

[ ] add backup-function in DataImportDialog for partorium.json

[ ] add MetaData field to part.json (i.e. DataImportDialog adds source file information)

[ ] add DataExport function (export from json to csv)

[ ] add 'Next' and 'Back' buttons to the NewPartDialog to allow browsing through the parts list

##


## To fix / bugs
[ ] after changing a part do not reset category filters in mainwindow

[ ] fix categories in mainwindow 

[ ] fix "QPixmap::scaled: Pixmap is a null pixmap"

[ ] when clicking "choose image" in newpartdialog for the first time, the default folder from the settings is not used

[x] fix tab order in newpartdialog

[ ] list of files shows multiple entries of the same files

[ ] fix missing images in MessageDialogs (final delete, etc.)

[ ] fix word-wrap for storage label (all labels)

##

## Improvements

[ ] Add icons from here: https://fonts.google.com/icons

[ ] Add input validation (name!) to newPartsDialog

[x] Add "next part" button to newpartdialog for fast entering of parts 

[ ] Automatic backup of .json file

[ ] validation of .json file before loading

[x] show all files in parts folder

[x] add filetype icons in parts folder view (lst_Files)

[x] create separate dialog to create categories

[ ] show message when part name already exists

[ ] add tooltips to UI elements

[x] add menu to delete items permanently (with request-dialog)

[x] change tab order for newPartsDialog

[ ] add auto-delimiter-recognition in importDataDialog

[x] check and adjust Tab-Orders in every Dialog

[x] load the images in a separate thread to improve the UI's responsiveness. 

[x] add delete image button in newpartdialog

[ ] sort items in lists alphabtically

##

## Changes

[ ] move "select data file" to settings dialog

##

## Design

[ ] change view of startup screen (no part selected)

[ ] add icons for module types

[ ] use icons for categories on the left side[]

##
