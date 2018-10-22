#include "viewtimesreportswidget.h"
#include <QHeaderView>
#include "qslog/QsLog.h"
#include "global.h"
#include <QSortFilterProxyModel>


ViewTimesReportsWidget::ViewTimesReportsWidget(DatabaseMgr *dbMgr, CasServer *casServer, Settings *settings, QWidget *parent) : QWidget(parent)
{
  this->dbMgr = dbMgr;
  this->casServer = casServer;
  this->settings = settings;

  updateViewTimeDates = true;
  firstLoad = true;
  busy = false;

  // UPC, Title, Producer, Category, Subcategory, Ch #, Play Time, Weeks
  viewTimesModel = new QStandardItemModel(0, 8);
  viewTimesModel->setHorizontalHeaderItem(0, new QStandardItem(QString("UPC")));
  viewTimesModel->setHorizontalHeaderItem(1, new QStandardItem(QString("Title")));
  viewTimesModel->setHorizontalHeaderItem(2, new QStandardItem(QString("Producer")));
  viewTimesModel->setHorizontalHeaderItem(3, new QStandardItem(QString("Category")));
  viewTimesModel->setHorizontalHeaderItem(4, new QStandardItem(QString("Subcategory")));
  viewTimesModel->setHorizontalHeaderItem(5, new QStandardItem(QString("Ch #")));
  viewTimesModel->setHorizontalHeaderItem(6, new QStandardItem(QString("Play Time")));
  viewTimesModel->setHorizontalHeaderItem(7, new QStandardItem(QString("# Weeks")));

  QSortFilterProxyModel *sortFilter = new QSortFilterProxyModel;
  sortFilter->setSourceModel(viewTimesModel);
  sortFilter->setSortRole(Qt::UserRole + 1);

  viewTimesTable = new QTableView;
  viewTimesTable->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
  viewTimesTable->horizontalHeader()->setStretchLastSection(true);
  viewTimesTable->horizontalHeader()->setStyleSheet("font:bold Arial;");
  viewTimesTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
  viewTimesTable->setSelectionMode(QAbstractItemView::SingleSelection);
  viewTimesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  viewTimesTable->setWordWrap(true);
  viewTimesTable->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
  viewTimesTable->verticalHeader()->hide();
  viewTimesTable->setSortingEnabled(true);
  viewTimesTable->setModel(sortFilter);
  viewTimesTable->setEditTriggers(QAbstractItemView::NoEditTriggers); // Make table read-only
  viewTimesTable->setSelectionMode(QAbstractItemView::SingleSelection); // Make table read-only
  viewTimesTable->sortByColumn(6, Qt::AscendingOrder);

  lblViewTimeDate = new QLabel(tr("Collection Date"));

  cmbViewTimeDate = new QComboBox;
  cmbViewTimeDate->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  // By setting the combobox to editable and without accepting any entries, the visible items in the list
  // can be limited. Otherwise the list could be as tall as the entire screen depending on how many items are in it
  cmbViewTimeDate->setEditable(true);
  cmbViewTimeDate->setInsertPolicy(QComboBox::NoInsert);
  connect(cmbViewTimeDate, SIGNAL(currentIndexChanged(int)), this, SLOT(viewTimeDateChanged(int)));

  lblBooth = new QLabel(tr("Booth"));
  cmbBooth = new QComboBox;
  cmbBooth->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  cmbBooth->setEditable(true);
  cmbBooth->setInsertPolicy(QComboBox::NoInsert);
  connect(cmbBooth, SIGNAL(currentIndexChanged(int)), this, SLOT(boothChanged(int)));

  reportCriteriaLayout = new QHBoxLayout;
  reportCriteriaLayout->addWidget(lblViewTimeDate);
  reportCriteriaLayout->addWidget(cmbViewTimeDate);
  reportCriteriaLayout->addWidget(lblBooth);
  reportCriteriaLayout->addWidget(cmbBooth);
  reportCriteriaLayout->addStretch(2);

  verticalLayout = new QVBoxLayout;
  verticalLayout->addLayout(reportCriteriaLayout);
  verticalLayout->addWidget(viewTimesTable);

  this->setLayout(verticalLayout);
}

ViewTimesReportsWidget::~ViewTimesReportsWidget()
{
}

bool ViewTimesReportsWidget::isBusy()
{
  return busy;
}

void ViewTimesReportsWidget::showEvent(QShowEvent *)
{
  QLOG_DEBUG() << QString("Showing the View Times tab");

  if (firstLoad)
  {
    QVariantList boothList = dbMgr->getOnlyBoothInfo(casServer->getDeviceAddresses());

    cmbBooth->setEnabled(false);
    cmbBooth->blockSignals(true);
    cmbBooth->addItem("All", "All");
    foreach (QVariant v, boothList)
    {
      QVariantMap booth = v.toMap();

      QString alias = booth["device_alias"].toString();
      cmbBooth->addItem((alias.isEmpty() ? booth["device_num"].toString() : alias), booth["device_num"].toString());
    }
    cmbBooth->blockSignals(false);

    firstLoad = false;
  }

  // TODO: As a temporary solution, only query viewtimes database for collection dates when flag set
  // Found that large viewtimes databases do not perform as well, need to work on an archiving old viewtimes
  if (updateViewTimeDates)
  {
    // Refresh list of collection dates every time widget is shown to make sure we have the latest collections
    QList<QDate> collectionDates = dbMgr->getViewTimeCollectionDates();

    cmbViewTimeDate->blockSignals(true);

    // Determine current index so we can return to this item afterwards
    int currentIndex = cmbViewTimeDate->currentIndex();
    QString currentText = cmbViewTimeDate->itemData(currentIndex).toString();

    cmbViewTimeDate->clear();
    cmbViewTimeDate->addItem("");
    foreach (QDate d, collectionDates)
    {
      cmbViewTimeDate->addItem(d.toString("MM/dd/yyyy"), d);
    }

    // After repopulating widget, make sure that the index we had is still pointing
    // to the same data, if not, search for it and update the currentIndex appropriately
    if (currentText != cmbViewTimeDate->itemData(currentIndex).toString())
    {
      for (int i = 0; i < cmbViewTimeDate->count(); ++i)
      {
        if (currentText == cmbViewTimeDate->itemData(i).toString())
        {
          currentIndex = i;
          break;
        }
      }
    }

    // Return to the previously selected item
    cmbViewTimeDate->setCurrentIndex(currentIndex);

    cmbViewTimeDate->blockSignals(false);

    updateViewTimeDates = false;
  }
  else
  {
    QLOG_DEBUG() << QString("Not time to refresh view time collection date list");
  }
}

void ViewTimesReportsWidget::setUpdateViewTimeDates(bool update)
{
  updateViewTimeDates = update;
}

void ViewTimesReportsWidget::viewTimeDateChanged(int index)
{
  if (index > 0)
  {
    QLOG_DEBUG() << "User selected view time collection date:" << cmbViewTimeDate->itemData(index).toDate();

    cmbBooth->setEnabled(true);
    cmbBooth->blockSignals(true);
    cmbBooth->setCurrentIndex(0);
    cmbBooth->blockSignals(false);

    if (viewTimesModel->rowCount() > 0)
      viewTimesModel->removeRows(0, viewTimesModel->rowCount());

    // Populate model with combined view times results
    QVariantList viewTimeCollection = dbMgr->getViewTimeCollection(cmbViewTimeDate->itemData(index).toDate());

    if (viewTimeCollection.count() > 0)
    {
      QLOG_DEBUG() << "Record count:" << viewTimeCollection.count();

      foreach (QVariant v, viewTimeCollection)
      {
        QVariantMap record = v.toMap();

        insertViewTime(record["upc"].toString(), record["title"].toString(), record["producer"].toString(), record["category"].toString(), record["subcategory"].toString(), record["channel_num"].toInt(), record["play_time"].toInt(), record["num_weeks"].toInt());
      }
    }
    else
    {
      QLOG_DEBUG() << "No records returned";
    }

    //viewTimesTable->reset();
    viewTimesTable->scrollToTop();
    viewTimesTable->sortByColumn(viewTimesTable->horizontalHeader()->sortIndicatorSection(), viewTimesTable->horizontalHeader()->sortIndicatorOrder());
  }
  else
  {
    QLOG_DEBUG() << "User selected the empty item in the view time collection date list";

    cmbBooth->setEnabled(false);

    if (viewTimesModel->rowCount() > 0)
      viewTimesModel->removeRows(0, viewTimesModel->rowCount());

    //viewTimesTable->reset();
    viewTimesTable->scrollToTop();
    viewTimesTable->sortByColumn(viewTimesTable->horizontalHeader()->sortIndicatorSection(), viewTimesTable->horizontalHeader()->sortIndicatorOrder());
  }
}

void ViewTimesReportsWidget::boothChanged(int index)
{
  if (index > -1)
  {
    QLOG_DEBUG() << "User selected booth:" << cmbBooth->itemData(index).toString();

    int deviceNum = 0;

    if (cmbBooth->itemData(index).toString() != "All")
    {
      deviceNum = cmbBooth->itemData(index).toInt();
    }

    if (viewTimesModel->rowCount() > 0)
      viewTimesModel->removeRows(0, viewTimesModel->rowCount());

    // Populate model with view times results
    QVariantList viewTimeCollection = dbMgr->getViewTimeCollection(cmbViewTimeDate->itemData(cmbViewTimeDate->currentIndex()).toDate(), deviceNum);

    if (viewTimeCollection.count() > 0)
    {
      QLOG_DEBUG() << "Record count:" << viewTimeCollection.count();

      foreach (QVariant v, viewTimeCollection)
      {
        QVariantMap record = v.toMap();

        insertViewTime(record["upc"].toString(), record["title"].toString(), record["producer"].toString(), record["category"].toString(), record["subcategory"].toString(), record["channel_num"].toInt(), record["play_time"].toInt(), record["num_weeks"].toInt());
      }
    }
    else
    {
      QLOG_DEBUG() << "No records returned";
    }

    //viewTimesTable->reset();
    viewTimesTable->scrollToTop();
    viewTimesTable->sortByColumn(viewTimesTable->horizontalHeader()->sortIndicatorSection(), viewTimesTable->horizontalHeader()->sortIndicatorOrder());
  }
}

void ViewTimesReportsWidget::insertViewTime(QString upc, QString title, QString producer, QString category, QString subcategory, int channelNum, int playTime, int numWeeks)
{
  // UPC, Title, Producer, Category, Subcategory, Ch #, Play Time, Weeks
  QStandardItem *upcField, *titleField, *producerField, *categoryField, *subcategoryField, *channelField, *playTimeField, *numWeeksField;

  upcField = new QStandardItem(upc);
  upcField->setData(upc);

  titleField = new QStandardItem(title);
  titleField->setData(title);

  producerField = new QStandardItem(producer);
  producerField->setData(producer);

  categoryField = new QStandardItem(category);
  categoryField->setData(category);

  subcategoryField = new QStandardItem(subcategory);
  subcategoryField->setData(subcategory);

  channelField = new QStandardItem(QString("%1").arg(channelNum));
  channelField->setData(channelNum);

  //playTimeField = new QStandardItem(Global::secondsToHMS(playTime) + QString(" (%1)").arg(playTime));
  playTimeField = new QStandardItem(Global::secondsToHMS(playTime));
  //playTimeField = new QStandardItem(QString("%1").arg(playTime));
  playTimeField->setData(playTime);

  numWeeksField = new QStandardItem(QString("%1").arg(numWeeks));
  numWeeksField->setData(numWeeks);

  int row = viewTimesModel->rowCount();

  viewTimesModel->setItem(row, 0, upcField);
  viewTimesModel->setItem(row, 1, titleField);
  viewTimesModel->setItem(row, 2, producerField);
  viewTimesModel->setItem(row, 3, categoryField);
  viewTimesModel->setItem(row, 4, subcategoryField);
  viewTimesModel->setItem(row, 5, channelField);
  viewTimesModel->setItem(row, 6, playTimeField);
  viewTimesModel->setItem(row, 7, numWeeksField);
}
