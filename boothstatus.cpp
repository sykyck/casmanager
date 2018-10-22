#include "boothstatus.h"
#include "global.h"
#include "qslog/QsLog.h"
#include <QMessageBox>
#include <QHeaderView>

BoothStatus::BoothStatus(DatabaseMgr *dbMgr, CasServer *casServer, Settings *settings, QWidget *parent) : QWidget(parent)
{
  this->dbMgr = dbMgr;
  this->casServer = casServer;
  this->settings = settings;
  firstLoad = true;
  scanningNetwork = false;
  busy = false;
  silentRestart = false;
  reloadVideoLibrary = false;
  numMinutes = 0;

  statusTimer = new QTimer;
  //statusTimer->setInterval(settings->getValue(BOOTH_STATUS_INTERVAL).toInt());
  statusTimer->setInterval(60000);
  connect(statusTimer, SIGNAL(timeout()), this, SLOT(updateBoothStatus()));

  btnCheckDevices = new QPushButton(tr("Update Status"));
  connect(btnCheckDevices, SIGNAL(clicked()), this, SLOT(checkDevicesClicked()));

  btnRestartDevices = new QPushButton(tr("Restart Devices"));
  connect(btnRestartDevices, SIGNAL(clicked()), this, SLOT(restartDevicesClicked()));

  connect(casServer, SIGNAL(scanningDevice(QString)), this, SLOT(scanningDevice(QString)));
  connect(casServer, SIGNAL(scanDeviceSuccess(QString)), this, SLOT(scanDeviceSuccess(QString)));
  connect(casServer, SIGNAL(scanDeviceFailed(QString)), this, SLOT(scanDeviceFailed(QString)));
  connect(casServer, SIGNAL(scanDevicesFinished()), this, SLOT(scanDevicesFinished()));

  connect(casServer, SIGNAL(getBoothDatabaseSuccess(QString)), this, SLOT(getBoothDatabaseSuccess(QString)));
  connect(casServer, SIGNAL(getBoothDatabaseFailed(QString)), this, SLOT(getBoothDatabaseFailed(QString)));
  connect(casServer, SIGNAL(getBoothDatabasesFinished()), this, SLOT(getBoothDatabasesFinished()));

  connect(casServer, SIGNAL(checkDeviceSuccess(QString,bool)), this, SLOT(checkDeviceSuccess(QString,bool)));
  connect(casServer, SIGNAL(checkDeviceFailed(QString)), this, SLOT(checkDeviceFailed(QString)));
  connect(casServer, SIGNAL(checkDevicesFinished()), this, SLOT(checkDevicesFinished()));

  connect(casServer, SIGNAL(restartDeviceSuccess(QString)), this, SLOT(restartDeviceSuccess(QString)));
  connect(casServer, SIGNAL(restartDeviceFailed(QString)), this, SLOT(restartDeviceFailed(QString)));
  connect(casServer, SIGNAL(restartDeviceInSession(QString)), this, SLOT(restartDeviceInSession(QString)));
  connect(casServer, SIGNAL(restartDevicesFinished()), this, SLOT(restartDevicesFinished()));  

  connect(dbMgr, SIGNAL(getBoothStatusesFinished(QList<QJsonObject>&,bool)), this, SLOT(getBoothStatusesFinished(QList<QJsonObject>&,bool)));
  connect(dbMgr, SIGNAL(deleteBoothStatusFinished(QString,bool)), this, SLOT(deleteBoothStatusFinished(QString,bool)));
  connect(dbMgr, SIGNAL(addBoothStatusFinished(QString,bool)), this, SLOT(addBoothStatusFinished(QString,bool)));

  // Name, Type, Address, Credits, Cash, Programmed, CC Charges, Last Used, Bill Acceptor, Status
  boothStatusModel = new QStandardItemModel(0, 10);
  boothStatusModel->setHorizontalHeaderItem(Name, new QStandardItem(QString("Name")));
  boothStatusModel->setHorizontalHeaderItem(Booth_Type, new QStandardItem(QString("Type")));
  boothStatusModel->setHorizontalHeaderItem(Address, new QStandardItem(QString("Address")));
  boothStatusModel->setHorizontalHeaderItem(Credits, new QStandardItem(QString("Credits")));
  boothStatusModel->setHorizontalHeaderItem(Cash, new QStandardItem(QString("Cash")));
  boothStatusModel->setHorizontalHeaderItem(Programmed, new QStandardItem(QString("Programmed")));
  boothStatusModel->setHorizontalHeaderItem(CC_Charges, new QStandardItem(QString("CC Charges")));
  boothStatusModel->setHorizontalHeaderItem(Last_Used, new QStandardItem(QString("Last Used")));
  boothStatusModel->setHorizontalHeaderItem(Bill_Acceptor, new QStandardItem(QString("Bill Acceptor"))); // In Service, Out of Service
  boothStatusModel->setHorizontalHeaderItem(Status, new QStandardItem(QString("Status"))); // Idle, In Session, Down

  lblCurrentBoothStatus = new QLabel(tr("Current Booth Status"));
  boothStatusView = new QTableView;
  boothStatusView->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
  boothStatusView->horizontalHeader()->setStretchLastSection(true);
  boothStatusView->horizontalHeader()->setStyleSheet("font:bold Arial;");
  boothStatusView->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
  boothStatusView->setSelectionMode(QAbstractItemView::SingleSelection);
  boothStatusView->setSelectionBehavior(QAbstractItemView::SelectRows);
  boothStatusView->setWordWrap(true);
  boothStatusView->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
  boothStatusView->verticalHeader()->hide();
  boothStatusView->setModel(boothStatusModel);
  boothStatusView->setAlternatingRowColors(true);
  // Make table read-only
  boothStatusView->setEditTriggers(QAbstractItemView::NoEditTriggers);
  boothStatusView->setSelectionMode(QAbstractItemView::NoSelection);

  buttonLayout = new QHBoxLayout;
  buttonLayout->addWidget(btnCheckDevices);
  buttonLayout->addWidget(btnRestartDevices);
  buttonLayout->addStretch(1);

  verticalLayout = new QVBoxLayout;
  verticalLayout->addWidget(lblCurrentBoothStatus);
  verticalLayout->addWidget(boothStatusView);
  verticalLayout->addLayout(buttonLayout);

  this->setLayout(verticalLayout);
}

BoothStatus::~BoothStatus()
{
  statusTimer->deleteLater();
}

bool BoothStatus::isBusy()
{
  return busy;
}

void BoothStatus::showEvent(QShowEvent *)
{
  QLOG_DEBUG() << QString("Showing Booth Status tab");

  if (firstLoad)
  {
    if (settings->getValue(DEVICE_LIST).toStringList().count() == 0)
    {
      QLOG_DEBUG() << "Device list is empty, need to scan network for devices";

      QMessageBox::information(this, SOFTWARE_NAME, tr("The network has not been scanned yet for CAS devices. At least one device needs to exist on the network in order to use this software. Click OK to begin scanning."));

      QLOG_DEBUG() << "Scanning for devices";

      btnCheckDevices->setEnabled(false);
      btnRestartDevices->setEnabled(false);

      scanningNetwork = true;
      busy = true;
      casServer->startScanDevices();
    }
    else
    {
      QLOG_DEBUG() << QString("Device list: %1").arg(settings->getValue(DEVICE_LIST).toStringList().join(", "));
      populateTable();
      updateBoothStatus(true);

      statusTimer->start();
      //dailyReportTimer->start();
    }

    firstLoad = false;
  }
}

void BoothStatus::scanningDevice(QString ipAddress)
{
  QLOG_DEBUG() << QString("Scanning IP address: %1...").arg(ipAddress);

  insertBoothStatus("", "", ipAddress, 0, 0, 0, 0, QDateTime(), "", "Contacting...");
}

void BoothStatus::scanDeviceSuccess(QString ipAddress)
{
  QLOG_DEBUG() << QString("Found booth at: %1").arg(ipAddress);

  int row = findBooth(ipAddress);

  if (row > -1)
  {
    QStandardItem *statusField = boothStatusModel->item(row, Status);
    statusField->setText("Responded");
    statusField->setData("Responded");

    QColor rowColor = Qt::green;
    statusField->setData(rowColor, Qt::BackgroundColorRole);
  }
  else
    QLOG_ERROR() << QString("Got response from unexpected booth address: %1").arg(ipAddress);
}

void BoothStatus::scanDeviceFailed(QString ipAddress)
{
  QLOG_DEBUG() << QString("No response from: %1").arg(ipAddress);

  int row = findBooth(ipAddress);

  if (row > -1)
  {
    QStandardItem *statusField = boothStatusModel->item(row, Status);
    statusField->setText("No Response");
    statusField->setData("No Response");

    QColor rowColor = Qt::red;
    statusField->setData(rowColor, Qt::BackgroundColorRole);
  }
  else
    QLOG_ERROR() << QString("Got response from unexpected booth address: %1").arg(ipAddress);
}

void BoothStatus::scanDevicesFinished()
{
  QLOG_DEBUG() << QString("Finished scanning for devices");

  busy = false;
  updateBoothStatus(true);
}

void BoothStatus::checkDeviceSuccess(QString ipAddress, bool inSession)
{
  QLOG_DEBUG() << QString("Booth at %1 is %2").arg(ipAddress).arg(inSession ? "In Session" : "Idle");

  int row = findBooth(ipAddress);

  if (row > -1)
  {
    QStandardItem *statusField = boothStatusModel->item(row, Status);
    statusField->setText((inSession ? "In Session" : "Idle"));
    statusField->setData((inSession ? "In Session" : "Idle"));

    if (inSession)
    {
      QColor rowColor = Qt::yellow;
      statusField->setData(rowColor, Qt::BackgroundColorRole);
    }
    else
    {
      QColor rowColor = Qt::green;
      statusField->setData(rowColor, Qt::BackgroundColorRole);
    }

    // Reset counter for this booth, this is used to keep track
    // of how many failed communication attempts are made to
    // determine when to raise an alarm
    noResponseCount[ipAddress] = 0;

    // If booth address is currently in the list then remove it
    if (settings->getValue(BOOTHS_DOWN).toStringList().contains(ipAddress))
    {
      QStringList boothsDownList = settings->getValue(BOOTHS_DOWN).toStringList();
      boothsDownList.removeOne(ipAddress);
      settings->setValue(BOOTHS_DOWN, boothsDownList);

      Global::sendAlert(QString("Booth %1, IP Address: %2 is back online").arg(casServer->ipAddressToDeviceNum(ipAddress)).arg(ipAddress));

      // Save settings
      dbMgr->setValue("all_settings", settings->getSettings());
    }
  }
  else
  {
    QLOG_ERROR() << QString("Got response from unexpected booth address: %1").arg(ipAddress);
  }
}

void BoothStatus::checkDeviceFailed(QString ipAddress)
{
  QLOG_ERROR() << QString("No response from booth at %1").arg(ipAddress);

  int row = findBooth(ipAddress);

  if (row > -1)
  {
    QStandardItem *statusField = boothStatusModel->item(row, Status);
    statusField->setText("Down");
    statusField->setData("Down");

    QColor rowColor = Qt::red;
    statusField->setData(rowColor, Qt::BackgroundColorRole);

    // Increment counter for this booth, this is used to keep track
    // of how many failed communication attempts are made to
    // determine when to raise an alarm
    if (!noResponseCount.contains(ipAddress))
      noResponseCount[ipAddress] = 0;

    ++noResponseCount[ipAddress];

    // If booth address not currently in the list and passed the threshold then add now
    if (noResponseCount[ipAddress] > settings->getValue(BOOTH_NO_RESPONSE_THRESHOLD).toInt() &&
        !settings->getValue(BOOTHS_DOWN).toStringList().contains(ipAddress))
    {
      QStringList boothsDownList = settings->getValue(BOOTHS_DOWN).toStringList();
      boothsDownList.append(ipAddress);
      settings->setValue(BOOTHS_DOWN, boothsDownList);

      Global::sendAlert(QString("Booth %1, IP Address: %2 is DOWN").arg(casServer->ipAddressToDeviceNum(ipAddress)).arg(ipAddress));

      // Save settings
      dbMgr->setValue("all_settings", settings->getSettings());
    }
  }
  else
  {
    QLOG_ERROR() << QString("Got response from unexpected booth address: %1").arg(ipAddress);
  }
}

void BoothStatus::checkDevicesFinished()
{
  //QMessageBox::information(this, tr("Results"), tr("Finished checking booths."));

  // We're sharing casServer among the different tabs so disconnect our slots
  /*disconnect(casServer, SIGNAL(getBoothDatabaseSuccess(QString)), 0, 0);
  disconnect(casServer, SIGNAL(getBoothDatabaseFailed(QString)),  0, 0);
  disconnect(casServer, SIGNAL(getBoothDatabasesFinished()),  0, 0);*/

  // Normally populateTable is only called when the form is first
  // shown but in the case of a network scan, we need to update the booths that were found

  populateTable();

  // If flag set then a movie change finished successfully and requested us to
  // get the latest versions of the casplayer database from each booth so
  // now we can reload the video library datagrid
  if (reloadVideoLibrary)
  {
    QLOG_DEBUG() << "Emitting signal to refresh video library";

    reloadVideoLibrary = false;

    emit refreshVideoLibrary();    
  }

  // Check daily meter report schedule
  if (settings->getValue(ENABLE_COLLECTION_SNAPSHOT).toBool() || (settings->getValue(ENABLE_DAILY_METERS).toBool() && settings->getValue(DAILY_METERS_REPORT_RECIPIENTS).toString().length() > 0))
  {
    // TODO: Change time check to work like the daily restart, had a problem when dailylight saving time ended (11/1/2015) where
    // the daily meters were sent 5 times before it finally stopped. Since it sends at 5 minutes past midnight I'm not sure if was
    // to do with daylight saving time or not

    // Build the next date/time the daily meters should be sent by adding the interval to
    // the last time it was sent then changing the time component to the time of day it should be sent
    // and set the time from the time of day the daily meters should be sent
    QDateTime nextDailyMeters = QDateTime::fromTime_t(settings->getValue(LAST_DAILY_METERS_DATE).toDateTime().toTime_t() + settings->getValue(DAILY_METERS_INTERVAL).toInt());
    nextDailyMeters.setTime(QTime::fromString(settings->getValue(DAILY_METERS_TIME).toString(), "hh:mm"));

    //QLOG_DEBUG() << "Next scheduled daily meters collection: " + nextDailyMeters.toString("yyyy-MM-dd hh:mm:ss") + ", current time: " + QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");

    if (nextDailyMeters <= QDateTime::currentDateTime())
    {
      QLOG_DEBUG() << "Time to send daily meters report";

      QVariantList metersList = dbMgr->performDailyCollection(casServer->getDeviceAddresses());

      if (metersList.count() > 0)
      {
        // Update when the last daily collection took place
        settings->setValue(LAST_DAILY_METERS_DATE, metersList.at(0).toMap()["captured"].toDateTime());

        // Save settings
        dbMgr->setValue("all_settings", settings->getSettings());

        if (settings->getValue(ENABLE_DAILY_METERS).toBool())
        {
          casServer->sendDailyMeters(metersList.at(0).toMap()["captured"].toDateTime(), metersList);
        }

        // if last collection snapshot != the current date &&
        // (first snapshot collection = current date OR
        // (current day of the week = the day collection snapshot should be sent AND
        //  current date - collection interval >= the last collection snapshot)
        // Disable Automatic sending of collection snapshot report
//        if (settings->getValue(LAST_COLLECTION_SNAPSHOT_DATE).toDateTime().date() != QDate::currentDate() &&
//            (settings->getValue(FIRST_COLLECTION_SNAPSHOT_DATE).toDateTime().date() == QDate::currentDate() ||
//            (QDate::currentDate().dayOfWeek() == settings->getValue(COLLECTION_SNAPSHOT_SCHEDULE_DAY).toInt() &&
//            QDate::currentDate().addDays(-settings->getValue(COLLECTION_SNAPSHOT_INTERVAL).toInt()) >= settings->getValue(LAST_COLLECTION_SNAPSHOT_DATE).toDateTime().date())))

//        {
//          metersList = dbMgr->getCollectionSnapshot(casServer->getDeviceAddresses(), settings->getValue(COLLECTION_SNAPSHOT_INTERVAL).toInt());

//          if (metersList.count() > 0)
//          {
//            // Update when the last collection snapshot took place
//            settings->setValue(LAST_COLLECTION_SNAPSHOT_DATE, metersList.at(0).toMap()["captured"].toDateTime());

//            // Save settings
//            dbMgr->setValue("all_settings", settings->getSettings());

//            casServer->sendCollectionSnapshot(metersList.at(0).toMap()["start_time"].toDateTime(), metersList.at(0).toMap()["captured"].toDateTime(), metersList);
//          }
//          else
//          {
//            QLOG_ERROR() << "Tried performing snapshot collection but no meters were returned";
//          }
//        }
      }
      else
      {
        QLOG_ERROR() << "Tried performing daily meter collection but no meters were returned";
      }
    }
    /*else
    {
      QLOG_DEBUG() << "Not time to send daily meter report yet";
    }*/
  }
  /*else
  {
    QLOG_DEBUG() << "Daily meters is disabled and/or there are no daily meter report recipients";
  }*/

  emit updateBoothStatusFinished();

  btnCheckDevices->setEnabled(true);
  btnRestartDevices->setEnabled(true);

  busy = false;
}

void BoothStatus::getBoothDatabaseSuccess(QString ipAddress)
{
  QLOG_DEBUG() << QString("Got database from: %1").arg(ipAddress);
}

void BoothStatus::getBoothDatabaseFailed(QString ipAddress)
{
  QLOG_ERROR() << QString("Could not get database from: %1").arg(ipAddress);
}

void BoothStatus::getBoothDatabasesFinished()
{  
  QLOG_DEBUG() << QString("Finished getting databases from booths");

  casServer->startCheckDevices();
}

void BoothStatus::restartDeviceSuccess(QString ipAddress)
{
  restartDeviceSuccessList.append(ipAddress);
}

void BoothStatus::restartDeviceInSession(QString ipAddress)
{
  restartDeviceInSessionList.append(ipAddress);
}

void BoothStatus::restartDeviceFailed(QString ipAddress)
{
  restartDeviceFailList.append(ipAddress);
}

void BoothStatus::restartDevicesFinished()
{
  QLOG_DEBUG() << QString("Finished restarting devices. Success: %1, In Session: %2, Failed: %3")
                  .arg(restartDeviceSuccessList.join(", "))
                  .arg(restartDeviceInSessionList.join(", "))
                  .arg(restartDeviceFailList.join(", "));

  btnCheckDevices->setEnabled(true);
  btnRestartDevices->setEnabled(true);

  busy = false;

  if (!silentRestart)
  {
    if (restartDeviceFailList.count() > 0 || restartDeviceInSessionList.count() > 0)
      QMessageBox::warning(this, tr("Restart Devices"), tr("Not all devices could be restarted. %1%2")
                           .arg(restartDeviceInSessionList.count() > 0 ? QString("The following are currently in session or performing a movie change and will restart when finished: %1").arg(restartDeviceInSessionList.join(", ")) : "")
                           .arg(restartDeviceFailList.count() > 0 ? QString("The following failed to respond: %1").arg(restartDeviceFailList.join(", ")) : ""));
    else
      QMessageBox::information(this, tr("Restart Devices"), tr("All devices are restarting."));
  }
  else
  {
    // Clear flag
    silentRestart = false;
  }
}

void BoothStatus::setSilentRestart(bool silent)
{
  restartDeviceSuccessList.clear();
  restartDeviceFailList.clear();
  restartDeviceInSessionList.clear();

  silentRestart = silent;
}

void BoothStatus::updateBoothStatusAfterMovieChange()
{
  QLOG_DEBUG() << "Updating booth statuses after movie change";

  reloadVideoLibrary = true;
  updateBoothStatus(true);
}

void BoothStatus::checkDevicesClicked()
{
  QLOG_DEBUG() << "User clicked the Update Status button";

  if (casServer->isBusy())
  {
    QLOG_DEBUG() << "casServer has another thread running, cannot update booth statuses";
    QMessageBox::information(this, tr("Update Status"), tr("Another operation is in progress, cannot check booth statuses right now."));
  }
  else
  {
    //btnCheckDevices->setEnabled(false);

    //busy = true;
    //casServer->startCheckDevices();

    updateBoothStatus(true);
  }
}

void BoothStatus::updateBoothStatus(bool forceUpdate)
{  
  if (++numMinutes >= settings->getValue(BOOTH_STATUS_INTERVAL).toInt() || forceUpdate)
  {
    numMinutes = 0;

    if (!busy && !casServer->isBusy())
    {
      btnCheckDevices->setEnabled(false);
      btnRestartDevices->setEnabled(false);

      /*disconnect(casServer, SIGNAL(getBoothDatabaseSuccess(QString)), 0, 0);
        disconnect(casServer, SIGNAL(getBoothDatabaseFailed(QString)),  0, 0);
        disconnect(casServer, SIGNAL(getBoothDatabasesFinished()),  0, 0);

        connect(casServer, SIGNAL(getBoothDatabaseSuccess(QString)), this, SLOT(getBoothDatabaseSuccess(QString)));
        connect(casServer, SIGNAL(getBoothDatabaseFailed(QString)), this, SLOT(getBoothDatabaseFailed(QString)));
        connect(casServer, SIGNAL(getBoothDatabasesFinished()), this, SLOT(getBoothDatabasesFinished()));
        */

      busy = true;

      // Request copy of database from each booth
      casServer->startGetBoothDatabases();
    }
    else
    {
      QLOG_DEBUG() << "Cannot check booth statuses right now because either the busy flag is set or the casServer is running another thread";
    }
  }
  // If the current day of the week = the day view times should be collected AND
  // the last view time collection != the current date AND
  // the current time is >= the time of day the view times should be collected
  else if (QDate::currentDate().dayOfWeek() == settings->getValue(VIEWTIME_COLLECTION_SCHEDULE_DAY).toInt() &&
           settings->getValue(LAST_VIEWTIME_COLLECTION).toDateTime().date() != QDate::currentDate() &&
           QTime::currentTime() >= settings->getValue(VIEWTIME_COLLECTION_SCHEDULE_TIME).toTime())

  {
    QLOG_DEBUG() << "Time to perform view time collection";

    if (!busy && !casServer->isBusy())
    {
      btnCheckDevices->setEnabled(false);
      btnRestartDevices->setEnabled(false);
      busy = true;

      viewtimeCollectSuccessList.clear();
      viewtimeCollectFailList.clear();
      viewtimeMergeSuccessList.clear();
      viewtimeMergeFailList.clear();
      viewtimeClearSuccessList.clear();
      viewtimeClearFailList.clear();
      viewtimeClearInSessionList.clear();     

      // Connect signal/slots as needed since view times are also collected from the movie change tab when necessary
      // These get disconnected when collectViewTimesFinished is called
      connect(casServer, SIGNAL(collectedViewTimesSuccess(QString)), this, SLOT(collectedViewTimesSuccess(QString)));
      connect(casServer, SIGNAL(collectedViewTimesFailed(QString)), this, SLOT(collectedViewTimesFailed(QString)));
      connect(casServer, SIGNAL(mergeViewTimesSuccess(QString)), this, SLOT(mergeViewTimesSuccess(QString)));
      connect(casServer, SIGNAL(mergeViewTimesFailed(QString)), this, SLOT(mergeViewTimesFailed(QString)));
      connect(casServer, SIGNAL(viewTimeCollectionFinished()), this, SLOT(collectViewTimesFinished()));

      // TODO: Clean up this code. This is a quick fix to deal with the view time collection crashing clerkstations. Clerkstations
      // should not have view times collected.
      QStringList onlyBoothAddresses;
      QVariantList boothList = dbMgr->getOnlyBoothInfo(casServer->getDeviceAddresses());

      foreach (QVariant v, boothList)
      {
        QVariantMap booth = v.toMap();
        onlyBoothAddresses.append(settings->getValue(ARCADE_SUBNET).toString() + booth["device_num"].toString());
      }

      // perform view time collection
      // update the last time of view time collection was done (current date/time)
      casServer->startViewTimeCollection(onlyBoothAddresses);
    }
    else
    {
      QLOG_DEBUG() << "Cannot collect view times right now because either the busy flag is set or the casServer is running another thread";
    }
  }
  /*else
  {
    QLOG_DEBUG() << "Not time to do anything...";
  }*/
}

void BoothStatus::insertBoothStatus(QString deviceAlias, QString deviceType, QString ipAddress, qreal credits, qreal cash, qreal programmed, qreal ccCharges, QDateTime lastUsed, QString billAcceptorStatus, QString status)
{
  // Name, Type, Address, Credits, Cash, Programmed, CC Charges, Last Used, Status
  QStandardItem *deviceAliasField, *boothTypeField, *ipAddressField, *creditsField, *cashField,
      *progField, *ccChargesField, *lastUsedField, *billAcceptorField, *statusField;

  deviceAliasField = new QStandardItem(deviceAlias);
  deviceAliasField->setData(deviceAlias);

  boothTypeField = new QStandardItem(deviceType);
  boothTypeField->setData(deviceType);

  ipAddressField = new QStandardItem(ipAddress);
  ipAddressField->setData(ipAddress);

  creditsField = new QStandardItem(QString("$%L1").arg(credits, 0, 'f', 2));
  creditsField->setData(credits);

  cashField = new QStandardItem(QString("$%L1").arg(cash, 0, 'f', 2));
  cashField->setData(cash);

  progField = new QStandardItem(QString("$%L1").arg(programmed, 0, 'f', 2));
  progField->setData(programmed);

  ccChargesField = new QStandardItem(QString("$%L1").arg(ccCharges, 0, 'f', 2));
  ccChargesField->setData(ccCharges);

  lastUsedField = new QStandardItem(lastUsed.isValid() ? lastUsed.toString("MM/dd/yyyy h:mm AP") : "");
  lastUsedField->setData(lastUsed);

  // Last Used doesn't make sense for theaters
  if (deviceType == "Theater")
  {
    lastUsedField->setText("N/A");
    lastUsedField->setData("");
  }

  billAcceptorField = new QStandardItem(billAcceptorStatus);
  billAcceptorField->setData(billAcceptorStatus);

  QColor rowColor = Qt::white;
  if (billAcceptorStatus == "In Service")
    rowColor = Qt::green;
  else if (billAcceptorStatus == "Out of Service")
    rowColor = Qt::red;

  billAcceptorField->setData(rowColor, Qt::BackgroundColorRole);

  statusField = new QStandardItem(status);
  statusField->setData(status);

  int row = boothStatusModel->rowCount();

  boothStatusModel->setItem(row, Name, deviceAliasField);
  boothStatusModel->setItem(row, Booth_Type, boothTypeField);
  boothStatusModel->setItem(row, Address, ipAddressField);
  boothStatusModel->setItem(row, Credits, creditsField);
  boothStatusModel->setItem(row, Cash, cashField);
  boothStatusModel->setItem(row, Programmed, progField);
  boothStatusModel->setItem(row, CC_Charges, ccChargesField);
  boothStatusModel->setItem(row, Last_Used, lastUsedField);
  boothStatusModel->setItem(row, Bill_Acceptor, billAcceptorField);
  boothStatusModel->setItem(row, Status, statusField);
}

void BoothStatus::populateTable()
{
  bool hideMeterColumns = false;
  bool hideBillAcceptorColumn = true;

  QVariantList boothList = dbMgr->getBoothInfo(casServer->getDeviceAddresses());

  /*if (!scanningNetwork)
  {
    foreach (QVariant v, boothList)
    {
      QVariantMap booth = v.toMap();

      if (booth["found"].toBool())
        insertBoothStatus(booth["device_alias"].toString(), booth["device_type"].toString(), settings->getValue(ARCADE_SUBNET).toString() + booth["device_num"].toString(), booth["current_credits"].toDouble() / 4, booth["current_cash"].toDouble() / 4, booth["current_prog_credits"].toDouble() / 4, booth["current_cc_charges"].toDouble() / 4, booth["last_used"].toDateTime(), "");
      else
        insertBoothStatus("", "", settings->getValue(ARCADE_SUBNET).toString() + booth["device_num"].toString(), 0, 0, 0, 0, QDateTime(), "");
    }
  }
  else
  {*/

  // Name, Type, Address, Credits, Cash, Programmed, CC Charges, Last Used, Status
  QStandardItem *deviceAliasField, *boothTypeField, *creditsField, *cashField,
      *progField, *ccChargesField, *lastUsedField, *billAcceptorField;

  foreach (QVariant v, boothList)
  {    
    QVariantMap booth = v.toMap();

    // If the user chose not to enable the booth meters then don't show
    // these columns since we don't want the user to know the meters are still being updated
    if (!booth["enable_meters"].toBool())
      hideMeterColumns = true;

    // TODO: Change this to consider each booth individually in case there are situations where some booths
    // don't have a bill acceptor
    if (booth["enable_bill_acceptor"].toBool())
      hideBillAcceptorColumn = false;

    // Booths are already in the table so update instead of inserting
    int row = findBooth(settings->getValue(ARCADE_SUBNET).toString() + booth["device_num"].toString());

    if (row > -1)
    {
      QLOG_DEBUG() << "found " + booth["device_num"].toString() + " in table";

      deviceAliasField = boothStatusModel->item(row, Name);
      deviceAliasField->setText(booth["device_alias"].toString());
      deviceAliasField->setData(booth["device_alias"].toString());

      boothTypeField = boothStatusModel->item(row, Booth_Type);
      boothTypeField->setText(booth["device_type"].toString());
      boothTypeField->setData(booth["device_type"].toString());          

      creditsField = boothStatusModel->item(row, Credits);
      creditsField->setText(QString("$%L1").arg(booth["current_credits"].toDouble() / 4, 0, 'f', 2));
      creditsField->setData(booth["current_credits"].toDouble() / 4);

      cashField = boothStatusModel->item(row, Cash);
      cashField->setText(QString("$%L1").arg(booth["current_cash"].toDouble() / 4, 0, 'f', 2));
      cashField->setData(booth["current_cash"].toDouble() / 4);

      progField = boothStatusModel->item(row, Programmed);
      progField->setText(QString("$%L1").arg(booth["current_prog_credits"].toDouble() / 4, 0, 'f', 2));
      progField->setData(booth["current_prog_credits"].toDouble() / 4);

      ccChargesField = boothStatusModel->item(row, CC_Charges);
      ccChargesField->setText(QString("$%L1").arg(booth["current_cc_charges"].toDouble() / 4, 0, 'f', 2));
      ccChargesField->setData(booth["current_cc_charges"].toDouble() / 4);

      lastUsedField = boothStatusModel->item(row, Last_Used);
      lastUsedField->setText(booth["last_used"].toDateTime().isValid() ? booth["last_used"].toDateTime().toString("MM/dd/yyyy h:mm AP") : "");
      lastUsedField->setData(booth["last_used"].toDateTime());

      // Last Used doesn't make sense for theaters
      if (booth["device_type"].toString() == "Theater")
      {
        lastUsedField->setText("N/A");
        lastUsedField->setData("");
      }

      QString billAcceptorStatus = "N/A";
      if (booth.contains("bill_acceptor_in_service") && booth["enable_bill_acceptor"].toBool())
        billAcceptorStatus = booth["bill_acceptor_in_service"].toBool() ? "In Service" : "Out of Service";

      billAcceptorField = boothStatusModel->item(row, Bill_Acceptor);
      billAcceptorField->setText(billAcceptorStatus);
      billAcceptorField->setData(billAcceptorStatus);

      QColor rowColor = Qt::white;
      if (billAcceptorStatus == "In Service")
        rowColor = Qt::green;
      else if (billAcceptorStatus == "Out of Service")
        rowColor = Qt::red;

      billAcceptorField->setData(rowColor, Qt::BackgroundColorRole);
    }
    else
    {
      //QLOG_ERROR() << QString("Could not find %1 to update in booth status table").arg(settings->getValue(ARCADE_SUBNET).toString() + booth["device_num"].toString());
      if (booth["found"].toBool())
      {
        QString boothAcceptorStatus = "N/A";
        if (booth.contains("bill_acceptor_in_service"))
          boothAcceptorStatus = booth["bill_acceptor_in_service"].toBool() ? "In Service" : "Out of Service";

        insertBoothStatus(booth["device_alias"].toString(), booth["device_type"].toString(), settings->getValue(ARCADE_SUBNET).toString() + booth["device_num"].toString(), booth["current_credits"].toDouble() / 4, booth["current_cash"].toDouble() / 4, booth["current_prog_credits"].toDouble() / 4, booth["current_cc_charges"].toDouble() / 4, booth["last_used"].toDateTime(), boothAcceptorStatus, "");
      }
      else
        insertBoothStatus("", "", settings->getValue(ARCADE_SUBNET).toString() + booth["device_num"].toString(), 0, 0, 0, 0, QDateTime(), "", "");
    }
  }

  if (hideMeterColumns)
  {
    QLOG_DEBUG() << "Hiding meter columns on Booth Status tab";

    boothStatusView->setColumnHidden(Credits, true);
    boothStatusView->setColumnHidden(Cash, true);
    boothStatusView->setColumnHidden(Programmed, true);
    boothStatusView->setColumnHidden(CC_Charges, true);
  }
  else
  {
    if (boothStatusView->isColumnHidden(Credits))
    {
      QLOG_DEBUG() << "Meter columns on Booth Status tab are currently hidden, now displaying";

      boothStatusView->setColumnHidden(Credits, false);
      boothStatusView->setColumnHidden(Cash, false);
      boothStatusView->setColumnHidden(Programmed, false);
      boothStatusView->setColumnHidden(CC_Charges, false);
    }
  }

  if (hideBillAcceptorColumn)
  {
    QLOG_DEBUG() << "Hiding bill acceptor column on Booth Status tab";
    boothStatusView->setColumnHidden(Bill_Acceptor, true);
  }
  else
  {
    if (boothStatusView->isColumnHidden(Bill_Acceptor))
    {
      QLOG_DEBUG() << "Bill Acceptor column on Booth Status tab is currently hidden, now displaying";

      boothStatusView->setColumnHidden(Bill_Acceptor, false);
    }
  }

  QLOG_DEBUG() << "Now clearing any entries in booth status table that did not respond to scan";

  // Now clear any device in the table that has a status of No Response since these we assume don't exist on the network
  while ( boothStatusModel->findItems("No Response", Qt::MatchExactly, Status).count() > 0 )
  {
    QList<QStandardItem *> resultList = boothStatusModel->findItems("No Response", Qt::MatchExactly, Status);

    QStandardItem *item = resultList.first();

    boothStatusModel->removeRow(item->row());
  }

  if (scanningNetwork)
  {
    scanningNetwork = false;

    if (settings->getValue(DEVICE_LIST).toStringList().count() > 0)
      QMessageBox::information(this, tr("Scan Results"), tr("%1 CAS device(s) found on the network.").arg(settings->getValue(DEVICE_LIST).toStringList().count()));
    else
      QMessageBox::warning(this, tr("Scan Results"), tr("No CAS devices found on the network."));

    // Now that the initial scan is finished we can start the timer to keep statuses updated
    statusTimer->start();
    //dailyReportTimer->start();
  }
  else
  {
    // Build status report for clerk stations to display
    QJsonArray boothStatusArray;
    for (int i = 0; i < boothStatusModel->rowCount(); ++i)
    {
      QJsonObject boothStatusObject;

      if (!boothStatusModel->item(i, Name)->text().isEmpty())
      {
        boothStatusObject["address"] = boothStatusModel->item(i, Name)->text();
      }
      else
      {
        boothStatusObject["address"] = QString("%1").arg(casServer->ipAddressToDeviceNum(boothStatusModel->item(i, Address)->text()).toInt() - 20);
      }

      boothStatusObject["device_type"] = boothStatusModel->item(i, Booth_Type)->text();

      boothStatusObject["bill_acceptor_in_service"] = boothStatusModel->item(i, Bill_Acceptor)->text();

      boothStatusObject["status"] = boothStatusModel->item(i, Status)->text();

      boothStatusArray.append(boothStatusObject);
    }

    boothStatuses["booths"] = boothStatusArray;
    boothStatuses["type"] = QString("booth_status");

    // Get current list of booth status docs (should only be one) to delete
    // before adding the new doc
    dbMgr->getBoothStatuses();
  }
}

int BoothStatus::findBooth(QString ipAddress)
{
  int row = -1;

  // Look in Address column for a matching address
  QList<QStandardItem *> resultList = boothStatusModel->findItems(ipAddress, Qt::MatchExactly, Address);

  if (resultList.count() > 0)
    row = resultList.first()->row();

  return row;
}

void BoothStatus::clearBoothStatuses()
{
  if (boothStatusModel->rowCount() > 0)
  {
    boothStatusModel->removeRows(0, boothStatusModel->rowCount());
  }
}

void BoothStatus::restartDevicesClicked()
{
  QLOG_DEBUG() << "User clicked the Restart Devices button";

  if (!busy && !casServer->isBusy())
  {
    if (QMessageBox::question(this, tr("Restart Devices"), tr("Are you sure you want to restart ALL booths?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
    {
      QLOG_DEBUG() << "User chose to continue with restarting devices";

      // This is called so lists are cleared as well as to indicate whether to provide feedback to user
      setSilentRestart(false);

      btnCheckDevices->setEnabled(false);
      btnRestartDevices->setEnabled(false);

      busy = true;

      casServer->startRestartDevices();
    }
    else
      QLOG_DEBUG() << "User canceled restarting devices";
  }
  else
  {
    QLOG_DEBUG() << "casServer has another thread running, cannot restart devices right now";
    QMessageBox::information(this, tr("Restart Devices"), tr("Another operation is in progress, cannot restart booths right now."));
  }
}

void BoothStatus::collectedViewTimesSuccess(QString ipAddress)
{
  viewtimeCollectSuccessList.append(ipAddress);
}

void BoothStatus::collectedViewTimesFailed(QString ipAddress)
{
  viewtimeCollectFailList.append(ipAddress);
}

void BoothStatus::mergeViewTimesSuccess(QString ipAddress)
{
  viewtimeMergeSuccessList.append(ipAddress);
}

void BoothStatus::mergeViewTimesFailed(QString ipAddress)
{
  viewtimeMergeFailList.append(ipAddress);
}

void BoothStatus::collectViewTimesFinished()
{
  bool clearViewTimes = false;

  QLOG_DEBUG() << QString("Finished view time collection");

  // Disconnect these signal/slots since the view times are also gathered from the movie change tab when necessary
  disconnect(casServer, SIGNAL(collectedViewTimesSuccess(QString)), this, SLOT(collectedViewTimesSuccess(QString)));
  disconnect(casServer, SIGNAL(collectedViewTimesFailed(QString)), this, SLOT(collectedViewTimesFailed(QString)));
  disconnect(casServer, SIGNAL(mergeViewTimesSuccess(QString)), this, SLOT(mergeViewTimesSuccess(QString)));
  disconnect(casServer, SIGNAL(mergeViewTimesFailed(QString)), this, SLOT(mergeViewTimesFailed(QString)));
  disconnect(casServer, SIGNAL(viewTimeCollectionFinished()), this, SLOT(collectViewTimesFinished()));

  // Get count of rows that have a status of "Failed" in Collection Status
  //QList<QStandardItem *> failList = collectionStatusModel->findItems("Failed", Qt::MatchExactly, Collection_Status);

  // Get count of rows that have a status of "Failed" in Data Merge Status
  //QList<QStandardItem *> mergeFailList = collectionStatusModel->findItems("Failed", Qt::MatchExactly, Data_Merge_Status);

  QLOG_DEBUG() << QString("Collected view times from %1 booth(s)").arg(viewtimeCollectSuccessList.count());

  if (viewtimeCollectFailList.count() > 0)
  {
    QLOG_ERROR() << QString("Failed to collect view times from %1 booth(s): %2").arg(viewtimeCollectFailList.count()).arg(viewtimeCollectFailList.join(", "));

    Global::sendAlert(QString("Failed to collect view times from %1 booth(s): %2").arg(viewtimeCollectFailList.count()).arg(viewtimeCollectFailList.join(", ")));
  }

  /*Global::sendAlert(QString("Collected view times from %1 booth(s). Failed to collect view times from %2 booth(s). Merged view times from %3 booth(s). Failed to merge view times from %4 booth(s)")
                    .arg(viewtimeCollectSuccessList.count())
                    .arg(viewtimeCollectFailList.count())
                    .arg(viewtimeMergeSuccessList.count())
                    .arg(viewtimeMergeFailList.count()));*/

  // If no view times collected
  if (viewtimeCollectFailList.count() == settings->getValue(DEVICE_LIST).toStringList().count())
  {
    QLOG_ERROR() << "Could not collect view times from any booth";

    Global::sendAlert("Could not collect view times from any booth");

    //QMessageBox::warning(this, tr("Collection Results"), tr("Could not collect view times from any booths. The Top 5 lists cannot be generated and view times cannot be cleared."));
  }
  else
  {
    if (viewtimeMergeFailList.count() > 0)
    {
      //QMessageBox::warning(this, tr("Collection Results"), tr("Could not merge view time data for analysis. Please wait a moment and then try performing another View Time Collection."));
      QLOG_ERROR() << QString("Could not merge view time data for analysis from %1 booth(s): %2").arg(viewtimeMergeFailList.count()).arg(viewtimeMergeFailList.join(", "));

      Global::sendAlert(QString("Could not merge view time data for analysis from %1 booth(s): %2").arg(viewtimeMergeFailList.count()).arg(viewtimeMergeFailList.join(", ")));
    }
    else
    {
      bool ok;
      int numMovies = dbMgr->getNumMoviesSent(&ok);

      if (ok)
      {
        // If no movies have ever been sent to the booths and there are no movies in the booths
        if (numMovies == 0)
        {
          QLOG_DEBUG() << "No movies are in the booths and none have ever been sent to the booths";

          // view times won't actually exist but store the timestamp anyway
          // do not set clear view times flag since it will encounter an error
          settings->setValue(LAST_VIEWTIME_COLLECTION, QDateTime::currentDateTime());

          dbMgr->setValue("all_settings", settings->getSettings());
        }
        else
        {
          // No data merge errors, clear view times since there are movies in the booths
          clearViewTimes = true;
        }
      }
      else
      {
        Global::sendAlert("Could not get a count of the number of movies in the dvd_copy_jobs_history table");
      }
    }

    // TODO: Should there be a limited number of retry attempts? Currently if this fails, it will collect view times again in 1 minute
    if (clearViewTimes)
    {
      // Not really necessary to connect/disconnect the signals related to clearing view times since no other tab is using this
      connect(casServer, SIGNAL(clearViewTimesSuccess(QString)), this, SLOT(clearViewTimesSuccess(QString)));
      connect(casServer, SIGNAL(clearViewTimesInSession(QString)), this, SLOT(clearViewTimesInSession(QString)));
      connect(casServer, SIGNAL(clearViewTimesFailed(QString)), this, SLOT(clearViewTimesFailed(QString)));
      connect(casServer, SIGNAL(clearViewTimesFinished()), this, SLOT(clearViewTimesFinished()));

      casServer->startClearViewTimes();
    }
    else
    {
      // Enable both tabs since operation finished
      // if (parentTab)
      //  parentTab->setEnabled(true);

      btnCheckDevices->setEnabled(true);
      btnRestartDevices->setEnabled(true);

      busy = false;
    }

  } // endif at least some view times collected
}

void BoothStatus::clearViewTimesSuccess(QString ipAddress)
{
  QLOG_DEBUG() << QString("Cleared view times from booth %1").arg(ipAddress);

  viewtimeClearSuccessList.append(ipAddress);
}

void BoothStatus::clearViewTimesInSession(QString ipAddress)
{
  QLOG_DEBUG() << QString("Booth %1 is currently in session. View times will be cleared after session ends").arg(ipAddress);

  viewtimeClearInSessionList.append(ipAddress);
}

void BoothStatus::clearViewTimesFailed(QString ipAddress)
{
  QLOG_ERROR() << QString("Could not clear view times from booth %1").arg(ipAddress);

  viewtimeClearFailList.append(ipAddress);
}

void BoothStatus::clearViewTimesFinished()
{
  QLOG_DEBUG() << QString("Clear view times finished");

  disconnect(casServer, SIGNAL(clearViewTimesSuccess(QString)), this, SLOT(clearViewTimesSuccess(QString)));
  disconnect(casServer, SIGNAL(clearViewTimesInSession(QString)), this, SLOT(clearViewTimesInSession(QString)));
  disconnect(casServer, SIGNAL(clearViewTimesFailed(QString)), this, SLOT(clearViewTimesFailed(QString)));
  disconnect(casServer, SIGNAL(clearViewTimesFinished()), this, SLOT(clearViewTimesFinished()));

  QString clearViewTimesStats = QString("Cleared view times from %1 booth(s). Failed to clear view times from %2 booth(s). Postponed clearing view times from %3 booth(s).")
                                .arg(viewtimeClearSuccessList.count())
                                .arg(viewtimeClearFailList.count())
                                .arg(viewtimeClearInSessionList.count());

  QLOG_DEBUG() << clearViewTimesStats;

  // Send alert only if none were cleared or one or more failed
  if (viewtimeClearSuccessList.count() == 0 || viewtimeClearFailList.count() > 0)
  {
    Global::sendAlert(clearViewTimesStats);
  }

  // Get count of rows that have a status of "Failed"
  // QList<QStandardItem *> failList = collectionStatusModel->findItems("Failed", Qt::MatchExactly, Clear_Views_Status);
  // QList<QStandardItem *> inSessionList = collectionStatusModel->findItems("In Session", Qt::MatchExactly, Clear_Views_Status);

  // If any failed view time collections

  // No view times cleared
  if (viewtimeClearFailList.count() == settings->getValue(DEVICE_LIST).toStringList().count())
  {
    //QMessageBox::warning(this, tr("Clear View Times"), tr("Could not clear view times from any booths."));
    QLOG_ERROR() << "Could not clear view times from any booths";

    //Global::sendAlert("Could not clear view times from any booths");
  }
  else
  {
    if (viewtimeClearFailList.count() > 0)
    {
      // Not all booths cleared
      //QMessageBox::warning(this, tr("Clear View Times"), tr("Cleared view times from %1 out of %2 booths. If any booth\'s Clear Views Status shows \"In Session\" then the view times will be cleared after the booth is no longer in use.").arg(collectionStatusModel->rowCount() - failList.count()).arg(collectionStatusModel->rowCount()));
      QLOG_ERROR() << QString("Failed to clear view times from: %1 booth(s)").arg(viewtimeClearFailList.count());

      //Global::sendAlert(QString("Failed to clear view times from: %1 booth(s)").arg(viewtimeClearFailList.count()));
    }

    // tell user all view times have been cleared
    //QMessageBox::information(this, tr("Clear View Times"), tr("Successfully cleared view times from booths. If any booth\'s Clear Views Status shows \"In Session\" then the view times will be cleared after the booth is no longer in use."));

    // Update when the last time view times were collected
    settings->setValue(LAST_VIEWTIME_COLLECTION, QDateTime::currentDateTime());

    dbMgr->setValue("all_settings", settings->getSettings());

    // When the view times table gets large it really hurts performance for the query to get collection dates
    // As a temporary workaround, only update the collection dates in the list when we emit this signal
    emit refreshViewtimeDates();
  }

  btnCheckDevices->setEnabled(true);
  btnRestartDevices->setEnabled(true);

  busy = false;
}

void BoothStatus::getBoothStatusesFinished(QList<QJsonObject> &boothDocs, bool ok)
{
  foreach (QJsonObject obj, boothDocs)
  {
    dbMgr->deleteBoothStatus(obj["_id"].toString(), obj["_rev"].toString());
  }

  QJsonDocument d(boothStatuses);
  dbMgr->addBoothStatus(d);
}

void BoothStatus::deleteBoothStatusFinished(QString id, bool ok)
{
  if (ok)
  {
    QLOG_DEBUG() << "Deleted old booth status";
  }
  else
  {
    QLOG_ERROR() << "Could not delete old booth status";
  }
}

void BoothStatus::addBoothStatusFinished(QString id, bool ok)
{
  if (ok)
  {
    QLOG_DEBUG() << "Added booth status";
  }
  else
  {
    QLOG_ERROR() << "Could not add booth status";
  }
}
