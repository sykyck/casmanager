#include "existingmoviechangewidget.h"
#include "qslog/QsLog.h"
#include <QSortFilterProxyModel>
#include <QHeaderView>
#include <QMessageBox>

ExistingMovieChangeWidget::ExistingMovieChangeWidget(QList<MovieChangeInfo> movieChangeSets, int maxMovieChangeSetSize, QWidget *parent) : QDialog(parent)
{
  //this->movieChangeSets = movieChangeSets;
  this->maxMovieChangeSetSize = maxMovieChangeSetSize;

  lblInstructions = new QLabel(tr("There are one or more existing movie change sets that have not been sent to the booths. You can either create a new set or add to an existing one."));
  lblInstructions->setWordWrap(true);

  lblMovieChangeSetSize = new QLabel;

  rdoNewMovieSet = new QRadioButton(tr("New Movie Change Set"));
  connect(rdoNewMovieSet, SIGNAL(clicked()), this, SLOT(movieSetChanged()));

  rdoExistingMovieSet = new QRadioButton(tr("Add to Existing Movie Change Set"));
  connect(rdoExistingMovieSet, SIGNAL(clicked()), this, SLOT(movieSetChanged()));

  cmbMovieSets = new QComboBox;
  cmbMovieSets->setMaxVisibleItems(10);

 // QLOG_DEBUG() << "Now populating cmbMovieSets";

  int setNum = 1;
  foreach (MovieChangeInfo m, movieChangeSets)
  {
    QVariant v;
    v.setValue(m);

    cmbMovieSets->addItem(QString("Set #%1").arg(setNum++), v);
  }

 // QLOG_DEBUG() << "Finished populating cmbMovieSets";

  connect(cmbMovieSets, SIGNAL(currentIndexChanged(int)), this, SLOT(itemSelectionChanged(int)));

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

  btnOK = new QPushButton(tr("OK"));
  connect(btnOK, SIGNAL(clicked()), this, SLOT(accept()));

  btnCancel = new QPushButton(tr("Cancel"));
  connect(btnCancel, SIGNAL(clicked()), this, SLOT(reject()));

  buttonLayout = new QDialogButtonBox;
  buttonLayout->addButton(btnOK, QDialogButtonBox::AcceptRole);
  buttonLayout->addButton(btnCancel, QDialogButtonBox::RejectRole);

  verticalLayout = new QVBoxLayout;
  verticalLayout->addWidget(lblInstructions);
  verticalLayout->addWidget(rdoNewMovieSet);
  verticalLayout->addWidget(rdoExistingMovieSet);
  verticalLayout->addWidget(cmbMovieSets);
  verticalLayout->addWidget(moviesTable);
  verticalLayout->addWidget(lblMovieChangeSetSize);
  verticalLayout->addWidget(buttonLayout);

  this->setLayout(verticalLayout);
  this->setWindowTitle(tr("Movie Change Set"));
}

MovieChangeInfo ExistingMovieChangeWidget::getMovieChangeSelection()
{
  MovieChangeInfo m;

  // Return an empty MovieChangeInfo object if new movie set selected, otherwise
  // return the selected movie set from the drop down menu
  if (rdoExistingMovieSet->isChecked())
  {
    m = cmbMovieSets->itemData(cmbMovieSets->currentIndex()).value<MovieChangeInfo>();
  }

  return m;
}

void ExistingMovieChangeWidget::showEvent(QShowEvent *)
{
  rdoNewMovieSet->setChecked(true);
  movieSetChanged();
  itemSelectionChanged(0);
  this->setFixedSize(this->sizeHint());
}

void ExistingMovieChangeWidget::accept()
{
  QString errorMessage = "";

  // Do not allow selecting a movie change set if it already has the maximum number of videos
  if (rdoExistingMovieSet->isChecked())
  {
    MovieChangeInfo m = cmbMovieSets->itemData(cmbMovieSets->currentIndex()).value<MovieChangeInfo>();

    if (m.DvdCopyTaskList().count() == maxMovieChangeSetSize)
      errorMessage = tr("This movie change set already contains the maximum number of videos which is %1. Select another movie change set or start a new one.").arg(maxMovieChangeSetSize);
  }

  if (errorMessage.isEmpty())
    QDialog::accept();
  else
  {
    QLOG_DEBUG() << QString("Problem with existing movie change set selection: %1").arg(errorMessage);
    QMessageBox::warning(this, tr("Movie Change Set"), errorMessage);
  }
}

void ExistingMovieChangeWidget::movieSetChanged()
{
  if (rdoNewMovieSet->isChecked())
  {
    cmbMovieSets->setEnabled(false);
    moviesTable->setEnabled(false);
    lblMovieChangeSetSize->setEnabled(false);
  }
  else
  {
    cmbMovieSets->setEnabled(true);
    moviesTable->setEnabled(true);
    lblMovieChangeSetSize->setEnabled(true);
  }
}

void ExistingMovieChangeWidget::itemSelectionChanged(int)
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

      lblMovieChangeSetSize->setText(tr("Video Count: %1").arg(selectedMovieChange.DvdCopyTaskList().count()));
    }
    else
      QLOG_ERROR() << "Cannot convert item data in cmbMovieSets to MovieChangeInfo!";
  }
  else
  {
    lblMovieChangeSetSize->setText("");
  }
}
