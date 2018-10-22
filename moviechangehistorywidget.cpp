#include "moviechangehistorywidget.h"
#include <QHeaderView>
#include "qslog/QsLog.h"
#include <QSortFilterProxyModel>

MovieChangeHistoryWidget::MovieChangeHistoryWidget(DatabaseMgr *dbMgr, CasServer *casServer, Settings *settings, QWidget *parent) : QWidget(parent)
{
  this->dbMgr = dbMgr;
  this->casServer = casServer;
  this->settings = settings;

  firstLoad = true;
  busy = false;

  // UPC, Title, Producer, Category, Subcategory, Ch #, Previous Movie
  movieChangeModel = new QStandardItemModel(0, 7);
  movieChangeModel->setHorizontalHeaderItem(UPC, new QStandardItem(QString("UPC")));
  movieChangeModel->setHorizontalHeaderItem(Title, new QStandardItem(QString("Title")));
  movieChangeModel->setHorizontalHeaderItem(Producer, new QStandardItem(QString("Producer")));
  movieChangeModel->setHorizontalHeaderItem(Category, new QStandardItem(QString("Category")));
  movieChangeModel->setHorizontalHeaderItem(Subcategory, new QStandardItem(QString("Subcategory")));
  movieChangeModel->setHorizontalHeaderItem(Channel_Num, new QStandardItem(QString("Ch #")));
  movieChangeModel->setHorizontalHeaderItem(Previous_Movie, new QStandardItem(QString("Previous Movie")));

  QSortFilterProxyModel *sortFilter = new QSortFilterProxyModel;
  sortFilter->setSourceModel(movieChangeModel);
  sortFilter->setSortRole(Qt::UserRole + 1);

  movieChangeTable = new QTableView;
  movieChangeTable->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
  movieChangeTable->horizontalHeader()->setStretchLastSection(true);
  movieChangeTable->horizontalHeader()->setStyleSheet("font:bold Arial;");
  movieChangeTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
  movieChangeTable->setSelectionMode(QAbstractItemView::SingleSelection);
  movieChangeTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  movieChangeTable->setWordWrap(true);
  movieChangeTable->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
  movieChangeTable->verticalHeader()->hide();
  movieChangeTable->setSortingEnabled(true);
  movieChangeTable->setModel(sortFilter);
  movieChangeTable->setEditTriggers(QAbstractItemView::NoEditTriggers); // Make table read-only
  movieChangeTable->setSelectionMode(QAbstractItemView::SingleSelection); // Make table read-only
  movieChangeTable->sortByColumn(Channel_Num, Qt::AscendingOrder);

  lblMovieChangeDate = new QLabel(tr("Movie Change Date"));

  cmbMovieChangeDate = new QComboBox;
  cmbMovieChangeDate->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  // By setting the combobox to editable and without accepting any entries, the visible items in the list
  // can be limited. Otherwise the list could be as tall as the entire screen depending on how many items are in it
  cmbMovieChangeDate->setEditable(true);
  cmbMovieChangeDate->setInsertPolicy(QComboBox::NoInsert);
  connect(cmbMovieChangeDate, SIGNAL(currentIndexChanged(int)), this, SLOT(movieChangeDateChanged(int)));

  reportCriteriaLayout = new QHBoxLayout;
  reportCriteriaLayout->addWidget(lblMovieChangeDate);
  reportCriteriaLayout->addWidget(cmbMovieChangeDate);
  reportCriteriaLayout->addStretch(2);

  verticalLayout = new QVBoxLayout;
  verticalLayout->addLayout(reportCriteriaLayout);
  verticalLayout->addWidget(movieChangeTable);

  this->setLayout(verticalLayout);
}

MovieChangeHistoryWidget::~MovieChangeHistoryWidget()
{
}

bool MovieChangeHistoryWidget::isBusy()
{
  return false;
}

void MovieChangeHistoryWidget::showEvent(QShowEvent *)
{
  QLOG_DEBUG() << QString("Showing the Movie Change > History tab");

  if (firstLoad)
  {
    // Currently nothing to do on first load
    firstLoad = false;
  }

  // Refresh list of movie change dates every time widget is shown to make sure we have the latest
  QStringList movieChangeDates = dbMgr->getMovieChangeDates();

  cmbMovieChangeDate->blockSignals(true);

  // Determine current index so we can return to this item afterwards
  int currentIndex = cmbMovieChangeDate->currentIndex();
  QString currentText = cmbMovieChangeDate->itemData(currentIndex).toString();

  cmbMovieChangeDate->clear();
  cmbMovieChangeDate->addItem("");
  foreach (QString d, movieChangeDates)
  {
    cmbMovieChangeDate->addItem(QDateTime::fromString(d, "yyyy-MM-dd hh:mm:ss").toString("MM/dd/yyyy h:mm AP"), d);
  }

  // After repopulating widget, make sure that the index we had is still pointing
  // to the same data, if not, search for it and update the currentIndex appropriately
  if (currentText != cmbMovieChangeDate->itemData(currentIndex).toString())
  {
    for (int i = 0; i < cmbMovieChangeDate->count(); ++i)
    {
      if (currentText == cmbMovieChangeDate->itemData(i).toString())
      {
        currentIndex = i;
        break;
      }
    }
  }

  // Return to the previously selected item
  cmbMovieChangeDate->setCurrentIndex(currentIndex);

  cmbMovieChangeDate->blockSignals(false);
}


void MovieChangeHistoryWidget::movieChangeDateChanged(int index)
{
  if (index > 0)
  {
    QLOG_DEBUG() << "User selected movie change date:" << cmbMovieChangeDate->itemData(index).toString();

    if (movieChangeModel->rowCount() > 0)
      movieChangeModel->removeRows(0, movieChangeModel->rowCount());

    // Populate model with movies in the movie change
    QVariantList movieChangeDetails = dbMgr->getMovieChangeDetails(cmbMovieChangeDate->itemData(index).toString());

    if (movieChangeDetails.count() > 0)
    {
      QLOG_DEBUG() << "Record count:" << movieChangeDetails.count();

      foreach (QVariant v, movieChangeDetails)
      {
        QVariantMap record = v.toMap();

        insertMovieChange(record["upc"].toString(), record["title"].toString(), record["producer"].toString(), record["category"].toString(), record["subcategory"].toString(), record["channel_num"].toInt(), record["previous_movie"].toString());
      }
    }
    else
    {
      QLOG_DEBUG() << "No records returned";
    }

    movieChangeTable->scrollToTop();
    movieChangeTable->sortByColumn(movieChangeTable->horizontalHeader()->sortIndicatorSection(), movieChangeTable->horizontalHeader()->sortIndicatorOrder());
  }
  else
  {
    QLOG_DEBUG() << "User selected the empty item in the movie change date list";

    if (movieChangeModel->rowCount() > 0)
      movieChangeModel->removeRows(0, movieChangeModel->rowCount());

    movieChangeTable->scrollToTop();
    movieChangeTable->sortByColumn(movieChangeTable->horizontalHeader()->sortIndicatorSection(), movieChangeTable->horizontalHeader()->sortIndicatorOrder());
  }
}

void MovieChangeHistoryWidget::insertMovieChange(QString upc, QString title, QString producer, QString category, QString subcategory, int channelNum, QString previousMovie)
{
  // UPC, Title, Producer, Category, Subcategory, Ch #, Previous Movie
  QStandardItem *upcField, *titleField, *producerField, *categoryField, *subcategoryField, *channelField, *previousMovieField;

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

  previousMovieField = new QStandardItem(previousMovie);
  previousMovieField->setData(previousMovie);

  int row = movieChangeModel->rowCount();

  movieChangeModel->setItem(row, UPC, upcField);
  movieChangeModel->setItem(row, Title, titleField);
  movieChangeModel->setItem(row, Producer, producerField);
  movieChangeModel->setItem(row, Category, categoryField);
  movieChangeModel->setItem(row, Subcategory, subcategoryField);
  movieChangeModel->setItem(row, Channel_Num, channelField);
  movieChangeModel->setItem(row, Previous_Movie, previousMovieField);
}
