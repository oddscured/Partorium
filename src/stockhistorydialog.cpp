#include "stockhistorydialog.h"
#include "ui_stockhistorydialog.h"
#include "jsonpartrepository.h"
#include "stockmanagementrepository.h"
#include "stockentry.h"
#include <QHeaderView>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>

StockHistoryDialog::StockHistoryDialog(JsonPartRepository* partRepo,
                                       StockManagementRepository* stockRepo,
                                       QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::StockHistoryDialog)
    , m_partRepo(partRepo)
    , m_stockRepo(stockRepo)
    , m_model(new QStandardItemModel(this))
    , m_proxyModel(new QSortFilterProxyModel(this))
{
    ui->setupUi(this);
    setWindowTitle(tr("Bestandsaenderungen"));

    m_proxyModel->setSourceModel(m_model);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxyModel->setFilterKeyColumn(-1);
    ui->tbl_StockHistory->setModel(m_proxyModel);
    ui->tbl_StockHistory->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tbl_StockHistory->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tbl_StockHistory->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tbl_StockHistory->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->tbl_StockHistory->horizontalHeader()->setStretchLastSection(true);

    hookUpSignals();
    loadEntries();
}

StockHistoryDialog::~StockHistoryDialog()
{
    delete ui;
}

void StockHistoryDialog::setInitialSearchText(const QString& text) {
    ui->lne_Search->setText(text);
    applySearchFilter(text);
}

void StockHistoryDialog::hookUpSignals() {
    connect(ui->lne_Search, &QLineEdit::textChanged, this, &StockHistoryDialog::applySearchFilter);
    connect(ui->btn_Close, &QPushButton::clicked, this, &QDialog::reject);
    connect(ui->btn_Delete, &QPushButton::clicked, this, &StockHistoryDialog::handleDeleteCurrentRow);
}

bool StockHistoryDialog::loadEntries() {
    if (!m_stockRepo) return false;
    if (!m_stockRepo->loadEntries(m_entries)) return false;
    rebuildModel();
    applySearchFilter(ui->lne_Search->text());
    return true;
}

void StockHistoryDialog::rebuildModel() {
    m_model->clear();
    m_model->setHorizontalHeaderLabels({
        tr("entryIndex"), tr("Datum"), tr("Aktion"), tr("ID"), tr("Bauteil"),
        tr("Projekt"), tr("Quelle"), tr("Menge"), tr("Gelöscht"), tr("Kommentar")
    });

    for (int i = 0; i < m_entries.size(); ++i) {
        const StockEntry& e = m_entries[i];
        if (e.deleted) continue; // deleted entries are hidden in table

        QList<QStandardItem*> row;
        row << new QStandardItem(QString::number(i));
        row << new QStandardItem(e.date.toLocalTime().toString("yyyy.MM.dd HH:mm"));
        const QString actionText = (e.type == "add") ? tr("Zugang")
                                   : (e.type == "remove") ? tr("Abgang")
                                                          : e.type;
        row << new QStandardItem(actionText);
        row << new QStandardItem(QString::number(e.partId));
        row << new QStandardItem(e.partName);
        row << new QStandardItem(e.title);
        row << new QStandardItem(e.source);
        row << new QStandardItem(QString::number(e.quantity));
        row << new QStandardItem(e.deleted ? "true" : "false");
        row << new QStandardItem(e.comment);
        m_model->appendRow(row);
    }

    // internal mapping column, not visible for users
    ui->tbl_StockHistory->setColumnHidden(0, true);
}

void StockHistoryDialog::applySearchFilter(const QString& text) {
    const QString pattern = text.trimmed();
    if (pattern.isEmpty()) {
        m_proxyModel->setFilterRegularExpression(QRegularExpression());
        return;
    }

    m_proxyModel->setFilterRegularExpression(
        QRegularExpression(QRegularExpression::escape(pattern), QRegularExpression::CaseInsensitiveOption));
}

void StockHistoryDialog::handleDeleteCurrentRow() {
    if (!m_partRepo || !m_stockRepo) return;

    const QModelIndex proxyIndex = ui->tbl_StockHistory->currentIndex();
    if (!proxyIndex.isValid()) {
        QMessageBox::information(this, tr("Hinweis"), tr("Bitte zuerst einen Eintrag auswaehlen."));
        return;
    }

    const QModelIndex proxyRowIndex = proxyIndex.sibling(proxyIndex.row(), 0);
    const QModelIndex sourceRowIndex = m_proxyModel->mapToSource(proxyRowIndex);
    if (!sourceRowIndex.isValid()) return;

    bool ok = false;
    const int entryIndex = m_model->item(sourceRowIndex.row(), 0)->text().toInt(&ok);
    if (!ok || entryIndex < 0 || entryIndex >= m_entries.size()) {
        QMessageBox::warning(this, tr("Fehler"), tr("Historieneintrag konnte nicht gelesen werden."));
        return;
    }

    StockEntry& entry = m_entries[entryIndex];
    if (entry.deleted) {
        QMessageBox::information(this, tr("Hinweis"), tr("Der Eintrag ist bereits geloescht."));
        return;
    }

    const auto answer = QMessageBox::question(
        this,
        tr("Buchung loeschen"),
        tr("Sind Sie sicher, dass diese Buchung geloescht werden soll?"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (answer != QMessageBox::Yes) {
        return;
    }

    auto partOpt = m_partRepo->getPart(entry.partId);
    if (!partOpt) {
        QMessageBox::warning(this, tr("Fehler"),
                             tr("Das zugehoerige Bauteil wurde nicht gefunden. Loeschen nicht moeglich."));
        return;
    }

    Part part = *partOpt;
    if (entry.type == "remove") {
        part.quantity += entry.quantity;
    } else if (entry.type == "add") {
        if (part.quantity - entry.quantity < 0) {
            QMessageBox::warning(this, tr("Nicht genug Bestand"),
                                 tr("Loeschen nicht moeglich: Der Bestand wuerde negativ werden."));
            return;
        }
        part.quantity -= entry.quantity;
    } else {
        QMessageBox::warning(this, tr("Fehler"),
                             tr("Unbekannter Historientyp. Loeschen nicht moeglich."));
        return;
    }

    if (!m_partRepo->updatePart(part)) {
        QMessageBox::warning(this, tr("Fehler"),
                             tr("Die Part-Menge konnte nicht aktualisiert werden."));
        return;
    }

    entry.deleted = true;
    if (!m_stockRepo->saveEntries(m_entries)) {
        QMessageBox::warning(this, tr("Fehler"),
                             tr("Historie konnte nicht gespeichert werden."));
        return;
    }

    rebuildModel();
    applySearchFilter(ui->lne_Search->text());
}
