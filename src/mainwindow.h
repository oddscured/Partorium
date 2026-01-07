#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>
#include <QListWidgetItem>
#include <QSettings>
#include "jsonpartrepository.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    JsonPartRepository* m_repo = nullptr;
    QStandardItemModel* m_imagesModel = nullptr;
    bool m_showDeletedParts = false;
    bool m_InitializeNewPartFileds = false;
    bool m_startWithRandom = false;


    //void buildMenus();
    void refillCategories();
    void applyFilters();                              // nutzt Suche + Kategorie
    void refreshPartList(const QVector<Part>& parts);
    void showPart(const Part& p);
    void selectPartById(int id);
    void chooseDatabasePath();
    void revealDatabaseFolder();
    void loadOrInitRepository(const QString& path = QString());
    void openSettingsDialog();
    void openListManager();

    // Ansichts-Menüs
    void startWithRandomPartIfEnabled(); // Anzeige eines zufälligen Bauteils beim Programmstart

    // Asynchrone Bildanzeige
    int m_imageRequestId = 0;
    void requestAndShowImageAsync(const QString& imagePath);

    //void on_act_ManagePresets_triggered();
    // Neue Option zum Löschen und Ändern

    // Dialoge
    void openBatchEditDialog(const QVector<int>& ids);

private slots:
    void onFileActivated(QListWidgetItem* item);
    void addNewPart();
    void openImportDataDialog();

    // Neue Option zum Löschen und Ändern
    void onPartsContextMenuRequested(const QPoint& pos);
    void editPart(int id);
    void deletePart(int id);
    void deletePartFinal(int id);
    void toggleShowDeletedParts(bool checked);
    void toggleFieldReset(bool checked);
    void restorePart(int id);
};

#endif // MAINWINDOW_H
