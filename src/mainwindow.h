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
    bool m_showDeletedParts = false;    // neue Option


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

    // Neue Option zum Löschen und Ändern


private slots:
    void onFileActivated(QListWidgetItem* item);
    void addNewPart();

    // Neue Option zum Löschen und Ändern
    void onPartsContextMenuRequested(const QPoint& pos);
    void editPart(int id);
    void deletePart(int id);
    void toggleShowDeletedParts(bool checked);
    void restorePart(int id);
};

#endif // MAINWINDOW_H
