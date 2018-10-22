#include "collectionreportswidget.h"
#include <QHeaderView>
#include <QMessageBox>
#include "qslog/QsLog.h"
#include "global.h"
#include <QSortFilterProxyModel>

CollectionReportsWidget::CollectionReportsWidget(DatabaseMgr *dbMgr, CasServer *casServer, Settings *settings, QTabWidget *parentTab, QWidget *parent) : QWidget(parent)
{
  this->dbMgr = dbMgr;
  this->casServer = casServer;
  this->settings = settings;
  this->parentTab = parentTab;

  firstLoad = true;
  busy = false;

  // device_alias, device_num, current_credits, total_credits, current_preview_credits, total_preview_credits, current_cash, total_cash, current_cc_charges, total_cc_charges, current_prog_credits, total_prog_credits
  collectionModel = new QStandardItemModel(0, 12);
  collectionModel->setHorizontalHeaderItem(0, new QStandardItem(QString("Name")));
  collectionModel->setHorizontalHeaderItem(1, new QStandardItem(QString("Address")));
  collectionModel->setHorizontalHeaderItem(2, new QStandardItem(QString("Credits")));
  collectionModel->setHorizontalHeaderItem(3, new QStandardItem(QString("NR Credits")));
  collectionModel->setHorizontalHeaderItem(4, new QStandardItem(QString("Cash")));
  collectionModel->setHorizontalHeaderItem(5, new QStandardItem(QString("NR Cash")));
  collectionModel->setHorizontalHeaderItem(6, new QStandardItem(QString("CC Charges")));
  collectionModel->setHorizontalHeaderItem(7, new QStandardItem(QString("NR CC Charges")));
  collectionModel->setHorizontalHeaderItem(8, new QStandardItem(QString("Previews")));
  collectionModel->setHorizontalHeaderItem(9, new QStandardItem(QString("NR Previews")));
  collectionModel->setHorizontalHeaderItem(10, new QStandardItem(QString("Programmed")));
  collectionModel->setHorizontalHeaderItem(11, new QStandardItem(QString("NR Programmed")));

  QSortFilterProxyModel *sortFilter = new QSortFilterProxyModel;
  sortFilter->setSourceModel(collectionModel);
  sortFilter->setSortRole(Qt::UserRole + 1);

  collectionTable = new QTableView;
  collectionTable->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
  collectionTable->horizontalHeader()->setStretchLastSection(true);
  collectionTable->horizontalHeader()->setStyleSheet("font:bold Arial;");
  collectionTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
  collectionTable->setSelectionMode(QAbstractItemView::SingleSelection);
  collectionTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  collectionTable->setWordWrap(true);
  collectionTable->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
  collectionTable->verticalHeader()->hide();
  collectionTable->setSortingEnabled(true);
  collectionTable->setModel(sortFilter);
  collectionTable->setEditTriggers(QAbstractItemView::NoEditTriggers); // Make table read-only
  collectionTable->setSelectionMode(QAbstractItemView::SingleSelection); // Make table read-only
  collectionTable->sortByColumn(1, Qt::AscendingOrder);

  lblCollectionDate = new QLabel(tr("Collection Date"));
  cmbCollectionDate = new QComboBox;
  cmbCollectionDate->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  // By setting the combobox to editable and without accepting any entries, the visible items in the list
  // can be limited. Otherwise the list could be as tall as the entire screen depending on how many items are in it
  cmbCollectionDate->setEditable(true);
  cmbCollectionDate->setInsertPolicy(QComboBox::NoInsert);
  connect(cmbCollectionDate, SIGNAL(currentIndexChanged(int)), this, SLOT(collectionDateChanged(int)));

  // Add the collection type combo box
  lblCollectionType = new QLabel(tr("Type"));
  cmbCollectionType = new QComboBox;
  cmbCollectionType->insertItem(0, tr("Weekly"));
  cmbCollectionType->insertItem(1, tr("Snapshot"));
  connect(cmbCollectionType, SIGNAL(currentIndexChanged(int)), this, SLOT(collectionTypeChanged(int)));
  cmbCollectionType->setProperty("currentIndex", 0);

  // Snap shot edit date, hidden by default
  edtSnapshotDate = new QDateEdit();
  edtSnapshotDate->hide();
  edtSnapshotDate->setCalendarPopup(true);
  edtSnapshotDate->setDisplayFormat("MM/dd/yyyy");
  connect(edtSnapshotDate, SIGNAL(dateChanged(QDate)), this, SLOT(snapshotDateChanged(QDate)));

  btnSendReport = new QPushButton(tr("Send Report"));
  connect(btnSendReport, SIGNAL(clicked()), this, SLOT(sendReportClicked()));

  lblTotalCredits = new QLabel(tr("Total Credits"));
  txtTotalCredits = new QLineEdit;
  txtTotalCredits->setReadOnly(true);

  lblTotalCash = new QLabel(tr("Total Cash"));
  txtTotalCash = new QLineEdit;
  txtTotalCash->setReadOnly(true);

  lblTotalCcCharges = new QLabel(tr("Total CC Charges"));
  txtTotalCcCharges = new QLineEdit;
  txtTotalCcCharges->setReadOnly(true);

  lblTotalPreviews = new QLabel(tr("Total Previews"));
  txtTotalPreviews = new QLineEdit;
  txtTotalPreviews->setReadOnly(true);

  lblTotalProgrammed = new QLabel(tr("Total Programmed"));
  txtTotalProgrammed = new QLineEdit;
  txtTotalProgrammed->setReadOnly(true);

  totalsLayout = new QHBoxLayout;
  totalsLayout->addWidget(lblTotalCredits);
  totalsLayout->addWidget(txtTotalCredits);
  totalsLayout->addWidget(lblTotalCash);
  totalsLayout->addWidget(txtTotalCash);
  totalsLayout->addWidget(lblTotalCcCharges);
  totalsLayout->addWidget(txtTotalCcCharges);
  totalsLayout->addWidget(lblTotalPreviews);
  totalsLayout->addWidget(txtTotalPreviews);
  totalsLayout->addWidget(lblTotalProgrammed);
  totalsLayout->addWidget(txtTotalProgrammed);
  totalsLayout->addStretch(2);

  reportCriteriaLayout = new QHBoxLayout;
  reportCriteriaLayout->addWidget(lblCollectionType);
  reportCriteriaLayout->addWidget(cmbCollectionType);
  reportCriteriaLayout->addWidget(lblCollectionDate);
  reportCriteriaLayout->addWidget(cmbCollectionDate);
  reportCriteriaLayout->addWidget(edtSnapshotDate);
  reportCriteriaLayout->addWidget(btnSendReport);
  reportCriteriaLayout->addStretch(2);

  verticalLayout = new QVBoxLayout;
  verticalLayout->addLayout(reportCriteriaLayout);
  verticalLayout->addWidget(collectionTable);
  verticalLayout->addLayout(totalsLayout);

  this->setLayout(verticalLayout);

  clearTotalFields();
}

CollectionReportsWidget::~CollectionReportsWidget()
{

}

bool CollectionReportsWidget::isBusy()
{
  return false;
}

void CollectionReportsWidget::showEvent(QShowEvent *)
{
  QLOG_DEBUG() << QString("Showing the Collections > Reports tab");

  // Refresh list of collection dates every time widget is shown to make sure we have the latest collections
  QList<QDate> collectionDates = dbMgr->getCollectionDates(casServer->getDeviceAddresses());

  cmbCollectionDate->blockSignals(true);

  // Determine current index so we can return to this item afterwards
  int currentIndex = cmbCollectionDate->currentIndex();
  QString currentText = cmbCollectionDate->itemData(currentIndex).toString();

  cmbCollectionDate->clear();
  cmbCollectionDate->addItem("");
  foreach (QDate d, collectionDates)
  {
    cmbCollectionDate->addItem(d.toString("MM/dd/yyyy"), d);
  }

  // After repopulating widget, make sure that the index we had is still pointing
  // to the same data, if not, search for it and update the currentIndex appropriately
  if (currentText != cmbCollectionDate->itemData(currentIndex).toString())
  {
    for (int i = 0; i < cmbCollectionDate->count(); ++i)
    {
      if (currentText == cmbCollectionDate->itemData(i).toString())
      {
        currentIndex = i;
        break;
      }
    }
  }

  // Return to the previously selected item
  cmbCollectionDate->setCurrentIndex(currentIndex);

  // Disable send report button if no date selected
  if (currentIndex < 1)
    btnSendReport->setEnabled(false);

  cmbCollectionDate->blockSignals(false);

  disconnect(casServer, SIGNAL(sendCollectionFailed(QString)), 0, 0);
  disconnect(casServer, SIGNAL(sendCollectionSuccess(QString)), 0, 0);
  disconnect(casServer, SIGNAL(sendCollectionFinished()), 0, 0);

  connect(casServer, SIGNAL(sendCollectionFailed(QString)), this, SLOT(sendCollectionFailed(QString)));
  connect(casServer, SIGNAL(sendCollectionSuccess(QString)), this, SLOT(sendCollectionSuccess(QString)));
  connect(casServer, SIGNAL(sendCollectionFinished()), this, SLOT(sendCollectionFinished()));
  connect(casServer,SIGNAL(sendCollectionSnapshotSuccess(QString)),this,SLOT(sendCollectionSuccess(QString)));
  connect(casServer,SIGNAL(sendCollectionSnapshotFailed(QString)),this,SLOT(sendCollectionFailed(QString)));
  connect(casServer, SIGNAL(sendCollectionSnapshotFinished()), this, SLOT(sendCollectionFinished()));

  // Update the min and max for the snapshot edit date widget
  QList<QDate> meterDates = dbMgr->getMeterDates();

  if (!meterDates.isEmpty()) {
    edtSnapshotDate->setMinimumDate(meterDates.last());
    edtSnapshotDate->setMaximumDate(meterDates.first());
  }
}

void CollectionReportsWidget::setCollectionModel(const QVariantList &model)
{
    currentCollection = model;

    if (currentCollection.count() > 0)
    {
      QLOG_DEBUG() << "Record count:" << currentCollection.count();

      qreal totalCredits = 0;
      qreal totalCash = 0;
      qreal totalCcCharges = 0;
      qreal totalPreviews = 0;
      qreal totalProgrammed = 0;

      foreach (QVariant v, currentCollection)
      {
        QVariantMap record = v.toMap();

        insertCollection(record);

        //  current_credits,  current_preview_credits, current_cash, current_cc_charges, current_prog_credits
        totalCredits += record["current_credits"].toInt();
        totalCash += record["current_cash"].toInt();
        totalCcCharges += record["current_cc_charges"].toInt();
        totalPreviews += record["current_preview_credits"].toInt();
        totalProgrammed += record["current_prog_credits"].toInt();
      }

      txtTotalCredits->setText(QString("$%L1").arg(totalCredits / 4, 0, 'f', 2));
      txtTotalCash->setText(QString("$%L1").arg(totalCash / 4, 0, 'f', 2));
      txtTotalCcCharges->setText(QString("$%L1").arg(totalCcCharges / 4, 0, 'f', 2));
      txtTotalPreviews->setText(QString("$%L1").arg(totalPreviews / 4, 0, 'f', 2));
      txtTotalProgrammed->setText(QString("$%L1").arg(totalProgrammed / 4, 0, 'f', 2));

      btnSendReport->setEnabled(true);
    }
    else
    {
      QLOG_DEBUG() << "No records returned";
    }

    //viewTimesTable->reset();
    collectionTable->scrollToTop();
    collectionTable->sortByColumn(collectionTable->horizontalHeader()->sortIndicatorSection(), collectionTable->horizontalHeader()->sortIndicatorOrder());
}

void CollectionReportsWidget::resetCollectionModel()
{
    QLOG_DEBUG() << "User selected the empty item in the collection date list";

    if (collectionModel->rowCount() > 0)
      collectionModel->removeRows(0, collectionModel->rowCount());

    //viewTimesTable->reset();
    collectionTable->scrollToTop();
    collectionTable->sortByColumn(collectionTable->horizontalHeader()->sortIndicatorSection(), collectionTable->horizontalHeader()->sortIndicatorOrder());

    btnSendReport->setEnabled(false);
}

void CollectionReportsWidget::collectionDateChanged(int index)
{
    if (cmbCollectionType->currentIndex() != 0) {
        // Ignore changes if type is snapshot.
        return;
    }
  clearTotalFields();

  if (index > 0)
  {
    QLOG_DEBUG() << "User selected collection date:" << cmbCollectionDate->itemData(index).toDate();

    if (collectionModel->rowCount() > 0)
      collectionModel->removeRows(0, collectionModel->rowCount());

    // Populate model collection results
    this->setCollectionModel(dbMgr->getCollection(cmbCollectionDate->itemData(index).toDate(), casServer->getDeviceAddresses()));

  }
  else
  {
    this->resetCollectionModel();
  }
}

void CollectionReportsWidget::collectionTypeChanged(int index)
{
    // Prepare for a fresh report.
    if (index == 0) {
        // Weekly report, hide the date edit.
        cmbCollectionDate->show();
        edtSnapshotDate->hide();
        // Disable send report button if no date selected
        btnSendReport->setEnabled(cmbCollectionDate->currentIndex() > 0);
        this->collectionDateChanged(cmbCollectionDate->currentIndex());
    }
    else if (index == 1) {
        // snapshot mode
        cmbCollectionDate->hide();
        edtSnapshotDate->show();
        // Disable send report button if no date selected
        btnSendReport->setEnabled(edtSnapshotDate->date().isValid());
        this->snapshotDateChanged(edtSnapshotDate->date());
    }
}

void CollectionReportsWidget::snapshotDateChanged(QDate selectedDate)
{
    if (cmbCollectionType->currentIndex() != 1) {
        // Ignore changes if type is weekly.
        return;
    }
    clearTotalFields();

    if (selectedDate.isValid())
    {
      QLOG_DEBUG() << "User selected snapshot date:" << selectedDate;

      if (collectionModel->rowCount() > 0)
        collectionModel->removeRows(0, collectionModel->rowCount());

      // Populate model collection results
      this->setCollectionModel(dbMgr->getCollectionSnapshot(casServer->getDeviceAddresses(), settings->getValue(COLLECTION_SNAPSHOT_INTERVAL).toInt(), selectedDate));
    }
    else
    {
      this->resetCollectionModel();
    }
}

void CollectionReportsWidget::sendReportClicked()
{
  QLOG_DEBUG() << "User clicked the Send Report button under Collection Reports tab";

  QString recipientSetting = cmbCollectionType->currentIndex() == 0
          ? COLLECTION_REPORT_RECIPIENTS
          : COLLECTION_SNAPSHOT_REPORT_RECIPIENTS;

  if (settings->getValue(recipientSetting).toString().length() == 0)
  {
    QLOG_DEBUG() << "No report recipients are setup";
    QMessageBox::information(this, tr("Send Report"), tr("There are no email recipients set up to receive the report. Go to Settings located under Tools on the main menu and enter one or more email addresses."));
  }
  else
  {
    if (QMessageBox::question(this, tr("Send Report"), tr("Do you want to email this collection report?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
    {
      if ((cmbCollectionType->currentIndex() == 0 && cmbCollectionDate->currentIndex() > 0)
              || (cmbCollectionType->currentIndex() == 1 && edtSnapshotDate->date().isValid())
              && collectionModel->rowCount() > 0)
      {
        QDate selectedDate = cmbCollectionType->currentIndex() == 0
                  ? cmbCollectionDate->itemData(cmbCollectionDate->currentIndex()).toDate()
                  : edtSnapshotDate->date();
        // Disable both tabs while operation is in progress
        if (parentTab)
          parentTab->setEnabled(false);


        if (cmbCollectionType->currentIndex() == 0) {
            casServer->sendCollection(selectedDate, currentCollection);
        }
        else if (cmbCollectionType->currentIndex() == 1) {
            casServer->sendCollectionSnapshot(currentCollection.at(0).toMap()["start_time"].toDateTime(),
                    currentCollection.at(0).toMap()["captured"].toDateTime(), currentCollection);
        }
      }
      else
      {
        QLOG_ERROR() << "No date and/or no collection data is in datagrid, cannot send collection report";
        QMessageBox::warning(this, tr("Send Report"), tr("There is no collection report to send. Select a collection date first before trying to send."));
      }
    }
    else
    {
      QLOG_DEBUG() << "User canceled sending collection report";
    }
  }
}

void CollectionReportsWidget::sendCollectionSuccess(QString message)
{
   QMessageBox::information(this, tr("Send Report Success"), message);
}

void CollectionReportsWidget::sendCollectionFailed(QString message)
{
  QMessageBox::warning(this, tr("Send Report Error"), message);
}

void CollectionReportsWidget::sendCollectionFinished()
{
  // Enable both tabs since operation finished
  if (parentTab)
    parentTab->setEnabled(true);
}

void CollectionReportsWidget::clearTotalFields()
{
  // Clear totals fields
  txtTotalCredits->setText(QString("$%L1").arg(0, 0, 'f', 2));
  txtTotalCash->setText(QString("$%L1").arg(0, 0, 'f', 2));
  txtTotalCcCharges->setText(QString("$%L1").arg(0, 0, 'f', 2));
  txtTotalPreviews->setText(QString("$%L1").arg(0, 0, 'f', 2));
  txtTotalProgrammed->setText(QString("$%L1").arg(0, 0, 'f', 2));
}

void CollectionReportsWidget::insertCollection(QVariantMap collection)
{
  // device_num, current_credits, total_credits, current_preview_credits, total_preview_credits, current_cash, total_cash, current_cc_charges, total_cc_charges, current_prog_credits, total_prog_credits
  QStandardItem *deviceAliasField, *deviceNumField, *currentCreditsField, *totalCreditsField, *currentPreviewsField, *totalPreviewsField, *currentCashField, *totalCashField, *currentCcChargesField, *totalCcChargesField, *currentProgCreditsField, *totalProgCreditsField;

  deviceAliasField = new QStandardItem(collection["device_alias"].toString());
  deviceAliasField->setData(collection["device_alias"].toString());

  deviceNumField = new QStandardItem(QString("%1%2").arg(settings->getValue(ARCADE_SUBNET).toString()).arg(collection["device_num"].toInt()));
  deviceNumField->setData(QString("%1").arg(collection["device_num"].toInt()));

  currentCreditsField = new QStandardItem(QString("$%L1 (%2)").arg(collection["current_credits"].toReal()  / 4, 0, 'f', 2).arg(collection["current_credits"].toInt()));
  currentCreditsField->setData(collection["current_credits"].toInt());

  totalCreditsField = new QStandardItem(QString("%1").arg(collection["total_credits"].toInt()));
  totalCreditsField->setData(collection["total_credits"].toInt());

  currentCashField = new QStandardItem(QString("$%L1 (%2)").arg(collection["current_cash"].toReal()  / 4, 0, 'f', 2).arg(collection["current_cash"].toInt()));
  currentCashField->setData(collection["current_cash"].toInt());

  totalCashField = new QStandardItem(QString("%1").arg(collection["total_cash"].toInt()));
  totalCashField->setData(collection["total_cash"].toInt());

  currentCcChargesField = new QStandardItem(QString("$%L1 (%2)").arg(collection["current_cc_charges"].toReal()  / 4, 0, 'f', 2).arg(collection["current_cc_charges"].toInt()));
  currentCcChargesField->setData(collection["current_cc_charges"].toInt());

  totalCcChargesField = new QStandardItem(QString("%1").arg(collection["total_cc_charges"].toInt()));
  totalCcChargesField->setData(collection["total_cc_charges"].toInt());

  currentPreviewsField = new QStandardItem(QString("$%L1 (%2)").arg(collection["current_preview_credits"].toReal()  / 4, 0, 'f', 2).arg(collection["current_preview_credits"].toInt()));
  currentPreviewsField->setData(collection["current_preview_credits"].toInt());

  totalPreviewsField = new QStandardItem(QString("%1").arg(collection["total_preview_credits"].toInt()));
  totalPreviewsField->setData(collection["total_preview_credits"].toInt());

  currentProgCreditsField = new QStandardItem(QString("$%L1 (%2)").arg(collection["current_prog_credits"].toReal()  / 4, 0, 'f', 2).arg(collection["current_prog_credits"].toInt()));
  currentProgCreditsField->setData(collection["current_prog_credits"].toInt());

  totalProgCreditsField = new QStandardItem(QString("%1").arg(collection["total_prog_credits"].toInt()));
  totalProgCreditsField->setData(collection["total_prog_credits"].toInt());

  int row = collectionModel->rowCount();

  collectionModel->setItem(row, 0, deviceAliasField);
  collectionModel->setItem(row, 1, deviceNumField);
  collectionModel->setItem(row, 2, currentCreditsField);
  collectionModel->setItem(row, 3, totalCreditsField);
  collectionModel->setItem(row, 4, currentCashField);
  collectionModel->setItem(row, 5, totalCashField);
  collectionModel->setItem(row, 6, currentCcChargesField);
  collectionModel->setItem(row, 7, totalCcChargesField);
  collectionModel->setItem(row, 8, currentPreviewsField);
  collectionModel->setItem(row, 9, totalPreviewsField);
  collectionModel->setItem(row, 10, currentProgCreditsField);
  collectionModel->setItem(row, 11, totalProgCreditsField);
}
