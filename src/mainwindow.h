#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QActionGroup>
#include <QStandardItemModel>
#include <QListWidgetItem>
#include <QTreeWidgetItem>
#include <QSettings>
#include <optional>
#include "jsonpartrepository.h"
#include "stockmanagementrepository.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    enum class GroupingMode {
        None,
        Category,
        Subcategory,
        Type,
        Format
    };

private:
    Ui::MainWindow *ui;
    JsonPartRepository* m_repo = nullptr;
    QStandardItemModel* m_imagesModel = nullptr;
    bool m_showDeletedParts = false;
    bool m_InitializeNewPartFileds = false;
    bool m_startWithRandom = false;
    GroupingMode m_groupingMode = GroupingMode::Category;
    QActionGroup* m_groupingActionGroup = nullptr;
    StockManagementRepository m_stockRepo;


    //void buildMenus();
    void refillCategories();
    void applyFilters();                              // nutzt Suche + Kategorie
    void refreshPartList(const QVector<Part>& parts);
    void showPart(const Part& p);
    void selectPartById(int id);
    void populatePartTree(const QVector<Part>& parts);
    QTreeWidgetItem* findPartItemById(int id) const;
    QString groupKeyForPart(const Part& p) const;
    QString groupingSettingKey() const;
    void setupGroupingMenu();
    void setGroupingMode(GroupingMode mode);
    void cycleGroupingMode();
    void updateCycleCategorizationButtonText();
    void chooseDatabasePath();
    void revealDatabaseFolder();
    void loadOrInitRepository(const QString& path = QString());
    void openSettingsDialog();
    void openListManager();
    std::optional<int> currentSelectedPartId() const;
    void refreshAfterStockChange(int partId);

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
    void addStockForSelectedPart();
    void removeStockForSelectedPart();
    void openStockHistoryForSelectedPart();

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
