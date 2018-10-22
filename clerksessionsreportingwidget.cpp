#include "clerksessionsreportingwidget.h"
#include <QHeaderView>
#include "qslog/QsLog.h"
#include "global.h"
#include <QSortFilterProxyModel>
#include <QMessageBox>
#include "cardauthorizewidget.h"

ClerkSessionsReportingWidget::ClerkSessionsReportingWidget(DatabaseMgr *dbMgr, CasServer *casServer, Settings *settings, QWidget *parent) : QWidget(parent)
{
  this->dbMgr = dbMgr;
  this->casServer = casServer;
  this->settings = settings;

  updateSessionDates = true;
  firstLoad = true;
  busy = false;

  connect(casServer, SIGNAL(clearClerkstationsFailed(QString)), this, SLOT(clearClerkstationsFailed(QString)));
  connect(casServer, SIGNAL(clearClerkstationsSuccess(QString)), this, SLOT(clearClerkstationsSuccess(QString)));
  connect(casServer, SIGNAL(clearClerkstationsFinished()), this, SLOT(clearClerkstationsFinished()));

  clerkSessionsModel = new QStandardItemModel(0, 6);
  clerkSessionsModel->setHorizontalHeaderItem(0, new QStandardItem(QString("Clerk")));
  clerkSessionsModel->setHorizontalHeaderItem(1, new QStandardItem(QString("Logged In")));
  clerkSessionsModel->setHorizontalHeaderItem(2, new QStandardItem(QString("Logged Out")));
  clerkSessionsModel->setHorizontalHeaderItem(3, new QStandardItem(QString("Total Sales")));
  clerkSessionsModel->setHorizontalHeaderItem(4, new QStandardItem(QString("Arcade Card Sales")));
  clerkSessionsModel->setHorizontalHeaderItem(5, new QStandardItem(QString("Preview Card Sales")));

  QSortFilterProxyModel *sortFilter = new QSortFilterProxyModel;
  sortFilter->setSourceModel(clerkSessionsModel);
  sortFilter->setSortRole(Qt::UserRole + 1);

  btnRefreshSessions = new QPushButton(tr("Refresh Sessions"));
  connect(btnRefreshSessions, SIGNAL(clicked()), this, SLOT(refreshSessionsClicked()));

  btnClearClerkstations = new QPushButton(tr("Clear Clerk Stations..."));
  connect(btnClearClerkstations, SIGNAL(clicked()), this, SLOT(clearClerkstationsClicked()));

  clerkSessionsTable = new QTableView;
  clerkSessionsTable->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
  clerkSessionsTable->horizontalHeader()->setStretchLastSection(true);
  clerkSessionsTable->horizontalHeader()->setStyleSheet("font:bold Arial;");
  clerkSessionsTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
  clerkSessionsTable->setSelectionMode(QAbstractItemView::SingleSelection);
  clerkSessionsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  clerkSessionsTable->setWordWrap(true);
  clerkSessionsTable->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
  clerkSessionsTable->verticalHeader()->hide();
  clerkSessionsTable->setSortingEnabled(true);
  clerkSessionsTable->setModel(sortFilter);
  clerkSessionsTable->setEditTriggers(QAbstractItemView::NoEditTriggers); // Make table read-only
  clerkSessionsTable->setSelectionMode(QAbstractItemView::SingleSelection); // Make table read-only
  clerkSessionsTable->sortByColumn(0, Qt::AscendingOrder);

  lblViewTimeDate = new QLabel(tr("Session Date"));

  cmbViewTimeDate = new QDateEdit;
  //cmbViewTimeDate->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  cmbViewTimeDate->setCalendarPopup(true);
  cmbViewTimeDate->setDisplayFormat("MM/dd/yyyy");
  cmbViewTimeDate->setMaximumDate(QDate::currentDate());
  connect(cmbViewTimeDate, SIGNAL(dateChanged(QDate)), this, SLOT(clerkSessionDateChanged(QDate)));

  reportCriteriaLayout = new QHBoxLayout;
  reportCriteriaLayout->addWidget(lblViewTimeDate);
  reportCriteriaLayout->addWidget(cmbViewTimeDate);
  reportCriteriaLayout->addWidget(btnRefreshSessions);
  reportCriteriaLayout->addStretch(4);
  reportCriteriaLayout->addWidget(btnClearClerkstations);

  verticalLayout = new QVBoxLayout;
  verticalLayout->addLayout(reportCriteriaLayout);
  verticalLayout->addWidget(clerkSessionsTable);

  this->setLayout(verticalLayout);
}

ClerkSessionsReportingWidget::~ClerkSessionsReportingWidget()
{
}

bool ClerkSessionsReportingWidget::isBusy()
{
  return busy;
}

void ClerkSessionsReportingWidget::showEvent(QShowEvent *)
{
  QLOG_DEBUG() << QString("Showing the User Sessions tab");

  if (firstLoad)
  {
    cmbViewTimeDate->setDate(QDate::currentDate());
    firstLoad = false;
  }

  if (updateSessionDates)
  {
    updateSessionDates = false;
  }

  cmbViewTimeDate->setMaximumDate(QDate::currentDate());
}

void ClerkSessionsReportingWidget::refreshSessionsClicked()
{
    if (clerkSessionsModel->rowCount() > 0)
      clerkSessionsModel->removeRows(0, clerkSessionsModel->rowCount());

    // Populate model sessions results
    QVariantList sessionsList = dbMgr->getclerkSessionsByDate(cmbViewTimeDate->date(), casServer->getDeviceAddresses());
    if (sessionsList.count() > 0)
    {
      QLOG_DEBUG() << "Clerk sessions count:" << sessionsList.count();

      foreach (QVariant v, sessionsList)
      {
        QVariantMap record = v.toMap();

        insertClerkSession(record["user"].toString(), record["deviceNum"].toString(), record["startTime"].toDateTime(), record["endTime"].toDateTime(), record["amount"].toDouble(), record["arcade_amount"].toDouble(), record["preview_amount"].toDouble());
      }
    }
    else
    {
      QLOG_DEBUG() << "No Clerk Session Records returned";
    }

    clerkSessionsTable->scrollToTop();
    clerkSessionsTable->sortByColumn(clerkSessionsTable->horizontalHeader()->sortIndicatorSection(), clerkSessionsTable->horizontalHeader()->sortIndicatorOrder());
}

void ClerkSessionsReportingWidget::clearClerkstationsClicked()
{
  QLOG_DEBUG() << "User clicked clear clerk stations button";

  // Authenticate user
  connect(casServer, SIGNAL(userAuthorizationReceived(bool,QString)), this, SLOT(userAuthorizationReceived(bool,QString)));

  CardAuthorizeWidget cardAuthWidget(casServer);

  if (cardAuthWidget.exec() == QDialog::Rejected)
  {
    // Disconnect the signal/slot since it's also used on the clerk stations tab
    disconnect(casServer, SIGNAL(userAuthorizationReceived(bool,QString)), this, SLOT(userAuthorizationReceived(bool,QString)));
  }
  else
  {
    busy = true;
    btnRefreshSessions->setEnabled(false);
    btnClearClerkstations->setEnabled(false);
  }
}

void ClerkSessionsReportingWidget::clerkSessionDateChanged(QDate selectedDate)
{
    if (selectedDate.isValid())
    {
      QLOG_DEBUG() << "User selected session date:" << selectedDate;

      if (clerkSessionsModel->rowCount() > 0)
        clerkSessionsModel->removeRows(0, clerkSessionsModel->rowCount());

      // Populate model sessions results
      QVariantList sessionsList = dbMgr->getclerkSessionsByDate(selectedDate, casServer->getDeviceAddresses());
      if (sessionsList.count() > 0)
      {
        QLOG_DEBUG() << "Clerk sessions count:" << sessionsList.count();

        foreach (QVariant v, sessionsList)
        {
          QVariantMap record = v.toMap();

          insertClerkSession(record["user"].toString(), record["deviceNum"].toString(), record["startTime"].toDateTime(), record["endTime"].toDateTime(), record["amount"].toDouble(), record["arcade_amount"].toDouble(), record["preview_amount"].toDouble());
        }
      }
      else
      {
        QLOG_DEBUG() << "No Clerk Session Records returned";
      }

      clerkSessionsTable->scrollToTop();
      clerkSessionsTable->sortByColumn(clerkSessionsTable->horizontalHeader()->sortIndicatorSection(), clerkSessionsTable->horizontalHeader()->sortIndicatorOrder());
    }
    else
    {
        if (clerkSessionsModel->rowCount() > 0)
          clerkSessionsModel->removeRows(0, clerkSessionsModel->rowCount());

        clerkSessionsTable->scrollToTop();
        clerkSessionsTable->sortByColumn(clerkSessionsTable->horizontalHeader()->sortIndicatorSection(), clerkSessionsTable->horizontalHeader()->sortIndicatorOrder());
    }
}

void ClerkSessionsReportingWidget::insertClerkSession(QString user, QString deviceNumber, QDateTime startTime, QDateTime endTime, double amount, double arcade_amount, double preview_amount)
{
  Q_UNUSED(deviceNumber)

  // User, Start Session Time, End Session Time, Amount Billed, Arcade Sales, Preview Sales
  QStandardItem *userName, *startSessionTime, *endSessionTime, *amountItem, *arcadeSalesItem, *previewSalesItem;

  userName = new QStandardItem(user);
  userName->setData(user);

  startSessionTime = new QStandardItem(startTime.toString("MM/dd/yyyy h:mm:ss AP"));
  startSessionTime->setData(startTime);

  endSessionTime = new QStandardItem(endTime.toString("MM/dd/yyyy h:mm:ss AP"));
  endSessionTime->setData(endTime);

  amountItem = new QStandardItem(QString("$%L1").arg(amount, 0, 'f', 2));
  amountItem->setData(amount);

  arcadeSalesItem = new QStandardItem(QString("$%L1").arg(arcade_amount, 0, 'f', 2));
  arcadeSalesItem->setData(arcade_amount);

  previewSalesItem = new QStandardItem(QString("$%L1").arg(preview_amount, 0, 'f', 2));
  previewSalesItem->setData(preview_amount);

  int row = clerkSessionsModel->rowCount();

  clerkSessionsModel->setItem(row, 0, userName);
  clerkSessionsModel->setItem(row, 1, startSessionTime);
  clerkSessionsModel->setItem(row, 2, endSessionTime);
  clerkSessionsModel->setItem(row, 3, amountItem);
  clerkSessionsModel->setItem(row, 4, arcadeSalesItem);
  clerkSessionsModel->setItem(row, 5, previewSalesItem);
}

void ClerkSessionsReportingWidget::clearClerkstationsSuccess(QString ipAddress)
{
  clearClerkstationsSuccessList.append(ipAddress);
}

void ClerkSessionsReportingWidget::clearClerkstationsFailed(QString ipAddress)
{
  clearClerkstationsFailureList.append(ipAddress);
}

void ClerkSessionsReportingWidget::clearClerkstationsFinished()
{
  if (clearClerkstationsSuccessList.count() > 0 && clearClerkstationsFailureList.count() == 0)
  {
    QMessageBox::information(this, tr("Clear Clerk Stations"), QString("All clerk stations were cleared."));
  }
  else
  {
    QLOG_ERROR() << QString("The following clerk stations could not be cleared: %1").arg(clearClerkstationsFailureList.join(", "));
    Global::sendAlert(QString("The following clerk stations could not be cleared: %1").arg(clearClerkstationsFailureList.join(", ")));

    QMessageBox::warning(this, tr("Clear Clerk Stations"), QString("%1 out of %2 clerk stations could be cleared. The following clerk stations returned an error: %3")
                         .arg(clearClerkstationsSuccessList.count())
                         .arg(clearClerkstationsSuccessList.count() + clearClerkstationsFailureList.count())
                         .arg(clearClerkstationsFailureList.join(", ")));
  }

  busy = false;
  btnRefreshSessions->setEnabled(true);
  btnClearClerkstations->setEnabled(true);

  refreshSessionsClicked();
}

void ClerkSessionsReportingWidget::userAuthorizationReceived(bool success, QString response)
{
  disconnect(casServer, SIGNAL(userAuthorizationReceived(bool,QString)), this, SLOT(userAuthorizationReceived(bool,QString)));

  if (success)
  {
    // Send command to all clerkstations to clear transactions and clerksessions tables
    if (QMessageBox::question(this, tr("Clear Clerk Stations"), tr("Clearing the clerk station(s) should only be done after installing equipment and training arcade employees. This clears all clerk sessions and transactions. IT DOES NOT CLEAR THE METERS. Are you sure you want to continue?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
    {
      QLOG_DEBUG() << "User chose to continue clearing clerk stations";

      clearClerkstationsSuccessList.clear();
      clearClerkstationsFailureList.clear();

      casServer->startClearClerkstations();
    }
    else
    {
      QLOG_DEBUG() << "User canceled clearing clerk stations";

      busy = false;
      btnRefreshSessions->setEnabled(true);
      btnClearClerkstations->setEnabled(true);
    }
  }
  else
  {
    QMessageBox::warning(this, tr("Failed"), response);

    busy = false;
    btnRefreshSessions->setEnabled(true);
    btnClearClerkstations->setEnabled(true);
  }
}
