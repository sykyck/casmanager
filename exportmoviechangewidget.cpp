#include "exportmoviechangewidget.h"
#include <QCalendarWidget>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <QHeaderView>
#include <QToolTip>
#include "qslog/QsLog.h"

ExportMovieChangeWidget::ExportMovieChangeWidget(QList<MovieChangeInfo> movieSetList, QList<MovieChangeInfo> exportedMovieSetList, bool importMode, QWidget *parent) : QDialog(parent)
{
  this->movieSetList = movieSetList;
  this->exportedMovieSetList = exportedMovieSetList;  
  this->importMode = importMode;
  chkShowExportedVideos = 0;

  if (!importMode)
    lblInstructions = new QLabel(tr("Select one or more movie change sets that you want to export to an external hard drive. To select a movie change set, check the box to the left of the set name.\n\nOnly movie sets that are complete will show up in the list below. If you do not see one then make sure all movies display \"True\" in the Complete column and \"Finished\" in the Transcode Status column.\n"));
  else
    lblInstructions = new QLabel(tr("Select one or more movie change sets that you want to import from an external hard drive. To select a movie change set, check the box to the left of the set name.\n\nMovie change sets that have already been imported will not be displayed in the list."));

  lblInstructions->setWordWrap(true);  

  lstMovieChangeSets = new QListWidget;
  lstMovieChangeSets->setToolTip(tr("Movie Change Sets"));
  connect(lstMovieChangeSets, SIGNAL(currentRowChanged(int)), this, SLOT(movieChangeSetSelectionChanged(int)));
  connect(lstMovieChangeSets, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(checkboxClicked(QListWidgetItem*)));

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

  connect(moviesTable, SIGNAL(clicked(QModelIndex)), this, SLOT(showVideoTooltip(QModelIndex)));
  moviesTable->setToolTip(tr("This list shows the movies in the currently\nhighlighted movie change set from the list above."));

  if (!importMode)
  {
    chkShowExportedVideos = new QCheckBox;
    chkShowExportedVideos->setChecked(false);
    connect(chkShowExportedVideos, SIGNAL(clicked()), this, SLOT(showExportedVideosClicked()));
  }

  /*txtNumVideos = new QLineEdit;
  txtNumVideos->setReadOnly(true);
  txtNumVideos->setEnabled(true);
  txtNumVideos->setText("0");*/

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
  //formLayout->addRow(tr("Movie Change"), cmbMovieSets);
  formLayout->addRow(lstMovieChangeSets);
  formLayout->addRow(moviesTable);
  //formLayout->addRow(tr("No. Videos"), txtNumVideos);

  if (!importMode)
    formLayout->addRow(tr("Show Already Exported"), chkShowExportedVideos);
  

  verticalLayout = new QVBoxLayout;
  verticalLayout->addWidget(lblInstructions);
  verticalLayout->addLayout(formLayout);
  verticalLayout->addWidget(buttonLayout);

  if (!importMode)
    this->setWindowTitle(tr("Export Movie Change Sets"));
  else
    this->setWindowTitle(tr("Import Movie Change Sets"));

  this->setLayout(verticalLayout);
  // Disable maximize,minimize and resizing of window
  this->setFixedSize(this->sizeHint());

  // Using a fixed width because the lstMovies is too wide
  this->setFixedWidth(400);
}

ExportMovieChangeWidget::~ExportMovieChangeWidget()
{
 // gridLayout->deleteLater();
}

QList<MovieChangeInfo> ExportMovieChangeWidget::getSelectedMovieSets()
{
  //return MovieChangeInfo();
  //return cmbMovieSets->itemData(cmbMovieSets->currentIndex()).value<MovieChangeInfo>();
  QList<MovieChangeInfo> selectedMovieSets;

  for (int i = 0; i < lstMovieChangeSets->count(); ++i)
  {
    QListWidgetItem *item = lstMovieChangeSets->item(i);

    if (item->checkState() == Qt::Checked)
    {
      selectedMovieSets.append(item->data(Qt::UserRole + 1).value<MovieChangeInfo>());
    }
  }

  return selectedMovieSets;
}

void ExportMovieChangeWidget::showEvent(QShowEvent *)
{
  if (chkShowExportedVideos)
  {
    chkShowExportedVideos->setChecked(false);
    showExportedVideosClicked();
  }
  else
  {
    populateMovieChangeCombobox(false);
    movieChangeSetSelectionChanged(0);
  }

  /*
  if (cmbMovieSets->count() > 0)
  {
    movieChangeSetSelectionChanged(0);
  }
  else
  {
    QLOG_DEBUG() << "No movie change sets need to be exported, only exported movie change sets exist";
    QMessageBox::warning(this, tr("Export Movie Change Set"), tr("There are no movie change sets that need to be exported. If you want to re-export a movie change set then tick the \"Show Already Exported\" checkbox."), QMessageBox::Ok);
  }*/
}

void ExportMovieChangeWidget::accept()
{
  bool itemChecked = false;

  // Make sure at least one item is checked in the list
  for (int i = 0; i < lstMovieChangeSets->count(); ++i)
  {
    QListWidgetItem *item = lstMovieChangeSets->item(i);

    if (item->checkState() == Qt::Checked)
    {
      itemChecked = true;
      break;
    }
  }

  if (itemChecked)
    QDialog::accept();
  else
  {
    QLOG_DEBUG() << QString("User did not checkmark any movie change set to %1").arg(importMode ? "import" : "export");
    QMessageBox::information(this, tr("%1 Videos").arg(importMode ? "Import" : "Export"), tr("Check the box of one or more movie change sets in the list to %1 the videos.").arg(importMode ? "import" : "export"));
  }
}

/*void ExportMovieChangeWidget::setNumVideos(int numVideos)
{
  txtNumVideos->setText(QString("%1").arg(numVideos));
}*/

void ExportMovieChangeWidget::populateMovieChangeCombobox(bool includeExported)
{
  lstMovieChangeSets->clear();
  int setNum = 1;

  // If already exported movie change sets should be included then append to end of list
  QList<MovieChangeInfo> combinedMovieChangeSets = movieSetList;
  if (includeExported)
    combinedMovieChangeSets += exportedMovieSetList;

  foreach (MovieChangeInfo m, combinedMovieChangeSets)
  {
    // Create an item for the list and enable a checkbox with an initial state
    QListWidgetItem *item = new QListWidgetItem(QString("Set #%1 (%2 video%3)").arg(setNum++).arg(m.NumVideos()).arg(m.NumVideos() > 1 ? "s" : ""), lstMovieChangeSets);
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    item->setCheckState(Qt::Unchecked);

    // Store the movie change set (MovieChangeInfo) object in the item
    QVariant v;
    v.setValue(m);
    item->setData(Qt::UserRole + 1, v);
  }

  // Select the first item in the list
  if (lstMovieChangeSets->count() > 0)
    lstMovieChangeSets->setCurrentRow(0);
}

void ExportMovieChangeWidget::showVideoTooltip(QModelIndex index)
{
  Q_UNUSED(index)

  QLOG_DEBUG() << "User clicked in the list of movies being shown for the currently selected movie change set";

  QToolTip::showText(QCursor::pos(), tr("This list shows the movies in the currently\nhighlighted movie change set from the list above."));
}

void ExportMovieChangeWidget::showExportedVideosClicked()
{
  if (chkShowExportedVideos)
  {
    if (chkShowExportedVideos->isChecked())
    {
      QLOG_DEBUG() << "User checked box to show already exported videos";

      if (QMessageBox::question(this, tr("Show Exported Videos"), tr("There are very few reasons to show already exported videos. Are you sure you want to show these?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
      {
        // Show all movie changes sets (even ones that were already exported)
        populateMovieChangeCombobox(true);
      }
      else
      {
        QLOG_DEBUG() << "User chose not to show already exported videos";
        chkShowExportedVideos->setChecked(false);
      }
    }
    else
    {
      // Show ONLY movie change sets that have not been exported yet
      populateMovieChangeCombobox(false);
    }

    movieChangeSetSelectionChanged(0);
  }
}

void ExportMovieChangeWidget::movieChangeSetSelectionChanged(int index)
{  
  Q_UNUSED(index)

  if (lstMovieChangeSets->currentRow() > -1)
  {
    QListWidgetItem *item = lstMovieChangeSets->currentItem();

    if (item->data(Qt::UserRole + 1).canConvert<MovieChangeInfo>())
    {
      MovieChangeInfo selectedMovieChange = item->data(Qt::UserRole + 1).value<MovieChangeInfo>();

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

      //setNumVideos(selectedMovieChange.numVideos);
    }
  }
  else
  {
    // No movie change set selected so clear movies view
    if (moviesModel->rowCount() > 0)
      moviesModel->removeRows(0, moviesModel->rowCount());
  }
}

void ExportMovieChangeWidget::checkboxClicked(QListWidgetItem *item)
{
  // If the item in the list that had its checkbox clicked is not currently
  // selected then select it now so the details list shows the videos in the set
  if (item != lstMovieChangeSets->currentItem())
    lstMovieChangeSets->setCurrentItem(item);
}
