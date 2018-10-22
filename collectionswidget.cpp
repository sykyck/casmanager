#include "collectionswidget.h"
#include "qslog/QsLog.h"
#include <QMessageBox>
#include <QHeaderView>

CollectionsWidget::CollectionsWidget(DatabaseMgr *dbMgr, CasServer *casServer, Settings *settings, QTabWidget *parentTab, QWidget *parent) : QWidget(parent)
{
  this->dbMgr = dbMgr;
  this->casServer = casServer;
  this->settings = settings;
  this->parentTab = parentTab;

  busy = false;
  verticalLayout = new QVBoxLayout;
  buttonLayout = new QHBoxLayout;
  totalLayout = new QHBoxLayout;

  // Name, Type, Address, Credits, Cash, Programmed, CC Charges, Last Used, Last Collection
  collectionStatusModel = new QStandardItemModel(0, 8);
  collectionStatusModel->setHorizontalHeaderItem(0, new QStandardItem(QString("Name")));  
  collectionStatusModel->setHorizontalHeaderItem(1, new QStandardItem(QString("Type")));
  collectionStatusModel->setHorizontalHeaderItem(2, new QStandardItem(QString("Address")));
  collectionStatusModel->setHorizontalHeaderItem(3, new QStandardItem(QString("Credits")));
  collectionStatusModel->setHorizontalHeaderItem(4, new QStandardItem(QString("Cash")));
  collectionStatusModel->setHorizontalHeaderItem(5, new QStandardItem(QString("Programmed")));
  collectionStatusModel->setHorizontalHeaderItem(6, new QStandardItem(QString("CC Charges")));
  //collectionStatusModel->setHorizontalHeaderItem(7, new QStandardItem(QString("Last Used")));
  //collectionStatusModel->setHorizontalHeaderItem(8, new QStandardItem(QString("Last Collection")));
  collectionStatusModel->setHorizontalHeaderItem(7, new QStandardItem(QString("Collection Status")));

  collectionStatusView = new QTableView;

  lblCollectionStatus = new QLabel(tr("Booth Collection"));

  collectionStatusView->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
  collectionStatusView->horizontalHeader()->setStretchLastSection(true);
  collectionStatusView->horizontalHeader()->setStyleSheet("font:bold Arial;");
  collectionStatusView->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
  collectionStatusView->setSelectionMode(QAbstractItemView::SingleSelection);
  collectionStatusView->setSelectionBehavior(QAbstractItemView::SelectRows);  
  collectionStatusView->setWordWrap(true);
  collectionStatusView->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
  collectionStatusView->verticalHeader()->hide();
  collectionStatusView->setModel(collectionStatusModel);
  // Make table read-only
  collectionStatusView->setEditTriggers(QAbstractItemView::NoEditTriggers);

  btnCollectBooths = new QPushButton(tr("Collect Booths"));
  connect(btnCollectBooths, SIGNAL(clicked()), this, SLOT(collectBoothsClicked()));

  btnToggleAlarm = new QPushButton(tr("Alarm Enabled"));
  btnToggleAlarm->setCheckable(true);
  btnToggleAlarm->setStyleSheet("QPushButton {background-color:green; color:white;} QPushButton:checked {background-color:red; color:white;}");
  connect(btnToggleAlarm, SIGNAL(clicked()), this, SLOT(toggleAlarmClicked()));
  alarmEnabled = true;

  btnClearCollection = new QPushButton(tr("Clear Collection"));
  btnClearCollection->setEnabled(false);
  connect(btnClearCollection, SIGNAL(clicked()), this, SLOT(clearCollectionClicked()));

  lblTotalCash = new QLabel(tr("Total Cash"));
  txtTotalCash = new QLineEdit(tr("$0.00"));
  txtTotalCash->setReadOnly(true);

  totalLayout->addWidget(lblTotalCash);
  totalLayout->addWidget(txtTotalCash);
  totalLayout->addWidget(btnClearCollection);
  totalLayout->addStretch(1);

  buttonLayout->addWidget(btnCollectBooths);
  buttonLayout->addWidget(btnToggleAlarm);
  buttonLayout->addStretch(1);

  verticalLayout->addWidget(lblCollectionStatus, 0, Qt::AlignLeft);
  verticalLayout->addWidget(collectionStatusView);
  verticalLayout->addLayout(totalLayout);
  verticalLayout->addSpacing(3);
  verticalLayout->addLayout(buttonLayout);

  this->setLayout(verticalLayout);

  // Perform a collection by clicking a button - contact each booth, request collection
  // Clicking a toggle button turns all alarms off and clicking again turns them all on

  connect(this->casServer, SIGNAL(toggleAlarmFailed(QString)), this, SLOT(toggleAlarmFailed(QString)));
  connect(this->casServer, SIGNAL(toggleAlarmSuccess(QString)), this, SLOT(toggleAlarmSuccess(QString)));
  connect(this->casServer, SIGNAL(toggleAlarmFinished()), this, SLOT(toggleAlarmFinished()));

  connect(this->casServer, SIGNAL(collectionFailed(QString)), this, SLOT(collectionFailed(QString)));
  connect(this->casServer, SIGNAL(collectionSuccess(QString)), this, SLOT(collectionSuccess(QString)));
  connect(this->casServer, SIGNAL(collectionFinished()), this, SLOT(collectionFinished()));
}

CollectionsWidget::~CollectionsWidget()
{
  verticalLayout->deleteLater();
}

bool CollectionsWidget::isBusy()
{
  return false;
}

void CollectionsWidget::showEvent(QShowEvent *)
{
  QLOG_DEBUG() << QString("Showing the Collections > Collect tab");

  disconnect(casServer, SIGNAL(sendCollectionFailed(QString)), 0, 0);
  disconnect(casServer, SIGNAL(sendCollectionSuccess(QString)), 0, 0);
  disconnect(casServer, SIGNAL(sendCollectionFinished()), 0, 0);

  connect(casServer, SIGNAL(sendCollectionFailed(QString)), this, SLOT(sendCollectionFailed(QString)));
  connect(casServer, SIGNAL(sendCollectionSuccess(QString)), this, SLOT(sendCollectionSuccess(QString)));
  connect(casServer, SIGNAL(sendCollectionFinished()), this, SLOT(sendCollectionFinished()));
}

void CollectionsWidget::collectBoothsClicked()
{
  QLOG_DEBUG() << "User clicked the Collect Booths button";

  if (casServer->isBusy())
  {
    QLOG_DEBUG() << "casServer has another thread running, cannot start collection";
    QMessageBox::warning(this, tr("Collection"), tr("Cannot collect booths because the Booth Status tab is checking the status of the arcade. Please wait a moment and try again."));
  }
  else
  {
    if (QMessageBox::question(this, tr("Collection"), tr("All meters will be collected and cleared. Are you sure you want to continue?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
    {
      QLOG_DEBUG() << "User chose to continue with collecting booths";

      // Clear table if it currently has rows
      if (collectionStatusModel->rowCount() > 0)
        collectionStatusModel->removeRows(0, collectionStatusModel->rowCount());

      btnCollectBooths->setEnabled(false);
      btnToggleAlarm->setEnabled(false);

      busy = true;

      // Disable both tabs while operation is in progress
      if (parentTab)
        parentTab->setEnabled(false);

      collectFailList.clear();
      collectSuccessList.clear();

      // Using device list, open each casplayer_<address>.sqlite database
      // device_num, device_type, device_type_id, device_alias, found, num_channels, last_used, current_cash, free_space_gb
      QVariantList boothList = dbMgr->getBoothInfo(casServer->getDeviceAddresses());

      foreach (QVariant v, boothList)
      {
        QVariantMap booth = v.toMap();

        // Populate table view with all booths that will be collected
        insertBoothCollection(booth["device_alias"].toString(), settings->getValue(ARCADE_SUBNET).toString() + booth["device_num"].toString(), booth["device_type"].toString(), 0, 0, 0, 0, QDateTime(), QDateTime(), "Pending");
      }

      // Request collection
      casServer->startCollection();
    }
    else
      QLOG_DEBUG() << "User canceled collecting booths";
  }
}

void CollectionsWidget::toggleAlarmClicked()
{
  QLOG_DEBUG() << "User clicked the Enable/Disable Alarm button";

  if (casServer->isBusy())
  {
    QLOG_DEBUG() << "casServer has another thread running, cannot start toggling booth alarms";
    QMessageBox::warning(this, tr("Booth Alarms"), tr("Cannot toggle booth alarms because the Booth Status tab is checking the status of the arcade. Please wait a moment and try again."));
  }
  else
  {
    // FIXME: If user leaves the alarms disabled and close the software then the booth alarms remain disabled. When the
    // software starts again, the alarms will still be disabled but it will think alarms are enabled. Need to either stop user
    // from closing software if alarms are disabled, have a time limit or check booths for alarm status on startup
    if (btnToggleAlarm->isChecked())
    {
      if (QMessageBox::question(this, tr("Disable Alarm"), tr("Are you sure you want to disable the alarm in all booths?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
      {
        QLOG_DEBUG() << "User chose to disable the alarm in all booths";

        btnCollectBooths->setEnabled(false);
        btnToggleAlarm->setEnabled(false);

        busy = true;

        toggleAlarmFailList.clear();
        toggleAlarmSuccessList.clear();

        alarmEnabled = false;

        // Request alarm to be disabled
        casServer->startToggleAlarmState(false);
      }
      else
      {
        QLOG_DEBUG() << "User canceled disabling the alarm in all booths";
        btnToggleAlarm->setChecked(false);
      }
    }
    else
    {
      if (QMessageBox::question(this, tr("Enable Alarm"), tr("Do you want to enable the alarm in all booths?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
      {
        QLOG_DEBUG() << "User chose to enable the alarm in all booths";

        btnCollectBooths->setEnabled(false);
        btnToggleAlarm->setEnabled(false);

        busy = true;

        toggleAlarmFailList.clear();
        toggleAlarmSuccessList.clear();

        alarmEnabled = true;

        // Request alarm to be enabled
        casServer->startToggleAlarmState(true);
      }
      else
      {
        QLOG_DEBUG() << "User canceled enabling the alarm in all booths";
        btnToggleAlarm->setChecked(true);
      }
    }

    if (btnToggleAlarm->isChecked())
      btnToggleAlarm->setText(tr("Alarm Disabled!!!"));
    else
      btnToggleAlarm->setText(tr("Alarm Enabled"));
  }
}

void CollectionsWidget::clearCollectionClicked()
{
  if (QMessageBox::question(this, tr("Clear Collection"), tr("Do you want to clear the total cash field and the booth details above?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
  {
    QLOG_DEBUG() << "User chose to clear collection";

    btnClearCollection->setEnabled(false);

    txtTotalCash->setText(tr("$0.00"));

    // Clear table if it currently has rows
    if (collectionStatusModel->rowCount() > 0)
      collectionStatusModel->removeRows(0, collectionStatusModel->rowCount());
  }
  else
  {
    QLOG_DEBUG() << "User canceled clearing collection";
  }
}

void CollectionsWidget::collectionSuccess(QString ipAddress)
{
  collectSuccessList.append(ipAddress);

  QString deviceNum = casServer->ipAddressToDeviceNum(ipAddress);
  QStringList deviceList;
  deviceList << deviceNum;

  QVariantList boothList = dbMgr->getLatestBoothCollection(deviceList);

  QVariantMap booth = boothList.first().toMap();

  // Update collection row
  updateBoothCollection(ipAddress, booth["current_credits"].toInt(), booth["current_cash"].toReal() / 4, booth["current_prog_credits"].toInt(), booth["current_cc_charges"].toReal() / 4, QDateTime(), QDateTime(), "Collected");
}

void CollectionsWidget::collectionFailed(QString ipAddress)
{
  collectFailList.append(ipAddress);

  updateBoothCollection(ipAddress, 0, 0, 0, 0, QDateTime(), QDateTime(), "Failed");
}

void CollectionsWidget::collectionFinished()
{
  QLOG_DEBUG() << QString("Finished booth collection. Success: %1, Failed: %2")
              .arg(collectSuccessList.join(", "))
              .arg(collectFailList.join(", "));

  btnCollectBooths->setEnabled(true);
  btnToggleAlarm->setEnabled(true);

  qreal totalCash = 0;

  for (int row = 0; row < collectionStatusModel->rowCount(); ++row)
  {
    totalCash += collectionStatusModel->item(row, 4)->data().toReal();
  }

  txtTotalCash->setText(QString("$%L1").arg(totalCash, 0, 'f', 2));

  btnClearCollection->setEnabled(true);

  if (collectFailList.count() > 0)
    QMessageBox::warning(this, tr("Collection"), tr("Not all devices could be collected. The following failed to respond: %1").arg(collectFailList.join(", ")));
  else
    QMessageBox::information(this, tr("Collection"), tr("All devices were collected."));

  if (settings->getValue(ENABLE_COLLECTION_REPORT).toBool() && settings->getValue(COLLECTION_REPORT_RECIPIENTS).toString().length() > 0)
  {
    QList<QDate> collectionDates = dbMgr->getCollectionDates(casServer->getDeviceAddresses());
    QVariantList currentCollection = dbMgr->getCollection(collectionDates.first(), casServer->getDeviceAddresses());
    casServer->sendCollection(collectionDates.first(), currentCollection);
  }
  else
  {
    // Enable tabs since operation finished
    if (parentTab)
      parentTab->setEnabled(true);

    busy = false;
  }
}

void CollectionsWidget::toggleAlarmSuccess(QString ipAddress)
{
  toggleAlarmSuccessList.append(ipAddress);
}

void CollectionsWidget::toggleAlarmFailed(QString ipAddress)
{
  toggleAlarmFailList.append(ipAddress);
}

void CollectionsWidget::toggleAlarmFinished()
{
  QLOG_DEBUG() << QString("Finished toggling alarm to %1. Success: %2, Failed: %3")
                  .arg(alarmEnabled ? "ENABLED" : "DISABLED")
                  .arg(toggleAlarmSuccessList.join(", "))
                  .arg(toggleAlarmFailList.join(", "));

  btnCollectBooths->setEnabled(true);
  btnToggleAlarm->setEnabled(true);

  if (toggleAlarmFailList.count() > 0)
    QMessageBox::warning(this, tr("Alarm"), tr("Alarm could not be %1 in all devices. The following failed to respond: %2").arg(alarmEnabled ? "ENABLED" : "DISABLED").arg(toggleAlarmFailList.join(", ")));
  else
    QMessageBox::information(this, tr("Alarm"), tr("Alarm was %1 in all devices.").arg(alarmEnabled ? "ENABLED" : "DISABLED"));

  busy = false;
}

void CollectionsWidget::sendCollectionSuccess(QString message)
{
  QMessageBox::information(this, tr("Send Report Success"), message);
}

void CollectionsWidget::sendCollectionFailed(QString message)
{
  QMessageBox::warning(this, tr("Send Report Error"), message);
}

void CollectionsWidget::sendCollectionFinished()
{
  // Enable both tabs since operation finished
  if (parentTab)
    parentTab->setEnabled(true);

  busy = false;
}

void CollectionsWidget::insertBoothCollection(QString deviceAlias, QString ipAddress, QString deviceType, int credits, qreal cash, int programmed, qreal ccCharges, QDateTime lastUsed, QDateTime lastCollection, QString collectionStatus)
{
  // Name, Type, Address, Credits, Cash, Programmed, CC Charges, Last Used, Last Collection, Collection Status
  QStandardItem *deviceAliasField, *ipAddressField, *boothTypeField, *creditsField, *cashField, *programmedField, *ccChargesField, *lastUsedField, *lastCollectionField, *collectionStatusField;

  deviceAliasField = new QStandardItem(deviceAlias);
  deviceAliasField->setData(deviceAlias);

  ipAddressField = new QStandardItem(ipAddress);
  ipAddressField->setData(ipAddress);

  boothTypeField = new QStandardItem(deviceType);
  boothTypeField->setData(deviceType);

  creditsField = new QStandardItem(QString("%1").arg(credits));
  creditsField->setData(credits);

  cashField = new QStandardItem(QString("$%L1").arg(cash, 0, 'f', 2));
  cashField->setData(cash);

  programmedField = new QStandardItem(QString("%1").arg(programmed));
  programmedField->setData(programmed);

  ccChargesField = new QStandardItem(QString("$%L1").arg(ccCharges, 0, 'f', 2));
  ccChargesField->setData(ccCharges);

  lastUsedField = new QStandardItem(lastUsed.isValid() ? lastUsed.toString("MM/dd/yyyy h:mm AP") : "");
  lastUsedField->setData(lastUsed);

  lastCollectionField = new QStandardItem(lastCollection.isValid() ? lastCollection.toString("MM/dd/yyyy h:mm AP") : "");
  lastCollectionField->setData(lastCollection);

  collectionStatusField = new QStandardItem(collectionStatus);
  collectionStatusField->setData(collectionStatus);

  int row = collectionStatusModel->rowCount();

  collectionStatusModel->setItem(row, 0, deviceAliasField);
  collectionStatusModel->setItem(row, 1, boothTypeField);
  collectionStatusModel->setItem(row, 2, ipAddressField);
  collectionStatusModel->setItem(row, 3, creditsField);
  collectionStatusModel->setItem(row, 4, cashField);
  collectionStatusModel->setItem(row, 5, programmedField);
  collectionStatusModel->setItem(row, 6, ccChargesField);
 // collectionStatusModel->setItem(row, 7, lastUsedField);
 // collectionStatusModel->setItem(row, 8, lastCollectionField);
  collectionStatusModel->setItem(row, 7, collectionStatusField);
}

void CollectionsWidget::updateBoothCollection(QString ipAddress, int credits, qreal cash, int programmed, qreal ccCharges, QDateTime lastUsed, QDateTime lastCollection, QString collectionStatus)
{
  int row = findDevice(ipAddress);

  if (row > -1)
  {
    QStandardItem *creditsField = collectionStatusModel->item(row, 3);
    creditsField->setText(QString("%1").arg(credits));
    creditsField->setData(credits);

    QStandardItem *cashField = collectionStatusModel->item(row, 4);
    cashField->setText(QString("$%L1").arg(cash, 0, 'f', 2));
    cashField->setData(cash);

    QStandardItem *programmedField = collectionStatusModel->item(row, 5);
    programmedField->setText(QString("%1").arg(programmed));
    programmedField->setData(programmed);

    QStandardItem *ccChargesField = collectionStatusModel->item(row, 6);
    ccChargesField->setText(QString("$%L1").arg(ccCharges, 0, 'f', 2));
    ccChargesField->setData(ccCharges);
/*
    QStandardItem *lastUsedField = collectionStatusModel->item(row, 7);
    lastUsedField->setText(lastUsed.isValid() ? lastUsed.toString("MM/dd/yyyy h:mm AP") : "");
    lastUsedField->setData(lastUsed);

    QStandardItem *lastCollectionField = collectionStatusModel->item(row, 8);
    lastCollectionField->setText(lastCollection.isValid() ? lastCollection.toString("MM/dd/yyyy h:mm AP") : "");
    lastCollectionField->setData(lastCollection);
*/
    QStandardItem *collectionStatusField = collectionStatusModel->item(row, 7);
    collectionStatusField->setText(collectionStatus);
    collectionStatusField->setData(collectionStatus);
  }
  else
  {
    QLOG_ERROR() << "Could not find IP Address:" << ipAddress << "to update collection in collectionswidget";
  }
}

int CollectionsWidget::findDevice(QString ipAddress)
{
  int row = -1;

  // Look in Address column for a matching ip address
  QList<QStandardItem *> resultList = collectionStatusModel->findItems(ipAddress, Qt::MatchExactly, 2);

  if (resultList.count() > 0)
    row = resultList.first()->row();

  return row;
}
