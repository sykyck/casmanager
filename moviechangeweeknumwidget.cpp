#include "moviechangeweeknumwidget.h"
#include <QCalendarWidget>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <QHeaderView>
#include "qslog/QsLog.h"

MovieChangeWeekNumWidget::MovieChangeWeekNumWidget(QList<MovieChangeInfo> movieSetList, int highestChannelNum, QWidget *parent) : QDialog(parent)
{
  lblInstructions = new QLabel(tr("Only movie sets that are complete will show up in the list below. If you do not see one, return to the Movie Library tab and make sure all movies display \"True\" in the Complete column and \"Finished\" in the Transcode Status column.\n"));
  lblInstructions->setWordWrap(true);  

  this->highestChannelNum = highestChannelNum;

  cmbMovieSets = new QComboBox;
  cmbMovieSets->setMaxVisibleItems(10);

  int setNum = 1;
  foreach (MovieChangeInfo m, movieSetList)
  {
    QVariant v;
    v.setValue(m);

    cmbMovieSets->addItem(QString("Set #%1").arg(setNum++), v);
  }

  connect(cmbMovieSets, SIGNAL(currentIndexChanged(int)), this, SLOT(movieChangeSetSelectionChanged(int)));

  moviesModel = new QStandardItemModel(0, 2);
  moviesModel->setHorizontalHeaderItem(0, new QStandardItem(QString("UPC")));
  moviesModel->setHorizontalHeaderItem(1, new QStandardItem(QString("Title")));

  QSortFilterProxyModel *sortFilter = new QSortFilterProxyModel;
  sortFilter->setSourceModel(moviesModel);
  sortFilter->setSortRole(Qt::UserRole + 1);

  moviesTable = new QTableView;
  moviesTable->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
  //moviesTable->horizontalHeader()->setStretchLastSection(true);
  moviesTable->horizontalHeader()->setStyleSheet("font:bold Arial;");
  moviesTable->setWordWrap(true);
  moviesTable->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
  moviesTable->verticalHeader()->hide();
  moviesTable->setSortingEnabled(true);
  moviesTable->setModel(sortFilter);
  moviesTable->setEditTriggers(QAbstractItemView::NoEditTriggers); // Make table read-only
  moviesTable->setSelectionMode(QAbstractItemView::NoSelection); // Make table read-only
  moviesTable->sortByColumn(1, Qt::AscendingOrder);

  /*
  // Default to current time
  QDateTime defaultDate =  QDateTime::currentDateTime();

  // Set the seconds of the current time to zero
  defaultDate.setTime(QTime(defaultDate.time().hour(), defaultDate.time().minute()));

  // disable date picker since this will complicate things
  scheduledDate = new QDateTimeEdit(defaultDate);
  scheduledDate->setDateTimeRange(defaultDate, QDateTime::currentDateTime().addYears(3));
  scheduledDate->setDisplayFormat("M/d/yyyy h:mm AP");
  scheduledDate->setCalendarPopup(true);
  scheduledDate->calendarWidget()->setGridVisible(true);
  scheduledDate->calendarWidget()->setFirstDayOfWeek(Qt::Monday);
*/

  chkOverrideViewtimes = new QCheckBox;
  chkOverrideViewtimes->setChecked(false);
  connect(chkOverrideViewtimes, SIGNAL(clicked()), this, SLOT(overrideViewtimesClicked()));
  // Currently this feature is disabled, only one customer needed it long ago
  chkOverrideViewtimes->setEnabled(false);

  txtNumVideos = new QLineEdit;
  txtNumVideos->setReadOnly(true);
  txtNumVideos->setEnabled(false);

  spnFirstChannel = new QSpinBox;
  spnFirstChannel->setRange(1, 1000);
  spnFirstChannel->setEnabled(false);
  connect(spnFirstChannel, SIGNAL(valueChanged(int)), this, SLOT(firstChannelChanged(int)));

  spnLastChannel = new QSpinBox;
  spnLastChannel->setRange(1, 9999);
  spnLastChannel->setReadOnly(true);
  spnLastChannel->setEnabled(false);

  chkAllMovieChangeSets = new QCheckBox;
  chkAllMovieChangeSets->setChecked(true);
  connect(chkAllMovieChangeSets, SIGNAL(clicked()), this, SLOT(allMovieChangeSetsClicked()));

  btnOK = new QPushButton(tr("OK"));
  connect(btnOK, SIGNAL(clicked()), this, SLOT(accept()));

  btnCancel = new QPushButton(tr("Cancel"));
  connect(btnCancel, SIGNAL(clicked()), this, SLOT(reject()));

  buttonLayout = new QDialogButtonBox;
  buttonLayout->addButton(btnOK, QDialogButtonBox::AcceptRole);
  buttonLayout->addButton(btnCancel, QDialogButtonBox::RejectRole);

  formLayout = new QFormLayout;
  // This size policy for the fields will make the cmbMovieSets stretch the full width of window
  //formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
  formLayout->addRow(tr("All Movie Sets"), chkAllMovieChangeSets);
  formLayout->addRow(tr("Movie Change"), cmbMovieSets);
  formLayout->addRow(moviesTable);
  //formLayout->addRow(tr("Start Time"), scheduledDate);
  formLayout->addRow(tr("Override Viewtimes"), chkOverrideViewtimes);
  formLayout->addRow(tr("No. Videos"), txtNumVideos);
  formLayout->addRow(tr("Starting Channel #"), spnFirstChannel);
  formLayout->addRow(tr("Ending Channel #"), spnLastChannel);

  verticalLayout = new QVBoxLayout;
  verticalLayout->addWidget(lblInstructions);
  verticalLayout->addLayout(formLayout);
  verticalLayout->addWidget(buttonLayout);

  this->setWindowTitle(tr("Select Movie Change"));
  this->setLayout(verticalLayout);
  // Disable maximize,minimize and resizing of window
  this->setFixedSize(this->sizeHint());

  // Using a fixed width because the lstMovies is too wide
  this->setFixedWidth(400);
}

MovieChangeWeekNumWidget::~MovieChangeWeekNumWidget()
{
 // gridLayout->deleteLater();
}

QList<MovieChangeInfo> MovieChangeWeekNumWidget::getSelectedMovieSet()
{
  QList<MovieChangeInfo> movieChangeSets;

  if (chkAllMovieChangeSets->isChecked())
  {
    for (int i = 0; i < cmbMovieSets->count(); ++i)
    {
      MovieChangeInfo movieChangeInfo = cmbMovieSets->itemData(i).value<MovieChangeInfo>();

      // Set properties to the expected state just in case
      movieChangeInfo.setOverrideViewtimes(false);
      movieChangeInfo.setFirstChannel(0);
      movieChangeInfo.setLastChannel(0);

      movieChangeSets.append(movieChangeInfo);
    }
  }
  else
  {
    MovieChangeInfo movieChangeInfo = cmbMovieSets->itemData(cmbMovieSets->currentIndex()).value<MovieChangeInfo>();

    if (chkOverrideViewtimes->isChecked())
    {
      movieChangeInfo.setOverrideViewtimes(true);
      movieChangeInfo.setFirstChannel(spnFirstChannel->value());
      movieChangeInfo.setLastChannel(spnLastChannel->value());
    }
    else
    {
      // Set properties to the expected state just in case
      movieChangeInfo.setOverrideViewtimes(false);
      movieChangeInfo.setFirstChannel(0);
      movieChangeInfo.setLastChannel(0);
    }

    movieChangeSets.append(movieChangeInfo);
  }



  return movieChangeSets;
}

void MovieChangeWeekNumWidget::showEvent(QShowEvent *)
{
  chkOverrideViewtimes->setChecked(false);
  overrideViewtimesClicked();

  movieChangeSetSelectionChanged(0);

  chkAllMovieChangeSets->setChecked(true);
  allMovieChangeSetsClicked();

//  MovieChangeInfo movieChange = cmbMovieSets->itemData(cmbMovieSets->currentIndex()).value<MovieChangeInfo>();
//  setNumVideos(movieChange.numVideos);

  // Call again to update last channel #
//  setFirstChannel(spnFirstChannel->value());
}

void MovieChangeWeekNumWidget::accept()
{
  // TODO: Is this really a problem, extending past the highest channel currently in the arcade?
  if (chkOverrideViewtimes->isChecked() && spnLastChannel->value() > highestChannelNum)
  {
    QLOG_DEBUG() << QString("User selected a movie change set and is overriding the view times but the selected channel range to replace: %0 - %1 extends past the highest channel # which is: %2")
                    .arg(spnFirstChannel->value())
                    .arg(spnLastChannel->value())
                    .arg(highestChannelNum);

    QMessageBox::warning(this, tr("Error"), tr("When overriding the view times, the channel number range cannot extend past the last channel number currently in the arcade."));

    return;
  }

  QDialog::accept();
}

void MovieChangeWeekNumWidget::setFirstChannel(int channelNum)
{
  spnFirstChannel->setValue(channelNum);
  firstChannelChanged(channelNum);
}

void MovieChangeWeekNumWidget::setLastChannel(int channelNum)
{
  spnLastChannel->setValue(channelNum);
}

void MovieChangeWeekNumWidget::setNumVideos(int numVideos)
{
  txtNumVideos->setText(QString("%1").arg(numVideos));
}

void MovieChangeWeekNumWidget::overrideViewtimesClicked()
{
  if (chkOverrideViewtimes->isChecked())
  {
    txtNumVideos->setEnabled(true);
    spnFirstChannel->setEnabled(true);
    spnLastChannel->setEnabled(true);
  }
  else
  {
    txtNumVideos->setEnabled(false);
    spnFirstChannel->setEnabled(false);
    spnLastChannel->setEnabled(false);
  }
}

void MovieChangeWeekNumWidget::firstChannelChanged(int channelNum)
{
  spnLastChannel->setValue(channelNum + txtNumVideos->text().toInt() - 1);
}

void MovieChangeWeekNumWidget::movieChangeSetSelectionChanged(int index)
{  
  if (cmbMovieSets->currentIndex() > -1)
  {
    if (cmbMovieSets->itemData(cmbMovieSets->currentIndex()).canConvert<MovieChangeInfo>())
    {
      MovieChangeInfo selectedMovieChange = cmbMovieSets->itemData(cmbMovieSets->currentIndex()).value<MovieChangeInfo>();

      // Clear existing records if they exist
      if (moviesModel->rowCount() > 0)
        moviesModel->removeRows(0, moviesModel->rowCount());

      foreach (DvdCopyTask m, selectedMovieChange.DvdCopyTaskList())
      {
        // UPC, Title
        QStandardItem *upcField, *titleField;

        upcField = new QStandardItem(m.UPC());
        upcField->setData(m.UPC());

        titleField = new QStandardItem(m.Title());
        titleField->setData(m.Title());

        int row = moviesModel->rowCount();

        moviesModel->setItem(row, 0, upcField);
        moviesModel->setItem(row, 1, titleField);
      }

      setNumVideos(selectedMovieChange.NumVideos());
      setFirstChannel(spnFirstChannel->value());
    }
  }
}

void MovieChangeWeekNumWidget::allMovieChangeSetsClicked()
{
  if (chkAllMovieChangeSets->isChecked())
  {
    cmbMovieSets->setEnabled(false);
    moviesTable->setEnabled(false);
  }
  else
  {
    QMessageBox::information(this, tr("Movie Sets"), tr("Unchecking this box means you will add only the currently selected movie set to the queue. Likewise, if the box is checked, all movie sets are added at once which is useful when there are many sets."));

    cmbMovieSets->setEnabled(true);
    moviesTable->setEnabled(true);
  }
}
