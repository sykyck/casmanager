#include "movielibrarywidget.h"
#include "addvideowidget.h"
#include "qslog/QsLog.h"
#include "global.h"
#include <QHeaderView>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QStandardItem>
#include <QVariant>
#include <QSortFilterProxyModel>
#include <QDebug>
#include "existingmoviechangewidget.h"
#include "exportmoviechangewidget.h"
#include "moviechangesetindex.h"
#include "filehelper.h"

// FIXME: Looks like if user clicks this tab during the intial booth status check, there is the possibility of interrupting it and results in an empty movie library datagrid

MovieLibraryWidget::MovieLibraryWidget(DatabaseMgr *dbMgr, CasServer *casServer, Settings *settings, QWidget *parent) : QWidget(parent)
{
  this->dbMgr = dbMgr;
  this->settings = settings;
  this->casServer = casServer;
  currentDvdIndex = 0;
  importExportButtonLayout = 0;
  buttonLayout = 0;
  btnExport = 0;
  btnImport = 0;
  btnDeleteVideo = 0;
  fsWatcher = 0;
  fileCopier = 0;
  totalFilesToExport = 0;
  firstLoad = true;
  busy = false;
  driveState = Unknown;
  userRequestedMetadata = false;
  userCanceledJob = false;

  mediaInfoWin = new MediaInfoWidget(this->dbMgr, this->settings);
  connect(mediaInfoWin, SIGNAL(accepted()), this, SLOT(updateMetadata()));
  connect(mediaInfoWin, SIGNAL(rejected()), this, SLOT(metadataUpdateCanceled()));

  videoInfoWin = new VideoInfoWidget(this->dbMgr, this->settings);
  connect(videoInfoWin, SIGNAL(recipientChanged(QString, QString, QString)), this, SLOT(recipientChanged(QString, QString, QString)));


  lookupService = new VideoLookupService(WEB_SERVICE_URL, settings->getValue(VIDEO_METADATA_PATH).toString(), settings->getValue(CUSTOMER_PASSWORD, "", true).toString());
  connect(lookupService, SIGNAL(videoLookupResults(QList<Movie>)), this, SLOT(checkUPCsResult(QList<Movie>)));
  connect(lookupService, SIGNAL(videoUpdateResults(bool,QString)), this, SLOT(checkVideoUpdateResult(bool,QString)));
  connect(lookupService, SIGNAL(deleteVideoLookupResults(bool,QString,QStringList)), this, SLOT(deleteVideoLookupResults(bool,QString,QStringList)));

  dvdCopier = new DvdCopier(settings->getValue(DVD_COPY_PROG).toString(), settings->getValue(DVD_COPIER_LOG_FILE).toString(), settings->getValue(DVD_COPIER_PROC_NAME).toString());
  connect(dvdCopier, SIGNAL(startedCopying()), this, SLOT(dvdCopierStarted()));
  connect(dvdCopier, SIGNAL(finishedCopying(bool,QString)), this, SLOT(dvdCopierFinished(bool,QString)));

  discAutoloader = new DiscAutoLoader(settings->getValue(AUTOLOADER_DVD_DRIVE_MOUNT).toString(),
                                      settings->getValue(AUTOLOADER_PROG_FILE).toString(),
                                      settings->getValue(AUTOLOADER_RESPONSE_FILE).toString(),
                                      settings->getValue(AUTOLOADER_DVD_DRIVE_STATUS_FILE).toString(),
                                      settings->getValue(DVD_MOUNT_TIMEOUT).toInt());
  connect(discAutoloader, SIGNAL(finishedLoadingDisc(bool,QString)), this, SLOT(loadDiscFinished(bool,QString)));
  connect(discAutoloader, SIGNAL(finishedUnloadingDisc(bool,QString)), this, SLOT(unloadDiscFinished(bool,QString)));

  upcLookupTimer = new QTimer;
  upcLookupTimer->setInterval(settings->getValue(UPC_LOOKUP_INTERVAL).toInt());
  connect(upcLookupTimer, SIGNAL(timeout()), this, SLOT(checkUPCs()));

  deleteUpcTimer = new QTimer;
  deleteUpcTimer->setInterval(settings->getValue(DELETE_UPC_INTERVAL).toInt());
  connect(deleteUpcTimer, SIGNAL(timeout()), this, SLOT(checkDeleteUPCs()));

  verticalLayout = new QVBoxLayout;

  // Week #, UPC, Title, Producer, Category, Subcategory, Status, Year, DvdCopyTask
  movieCopyJobModel = new QStandardItemModel(0, 10);
  movieCopyJobModel->setHorizontalHeaderItem(0, new QStandardItem(QString("Week #")));
  movieCopyJobModel->setHorizontalHeaderItem(1, new QStandardItem(QString("UPC")));
  movieCopyJobModel->setHorizontalHeaderItem(2, new QStandardItem(QString("Title")));
  movieCopyJobModel->setHorizontalHeaderItem(3, new QStandardItem(QString("Producer")));
  movieCopyJobModel->setHorizontalHeaderItem(4, new QStandardItem(QString("Category")));
  movieCopyJobModel->setHorizontalHeaderItem(5, new QStandardItem(QString("Subcategory")));
  movieCopyJobModel->setHorizontalHeaderItem(6, new QStandardItem(QString("Copy Status")));
  movieCopyJobModel->setHorizontalHeaderItem(7, new QStandardItem(QString("Transcode Status")));
  movieCopyJobModel->setHorizontalHeaderItem(8, new QStandardItem(QString("Year")));
  movieCopyJobModel->setHorizontalHeaderItem(9, new QStandardItem(QString("DvdCopyTask")));

  // Week #, Complete, UPC, Title, Producer, Category, Subcategory, File Size, Year, DvdCopyTask
  movieChangeSetsModel = new QStandardItemModel(0, 11);
  movieChangeSetsModel->setHorizontalHeaderItem(0, new QStandardItem(QString("Week #")));
  movieChangeSetsModel->setHorizontalHeaderItem(1, new QStandardItem(QString("UPC")));
  movieChangeSetsModel->setHorizontalHeaderItem(2, new QStandardItem(QString("Title")));
  movieChangeSetsModel->setHorizontalHeaderItem(3, new QStandardItem(QString("Producer")));
  movieChangeSetsModel->setHorizontalHeaderItem(4, new QStandardItem(QString("Category")));
  movieChangeSetsModel->setHorizontalHeaderItem(5, new QStandardItem(QString("Subcategory")));
  movieChangeSetsModel->setHorizontalHeaderItem(6, new QStandardItem(QString("File Size")));
  movieChangeSetsModel->setHorizontalHeaderItem(7, new QStandardItem(QString("Transcode Status")));
  movieChangeSetsModel->setHorizontalHeaderItem(8, new QStandardItem(QString("Complete")));
  movieChangeSetsModel->setHorizontalHeaderItem(9, new QStandardItem(QString("Year")));
  movieChangeSetsModel->setHorizontalHeaderItem(10, new QStandardItem(QString("DvdCopyTask")));

  // Ch #, UPC, Title, Producer, Category, Subcategory, Date Added
  libraryModel = new QStandardItemModel(0, 7);
  libraryModel->setHorizontalHeaderItem(0, new QStandardItem(QString("Ch #")));
  libraryModel->setHorizontalHeaderItem(1, new QStandardItem(QString("UPC")));
  libraryModel->setHorizontalHeaderItem(2, new QStandardItem(QString("Title")));
  libraryModel->setHorizontalHeaderItem(3, new QStandardItem(QString("Producer")));
  libraryModel->setHorizontalHeaderItem(4, new QStandardItem(QString("Category")));
  libraryModel->setHorizontalHeaderItem(5, new QStandardItem(QString("Subcategory")));
  libraryModel->setHorizontalHeaderItem(6, new QStandardItem(QString("Date Added")));

  movieCopyJobTable = new QTableView;
  movieChangeSetsTable = new QTableView;
  currentLibraryTable = new QTableView;

  lblCurrentJob = new QLabel(tr("Current DVD Copy Job"));
  lblMovieChangeSets = new QLabel(tr("Movie Change Sets"));
  lblLibrary = new QLabel(tr("Current Video Library"));
  lblTotalChannels = new QLabel;

  btnNewJob = new QPushButton(tr("New DVD Copy Job"));
  connect(btnNewJob, SIGNAL(clicked()), this, SLOT(addNewJob()));

  btnDeleteJob = new QPushButton(tr("Delete Job"));
  btnDeleteJob->setEnabled(false);
  connect(btnDeleteJob, SIGNAL(clicked()), this, SLOT(deleteJob()));

  btnRetryJob = new QPushButton(tr("Retry Copy Job"));
  btnRetryJob->setEnabled(false);
  connect(btnRetryJob, SIGNAL(clicked()), this, SLOT(retryCopyProcess()));

  btnShowAutoloaderCP = new QPushButton(tr("Autoloader Control Panel"));
  btnShowAutoloaderCP->setEnabled(false);
  connect(btnShowAutoloaderCP, SIGNAL(clicked()), this, SLOT(showAutoloaderCP()));

  btnDownloadMetadata = new QPushButton(tr("Download Metadata"));
  connect(btnDownloadMetadata, SIGNAL(clicked()), this, SLOT(downloadMetadataClicked()));

  btnCancelJob = new QPushButton(tr("Cancel Copy Job"));
  btnCancelJob->setEnabled(false);
  connect(btnCancelJob, SIGNAL(clicked()), this, SLOT(cancelJobClicked()));

  //connect(tableCards, SIGNAL(clicked(QModelIndex)), this, SLOT(onCardClicked(QModelIndex)));
  //tableCards->setStyleSheet("QTableView {font:22px Arial;} QTableView::item {padding:10px 5px 10px 5px;} QScrollBar:vertical {border-width:1px; border-style:solid; border-top-color:#444444; border-left-color:#444444; border-right-color:#AAAAAA; border-bottom-color:#AAAAAA; background: rgb(94, 173, 42); width: 32px; margin: 36px 0 36px 0;} QScrollBar::handle:vertical {border-width:2px; border-style:solid; border-top-color:#DDDDDD; border-left-color:#DDDDDD; border-right-color:#666666; border-bottom-color:#666666; background: #a9a9a9; min-height: 20px;} QScrollBar::add-line:vertical {border-width:2px; border-style:solid; border-top-color:#DDDDDD; border-left-color:#DDDDDD; border-right-color:#666666; border-bottom-color:#666666; background-color: #a9a9a9; background-image:url(:/images/scroll-down-arrow.png); background-position:-2 0; height: 32px; subcontrol-position: bottom; subcontrol-origin: margin;} QScrollBar::sub-line:vertical {border-width:2px; border-style:solid; border-top-color:#DDDDDD; border-left-color:#DDDDDD; border-right-color:#666666; border-bottom-color:#666666; background-color: #a9a9a9; background-image:url(:/images/scroll-up-arrow.png); background-position:-2 0; height: 32px; subcontrol-position: top; subcontrol-origin: margin;} QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {background: none;}");

  movieCopyJobTable->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
  movieCopyJobTable->horizontalHeader()->setStretchLastSection(true);
  movieCopyJobTable->horizontalHeader()->setStyleSheet("font:bold Arial;");
  movieCopyJobTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
  movieCopyJobTable->setSelectionMode(QAbstractItemView::SingleSelection);
  movieCopyJobTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  movieCopyJobTable->setWordWrap(true);
  movieCopyJobTable->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
  movieCopyJobTable->verticalHeader()->hide();
  movieCopyJobTable->setModel(movieCopyJobModel);
  movieCopyJobTable->setColumnHidden(0, true);
  movieCopyJobTable->setColumnHidden(movieCopyJobModel->columnCount() - 1, true);
  movieCopyJobTable->setColumnHidden(movieCopyJobModel->columnCount() - 2, true);
  // Make table read-only
  movieCopyJobTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  movieCopyJobTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
  connect(movieCopyJobTable, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(editDvdCopyTask(QModelIndex)));

  QSortFilterProxyModel *sortFilter = new QSortFilterProxyModel;
  sortFilter->setSourceModel(movieChangeSetsModel);
  sortFilter->sort(0, Qt::DescendingOrder);
  sortFilter->sort(10, Qt::DescendingOrder);

  movieChangeSetsTable->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
  movieChangeSetsTable->horizontalHeader()->setStretchLastSection(true);
  movieChangeSetsTable->horizontalHeader()->setStyleSheet("font:bold Arial;");
  movieChangeSetsTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
  movieChangeSetsTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
  movieChangeSetsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  movieChangeSetsTable->setWordWrap(true);
  movieChangeSetsTable->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
  movieChangeSetsTable->verticalHeader()->hide();
  movieChangeSetsTable->setModel(sortFilter);
  // FIXME: Sorting doesn't match the underlying model!
  //movieChangeSetsTable->setSortingEnabled(true);
  movieChangeSetsTable->setAlternatingRowColors(true);
  movieChangeSetsTable->setColumnHidden(0, true);
  movieChangeSetsTable->setColumnHidden(movieChangeSetsModel->columnCount() - 1, true);
  movieChangeSetsTable->setColumnHidden(movieChangeSetsModel->columnCount() - 2, true);
  movieChangeSetsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  movieChangeSetsTable->installEventFilter(this);
  connect(movieChangeSetsTable, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(editMovieChangeSetItem(QModelIndex)));


  currentLibraryTable->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
  currentLibraryTable->horizontalHeader()->setStretchLastSection(true);
  currentLibraryTable->horizontalHeader()->setStyleSheet("font:bold Arial;");
  currentLibraryTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
  currentLibraryTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
  currentLibraryTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  currentLibraryTable->setWordWrap(true);
  currentLibraryTable->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
  currentLibraryTable->verticalHeader()->hide();
  currentLibraryTable->setModel(libraryModel);
  currentLibraryTable->setAlternatingRowColors(true);
  currentLibraryTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  currentLibraryTable->installEventFilter(this);
  connect(currentLibraryTable, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(viewVideoLibraryEntry(QModelIndex)));

  // Only add DVD copying related controls if mode not No_DVD_Copying
  if (((Settings::App_Mode)settings->getValue(APP_MODE).toInt()) != Settings::No_DVD_Copying)
  {
    buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(btnNewJob, 0, Qt::AlignLeft);
    buttonLayout->addWidget(btnCancelJob, 0, Qt::AlignLeft);
    buttonLayout->addWidget(btnRetryJob, 0, Qt::AlignLeft);
    buttonLayout->addWidget(btnDeleteJob, 0, Qt::AlignLeft);    
    buttonLayout->addWidget(btnDownloadMetadata, 0, Qt::AlignLeft);

    buttonLayout->addStretch(1);
    buttonLayout->addWidget(btnShowAutoloaderCP, 0, Qt::AlignRight);

    verticalLayout->addWidget(lblCurrentJob, 0, Qt::AlignLeft);
    verticalLayout->addWidget(movieCopyJobTable);
    verticalLayout->addLayout(buttonLayout);
  }

  verticalLayout->addWidget(lblMovieChangeSets, 0, Qt::AlignLeft);
  verticalLayout->addWidget(movieChangeSetsTable);

  // Do not show the current videos that are in the booths if in DVD_Copying_Only mode
  if (((Settings::App_Mode)settings->getValue(APP_MODE).toInt()) == Settings::DVD_Copying_Only)
  {
    btnExport = new QPushButton(tr("Export Videos"));
    connect(btnExport, SIGNAL(clicked()), this, SLOT(exportButtonClicked()));

    importExportButtonLayout = new QHBoxLayout;
    importExportButtonLayout->addWidget(btnExport, 0, Qt::AlignLeft);
    verticalLayout->addLayout(importExportButtonLayout);
  }
  else
  {
    // Show the Import Videos below the movie change if in No_DVD_Copying mode
    if (((Settings::App_Mode)settings->getValue(APP_MODE).toInt()) == Settings::No_DVD_Copying)
    {
      btnImport = new QPushButton(tr("Import Videos"));
      connect(btnImport, SIGNAL(clicked()), this, SLOT(importButtonClicked()));

      importExportButtonLayout = new QHBoxLayout;
      importExportButtonLayout->addWidget(btnImport, 0, Qt::AlignLeft);
      verticalLayout->addLayout(importExportButtonLayout);
    }

    btnDeleteVideo = new QPushButton("Delete Video");
    connect(btnDeleteVideo, SIGNAL(clicked()), this, SLOT(deleteVideoLibraryEntry()));

    videoLibraryLayout = new QHBoxLayout;
    videoLibraryLayout->addWidget(lblLibrary, 0, Qt::AlignLeft);
    videoLibraryLayout->addWidget(lblTotalChannels, 0, Qt::AlignLeft);
    videoLibraryLayout->addStretch(2);

    QHBoxLayout *videoLibraryButtonLayout = new QHBoxLayout;
    videoLibraryButtonLayout->addWidget(btnDeleteVideo, 0, Qt::AlignLeft);

    verticalLayout->addLayout(videoLibraryLayout);
    verticalLayout->addWidget(currentLibraryTable);
    verticalLayout->addLayout(videoLibraryButtonLayout);
  }

  // Only setup the following objects if mode is not All_Functions
  if (((Settings::App_Mode)settings->getValue(APP_MODE).toInt()) != Settings::All_Functions)
  {
    fsWatcher = new FileSystemWatcher;
    fsWatcher->addMountPoint(settings->getValue(EXT_HD_MOUNT_PATH).toString(), settings->getValue(EXT_HD_MOUNT_TIMEOUT).toInt());

    connect(fsWatcher, SIGNAL(mounted(QString)), this, SLOT(externalDriveMounted(QString)));
    connect(fsWatcher, SIGNAL(timeout()), this, SLOT(externalDriveTimeout()));

    fileCopier = new FileCopier;
    connect(fileCopier, SIGNAL(filesToCopy(int)), this, SLOT(determinedNumFilesToCopy(int)));
    connect(fileCopier, SIGNAL(copyProgress(int)), this, SLOT(copyProgress(int)));
    connect(fileCopier, SIGNAL(finished()), this, SLOT(finishedCopying()));
  }

  this->setLayout(verticalLayout);
}

MovieLibraryWidget::~MovieLibraryWidget()
{
  // clearMovieChangeSets();
  // clearMovieCopyJobs();
  // clearMovieLibrary();
  // libraryModel->deleteLater();
  // movieChangeSetsModel->deleteLater();
  // movieCopyJobModel->deleteLater();
  upcLookupTimer->deleteLater();
  deleteUpcTimer->deleteLater();
  lookupService->deleteLater();

  // FIX: Doesn't seem to work
  if (mediaInfoWin->isVisible())
    mediaInfoWin->close();

  mediaInfoWin->deleteLater();
  // verticalLayout->deleteLater();

  if (fsWatcher)
    fsWatcher->deleteLater();

  if (fileCopier)
    fileCopier->deleteLater();

  // TODO: Make sure to delete widgets that aren't in a layout, like those that aren't used because of the current application mode
}

bool MovieLibraryWidget::isBusy()
{
  // TODO: This will be a problem if the table has jobs that have failed and the user does not delete them
  if (busy || movieCopyJobModel->rowCount() > 0 || (fileCopier && fileCopier->isBusy()))
    return true;
  else
    return false;
}

bool MovieLibraryWidget::eventFilter(QObject *obj, QEvent *event)
{
  if (event->type() == QEvent::KeyPress)
  {
    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

    if (keyEvent->key() == Qt::Key_Delete)
    {
      if (movieChangeSetsTable->hasFocus())
      {
        deleteMovieChangeEntry();
        return true;
      }
      else if (currentLibraryTable->hasFocus())
      {
        deleteVideoLibraryEntry();
        return true;
      }
      else
      {
        QLOG_DEBUG() << QString("Delete key pressed without a table in focus");
        QMessageBox::information(this, tr("Delete"), tr("Click an entry in the Movie Change Sets or Current Video Library table to delete."));

        // standard event processing
        return QWidget::eventFilter(obj, event);
      }
    }
    else
    {
      // standard event processing
      return QWidget::eventFilter(obj, event);
    }
  }
  else
  {
    // standard event processing
    return QWidget::eventFilter(obj, event);
  }
}

void MovieLibraryWidget::showEvent(QShowEvent *)
{
  QLOG_DEBUG() << QString("Show Movie Library tab");

  // FIXME: The autoloader button is getting disabled or enabled in situations where a dvd copy job is over but failed items remain in the datagrid. Need to have flag for indicating if a copy job is actually in progress
  if (settings->getValue(ENABLE_AUTO_LOADER).toBool() && movieCopyJobModel->rowCount() == 0)
  {
    btnShowAutoloaderCP->setEnabled(true);
  }
  else
  {
    btnShowAutoloaderCP->setEnabled(false);
  }

  // On first load, populate movie change sets, current job and current library
  if (firstLoad)
  {
    // Week #, UPC, Title, Producer, Category, Subcategory, Front Cover, Back Cover, Copied, Year
    QList<DvdCopyTask> unfinishedTasks = dbMgr->getDvdCopyTasks(DvdCopyTask::Finished, true);

    // Add each DVD copy task that is not finished to the table view
    foreach (DvdCopyTask dvdCopyTask, unfinishedTasks)
    {
      insertDvdCopyJobRow(dvdCopyTask);
    }

    // Enable the retry and delete buttons if unfinished jobs exist
    if (unfinishedTasks.count() > 0)
    {
      btnRetryJob->setEnabled(true);
      btnDeleteJob->setEnabled(true);
    }

    /*
    QList<DvdCopyTask> finishedTasks = dbMgr->getDvdCopyTasks(DvdCopyTask::Finished);

    // Add each DVD copy task that is finished to the table view
    foreach (DvdCopyTask dvdCopyTask, finishedTasks)
    {
      insertMovieChangeSetRow(dvdCopyTask);
    }*/

    firstLoad = false;

    populateVideoLibrary();    

    upcLookupTimer->start();
    deleteUpcTimer->start();

    checkUPCs();
    checkDeleteUPCs();
  }

  refreshMovieChangeSetsView();
}

void MovieLibraryWidget::updateEncodeStatus(QString upc, DvdCopyTask::Status status, quint64 fileSize)
{
  // Updates the encode column in the datagrid based on the dvd_copy_jobs table
  int row = findDvdCopyJobRow(upc);

  if (row > -1)
  {
    QLOG_DEBUG() << QString("Updating transcode status for UPC: %1 in the current copy job view").arg(upc);

    QStandardItem *objectField = movieCopyJobModel->item(row, movieCopyJobModel->columnCount() - 1);
    QStandardItem *transcodedField = movieCopyJobModel->item(row, 7);
    DvdCopyTask task = objectField->data().value<DvdCopyTask>();

    task.setTranscodeStatus(status);
    transcodedField->setText(task.TranscodeStatusToString());
    transcodedField->setData(status);

    if (fileSize > 0)
    {
      task.setFileLength(fileSize);
    }

    QVariant v;
    v.setValue(task);
    objectField->setData(v);
  }
  else
  {
    row = findMovieChangeSetRow(upc);

    if (row > -1)
    {
      QLOG_DEBUG() << QString("Updating transcode status for UPC: %1 in the movie change sets view").arg(upc);

      QStandardItem *objectField = movieChangeSetsModel->item(row, movieChangeSetsModel->columnCount() - 1);
      QStandardItem *transcodedField = movieChangeSetsModel->item(row, 7);
      DvdCopyTask task = objectField->data().value<DvdCopyTask>();

      task.setTranscodeStatus(status);
      transcodedField->setText(task.TranscodeStatusToString());
      transcodedField->setData(status);

      if (fileSize > 0)
      {
        QStandardItem *fileSizeField = movieChangeSetsModel->item(row, 6);
        fileSizeField->setText(QString("%1 GB").arg(qreal(fileSize) / (1024 * 1024 * 1024), 0, 'f', 2));
        fileSizeField->setData(int(fileSize));

        task.setFileLength(fileSize);
      }

      QVariant v;
      v.setValue(task);
      objectField->setData(v);
    }
  }
}

void MovieLibraryWidget::populateVideoLibrary()
{
  clearMovieLibrary();

  // Load current video library
  foreach (Movie m, dbMgr->getVideos())
    insertVideoRow(m);

  lblTotalChannels->setText(tr("(Total Videos: %1)").arg(libraryModel->rowCount()));
}

void MovieLibraryWidget::addNewJob()
{
  QLOG_DEBUG() << QString("User clicked the New DVD Copy Job button");

  // Only allow adding new job if there is nothing currently defined
  if (movieCopyJobModel->rowCount() > 0)
  {
    QLOG_DEBUG() << QString("User tried to create new DVD copy job but there is already a job in the queue");
    QMessageBox::warning(this, tr("New DVD Copy Job"), tr("A new job cannot be created until the current one is finished. If you do not want the current job then use the Delete Job button."));
  }
  else
  {
    bool continueDvdCopyJob = true;

    // TODO: Before allowing UPCs to be scanned into widget, make sure there will be enough disk space on the server
    // to copy and transcode the DVDs. To estimate the required amount we'll assume all movies are an average size of 5 GB
    // and transcoded videos are 2.5 GB and the maximum number of UPCs are going to be entered (usually 12) so if the free
    // disk space is less than this amount then stop user now

    // If the latest movie change set has a year that differs from current date
    // then ignore year and start from week 1 of the current year
    MovieChangeInfo targetMovieChangeSet = dbMgr->getLatestMovieSet();
    targetMovieChangeSet.setNumVideos(0);
    if (targetMovieChangeSet.Year() != QDate::currentDate().year())
    {
      targetMovieChangeSet.setYear(QDate::currentDate().year());
      targetMovieChangeSet.setWeekNum(1);
    }
    else
    {
      // Same year so just increment week #
      targetMovieChangeSet.setWeekNum(targetMovieChangeSet.WeekNum() + 1);
    }

    // If there are any movie change sets that have not been sent to booths yet then
    // ask user if he/she wants to add to one of the existing sets or create a new set
    QList<MovieChangeInfo> movieChangeSets = dbMgr->getAvailableMovieChangeSets(true);

    // If any of the movie change sets returned are currently being exported then remove from list
    if (selectedImportExportMovieChangeSets.count() > 0 && movieChangeSets.count() > 0)
    {
      foreach (MovieChangeInfo exportMovieChange, selectedImportExportMovieChangeSets)
      {
        for (int i = 0; i < movieChangeSets.count(); ++i)
        {
          if (movieChangeSets.at(i).WeekNum() == exportMovieChange.WeekNum() &&
              movieChangeSets.at(i).Year() == exportMovieChange.Year())
          {
            QLOG_DEBUG() << QString("Removing movie change set (week #: %1, year: %2) from available movie change sets that can be added to because it's being exported").arg(exportMovieChange.WeekNum()).arg(exportMovieChange.Year());

            movieChangeSets.removeAt(i);

            break;
          }
        }
      }
    }

    if (movieChangeSets.count() > 0)
    {
      ExistingMovieChangeWidget selectMovieChangeSet(movieChangeSets, settings->getValue(MAX_MOVIE_CHANGE_SET_SIZE).toInt());
      if (selectMovieChangeSet.exec() == QDialog::Accepted)
      {
        MovieChangeInfo movieChange = selectMovieChangeSet.getMovieChangeSelection();

        if (movieChange.WeekNum() == 0)
        {
          QLOG_DEBUG() << "User chose to create new movie change set";
        }
        else
        {
          targetMovieChangeSet = movieChange;
          QLOG_DEBUG() << "User selected an existing movie change set to add to. Week #" << targetMovieChangeSet.WeekNum() << ", Year:" << targetMovieChangeSet.Year();
        }
      }
      else
      {
        QLOG_DEBUG() << "User canceled selecting a movie change set for the DVD copy job";
        continueDvdCopyJob = false;
      }
    }
    else
    {
      QLOG_DEBUG() << "No existing movie change sets, creating new movie change set. Week #" << targetMovieChangeSet.WeekNum() << ", Year:" << targetMovieChangeSet.Year();
    }

    if (continueDvdCopyJob)
    {
      busy = true;

      AddVideoWidget addVideosWin(targetMovieChangeSet.NumVideos(), settings, dbMgr, this);

      if (addVideosWin.exec() == QDialog::Accepted)
      {
        QStringList queue = addVideosWin.getDvdQueue();

        // Create the directory for the DVD copy job
        QDir videoPath(QString("%1/%2/%3").arg(settings->getValue(VIDEO_PATH).toString()).arg(targetMovieChangeSet.Year()).arg(targetMovieChangeSet.WeekNum()));

        if (!videoPath.mkpath(videoPath.absolutePath()))
        {
          QLOG_ERROR() << QString("Could not create directory for DVD Copy Job: %1").arg(videoPath.absolutePath());
          QMessageBox::warning(this, tr("Error"), tr("Could not create directory for DVD Copy Job! The copy process has been canceled."));
        }
        else
        {
          QLOG_DEBUG() << QString("Created DVD copy job directory: %1").arg(videoPath.absolutePath());

          QStringList errors;
          int numTasks = 0;
          int seqNum = 1;

          // Week #, UPC, Title, Producer, Category, Subcategory, Front Cover, Back Cover, Copied, Year
          // Format: UPC|category
          foreach (QString item, queue)
          {
            QLOG_DEBUG() << item;

            QStringList record = item.split("|");

            DvdCopyTask task;
            task.setSeqNum(seqNum++);
            task.setUPC(record[0]);

            // Only set category if this field was visible in the widget and the data is available
            if (record.count() == 2 && settings->getValue(SHOW_EXTRA_ADD_VIDEO_FIELDS).toBool())
              task.setCategory(record[1]);

            task.setYear(targetMovieChangeSet.Year());
            task.setWeekNum(targetMovieChangeSet.WeekNum());
            task.setTaskStatus(DvdCopyTask::Pending);

            // Build path to DVD directory which is named after the UPC
            QDir videoSubdirPath = QDir(videoPath.absoluteFilePath(task.UPC()));

            // Create the UPC directory
            if (videoPath.mkpath(videoSubdirPath.absolutePath()))
            {
              QLOG_DEBUG() << QString("Created DVD directory: %1").arg(videoSubdirPath.absolutePath());

              dbMgr->insertDvdCopyTask(task);

              insertDvdCopyJobRow(task);

              ++numTasks;
            }
            else
            {
              QLOG_ERROR() << QString("Could not create DVD directory: %1").arg(videoSubdirPath.absolutePath());
              errors.append(task.UPC());
            }
          }

          bool ok = true;

          // If there were any errors when creating the UPC directories then
          // check to see if we actually need to proceed with the job
          if (errors.length() > 0)
          {
            if (numTasks > 0)
              QMessageBox::warning(this, tr("Warning"), tr("Could not create directories for the following UPC(s): %1. Make sure to remove these from your DVD disc autoloader before continuing.").arg(errors.join(", ")));
            else
            {
              ok = false;
              QMessageBox::warning(this, tr("Error"), tr("Could not create any directories for the UPC(s). The copy process has been canceled."));
              QLOG_DEBUG() << QString("No tasks could be added to the DVD copy job");
            }
          }

          if (ok)
          {
            if (settings->getValue(ENABLE_AUTO_LOADER).toBool())
              QMessageBox::information(this, tr("Get ready"), tr("The DVD copy job is ready to begin. Make sure you have loaded the DVD(s) in the disc autloader."));

            startCopyProcess();
            checkUPCs();
            btnCancelJob->setEnabled(true);
            btnNewJob->setEnabled(false);
            btnShowAutoloaderCP->setEnabled(false);
          }
        }

      } // endif submitted dvd queue
      else
      {
        QLOG_DEBUG() << "User canceled submitting UPC list for new DVD copy job";
        busy = false;
      }

    } // endif submitted week num
    else
    {
      QLOG_DEBUG() << "User canceled starting a new DVD copy job";
      busy = false;
    }
  } // endif no DVD copy job in progress
}

void MovieLibraryWidget::startCopyProcess()
{
  // If we are not running the copy process and there are items in the queue then start
  if (movieCopyJobModel->rowCount() > 0)
  {
    QStandardItem *objectField = movieCopyJobModel->item(currentDvdIndex, movieCopyJobModel->columnCount() - 1);
    DvdCopyTask task = objectField->data().value<DvdCopyTask>();

    // When the disc auto loader is not used, we need to prompt the user for each DVD
    if (!settings->getValue(ENABLE_AUTO_LOADER).toBool())
    {
      // Eject drive tray (should receive ACK from dvd-copy-bot)
      // Tell user to insert disc
      // User clicks the OK button
      // Load drive tray (should receive ACK from dvd-copy-bot)
      // Start DVD copy (should receive ACK from dvd-copy-bot)
      // Wait for response from bot that it finished (should receive SUCCESS/FAILURE from dvd-copy-bot)
      // loop
      // handle dvd decrypter hung, need to kill process
      // See if running separate instances of dvd decrypter with different evxe names and log locations for situations where a preview and arcade copying is going on
      // Need to allow client to continue running while we wait for user to respond, otherwise we lose our connection to the dvd-copy-bot
      QLOG_DEBUG() << "Ejecting DVD drive" << settings->getValue(WINE_DVD_DRIVE_LETTER).toString();
      QStringList arguments;
      arguments << settings->getValue(DVD_DRIVE_MOUNT).toString();
      QProcess::execute("eject", arguments);

      //      client.sendMessage(QString("EJECT %1").arg(settings->DvdCopyingDrive()));
      QMessageBox::information(this, tr("Get ready"), tr("Ready to copy movie with UPC: %1. Put the disc in the DVD drive tray that just opened and then click OK.").arg(task.UPC()));
      QLOG_DEBUG() << "Loading DVD drive" << settings->getValue(WINE_DVD_DRIVE_LETTER).toString();

      arguments.clear();
      arguments << "-t" << settings->getValue(DVD_DRIVE_MOUNT).toString();
      QProcess::execute("eject", arguments);

      QString destPath = QString("%1\\%2\\%3")
                         .arg(settings->getValue(WINE_DVD_COPY_DEST_DRIVE_LETTER).toString())
                         .arg(task.Year())
                         .arg(task.WeekNum());

      /*
      if (!settings->TestMode())
      {
        QLOG_DEBUG() << QString("Starting copy process for UPC: %1. Destination path: %2").arg(task.UPC()).arg(destPath);

        client.sendMessage(QString("COPY %1 %2")
                           .arg(settings->DvdCopyingDrive())
                           .arg(destPath));
      }
      else
      {
        QLOG_DEBUG() << QString("Starting test copy process for UPC: %1. Destination path: %2").arg(task.UPC()).arg(destPath);

        client.sendMessage(QString("TEST-COPY %1").arg(destPath));
      }*/

      QLOG_DEBUG() << QString("Starting copy process for UPC: %1. Destination path: %2").arg(task.UPC()).arg(destPath);

      prepareDestination(QString("%1/%2/%3").arg(settings->getValue(VIDEO_PATH).toString()).arg(task.Year()).arg(task.WeekNum()));

      // Start copy process
      dvdCopier->copyDvd(settings->getValue(WINE_DVD_DRIVE_LETTER).toString(),
                         settings->getValue(DVD_DRIVE_MOUNT).toString(),
                         QString("%1:\\\\%2\\\\%3").arg(settings->getValue(WINE_DVD_COPY_DEST_DRIVE_LETTER).toString()).arg(task.Year()).arg(task.WeekNum()),
                         settings->getValue(DVD_COPIER_TIMEOUT).toInt());
    }
    else
    {
      // For every disc to copy:
      //   Eject DVD drive tray
      //   Autoloader: LOAD
      //   Close DVD drive tray
      //   Copy disc
      //   Eject DVD drive tray
      //   Autoloader: PICK
      //   Close DVD drive tray
      //   Autoloader: UNLOAD


      // Initialize autoloader
      // For every disc to copy:
      //   Eject DVD drive tray
      //   Autoloader: LOAD
      //   if no error then
      //     Close DVD drive tray
      //     Copy disc
      //     Eject DVD drive tray
      //     Autoloader: PICK
      //     if no error then
      //       Close DVD drive tray
      //       Autoloader: UNLOAD
      //       if no error then
      //         DVD copy was successful
      //       else
      //         show unload error (DVD drive tray open or operation N/A)
      //       endif
      //     else
      //       show pick error (DVD drive tray NOT open or operation N/A)
      //     endif
      //   else
      //     show load error (autoloader empty or DVD drive tray not open)
      //   endif

      discAutoloader->loadDisc();

    }

    //QDir videoPath(QString("%1/%2/%3").arg(settings->getValue(VIDEO_PATH).toString()).arg(task.Year()).arg(task.WeekNum()));

    //QStringList arguments;
    //arguments << "V" << QString("%1").arg(task.Year()) << QString("%1").arg(task.WeekNum());
    //dvdCopyProc->start(settings->DvdCopyProgFile(), arguments);
  }
  else
    QLOG_DEBUG() << QString("Not starting copy process, either the process is already running or the job queue is empty");
}

void MovieLibraryWidget::retryCopyProcess()
{
  QLOG_DEBUG() << QString("User clicked the Retry Copy Job button");

  btnCancelJob->setEnabled(true);
  btnNewJob->setEnabled(false);
  btnRetryJob->setEnabled(false);
  btnDeleteJob->setEnabled(false);

  currentDvdIndex = 0;

  // Reset status of to Pending
  for (int i = 0; i < movieCopyJobModel->rowCount(); ++i)
  {
    QStandardItem *statusField = movieCopyJobModel->item(i, 6);
    QStandardItem *objectField = movieCopyJobModel->item(i, movieCopyJobModel->columnCount() - 1);

    DvdCopyTask task = objectField->data().value<DvdCopyTask>();
    task.setTaskStatus(DvdCopyTask::Pending);

    statusField->setText(task.TaskStatusToString());
    statusField->setData(DvdCopyTask::Pending);

    QVariant v;
    v.setValue(task);
    objectField->setData(v);

    dbMgr->updateDvdCopyTask(task);
  }

  if (settings->getValue(ENABLE_AUTO_LOADER).toBool())
  {
    // Tell user to load the DVDs in the correct order
    QMessageBox::information(this, tr("Retry DVD Copy Job"), tr("The DVD copy job is ready to begin. Make sure you have loaded the DVD(s) in the disc autoloader in the correct order."));
  }

  // call startCopyProcess
  startCopyProcess();
}

void MovieLibraryWidget::deleteJob()
{
  QLOG_DEBUG() << "User clicked the Delete Job button";

  QModelIndexList selectedRows = movieCopyJobTable->selectionModel()->selectedRows(9);

  if (selectedRows.count() == 0)
  {
    QLOG_DEBUG() << "No rows selected to delete";
    QMessageBox::warning(this, tr("Delete DVD Copy Job"), tr("Select one or more rows in the table above to delete."));
  }
  else
  {
    // Delete one or more rows from the model and database
    //qDebug() << selectedRows.count() << "rows selected";
    while (selectedRows.count() > 0)
    {
      QModelIndex index = selectedRows.at(0);

      DvdCopyTask task = index.data(Qt::UserRole + 1).value<DvdCopyTask>();

      QLOG_DEBUG() << "Deleting job with UPC:" << task.UPC() << ", Year:" << task.Year() << ", Week #:" << task.WeekNum();

      // If dvd copy task is not complete then insert into delete_lookup_queue table. Records in this table will be sent
      // to the web service to be deleted from the lookup queue so they don't sit in our database
      if (!task.Complete())
      {
        dbMgr->insertDeleteUPC(task.UPC());

        // 5 minutes is given in case user deletes more UPCs, that way they can be sent all at once
        deleteUpcTimer->start();
      }

      dbMgr->deleteDvdCopyTask(task);
      //QLOG_DEBUG() << "Row:" << index.row() << ", column:" << index.column() << "selected" << ", data:" << task.WeekNum() << task.Year();
      movieCopyJobModel->removeRow(index.row());

      // get list of files and delete each one, directory must be empty to delete it
      QDir videoPath(QString("%1/%2/%3/%4").arg(settings->getValue(VIDEO_PATH).toString()).arg(task.Year()).arg(task.WeekNum()).arg(task.UPC()));

      if (videoPath.exists())
      {
        QStringList fileList = videoPath.entryList(QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden);

        QLOG_DEBUG() << "Files to delete:" << fileList.join(", ");

        bool deleteError = false;

        // Delete each file found
        foreach (QString filename, fileList)
        {
          if (videoPath.remove(filename))
            QLOG_DEBUG() << "Deleted" << filename;
          else
          {
            QLOG_ERROR() << "Could not delete" << filename;
            deleteError = true;
          }
        }

        if (!deleteError)
        {
          QLOG_DEBUG() << "All files deleted, now deleting directory" << videoPath.absolutePath();
          if (videoPath.rmdir(videoPath.absolutePath()))
            QLOG_DEBUG() << "Deleted directory";
          else
          {
            QLOG_ERROR() << "Could not delete directory";
            QMessageBox::warning(this, tr("Error"), tr("All video files were deleted but directory could not be deleted."));
          }
        }
        else
        {
          QLOG_ERROR() << "Could not delete all files";
          QMessageBox::warning(this, tr("Error"), tr("Could not delete all of the video files."));
        }
      }
      else
      {
        QLOG_DEBUG() << "Could not find" << videoPath.absolutePath() << "to delete";
      }

      selectedRows = movieCopyJobTable->selectionModel()->selectedRows(9);
    }
  }

  // Disable buttons if the model is now empty
  if (movieCopyJobModel->rowCount() == 0)
  {
    btnRetryJob->setEnabled(false);
    btnDeleteJob->setEnabled(false);
  }
}

void MovieLibraryWidget::dvdCopierStarted()
{
  // Update status of current DVD
  QLOG_DEBUG() << QString("Copy process started, updating status of DVD copy task. Row index: %1").arg(currentDvdIndex);

  QStandardItem *statusField = movieCopyJobModel->item(currentDvdIndex, 6);
  QStandardItem *objectField = movieCopyJobModel->item(currentDvdIndex, movieCopyJobModel->columnCount() - 1);

  DvdCopyTask task = objectField->data().value<DvdCopyTask>();
  task.setTaskStatus(DvdCopyTask::Working);

  QVariant v;
  v.setValue(task);
  objectField->setData(v);

  statusField->setText(task.TaskStatusToString());
  statusField->setData(DvdCopyTask::Working);

  dbMgr->updateDvdCopyTask(task);
}

void MovieLibraryWidget::checkUPCs()
{
  btnDownloadMetadata->setEnabled(false);

  // If there are any UPCs in the database that are missing metadata then submit the list of UPCs
  // to the web server.
  // If the web server identifies any of the UPCs then the metadata is received and the video cover files
  // are downloaded.
  // Once everything is downloaded, the appropriate records in the models are updated along with the database
  // and the video cover files are moved to the appropriate directories

  QStringList upcList = getUPCsNeedingMetadata();

  QLOG_DEBUG() << QString("Found %1 UPC(s) that need metadata").arg(upcList.count());
  if (upcList.count() > 0)
  {
    busy = true;
    lookupService->startVideoLookup(upcList);
  }
  else
  {
    if (userRequestedMetadata)
    {
      QMessageBox::information(this, tr("Download Metadata"), tr("Nothing to download, videos already have complete metadata."));
      userRequestedMetadata = false;
    }

    btnDownloadMetadata->setEnabled(true);
  }
}

void MovieLibraryWidget::checkUPCsResult(QList<Movie> videoList)
{
  QLOG_DEBUG() << QString("Received metadata for %1 UPC(s)").arg(videoList.count());

  // If any UPCs returned then identify the UPCs that should be updated in the database
  // and move the video cover files to the appropriate directories
  if (videoList.count() > 0)
  {
    QStringList downloadedMetadata;
    // FIXME: There is a small chance that the user assigned images to the video right as the metadata was
    // downloaded from the server. If this happens, the front and back covers will be empty strings and
    // overwrite the xml file. This situation happened at a store where the user set the movie title and producer
    // but no images, then deleted the movie because the copy failed. It was then added again and copied successfully but
    // during this time, the metadata was downloaded from the server without covers and written to the mediaInfo.xml
    // right after the user had just assigned images, so the xml file was complete for a fraction of a second and then
    // the cover image references in the xml file were cleared
    foreach (Movie video, videoList)
    {
      // Build list of video titles that we downloaded metadata for
      downloadedMetadata.append(video.Title().isEmpty() ? video.UPC() : video.Title());

      int row = findDvdCopyJobRow(video.UPC());

      if (row > -1)
      {
        QStandardItem *objectField = movieCopyJobModel->item(row, movieCopyJobModel->columnCount() - 1);
        DvdCopyTask task = objectField->data().value<DvdCopyTask>();

        QLOG_DEBUG() << QString("Updating metadata in current DVD copy jobs for UPC: %1").arg(task.UPC());

        task.setTitle(video.Title());
        task.setProducer(video.Producer());
        task.setCategory(video.Category());
        task.setSubcategory(video.Subcategory());

        if (video.FrontCover().length() > 0)
          task.setFrontCover(video.FrontCover());

        if (video.BackCover().length() > 0)
          task.setBackCover(video.BackCover());

        QVariant v;
        v.setValue(task);
        objectField->setData(v);

        // Week #, UPC, Title, Producer, Category, Subcategory, Status, Year, DvdCopyTask
        QStandardItem *titleField, *producerField, *categoryField, *subcategoryField;

        titleField = movieCopyJobModel->item(row, 2);
        titleField->setText(task.Title());
        titleField->setData(task.Title());

        producerField = movieCopyJobModel->item(row, 3);
        producerField->setText(task.Producer());
        producerField->setData(task.Producer());

        categoryField = movieCopyJobModel->item(row, 4);
        categoryField->setText(task.Category());
        categoryField->setData(task.Category());

        subcategoryField = movieCopyJobModel->item(row, 5);
        subcategoryField->setText(task.Subcategory());
        subcategoryField->setData(task.Subcategory());

        dbMgr->updateDvdCopyTask(task);

        // Move images to DVD directory

        QDir metadataPath(settings->getValue(VIDEO_METADATA_PATH).toString());
        QDir videoSubdirPath(QString("%1/%2/%3/%4").arg(settings->getValue(VIDEO_PATH).toString()).arg(task.Year()).arg(task.WeekNum()).arg(task.UPC()));

        if (video.FrontCover().length() > 0)
          videoSubdirPath.rename(metadataPath.absoluteFilePath(task.FrontCover()), videoSubdirPath.absoluteFilePath(task.FrontCover()));

        if (video.BackCover().length() > 0)
          videoSubdirPath.rename(metadataPath.absoluteFilePath(task.BackCover()), videoSubdirPath.absoluteFilePath(task.BackCover()));

        // Set the video file size in the XML file
        QFileInfo videoFile(videoSubdirPath.absoluteFilePath(QString("%1.VOB").arg(video.UPC())));
        if (videoFile.exists())
          video.setFileLength(videoFile.size());
        else
        {
          // If .VOB file not found then try .ts file
          videoFile.setFile(videoSubdirPath.absoluteFilePath(QString("%1.ts").arg(video.UPC())));
          if (videoFile.exists())
            video.setFileLength(videoFile.size());
        }

        // Write mediaInfo.xml file to directory
        if (video.writeXml(videoSubdirPath.absoluteFilePath("mediaInfo.xml")))
        {
          QLOG_DEBUG() << QString("Wrote XML file: %1").arg(videoSubdirPath.absoluteFilePath("mediaInfo.xml"));
        }
      }

      row = findMovieChangeSetRow(video.UPC());

      if (row > -1)
      {
        QStandardItem *objectField = movieChangeSetsModel->item(row, movieChangeSetsModel->columnCount() - 1);
        DvdCopyTask task = objectField->data().value<DvdCopyTask>();

        QLOG_DEBUG() << QString("Updating metadata in movie sets for UPC: %1").arg(task.UPC());

        task.setTitle(video.Title());
        task.setProducer(video.Producer());
        task.setCategory(video.Category());
        task.setSubcategory(video.Subcategory());

        if (video.FrontCover().length() > 0)
          task.setFrontCover(video.FrontCover());

        if (video.BackCover().length() > 0)
          task.setBackCover(video.BackCover());

        video.setFileLength(task.FileLength());

        QVariant v;
        v.setValue(task);
        objectField->setData(v);

        // Week #, Complete, UPC, Title, Producer, Category, Subcategory, File Size, Year, DvdCopyTask
        QStandardItem *completeField, *titleField, *producerField, *categoryField, *subcategoryField;

        completeField = movieChangeSetsModel->item(row, 8);
        completeField->setText((task.Complete() ? "True" : "False"));
        completeField->setData(task.Complete());

        titleField = movieChangeSetsModel->item(row, 2);
        titleField->setText(task.Title());
        titleField->setData(task.Title());

        producerField = movieChangeSetsModel->item(row, 3);
        producerField->setText(task.Producer());
        producerField->setData(task.Producer());

        categoryField = movieChangeSetsModel->item(row, 4);
        categoryField->setText(task.Category());
        categoryField->setData(task.Category());

        subcategoryField = movieChangeSetsModel->item(row, 5);
        subcategoryField->setText(task.Subcategory());
        subcategoryField->setData(task.Subcategory());

        dbMgr->updateDvdCopyTask(task);

        // Move images to DVD directory

        QDir metadataPath(settings->getValue(VIDEO_METADATA_PATH).toString());
        QDir videoSubdirPath(QString("%1/%2/%3/%4").arg(settings->getValue(VIDEO_PATH).toString()).arg(task.Year()).arg(task.WeekNum()).arg(task.UPC()));

        if (video.FrontCover().length() > 0)
          videoSubdirPath.rename(metadataPath.absoluteFilePath(task.FrontCover()), videoSubdirPath.absoluteFilePath(task.FrontCover()));

        if (video.BackCover().length() > 0)
          videoSubdirPath.rename(metadataPath.absoluteFilePath(task.BackCover()), videoSubdirPath.absoluteFilePath(task.BackCover()));

        // Set the video file size in the XML file
        QFileInfo videoFile(videoSubdirPath.absoluteFilePath(QString("%1.VOB").arg(video.UPC())));
        if (videoFile.exists())
          video.setFileLength(videoFile.size());
        else
        {
          // If .VOB file not found then try .ts file
          videoFile.setFile(videoSubdirPath.absoluteFilePath(QString("%1.ts").arg(video.UPC())));
          if (videoFile.exists())
            video.setFileLength(videoFile.size());
        }

        // Write mediaInfo.xml file to directory
        if (video.writeXml(videoSubdirPath.absoluteFilePath("mediaInfo.xml")))
        {
          QLOG_DEBUG() << QString("Wrote XML file: %1").arg(videoSubdirPath.absoluteFilePath("mediaInfo.xml"));
        }
      }

    } // end foreach video

    // Show list of videos that got metadata and/if any are remaining
    if (userRequestedMetadata)
    {
      QMessageBox::information(this, tr("Download Metadata"), tr("Metadata was downloaded for the following video(s):\n%1").arg(downloadedMetadata.join("\n")));

      QStringList upcList = getUPCsNeedingMetadata();
      if (upcList.count() > 0)
      {
        QMessageBox::information(this, tr("Download Metadata"), tr("The following video(s) still have incomplete metadata:\n%1").arg(upcList.join("\n")));
      }

      userRequestedMetadata = false;
    }

  } // endif videoList not empty
  else
  {
    // If user requested the metadata then show list of videos that still need metadata
    if (userRequestedMetadata)
    {
      QStringList upcList = getUPCsNeedingMetadata();

      QMessageBox::information(this, tr("Download Metadata"), tr("No new metadata was found on server. The following video(s) still have incomplete metadata:\n%1").arg(upcList.join("\n")));
      userRequestedMetadata = false;
    }
  }

  btnDownloadMetadata->setEnabled(true);

  busy = false;  
}

void MovieLibraryWidget::checkVideoUpdateResult(bool success, const QString &message)
{
  if (success)
  {
    QLOG_DEBUG() << QString("Updated video metadata on server. Response: %1").arg(message);
  }
  else
  {
    QLOG_ERROR() << QString("Could not update video metadata on server. Response: %1").arg(message);
    QMessageBox::warning(this, tr("Server Error"), tr("Could not update video metadata on server. Response: %1").arg(message));
  }

  busy = false;
}

void MovieLibraryWidget::editDvdCopyTask(const QModelIndex &index)
{
  QLOG_DEBUG() << QString("User double-clicked video in DVD copy jobs datagrid to edit");

  if (!mediaInfoWin->isVisible())
  {
    QStandardItem *objectField = movieCopyJobModel->item(index.row(), movieCopyJobModel->columnCount() - 1);
    DvdCopyTask task = objectField->data().value<DvdCopyTask>();

    Movie movie;
    movie.setUPC(task.UPC());
    movie.setTitle(task.Title());
    movie.setProducer(task.Producer());
    movie.setCategory(task.Category());
    movie.setSubcategory(task.Subcategory());

    // Build path to front cover and back cover
    QString frontCover = "";
    QString backCover = "";

    QDir videoSubdirPath(QString("%1/%2/%3/%4").arg(settings->getValue(VIDEO_PATH).toString()).arg(task.Year()).arg(task.WeekNum()).arg(task.UPC()));
    if (!task.FrontCover().isEmpty())
    {
      frontCover = videoSubdirPath.absoluteFilePath(task.FrontCover());
      backCover = videoSubdirPath.absoluteFilePath(task.BackCover());
    }

    mediaInfoWin->populateForm(videoSubdirPath.absolutePath(), movie, frontCover, backCover);
    mediaInfoWin->show();
  }
  else
  {
    //QMessageBox::information(this, tr("Video Metadata"), tr("You are already editing a video's metadata. Finish this before editing another one."));

    mediaInfoWin->show();
    mediaInfoWin->raise();
    mediaInfoWin->activateWindow();
  }
}

void MovieLibraryWidget::editMovieChangeSetItem(const QModelIndex &index)
{
  QLOG_DEBUG() << QString("User double-clicked video in movie change set datagrid to edit");

  if (!mediaInfoWin->isVisible())
  {
    QStandardItem *objectField = movieChangeSetsModel->item(index.row(), movieChangeSetsModel->columnCount() - 1);
    DvdCopyTask task = objectField->data().value<DvdCopyTask>();

    bool allowEdit = true;

    // Make sure selected item is not among the videos being exported
    foreach (MovieChangeInfo m, selectedImportExportMovieChangeSets)
    {
      foreach (DvdCopyTask t, m.DvdCopyTaskList())
      {
        if (task.UPC() == t.UPC())
        {
          allowEdit = false;
          break;
        }
      }

      if (!allowEdit)
        break;
    }

    if (allowEdit)
    {
      Movie movie;
      movie.setUPC(task.UPC());
      movie.setTitle(task.Title());
      movie.setProducer(task.Producer());
      movie.setCategory(task.Category());
      movie.setSubcategory(task.Subcategory());

      // Build path to front cover and back cover
      QString frontCover = "";
      QString backCover = "";

      QDir videoSubdirPath(QString("%1/%2/%3/%4").arg(settings->getValue(VIDEO_PATH).toString()).arg(task.Year()).arg(task.WeekNum()).arg(task.UPC()));
      if (!task.FrontCover().isEmpty())
      {
        frontCover = videoSubdirPath.absoluteFilePath(task.FrontCover());
        backCover = videoSubdirPath.absoluteFilePath(task.BackCover());
      }

      mediaInfoWin->populateForm(videoSubdirPath.absolutePath(), movie, frontCover, backCover);
      mediaInfoWin->show();
    }
    else
    {
      QLOG_DEBUG() << "Do not allow user to edit a video that is being exported";
      QMessageBox::warning(this, tr("Video Metadata"), tr("The metadata for this video cannot be modified because it is among the movie change set(s) that are currently being exported to the external hard drive."));
    }
  }
  else
  {
    //QMessageBox::information(this, tr("Video Metadata"), tr("You are already editing a video's metadata. Finish this before editing another one."));

    mediaInfoWin->show();
    mediaInfoWin->raise();
    mediaInfoWin->activateWindow();
  }
}

void MovieLibraryWidget::updateMetadata()
{
  Movie updatedMovie = mediaInfoWin->getMovie();

  if (mediaInfoWin->movieChanged() || mediaInfoWin->coverFilesChanged())
  {
    QLOG_DEBUG() << QString("Metadata for UPC: %1 was changed by the user").arg(updatedMovie.UPC());

    QStandardItem *weekNumField, *titleField, *producerField, *categoryField, *subcategoryField, *completeField, *yearField, *objectField;

    // Find UPC and update it in the model and database
    int row = findDvdCopyJobRow(updatedMovie.UPC());
    if (row > -1)
    {
      // Update title, producer, category, subcategory, object
      weekNumField = movieCopyJobModel->item(row, 0);
      titleField = movieCopyJobModel->item(row, 2);
      producerField = movieCopyJobModel->item(row, 3);
      categoryField = movieCopyJobModel->item(row, 4);
      subcategoryField = movieCopyJobModel->item(row, 5);
      yearField = movieCopyJobModel->item(row, 8);
      objectField = movieCopyJobModel->item(row, movieCopyJobModel->columnCount() - 1);

      titleField->setText(updatedMovie.Title());
      titleField->setData(updatedMovie.Title());

      producerField->setText(updatedMovie.Producer());
      producerField->setData(updatedMovie.Producer());

      categoryField->setText(updatedMovie.Category());
      categoryField->setData(updatedMovie.Category());

      subcategoryField->setText(updatedMovie.Subcategory());
      subcategoryField->setData(updatedMovie.Subcategory());

      DvdCopyTask task = objectField->data().value<DvdCopyTask>();
      task.setTitle(updatedMovie.Title());
      task.setProducer(updatedMovie.Producer());
      task.setCategory(updatedMovie.Category());
      task.setSubcategory(updatedMovie.Subcategory());
      task.setFrontCover(updatedMovie.FrontCover());
      task.setBackCover(updatedMovie.BackCover());

      QVariant v;
      v.setValue(task);
      objectField->setData(v);

      dbMgr->updateDvdCopyTask(task);

      QDir videoSubdirPath(QString("%1/%2/%3/%4").arg(settings->getValue(VIDEO_PATH).toString()).arg(yearField->data().toInt()).arg(weekNumField->data().toInt()).arg(updatedMovie.UPC()));

      // Set the video file size in the XML file since the file might not have existed before the DVD copy finished
      QFileInfo videoFile(videoSubdirPath.absoluteFilePath(QString("%1.VOB").arg(updatedMovie.UPC())));
      if (videoFile.exists())
        updatedMovie.setFileLength(videoFile.size());
      else
      {
        // If .VOB file not found then try .ts file
        videoFile.setFile(videoSubdirPath.absoluteFilePath(QString("%1.ts").arg(updatedMovie.UPC())));
        if (videoFile.exists())
          updatedMovie.setFileLength(videoFile.size());
      }

      // Write mediaInfo.xml file to directory
      if (updatedMovie.writeXml(videoSubdirPath.absoluteFilePath("mediaInfo.xml")))
      {
        QLOG_DEBUG() << QString("Wrote XML file: %1").arg(videoSubdirPath.absoluteFilePath("mediaInfo.xml"));
      }
    }
    else
    {
      row = findMovieChangeSetRow(updatedMovie.UPC());
      if (row > -1)
      {
        // Update complete, title, producer, category, subcategory, object
        weekNumField = movieChangeSetsModel->item(row, 0);
        titleField = movieChangeSetsModel->item(row, 2);
        producerField = movieChangeSetsModel->item(row, 3);
        categoryField = movieChangeSetsModel->item(row, 4);
        subcategoryField = movieChangeSetsModel->item(row, 5);
        completeField = movieChangeSetsModel->item(row, 8);
        yearField = movieChangeSetsModel->item(row, 9);
        objectField = movieChangeSetsModel->item(row, movieChangeSetsModel->columnCount() - 1);

        titleField->setText(updatedMovie.Title());
        titleField->setData(updatedMovie.Title());

        producerField->setText(updatedMovie.Producer());
        producerField->setData(updatedMovie.Producer());

        categoryField->setText(updatedMovie.Category());
        categoryField->setData(updatedMovie.Category());

        subcategoryField->setText(updatedMovie.Subcategory());
        subcategoryField->setData(updatedMovie.Subcategory());

        DvdCopyTask task = objectField->data().value<DvdCopyTask>();
        task.setTitle(updatedMovie.Title());
        task.setProducer(updatedMovie.Producer());
        task.setCategory(updatedMovie.Category());
        task.setSubcategory(updatedMovie.Subcategory());
        task.setFrontCover(updatedMovie.FrontCover());
        task.setBackCover(updatedMovie.BackCover());

        completeField->setText((task.Complete() ? "True" : "False"));
        completeField->setData(task.Complete());

        QVariant v;
        v.setValue(task);
        objectField->setData(v);

        dbMgr->updateDvdCopyTask(task);

        QDir videoSubdirPath(QString("%1/%2/%3/%4").arg(settings->getValue(VIDEO_PATH).toString()).arg(yearField->data().toInt()).arg(weekNumField->data().toInt()).arg(updatedMovie.UPC()));

        // Set the video file size in the XML file since the file might not have existed before the DVD copy finished
        QFileInfo videoFile(videoSubdirPath.absoluteFilePath(QString("%1.VOB").arg(updatedMovie.UPC())));
        if (videoFile.exists())
          updatedMovie.setFileLength(videoFile.size());
        else
        {
          // If .VOB file not found then try .ts file
          videoFile.setFile(videoSubdirPath.absoluteFilePath(QString("%1.ts").arg(updatedMovie.UPC())));
          if (videoFile.exists())
            updatedMovie.setFileLength(videoFile.size());
        }

        // Write mediaInfo.xml file to directory
        if (updatedMovie.writeXml(videoSubdirPath.absoluteFilePath("mediaInfo.xml")))
        {
          QLOG_DEBUG() << QString("Wrote XML file: %1").arg(videoSubdirPath.absoluteFilePath("mediaInfo.xml"));
        }
      }
      else
      {
        QLOG_ERROR() << QString("Could not find UPC: %1 in either model to update").arg(updatedMovie.UPC());
        return;
      }
    }

    // Upload images and metadata to server if they changed
    if (mediaInfoWin->coverFilesChanged())
    {
      // Only upload to our web server if flag set
      if (settings->getValue(UPLOAD_MOVIE_METADATA).toBool())
      {
        QDir videoSubdirPath(QString("%1/%2/%3/%4").arg(settings->getValue(VIDEO_PATH).toString()).arg(yearField->data().toInt()).arg(weekNumField->data().toInt()).arg(updatedMovie.UPC()));

        QLOG_DEBUG() << QString("Uploading front cover file: %1 and back cover file: %2...")
                        .arg(videoSubdirPath.absoluteFilePath(updatedMovie.FrontCover()))
                        .arg(videoSubdirPath.absoluteFilePath(updatedMovie.BackCover()));

        busy = true;
        lookupService->startVideoUpdate(updatedMovie, videoSubdirPath.absoluteFilePath(updatedMovie.FrontCover()), videoSubdirPath.absoluteFilePath(updatedMovie.BackCover()));
      }
    }
    else
    {
      QLOG_DEBUG() << QString("Front/back cover files did not change, only sending metadata");
      busy = true;
      lookupService->startVideoUpdate(updatedMovie);
    }

  } // endif metadata changed
  else
  {
    QLOG_DEBUG() << QString("User viewed the metadata of the video but did not change it");
  }
}

void MovieLibraryWidget::metadataUpdateCanceled()
{
  QLOG_DEBUG() << QString("User canceled updating the metadata of the video");
}

/*void MovieLibraryWidget::importVideosFromDisk()
{
  // TODO: Seems to not work right, shows all kinds of directories...Also hide this since the user doesn't need to be using

  if (QMessageBox::question(this, tr("Import Videos"), tr("This is an advanced feature that allows selecting videos that were copied/transcoded from DVDs outside of the CAS Manager software. Once imported, the videos can be used in a movie change. Do you want to continue?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
  {
    // Look in videos directory, current year for week directories that are not in database
    // if any directories found then
    //   look at the mediaInfo.xml files within each and make sure all files exist
    //   verify the UPCs don't exist in database
    //   if everything is okay then import into database
    //   else tell user what the problems are so they can be corrected

    QDir videoPath(QString("%1/%2").arg(settings->getValue(VIDEO_PATH).toString()).arg(QDate::currentDate().year()));

    if (videoPath.exists())
    {
      int numVideosImported = 0;
      QStringList importedWeekList;

      // Get list of directories in the current year directory
      QStringList filter;
      filter << "*";
      QFileInfoList weekDirList = videoPath.entryInfoList(filter, QDir::Dirs);

      foreach (QFileInfo d, weekDirList)
      {
        QList<Movie> completeMovieList;
        QStringList incompleteUpcList;

        // TODO: No longer need to replace week and Dl
        // If year and week num exist in dvd_copy_jobs or dvd_copy_jobs_history then skip
        QString weekNum = d.baseName().replace("week", "").replace("Dl", "");
        if (dbMgr->movieSetExists(QDate::currentDate().year(), weekNum.toInt()))
          QLOG_DEBUG() << QString("Ignoring: %1 since it is already a movie set in the database").arg(d.absoluteFilePath());
        else
        {
          QLOG_DEBUG() << QString("Looking in: %1 for videos to import").arg(d.absoluteFilePath());

          QDir weekDir(d.absoluteFilePath());

          // Get list of all directories (should be UPCs) in this week directory
          QFileInfoList upcDirList = weekDir.entryInfoList(QDir::AllDirs | QDir::NoDotAndDotDot);

          foreach (QFileInfo upcDir, upcDirList)
          {
            if (videoFilesComplete(upcDir.absoluteFilePath()))
            {
              QDir ud(upcDir.absoluteFilePath());
              completeMovieList.append(Movie(ud.absoluteFilePath("mediaInfo.xml"), 0));
            }
            else
            {
              incompleteUpcList.append(upcDir.baseName());
            }
          }

          if (incompleteUpcList.count() == 0 && completeMovieList.count() > 0)
          {
            QStringList duplicateUpcList;

            // Verify UPCs don't exist in database
            foreach (Movie m, completeMovieList)
            {
              if (dbMgr->upcExists(m.UPC()))
              {
                duplicateUpcList.append(m.UPC());
              }
            }

            if (duplicateUpcList.count() > 0)
            {
              // Found UPCs that already exist in our database
              QLOG_DEBUG() << QString("Duplicate UPCs: %1 found in: %2").arg(duplicateUpcList.join(", ")).arg(d.baseName());
              QMessageBox::warning(this, tr("Import Videos"), tr("Found duplicate UPCs: %1 in %2, ignoring this directory.").arg(duplicateUpcList.join(", ")).arg(d.baseName()));
            }
            else
            {
              // UPCs are fine, import into database
              foreach (Movie m, completeMovieList)
              {
                DvdCopyTask importTask;
                importTask.setYear(QDate::currentDate().year());
                importTask.setWeekNum(weekNum.toInt());
                importTask.setUPC(m.UPC());
                importTask.setTitle(m.Title());
                importTask.setProducer(m.Producer());
                importTask.setCategory(m.Category());
                importTask.setSubcategory(m.Subcategory());
                importTask.setFrontCover(m.FrontCover());
                importTask.setBackCover(m.BackCover());
                importTask.setFileLength(m.FileLength());
                importTask.setTaskStatus(DvdCopyTask::Finished);

                // Assume the video has already been transcoded
                importTask.setTranscodeStatus(DvdCopyTask::Finished);

                // Insert complete information on video into database
                // Should really use insertDvdCopyTask() method but I don't have the time to modify and test it
                if (dbMgr->importDvdCopyTask(importTask))
                {
                  // Update table view
                  insertMovieChangeSetRow(importTask);

                  ++numVideosImported;
                }
                else
                  QLOG_ERROR() << QString("Could not add: %1 to database").arg(m.UPC());
              }

              importedWeekList.append(d.baseName());
            }
          }
          else
          {
            // Not all video files were found
            QLOG_DEBUG() << QString("Incomplete videos found: %1 cannot import from: %2").arg(incompleteUpcList.join(", ")).arg(d.absoluteFilePath());
            QMessageBox::warning(this, tr("Import Videos"), tr("Found videos in: %1 but the following were incomplete: %2. No videos were imported.").arg(d.absoluteFilePath()).arg(incompleteUpcList.join(", ")));
          }

        } // endif year and weekNum is already a movie set

      } // foreach week dir in video path

      if (importedWeekList.count() > 0)
      {
        QLOG_DEBUG() << QString("Imported %1 videos from: %2").arg(numVideosImported).arg(importedWeekList.join(", "));
        QMessageBox::information(this, tr("Import Videos"), tr("Imported %1 videos from: %2.").arg(numVideosImported).arg(importedWeekList.join(", ")));
      }
      else
      {
        QLOG_DEBUG() << "No videos were imported";
        QMessageBox::information(this, tr("Import Videos"), tr("Could not find any new videos to import."));
      }

    } // endif video path exists
    else
    {
      QLOG_DEBUG() << QString("The video directory: %1 does not exist").arg(videoPath.absolutePath());
      QMessageBox::warning(this, tr("Import Videos"), tr("The video directory: %1 does not exist.").arg(videoPath.absolutePath()));
    }

  } // endif user chose to import videos
}*/

void MovieLibraryWidget::deleteMovieChangeEntry()
{
  QModelIndexList selectedRows = movieChangeSetsTable->selectionModel()->selectedRows(10);

  if (selectedRows.count() == 0)
  {
    QLOG_DEBUG() << "User pressed Delete key without row selected in movie change sets table";
    QMessageBox::warning(this, tr("Delete Video"), tr("Select one or more rows in the table to delete."));
  }
  else
  {
    QLOG_DEBUG() << "User selected" << selectedRows.count() << "rows to delete from the movie change set datagrid";

    bool allowDelete = true;

    // Make sure none of the videos the user wants to delete are currently being exported
    for (int i = 0; i < selectedRows.count() && allowDelete; ++i)
    {
      QModelIndex index = selectedRows.at(i);
      DvdCopyTask task = index.data(Qt::UserRole + 1).value<DvdCopyTask>();

      // Make sure selected item is not among the videos being exported
      foreach (MovieChangeInfo m, selectedImportExportMovieChangeSets)
      {
        foreach (DvdCopyTask t, m.DvdCopyTaskList())
        {
          if (task.UPC() == t.UPC())
          {
            allowDelete = false;
            break;
          }
        }

        if (!allowDelete)
          break;
      }
    }

    if (allowDelete)
    {
      // ask user if he wants to delete video
      // delete entire directory and entry in database if it's not currently in an active moiv change
      if (QMessageBox::question(this, tr("Delete Video"), tr("When a video is deleted, it will be removed from the system and no longer available. This is useful if you copied the wrong DVD for instance. Are you sure you want to delete the video(s)?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
      {
        QLOG_DEBUG() << "User chose to delete video(s)";

        // Delete one or more rows from the model and database
        //qDebug() << selectedRows.count() << "rows selected";
        while (selectedRows.count() > 0)
        {
          QModelIndex index = selectedRows.at(0);

          DvdCopyTask task = index.data(Qt::UserRole + 1).value<DvdCopyTask>();

          // If dvd copy task is not complete then insert into delete_lookup_queue table. Records in this table will be sent
          // to the web service to be deleted from the lookup queue so they don't sit in our database
          if (!task.Complete())
          {
            dbMgr->insertDeleteUPC(task.UPC());

            // 5 minutes is given in case user deletes more UPCs, that way they can be sent all at once
            deleteUpcTimer->start();
          }

          dbMgr->deleteDvdCopyTask(task);

          //QLOG_DEBUG()  << "Row:" << index.row() << ", column:" << index.column() << "selected" << ", data:" << task.UPC() << task.WeekNum() << task.Year();

          // If transcoding in progress for video being deleted, then need to cancel transcode job first!
          if (task.TranscodeStatus() == DvdCopyTask::Working)
          {
            QLOG_DEBUG()  << QString("Aborting video transcoding for UPC: %1 since user deleted it").arg(task.UPC());
            emit abortTranscoding();
          }

          // get list of files and delete each one, directory must be empty to delete it
          QDir videoPath(QString("%1/%2/%3/%4").arg(settings->getValue(VIDEO_PATH).toString()).arg(task.Year()).arg(task.WeekNum()).arg(task.UPC()));

          if (videoPath.exists())
          {
            QStringList fileList = videoPath.entryList(QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden);

            QLOG_DEBUG() << "Files to delete:" << fileList.join(", ");

            bool deleteError = false;

            // Delete each file found
            foreach (QString filename, fileList)
            {
              if (videoPath.remove(filename))
                QLOG_DEBUG() << "Deleted" << filename;
              else
              {
                QLOG_ERROR() << "Could not delete" << filename;
                deleteError = true;
              }
            }

            if (!deleteError)
            {
              QLOG_DEBUG() << "All files deleted, now deleting directory" << videoPath.absolutePath();
              if (videoPath.rmdir(videoPath.absolutePath()))
                QLOG_DEBUG() << "Deleted directory";
              else
              {
                QLOG_ERROR() << "Could not delete directory";
                QMessageBox::warning(this, tr("Error"), tr("All video files were deleted but directory could not be deleted."));
              }
            }
            else
            {
              QLOG_ERROR() << "Could not delete all files";
              QMessageBox::warning(this, tr("Error"), tr("Could not delete all of the video files."));
            }
          }
          else
          {
            QLOG_ERROR() << "Could not find" << videoPath.absolutePath() << "to delete";
            QMessageBox::warning(this, tr("Error"), tr("Video was deleted from database but could not find the files to delete."));
          }

          movieChangeSetsModel->removeRow(index.row());
          selectedRows = movieChangeSetsTable->selectionModel()->selectedRows(10);

        } // end while another row selected
      }
      else
        QLOG_DEBUG() << "User chose not to delete video";
    }
    else
    {
      QLOG_DEBUG() << "Do not allow user to delete a video that is being exported";
      QMessageBox::warning(this, tr("Video Metadata"), tr("Cannot perform delete because one or more of the videos you selected to delete are among the movie change set(s) that are currently being exported to the external hard drive."));
    }
  }
}

void MovieLibraryWidget::showAutoloaderCP()
{
  QLOG_DEBUG() << "User clicked the Autoloader Control Panel button";

  // Show widget with buttons: Load Disc, Unload Disc and Reject Disc
  // Show instructions on using it
  // When a button is clicked all buttons get disabled until action completes
  // Show message with result of action
  // Re-enable the buttons

  // FIXME: Make this VERY CLEAR that this is only for troubleshooting, not copying dvds!!!
  DiscAutoloaderControlPanel cp(settings);
  cp.exec();
}
/*
void MovieLibraryWidget::restartVMclicked()
{
  QLOG_DEBUG() << "User clicked Restart DVD Copier";

  if (QMessageBox::question(this, tr("Restart?"), tr("If you are having problems copying DVDs, such as the drive not ejecting, then restarting the DVD copier can help. WARNING!!! If a customer is using a preview booth, you should not restart the DVD copier. Are you sure you want to continue?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
  {
    QLOG_DEBUG() << "User chose to restart the VM";
    client.sendMessage("REBOOT");
    QMessageBox::information(this, tr("Restart"), tr("The DVD Copier is being restarted. Please wait..."));
  }
  else
  {
    QLOG_DEBUG() << "User chose not to restart the VM";
  }
}*/

void MovieLibraryWidget::dvdCopierFinished(bool success, QString errorMessage)
{
  bool fatalError = false;

  // Clear the success flag if the job was canceled, there's a chance the user canceled the job right
  // when the disc finished loading but before the dvd copy process started
  if (userCanceledJob && success)
  {
    QLOG_DEBUG() << "Changing success flag to false in dvdCopierFinished because the user canceled the copy job";
    success = false;
  }

  // Update status of current DVD
  QLOG_DEBUG() << QString("Copy process %1. Updating row index: %2").arg(success ? "successful" : "failed").arg(currentDvdIndex);

  QStandardItem *statusField = movieCopyJobModel->item(currentDvdIndex, 6);
  QStandardItem *objectField = movieCopyJobModel->item(currentDvdIndex, movieCopyJobModel->columnCount() - 1);

  DvdCopyTask task = objectField->data().value<DvdCopyTask>();

  if (success)
  {
    // Couldn't move video file so added a wait to see if it was due to the file still being open
    sleep(1);

    task.setTaskStatus(DvdCopyTask::Finished);
    statusField->setText(task.TaskStatusToString());
    statusField->setData(int(task.TaskStatus()));

    QDir videoPath(QString("%1/%2/%3").arg(settings->getValue(VIDEO_PATH).toString()).arg(task.Year()).arg(task.WeekNum()));

    // Build path to DVD directory which is named after the UPC
    QDir videoSubdirPath = QDir(videoPath.absoluteFilePath(task.UPC()));

    // Create the UPC directory
    if (videoPath.mkpath(videoSubdirPath.absolutePath()))
    {
      QLOG_DEBUG() << QString("Created directory: %1").arg(videoSubdirPath.absolutePath());

      // /media/casserver/videos
      //  2014
      //    19
      //      upc
      //      upc
      //      ...
      //    20
      //      upc
      //      upc
      //      ...

      // Get list of files in video directory
      // Find the VOB file, delete the other files
      // FIXME: Only allow one VOB file, causes problem if more than one. Figure out why another VOB file was sitting there, might be because DVD Decrypter failed but left a VOB file
      QFileInfoList fileList = videoPath.entryInfoList(QDir::Files);
      foreach (QFileInfo f, fileList)
      {
        // suffix is everything after the last period in a filename
        if (f.suffix().toUpper() == "VOB")
        {
          task.setFileLength(f.size());

          // Move video file to appropriate directory and rename file to <upc>.VOB
          if (videoPath.rename(f.absoluteFilePath(), videoSubdirPath.absoluteFilePath(QString("%1.VOB").arg(task.UPC()))))
          {
            QLOG_DEBUG() << QString("Moved %1 to %2. File size: %3").arg(f.absoluteFilePath()).arg(videoSubdirPath.absoluteFilePath(QString("%1.VOB").arg(task.UPC()))).arg(task.FileLength());

            // if the mediaInfo.xml file exists then update it with the video file size
            if (QFile::exists(videoSubdirPath.absoluteFilePath("mediaInfo.xml")))
            {
              Movie movie(videoSubdirPath.absoluteFilePath("mediaInfo.xml"), 0);

              QFileInfo videoFile(videoSubdirPath.absoluteFilePath(QString("%1.VOB").arg(task.UPC())));
              movie.setFileLength(videoFile.size());

              // Write mediaInfo.xml file to directory
              if (movie.writeXml(videoSubdirPath.absoluteFilePath("mediaInfo.xml")))
                QLOG_DEBUG() << QString("Wrote XML file: %1").arg(videoSubdirPath.absoluteFilePath("mediaInfo.xml"));
              else
              {
                QLOG_ERROR() << QString("Could not update file size in mediaInfo.xml file: %1").arg(videoSubdirPath.absoluteFilePath("mediaInfo.xml"));
                QMessageBox::warning(this, tr("Error"), tr("Could not update file size in mediaInfo.xml for UPC: %1.").arg(task.UPC()));
              }
            }
          }
          else
          {
            QLOG_ERROR() << QString("Could not move %1 to %2. File size: %3").arg(f.absoluteFilePath()).arg(videoSubdirPath.absoluteFilePath(QString("%1.VOB").arg(task.UPC()))).arg(task.FileLength());
            QMessageBox::warning(this, tr("Error"), tr("Could not move video file after copying DVD with UPC: %1. DVD copy process cannot continue!").arg(task.UPC()));
            fatalError = true;
          }
        }
        else
        {
          QLOG_DEBUG() << QString("Deleting file: %1 after DVD copy process").arg(f.absoluteFilePath());
          videoPath.remove(f.absoluteFilePath());
        }
      }

    } // endif created UPC directory
    else
    {
      QLOG_ERROR() << QString("Could not create directory: %1").arg(videoSubdirPath.absolutePath());
      QMessageBox::warning(this, tr("Error"), tr("Could not create directory for video with UPC: %1. DVD copy process cannot continue!").arg(task.UPC()));
      fatalError = true;
    }

  } // endif success
  else
  {
    QLOG_ERROR() << QString("Could not copy DVD with UPC: %1").arg(task.UPC());

    task.setTaskStatus(DvdCopyTask::Failed);
    statusField->setText(task.TaskStatusToString());
    statusField->setData(int(task.TaskStatus()));

    if (!settings->getValue(ENABLE_AUTO_LOADER).toBool())
    {
      // Give the user the option to retry copying the DVD
      // If user chooses to retry then decrement currentDvdIndex so we remain at the same
      // row in the table after flowing through the next code section
      if (QMessageBox::question(this, tr("Error"), tr("Could not copy DVD. Would you like to try again?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
      {
        QLOG_DEBUG() << "User chose to retry copying DVD";
        currentDvdIndex--;
      }
      else
      {
        QLOG_DEBUG() << "User chose not to retry copying DVD";
      }
    }
  }

  QVariant v;
  v.setValue(task);
  objectField->setData(v);

  // Update task in database
  dbMgr->updateDvdCopyTask(task);

  if (settings->getValue(ENABLE_AUTO_LOADER).toBool())
  {
    // If not successful copying DVD then send disc down the reject chute
    bool rejectDisc = !success;

    discAutoloader->unloadDisc(rejectDisc);
  }
  else
  {
    currentDvdIndex++;

    if (!fatalError)
    {
      if (currentDvdIndex < movieCopyJobModel->rowCount())
        startCopyProcess();
      else
      {
        QLOG_DEBUG() << "Ejecting DVD drive" << settings->getValue(WINE_DVD_DRIVE_LETTER).toString();

        QStringList arguments;
        arguments << settings->getValue(DVD_DRIVE_MOUNT).toString();
        QProcess::execute("eject", arguments);

        finishCopyJob();

      } // endif DVD copy job queue empty

    } // endif not fatal error
  }

  busy = false;
}

void MovieLibraryWidget::prepareDestination(QString destPath)
{
  // Delete any files found in the dest path so we don't run into problems later
  // For instance if a .VOB file from a previous failed copy attempt existed then
  // it would get mixed up with the current DVD being copied when it's time to move
  // the .VOB file to the UPC directory
  QDir destDir(destPath);

  QFileInfoList fileList = destDir.entryInfoList(QDir::Files);

  if (fileList.count() > 0)
  {
    QLOG_DEBUG() << QString("Found %1 file(s) in destination path: %2 that must be deleted before copying DVD").arg(fileList.count()).arg(destPath);

    foreach (QFileInfo f, fileList)
    {
      QLOG_DEBUG() << QString("Deleting file: %1 before copying DVD").arg(f.absoluteFilePath());

      if (!destDir.remove(f.absoluteFilePath()))
        QLOG_ERROR() << QString("Could not delete file: %1").arg(f.absoluteFilePath());
    }
  }
  else
  {
    QLOG_DEBUG() << QString("Destination path: %1 is empty so no files have to be deleted before copying DVD").arg(destPath);
  }
}

bool MovieLibraryWidget::freeUpDiskSpace(qreal requiredDiskSpace, qreal maxCapacity, qreal freeDiskSpace)
{
  bool success = true;

  QLOG_DEBUG() << QString("There is not enough free disk space on the hard drive");

  // If the required disk space is greater than the max capacity
  if (requiredDiskSpace > maxCapacity)
  {
    QLOG_DEBUG() << QString("The required disk space exceeds the hard drive's maximum capacity");

    success = false;

    QMessageBox::warning(this, tr("Import/Export Error"), tr("The selected movie change set(s) to copy exceeds the hard drive's maximum capacity. Contact tech support."));
  }
  else
  {
    // Used only if exporting to the external drive
    MovieChangeSetIndex indexFile;

    // List of movie change sets either on server or external drive, sorted by oldest to newest
    QList<ExportedMovieChangeSet> movieChangeSetsOnDrive;

    // List of movie change sets on drive we want to delete
    QList<ExportedMovieChangeSet> deleteMovieChangeSets;

    // Where to check disk space, which could differ from targetPath if dealing with the internal drive
    QString drivePath;

    // Where to delete movie change sets from
    QString targetPath;

    // If drive state is Export then we need to use the .video-index file to determine oldest movie change sets
    // else use the cas-mgr.sqlite database
    if (driveState == Export)
    {
      targetPath = drivePath = externalDriveMountPath;

      // Get list of movie change sets on external hard drive from .video-index
      if (MovieChangeSetIndex::FileExists(QString("%1/.video-index").arg(targetPath)))
      {
        QLOG_DEBUG() << QString("%1 exists").arg(QString("%1/.video-index").arg(targetPath));

        // Load video-index file to determine what's on the drive and what the oldest change sets are
        if (indexFile.loadMovieChangeSetList(QString("%1/.video-index").arg(targetPath)))
        {
          QLOG_DEBUG() << QString("%1 loaded").arg(QString("%1/.video-index").arg(targetPath));

          // The act of getting the list of ExportedMovieChangeSet objects will sort them by export date, oldest to newest
          movieChangeSetsOnDrive = indexFile.getMovieChangeSetList();
        }
        else
        {
          // Could not load index file
          QLOG_ERROR() << QString("%1 could not be loaded").arg(QString("%1/.video-index").arg(targetPath));

          success = false;

          QMessageBox::warning(this, tr("Export Error"), tr("Problem encountered while indexing movie change sets are on the external hard drive. Contact tech support."));
        }
      }
      else
      {
        // Index file does not exist!
        QLOG_ERROR() << QString("%1 does not exist so cannot determine what files can be deleted").arg(QString("%1/.video-index").arg(targetPath));

        success = false;

        QMessageBox::warning(this, tr("Export Error"), tr("Missing expected files on external hard drive. Cannot determine what movie change sets are old enough to delete. Contact tech support."));
      }
    }
    else if (driveState == Import)
    {
      drivePath = settings->getValue(INTERNAL_DRIVE_PATH).toString();
      targetPath = settings->getValue(VIDEO_PATH).toString();

      // Get movie change sets that are in the history table which are ones that have been sent to the booths or exported to an external hard drive
      QList<MovieChangeInfo> exportedMovieChangeSets = dbMgr->getExportedMovieChangeSets();

      // Copy relevant data from MovieChangeInfo object to ExportedMovieChangeSet object
      // so later on we're dealing with the same type list no matter if its Import or Export
      foreach (MovieChangeInfo movieChangeSet, exportedMovieChangeSets)
      {
        // The exported and upcList won't be set because there aren't needed plus there is no export date in the database
        ExportedMovieChangeSet e;
        e.weekNum = movieChangeSet.WeekNum();
        e.year = movieChangeSet.Year();
        e.path = QString("%1/%2").arg(e.year).arg(e.weekNum);

        // Verify the movie change set still exists on drive, it may have already been deleted
        QString movieChangeSetPath = QString("%1/%2").arg(targetPath).arg(e.path);

        if (QDir(movieChangeSetPath).exists())
          movieChangeSetsOnDrive.append(e);
        else
          QLOG_DEBUG() << QString("Ignoring movie change set: %1 because it no longer exists on internal hard drive").arg(movieChangeSetPath);
      }
    }
    else
    {
      // Unknown state!!!
      success = false;

      QMessageBox::warning(this, tr("Import/Export Error"), tr("Unknown state encountered while trying to free up disk space for movies. Contact tech support."));
    }

    if (success)
    {
      // The amount of free space on drive after the identified movie change sets are deleted
      qreal potentialFreeSpace = freeDiskSpace;

      if (movieChangeSetsOnDrive.count() > 0)
      {
        // Starting from the oldest movie change set (by export date, not week/week #) calculate the
        // potential free space from deleting these sets until there will be enough space to satisfy
        // the required disk space. Make sure to exit loop early if we run out of movie change sets
        while (requiredDiskSpace > potentialFreeSpace && movieChangeSetsOnDrive.count() > 0)
        {
          // Get next oldest movie change set on ext hard drive
          ExportedMovieChangeSet e = movieChangeSetsOnDrive.takeFirst();

          QString movieChangeSetPath = QString("%1/%2").arg(targetPath)
                                       .arg(e.path);

          // Determine the amount of space it uses and add this to the potential free space if it were to be deleted
          qreal diskUsage = Global::diskUsage(movieChangeSetPath);
          potentialFreeSpace += diskUsage;

          QLOG_DEBUG() << QString("Disk space being used by %1: %2 GB")
                          .arg(movieChangeSetPath)
                          .arg(diskUsage / (1024 * 1024 * 1024));

          deleteMovieChangeSets.append(e);
        }

        if (requiredDiskSpace > potentialFreeSpace)
        {
          QLOG_DEBUG() << QString("Cannot free up enough disk space for the selected movie change set(s)");

          success = false;

          QMessageBox::warning(this, tr("Import/Export Error"), tr("Cannot fit the selected movie change set(s) on the hard drive. Even if all the existing movie change sets are deleted from the drive first, there would still not be enough space. Contact tech support."));
        }
        else
        {
          QLOG_DEBUG() << QString("There should be %1 GB of free space after deleting movie change sets from drive...").arg(potentialFreeSpace / (1024 * 1024 * 1024));

          foreach (ExportedMovieChangeSet e, deleteMovieChangeSets)
          {
            QString movieChangeSetPath = QString("%1/%2").arg(targetPath)
                                         .arg(e.path);

            // Delete the movie change set from the external hard drive
            if (FileHelper::recursiveRmdir(movieChangeSetPath))
            {
              QLOG_DEBUG() << QString("Deleted movie change set: %1").arg(movieChangeSetPath);
            }
            else
            {
              QLOG_ERROR() << QString("Could not delete movie change set: %1").arg(movieChangeSetPath);

              success = false;

              QMessageBox::warning(this, tr("Import/Export Error"), tr("Could not delete an old movie change set from the hard drive. Contact tech support."));
              break;
            }

            // Update the .video-index only if exporting to the external drive
            if (driveState == Export)
            {
              indexFile.removeMovieChangeSet(e);
            }

          } // end foreach movie change set to delete

          // Only write .video-index file if exporting to external drive
          if (driveState == Export)
          {
            // Save new version of video index file
            if (indexFile.saveMovieChangeSetList(QString("%1/.video-index").arg(targetPath)))
              QLOG_DEBUG() << QString("Saved updated video index file after deleting movie change sets from drive");
            else
            {
              QLOG_ERROR() << QString("Could not save updated video index file");
              success = false;
              QMessageBox::warning(this, tr("Export Error"), tr("There was a problem updating information on the external hard drive. Contact tech support."));
            }
          }

          // Only check the free space again if there were no problems deleting the movie change sets from the drive
          if (success)
          {
            // Verify there is now enough free disk space
            freeDiskSpace = Global::diskFreeSpace(drivePath);

            // If we still don't have enough free space then something must've gone wrong
            if (requiredDiskSpace > freeDiskSpace)
            {
              QLOG_ERROR() << QString("After deleting old movie change set(s) from the hard drive, there is still not enough free disk space on %1 even though there should've been. Free space now: %2 GB")
                              .arg(targetPath)
                              .arg(freeDiskSpace / (1024 * 1024 * 1024));

              success = false;

              QMessageBox::warning(this, tr("Import/Export Error"), tr("After deleting old movie change set(s) from the hard drive, there is still not enough free disk space even though there should've been. Contact tech support."));

            } // endif unexpected free space amount after deleting

          } // endif successfully freed enough disk space

        } // endif potentially able to free enough disk space

      } // endif one or more movie changes sets found
      else
      {
        QLOG_DEBUG() << QString("No movie change sets found so cannot free up any disk space");

        success = false;

        QMessageBox::warning(this, tr("Import/Export Error"), tr("Cannot make room for movie change set(s) on hard drive because it appears there are no old movie change sets to delete."));
      }

    } // endif successfully loaded movie change sets
    else
    {
      QLOG_DEBUG() << QString("Could not load movie change sets so cannot free up any disk space");

      success = false;
    }

  } // endif required disk space <= max capacity

  return success;
}

QStringList MovieLibraryWidget::getUPCsNeedingMetadata()
{
  QStringList upcList;

  for (int i = 0; i < movieCopyJobModel->rowCount(); ++i)
  {
    QStandardItem *objectField = movieCopyJobModel->item(i, movieCopyJobModel->columnCount() - 1);
    DvdCopyTask task = objectField->data().value<DvdCopyTask>();

    if (!task.Complete())
      upcList.append(task.UPC());
  }

  for (int i = 0; i < movieChangeSetsModel->rowCount(); ++i)
  {
    QStandardItem *objectField = movieChangeSetsModel->item(i, movieChangeSetsModel->columnCount() - 1);
    DvdCopyTask task = objectField->data().value<DvdCopyTask>();

    if (!task.Complete())
      upcList.append(task.UPC());
  }

  return upcList;
}

void MovieLibraryWidget::loadDiscFinished(bool success, QString errorMessage)
{
  QStandardItem *objectField = movieCopyJobModel->item(currentDvdIndex, movieCopyJobModel->columnCount() - 1);
  DvdCopyTask task = objectField->data().value<DvdCopyTask>();

  if (!userCanceledJob)
  {
    if (success)
    {
      // txtAutoloaderStatus->append("[" + QDateTime::currentDateTime().toString("MM/dd/yyyy h:mm:ss AP") + "] - Disc with UPC: " + upcField->text() + " - loaded. ");
      prepareDestination(QString("%1/%2/%3").arg(settings->getValue(VIDEO_PATH).toString()).arg(task.Year()).arg(task.WeekNum()));

      // Start copy process
      dvdCopier->copyDvd(settings->getValue(WINE_AUTOLOADER_DVD_DRIVE_LETTER).toString(),
                         settings->getValue(AUTOLOADER_DVD_DRIVE_MOUNT).toString(),
                         QString("%1:\\\\%2\\\\%3").arg(settings->getValue(WINE_DVD_COPY_DEST_DRIVE_LETTER).toString()).arg(task.Year()).arg(task.WeekNum()),
                         settings->getValue(DVD_COPIER_TIMEOUT).toInt());
    }
    else
    {
      //txtAutoloaderStatus->append("[" + QDateTime::currentDateTime().toString("MM/dd/yyyy h:mm:ss AP") + "] - Disc with UPC: " + upcField->text() + " - " + errorMessage);

      QMessageBox::warning(this, "Load Disc Error", errorMessage + " Start the copy job when you are ready to try again.");

      finishCopyJob();
    }
  }
  else
  {
    // If user canceled the job then unload the disc now
    // When the unload operation finishes then the finishCopyJob method is called
    // The unloadDisc method is called after 5 seconds since it seems calling it immediately
    // causes the drive tray to open and then immediately close
    QTimer::singleShot(5000, discAutoloader, SLOT(unloadDisc()));
  }
}

void MovieLibraryWidget::unloadDiscFinished(bool success, QString errorMessage)
{
  if (!userCanceledJob)
  {
    if (success)
    {
      QStandardItem *upcField = movieCopyJobModel->item(currentDvdIndex, 0);

      currentDvdIndex++;

      if (currentDvdIndex < movieCopyJobModel->rowCount())
        discAutoloader->loadDisc();
      else
      {
        finishCopyJob();
      }
    }
    else
    {
      QStandardItem *upcField = movieCopyJobModel->item(currentDvdIndex, 0);
      //txtAutoloaderStatus->append("[" + QDateTime::currentDateTime().toString("MM/dd/yyyy h:mm:ss AP") + "] - Disc with UPC: " + upcField->text() + " - " + errorMessage);

      QMessageBox::warning(this, "Unload Disc Error", errorMessage + " Start the copy job when you are ready to try again.");

      finishCopyJob();
    }
  }
  else
  {
    userCanceledJob = false;

    // If user canceled job show error message if the disc could not be unloaded and then end the copy job
    if (!success)
    {
       QMessageBox::warning(this, "Unload Disc Error", errorMessage);
    }

    finishCopyJob();
  }
}

void MovieLibraryWidget::exportButtonClicked()
{
  /*
   *User clicks the Export Videos button.
   *Get a list of all movie change sets (both ones that have not been exported and those that have)
   *By default show only movie change sets that have not been exported. User has the option to show already exported movie change sets
    User selects the movie change set to export
    Software instructs user to plug in external hard drive and waits for it to be accessible. It times out in 1 minute if no drive detected. Looks at /media/cas-videos and checks df for a drive mount matching that path
    Load .video-index file from drive
    Get amount of free space on drive
    If the selected movie change set does not exist (based on .video-index file) on the external hard drive then
            while there is not enough free space on drive
              delete oldest movie change set
              update .video-index file
            loop
            If there is enough space for the videos on the hard drive then
              Movie change set is copied to the external hard drive
              When finished the hard drive is unmounted and the user is told to unplug the drive.
            else
              Tell user not enough space could be freed on drive to fit the selected movie change. Contact tech support.
    else
            User is told the movies are already on the external hard drive and are returned to the movie change selection screen
  */

  QLOG_DEBUG() << "User clicked Export Videos button";

  if (mediaInfoWin->isVisible())
  {
    QLOG_DEBUG() << "User left the video metadata window open, ask this to be closed before allowing export operation";

    QMessageBox::warning(this, tr("Export Videos"), tr("Close the video metadata window before exporting."));
    mediaInfoWin->raise();
    mediaInfoWin->activateWindow();
    return;
  }

  QList<MovieChangeInfo> movieChangeSets = dbMgr->getAvailableMovieChangeSets();
  QList<MovieChangeInfo> exportedMovieChangeSets = dbMgr->getExportedMovieChangeSets();

  if (movieChangeSets.count() > 0 || exportedMovieChangeSets.count() > 0)
  {
    QLOG_DEBUG() << QString("Found %1 available and %2 already exported movie change sets").arg(movieChangeSets.count()).arg(exportedMovieChangeSets.count());

    ExportMovieChangeWidget exportMovieChange(movieChangeSets, exportedMovieChangeSets);

    if (exportMovieChange.exec() == QDialog::Accepted)
    {
      busy = true;

      // Disable button until operation finished
      btnExport->setEnabled(false);

      selectedImportExportMovieChangeSets = exportMovieChange.getSelectedMovieSets();
      pendingImportExportMovieChangeSets = selectedImportExportMovieChangeSets;

      // Indicate the drive will be used for exporting videos so the method that
      // is called when the drive is mounted knows how to proceed
      driveState = Export;

      QMessageBox::information(this, tr("Plug In External Drive"), tr("Plug the external hard drive into a <span style=\"font-weight:bold; color:blue;\">BLUE</span> USB port and click OK."));
      fsWatcher->startWatching();

      emit waitingForExternalDrive();
    }
  }
  else
  {
    QLOG_DEBUG() << "No movie change sets are available to export";
    QMessageBox::information(this, tr("Export Movie Change Set"), tr("There are no movie change sets available to export. If you were expecting a movie change set then make sure all movies display \"True\" in the Complete column and \"Finished\" in the Transcode Status column."), QMessageBox::Ok);
  }

}

void MovieLibraryWidget::importButtonClicked()
{
  /*
    Software instructs user to plug in external hard drive and waits for it to be accessible. It times out in 1 minute if no drive detected.
    Software lists the movie change sets available on the external drive that have not been imported yet.
    User selects one or more movie change sets to import.
    Videos are validated: all files must be present in each directory, mediaInfo.xml must be valid and the file size must match the .ts file, JPG files must be valid and must not exist already in the database
    Videos are copied to internal hard drive, if the movie change set is greater than the max size then they are split into multiple movie change sets.
    When finished, if there are any other movie change sets available to import, the user is shown the list of movie change sets screen.
    Otherwise, the drive is unmounted and the user is told to unplug the drive
   */

  QLOG_DEBUG() << "User clicked Import Videos button";

  busy = true;

  // Disable button until operation finished
  btnImport->setEnabled(false);

  driveState = Import;

  QMessageBox::information(this, tr("Plug In External Drive"), tr("Plug the external hard drive into a <span style=\"font-weight:bold; color:blue;\">BLUE</span> USB port and click OK."));
  fsWatcher->startWatching();

  emit waitingForExternalDrive();
}

void MovieLibraryWidget::externalDriveMounted(QString path)
{
  // FIXME: Handle situation where drive gets mounted to /media/cas-videos_, maybe use a wildcard???
  // FIXME: Add option to "prepare" a drive that doesn't have a .video-index file which would format as ext4, label as cas-videos and take ownership of the drive

  bool unmountDrive = false;
  QString unmountMsg = "";
  bool continueImportExport = true;

  fsWatcher->stopWatching();
  emit finishedWaitingForExternalDrive();

  QLOG_DEBUG() << QString("External drive detected: %1").arg(path);
  externalDriveMountPath = path;

  if (driveState == Export)
  {
    //QMessageBox::information(this, tr("Found"), "External drive detected, now copying movies to drive...");

    qreal freespace = Global::diskFreeSpace(externalDriveMountPath);
    qreal maxCapacity = Global::maxDiskCapacity(externalDriveMountPath);
    QLOG_DEBUG() << QString("Max Capacity: %1 GB, Free space: %2 GB")
                    .arg(maxCapacity / (1024 * 1024 * 1024))
                    .arg(freespace / (1024 * 1024 * 1024));

    // If there is not enough free space on external drive then identify the oldest movie change sets
    // and delete sets until there is enough space to fit the new movie change set(s)

    // determine amount of space required for the selected movie change sets
    qreal requiredDiskSpace = 0;
    foreach (MovieChangeInfo movieChangeSet, selectedImportExportMovieChangeSets)
    {
      QString movieChangeSetPath = QString("%1/%2/%3").arg(settings->getValue(VIDEO_PATH).toString())
                                   .arg(movieChangeSet.Year())
                                   .arg(movieChangeSet.WeekNum());

      requiredDiskSpace += Global::diskUsage(movieChangeSetPath);
    }

    QLOG_DEBUG() << QString("Disk space required to export selected movie change set(s): %1 GB")
                    .arg(requiredDiskSpace / (1024 * 1024 * 1024));

    // If there is not enough free space on external hard drive
    if (requiredDiskSpace > freespace)
    {
      // Try freeing up disk space
      continueImportExport = freeUpDiskSpace(requiredDiskSpace, maxCapacity, freespace);
    }
    else
    {
      QLOG_DEBUG() << QString("External hard drive has enough space for exporting selected movie change sets");
    }

    // Verify none of the selected movie change sets exist on the external drive
    foreach (MovieChangeInfo movieChangeSet, selectedImportExportMovieChangeSets)
    {
      QString destMovieChangeSetPath = QString("%1/%2/%3").arg(externalDriveMountPath)
                                       .arg(movieChangeSet.Year())
                                       .arg(movieChangeSet.WeekNum());

      if (QDir(destMovieChangeSetPath).exists())
      {
        // This movie change set directory already exists on the external drive
        QLOG_DEBUG() << QString("%1 already exists on external drive, cannot start export").arg(destMovieChangeSetPath);

        continueImportExport = false;

        QStringList videoTitles;
        int numTitles = 0;
        foreach (DvdCopyTask t, movieChangeSet.DvdCopyTaskList())
        {
          videoTitles.append("\"" + t.Title() + "\"");
          if (++numTitles >= 3)
            break;
        }

        QMessageBox::warning(this, tr("Export Error"), tr("One of the movie change sets selected to be exported appears to already be on the external hard drive. Some of the titles from this movie change set include: %1. Export cannot continue, contact tech support.").arg(videoTitles.join(", ")));
      }
    }

    if (continueImportExport)
    {
      // Start copy process which works by looking at the list of selected movie change sets
      // and copies the videos for a movie change set. When finished copying a movie change set
      // this method is called again until the the list is empty
      copyNextMovieChangeSet();
    }
    else
    {
      QLOG_DEBUG() << "Canceling export of selected movie change set(s)";

      unmountDrive = true;
    }
  }
  else if (driveState == Import)
  {
    // Load .video-index file
    MovieChangeSetIndex indexFile;

    if (indexFile.loadMovieChangeSetList(QString("%1/.video-index").arg(externalDriveMountPath)))
    {
      // Compare to current movies on server
      // Build list of movie change sets that are not on server
      // Show list to user and allow 1 or more to be imported

      QList<ExportedMovieChangeSet> availableMovieChangeSets;

      foreach (ExportedMovieChangeSet e, indexFile.getMovieChangeSetList())
      {
        QLOG_DEBUG() << "Checking movie change set against the database to determine if it should be available to import. Export Date:" << e.exported << ", Year:" << e.year << ", Week #:" << e.weekNum << ", Path:" << e.path << ", UPCs:" << e.upcList;
        QList<DvdCopyTask> movies = dbMgr->getDvdCopyTasks(e.year, e.weekNum, true);

        if (movies.count() > 0)
        {
          QLOG_DEBUG() << e.year << e.weekNum << "already exists, skipping movie change set";
        }
        else
        {
          QLOG_DEBUG() << "Movie change set is not in the system, checking each UPC";

          QStringList duplicateUPCs;
          foreach (QString upc, e.upcList)
          {
            if (dbMgr->upcExists(upc))
            {
              duplicateUPCs.append(upc);
            }
          }

          if (duplicateUPCs.count() > 0)
          {
            // The movie change set is not in our database but one or more
            // UPCs in the set already exist. Reject the movie change set
            QLOG_ERROR() << "Cannot show movie change set in list of available sets to import because it contains UPCs that are already in the system:" << duplicateUPCs;
          }
          else
          {
            QLOG_DEBUG() << "No duplicate UPCs found, adding to available movie change sets list";

            // Show movie change set in available sets to import
            availableMovieChangeSets.append(e);
          }

        } // endif movie change set already exists

      } // end foreach movie change set on drive

      if (availableMovieChangeSets.count() > 0)
      {
        int numInvalidMovieChangeSets = 0;

        QLOG_DEBUG() << QString("%1 movie change set(s) are available for importing, now validating metadata of each video").arg(availableMovieChangeSets.count());

        // Using the list of ExportedMovieChangeSet objects that we know are okay to import, convert to a list of
        // MovieChangeInfo objects by loading the mediaInfo.xml for each video to get the metadata
        QList<MovieChangeInfo> importableMovieChangeSets;

        foreach (ExportedMovieChangeSet e, availableMovieChangeSets)
        {
          bool validMovieChangeSet = true;
          MovieChangeInfo movieChangeSet;
          int seqNum = 1;

          foreach (QString upc, e.upcList)
          {
            QString mediaInfoFilePath = QString("/media/cas-videos/%1/%2/mediaInfo.xml").arg(e.path).arg(upc);
            Movie mediaInfo(mediaInfoFilePath, 0);

            if (mediaInfo.isValid())
            {
              DvdCopyTask dvdCopyTask;
              dvdCopyTask.setYear(e.year);
              dvdCopyTask.setWeekNum(e.weekNum);
              dvdCopyTask.setSeqNum(seqNum++);
              dvdCopyTask.setUPC(upc);
              dvdCopyTask.setTitle(mediaInfo.Title());
              dvdCopyTask.setProducer(mediaInfo.Producer());
              dvdCopyTask.setCategory(mediaInfo.Category());
              dvdCopyTask.setSubcategory(mediaInfo.Subcategory());
              dvdCopyTask.setFrontCover(mediaInfo.FrontCover());
              dvdCopyTask.setBackCover(mediaInfo.BackCover());
              dvdCopyTask.setFileLength(mediaInfo.FileLength());
              dvdCopyTask.setTaskStatus(DvdCopyTask::Finished);
              dvdCopyTask.setTranscodeStatus(DvdCopyTask::Finished);

              // Verify the front/back cover and video file exists
              if (!QFile::exists(QString("/media/cas-videos/%1/%2/%3").arg(e.path).arg(upc).arg(dvdCopyTask.FrontCover())) ||
                  !QFile::exists(QString("/media/cas-videos/%1/%2/%3").arg(e.path).arg(upc).arg(dvdCopyTask.BackCover())) ||
                  !QFile::exists(QString("/media/cas-videos/%1/%2/%3.ts").arg(e.path).arg(upc).arg(upc)))
              {
                QLOG_ERROR() << QString("Missing front/back covers and/or the video file in: %1").arg(QString("/media/cas-videos/%1/%2").arg(e.path).arg(upc));
                validMovieChangeSet = false;
              }
              else
              {
                // Verify the file size of the video matches the mediaInfo.xml
                QFileInfo videoFile(QString("/media/cas-videos/%1/%2/%3.ts").arg(e.path).arg(upc).arg(upc));
                if (dvdCopyTask.FileLength() != videoFile.size())
                {
                  QLOG_ERROR() << QString("Video file size does not match XML file: %1 bytes, %2").arg(videoFile.size()).arg(QString("/media/cas-videos/%1/%2/%3.ts").arg(e.path).arg(upc).arg(upc));
                  validMovieChangeSet = false;
                }

                // Verify the front cover file is not empty
                // TODO: Make sure it is actually a JPEG file
                QFileInfo frontCoverFile(QString("/media/cas-videos/%1/%2/%3.jpg").arg(e.path).arg(upc).arg(upc));
                if (frontCoverFile.size() == 0)
                {
                  QLOG_ERROR() << QString("Front cover file size is empty");
                  validMovieChangeSet = false;
                }

                // Verify the back cover file is not empty
                // TODO: Make sure it is actually a JPEG file
                QFileInfo backCoverFile(QString("/media/cas-videos/%1/%2/%3b.jpg").arg(e.path).arg(upc).arg(upc));
                if (backCoverFile.size() == 0)
                {
                  QLOG_ERROR() << QString("Back cover file size is empty");
                  validMovieChangeSet = false;
                }
              }

              QList<DvdCopyTask> dvdCopyTaskList = movieChangeSet.DvdCopyTaskList();
              dvdCopyTaskList.append(dvdCopyTask);
              movieChangeSet.setDvdCopyTaskList(dvdCopyTaskList);
            }
            else
            {
              QLOG_ERROR() << QString("%1 was not found or invalid").arg(mediaInfoFilePath);
              validMovieChangeSet = false;
            }

          } // end foreach UPC in current movie change set

          // Only add movie change set to available list if all mediaInfo.xml files were valid
          if (validMovieChangeSet)
          {
            movieChangeSet.setNumVideos(movieChangeSet.DvdCopyTaskList().count());
            movieChangeSet.setWeekNum(e.weekNum);
            movieChangeSet.setYear(e.year);

            importableMovieChangeSets.append(movieChangeSet);
          }
          else
          {
            ++numInvalidMovieChangeSets;
          }

        } // end foreach available movie change set

        // Tell user errors were found with one or more movie change sets
        if (numInvalidMovieChangeSets > 0)
        {
          if (numInvalidMovieChangeSets == availableMovieChangeSets.count())
          {
            unmountDrive = true;

            QLOG_ERROR() << QString("None of the available movie change sets can be imported due to invalid metadata and/or missing files");
            QMessageBox::warning(this, tr("Import Error"), tr("All of the movie change sets that could be imported have invalid metadata or missing files. No videos can be imported. Contact tech support."));
          }
          else
          {
            QLOG_ERROR() << QString("%1 of the movie change sets that could be imported have invalid metadata and/or missing files").arg(numInvalidMovieChangeSets);
            QMessageBox::warning(this, tr("Import Error"), tr("%1 of the movie change sets that could be imported have invalid metadata or missing files. These will not be included in the list on the next screen. Contact tech support.").arg(numInvalidMovieChangeSets));
          }
        }

        // Only show import movie change sets widget if there are any movie change sets to show after validation
        if (importableMovieChangeSets.count() > 0)
        {
          ExportMovieChangeWidget importMovieChangesWidget(importableMovieChangeSets, QList<MovieChangeInfo>(), true);
          if (importMovieChangesWidget.exec() == QDialog::Accepted)
          {
            // Start copying selected movie change sets to local drive

            // Get the movie change set(s) the user selected and begin copying
            selectedImportExportMovieChangeSets = importMovieChangesWidget.getSelectedMovieSets();
            pendingImportExportMovieChangeSets = selectedImportExportMovieChangeSets;

            qreal freespace = Global::diskFreeSpace("/dev/sda1");
            qreal maxCapacity = Global::maxDiskCapacity("/dev/sda1");
            QLOG_DEBUG() << QString("Max Capacity: %1 GB, Free space: %2 GB")
                            .arg(maxCapacity / (1024 * 1024 * 1024))
                            .arg(freespace / (1024 * 1024 * 1024));

            // determine amount of space required for the selected movie change sets
            qreal requiredDiskSpace = 0;
            foreach (MovieChangeInfo movieChangeSet, selectedImportExportMovieChangeSets)
            {
              QString movieChangeSetPath = QString("%1/%2/%3").arg(externalDriveMountPath)
                                           .arg(movieChangeSet.Year())
                                           .arg(movieChangeSet.WeekNum());

              requiredDiskSpace += Global::diskUsage(movieChangeSetPath);
            }

            QLOG_DEBUG() << QString("Disk space required to import selected movie change set(s): %1 GB")
                            .arg(requiredDiskSpace / (1024 * 1024 * 1024));

            // If there is not enough free space on external hard drive
            if (requiredDiskSpace > freespace)
            {
              // Try freeing up disk space
              continueImportExport = freeUpDiskSpace(requiredDiskSpace, maxCapacity, freespace);
            }
            else
            {
              QLOG_DEBUG() << QString("Internal hard drive has enough space for importing selected movie change sets");
            }

            // Verify none of the selected movie change sets exist on the drive
            foreach (MovieChangeInfo movieChangeSet, selectedImportExportMovieChangeSets)
            {
              QString destMovieChangeSetPath = QString("%1/%2/%3").arg(settings->getValue(VIDEO_PATH).toString())
                                               .arg(movieChangeSet.Year())
                                               .arg(movieChangeSet.WeekNum());

              if (QDir(destMovieChangeSetPath).exists())
              {
                // This movie change set directory already exists on the drive
                QLOG_DEBUG() << QString("%1 already exists on the drive, cannot start import").arg(destMovieChangeSetPath);

                continueImportExport = false;

                QStringList videoTitles;
                int numTitles = 0;
                foreach (DvdCopyTask t, movieChangeSet.DvdCopyTaskList())
                {
                  videoTitles.append("\"" + t.Title() + "\"");
                  if (++numTitles >= 3)
                    break;
                }

                QMessageBox::warning(this, tr("Import Error"), tr("One of the movie change sets selected to be imported appears to already be on the hard drive. Some of the titles from this movie change set include: %1. Import cannot continue, contact tech support.").arg(videoTitles.join(", ")));
              }
            }

            if (continueImportExport)
            {
              copyNextMovieChangeSet();
            }
            else
            {
              QLOG_DEBUG() << "Canceling import of selected movie change set(s)";

              unmountDrive = true;
            }
          }
          else
          {
            QLOG_DEBUG() << "User canceled importing movie change sets";
            unmountDrive = true;
          }
        } // endif importable movie change sets exist
        else
        {
          QLOG_DEBUG() << "Cannot show import movie change sets widget since all have invalid metadata";
        }

      } // endif available movie change sets found
      else
      {
        // No new movie change sets are available to import from external hard drive
        QLOG_DEBUG() << "No new movie change sets found on external hard drive";

        unmountDrive = true;
        unmountMsg = "No new movie change sets found on external hard drive.";
      }
    } // endif successfully open video index file
    else
    {
      unmountDrive = true;

      QLOG_ERROR() << QString("Could not open video index file %1 to determine files that can be imported").arg(QString("%1/.video-index").arg(externalDriveMountPath));
      QMessageBox::warning(this, tr("Unrecognized Drive"), tr("The external hard drive is missing the expected files. Cannot show list of movies that can be imported. Contact tech support."));
    }

  } // endif drive state is Import
  else
  {
    unmountDrive = true;

    QLOG_ERROR() << "Drive was mounted but the state is unknown";
    QMessageBox::warning(this, tr("Unknown State"), tr("An external drive was detected but it was not expected. Contact tech support."));
  }

  if (unmountDrive)
  {
    // Unmount external drive

    if (Global::unmountFilesystem("/media/cas-videos"))
    {
      QLOG_DEBUG() << "Unmounted external hard drive";
      QMessageBox::information(this, tr("Import/Export Videos"), QString("%1%2").arg(!unmountMsg.isEmpty() ? unmountMsg.append(" ") : "").arg("You can now unplug the external drive from the computer."));
    }
    else
    {
      QLOG_ERROR() << "Could not unmount external hard drive";
      QMessageBox::warning(this, tr("Import/Export Videos"), QString("%1%2").arg(!unmountMsg.isEmpty() ? unmountMsg.append(" ") : "").arg("Could not disconnect hard drive! Wait a minute before unplugging the external drive from the computer."));
    }

    busy = false;

    selectedImportExportMovieChangeSets.clear();
    pendingImportExportMovieChangeSets.clear();

    if (btnExport)
      btnExport->setEnabled(true);

    if (btnImport)
      btnImport->setEnabled(true);
  }
}

void MovieLibraryWidget::externalDriveTimeout()
{
  fsWatcher->stopWatching();
  emit finishedWaitingForExternalDrive();

  QLOG_DEBUG() << "Timed out waiting for external hard drive to be plugged in";

  if (QMessageBox::question(this, tr("No External Drive"), tr("Timed out while waiting for external hard drive to be plugged in. Do you want to try detecting the hard drive again?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
  {
    QLOG_DEBUG() << "User chose to try detecting the external hard drive again";
    fsWatcher->startWatching();

    emit waitingForExternalDrive();
  }
  else
  {
    QLOG_DEBUG() << "User chose to cancel detecting the external hard drive";

    // Enable button now since user chose not to try detecting drive again
    if (btnExport)
      btnExport->setEnabled(true);

    if (btnImport)
      btnImport->setEnabled(true);

    busy = false;
  }
}

void MovieLibraryWidget::determinedNumFilesToCopy(int totalFiles)
{
  totalFilesToExport = totalFiles;
  emit initializeCopyProgress(totalFilesToExport);
}

void MovieLibraryWidget::copyProgress(int numFiles)
{
  QLOG_DEBUG() << QString("%1/%2 files copied").arg(numFiles).arg(totalFilesToExport);
  emit updateCopyProgress(numFiles, QString("Copying movie change set %1 of %2...")
                          .arg(selectedImportExportMovieChangeSets.count() - pendingImportExportMovieChangeSets.count())
                          .arg(selectedImportExportMovieChangeSets.count()));
}

void MovieLibraryWidget::finishedCopying()
{
  bool unmountDrive = false;
  QString unmountMsg = "";

  if (fileCopier->isSuccess())
    QLOG_DEBUG() << "All files were successfully copied in movie change set";
  else
    QLOG_ERROR() << "There was a problem copying the movie change set";

  if (driveState == Export)
  {
    QLOG_DEBUG() << QString("Successfully copied movie change set to external drive");

    // Update video index file on hard drive to include newly added movie change set
    ExportedMovieChangeSet exportedMCset;
    exportedMCset.weekNum = currentImportExportMovieChangeSet.WeekNum();
    exportedMCset.year = currentImportExportMovieChangeSet.Year();
    exportedMCset.path = QString("%1/%2").arg(currentImportExportMovieChangeSet.Year()).arg(currentImportExportMovieChangeSet.WeekNum());
    exportedMCset.exported = QDateTime::currentDateTime();

    foreach (DvdCopyTask t, currentImportExportMovieChangeSet.DvdCopyTaskList())
      exportedMCset.upcList.append(t.UPC());

    MovieChangeSetIndex indexFile;
    indexFile.loadMovieChangeSetList(QString("%1/.video-index").arg(externalDriveMountPath));
    indexFile.addMovieChangeSet(exportedMCset);
    indexFile.saveMovieChangeSetList(QString("%1/.video-index").arg(externalDriveMountPath));

    // Move the exported movie change set records to history table as a way to indicate that it was exported
    dbMgr->archiveDvdCopyTasks(currentImportExportMovieChangeSet.Year(), currentImportExportMovieChangeSet.WeekNum());

    // If there are more movie change sets to copy then start copying the next one
    if (!pendingImportExportMovieChangeSets.isEmpty())
      copyNextMovieChangeSet();
    else
    {
      // Update movie change sets view so it excludes the movie change sets that were just exported
      refreshMovieChangeSetsView();

      // Enable button since operation is finished
      btnExport->setEnabled(true);

      // It seems when writing a smaller amount of data to a mounted device, if there isn't a small
      // timeout before calling umount, it will fail since the files haven't been comletely written
      // to the disk. This problem was encountered when copying only 2 videos (2.3 GB) but would
      // never happen when copying 12 videos (11 GB or more)
      sleep(1);

      //QMessageBox::information(this, tr("Finished"), "Finished copying movies to drive.");

      emit cleanupCopyProgress();

      unmountDrive = true;
      unmountMsg = "Finished exporting movies.";
    }
  }
  else if (driveState == Import)
  {
    QLOG_DEBUG() << "Finished copying movie change set to local drive, now importing into database";

    int numErrors = 0;
    // Import movie change set that was copied to the local drive into database
    foreach (DvdCopyTask t, currentImportExportMovieChangeSet.DvdCopyTaskList())
    {
      // Insert complete information on video into database
      // Should really use insertDvdCopyTask() method but I don't have the time to modify and test it
      if (!dbMgr->importDvdCopyTask(t))
      {
        ++numErrors;
      }
    }

    if (numErrors > 0)
    {
      QLOG_ERROR() << QString("Could not import %1 video(s) into the database").arg(numErrors);
      QMessageBox::warning(this, tr("Import Error"), tr("Could not import movie change set into database. Please contact tech support."));
    }

    // If there are more movie change sets to copy then start copying the next one
    if (!pendingImportExportMovieChangeSets.isEmpty())
      copyNextMovieChangeSet();
    else
    {
      // Update movie change sets view so it excludes the movie change sets that were just exported
      refreshMovieChangeSetsView();

      // Enable button since operation is finished
      btnImport->setEnabled(true);

      emit cleanupCopyProgress();

      unmountDrive = true;
      unmountMsg = "Finished importing movies.";
    }
  }
  else
  {
    unmountDrive = true;

    QLOG_ERROR() << QString("Finished copying but we're in an unknown drive state!");
    QMessageBox::warning(this, tr("Error"), tr("Finished copying movies but the current state is unknown. Contact tech support."));
  }

  if (unmountDrive)
  {
    if (Global::unmountFilesystem("/media/cas-videos"))
    {
      QLOG_DEBUG() << "Unmounted external hard drive";
      QMessageBox::information(this, tr("Finished"), QString("%1%2").arg(!unmountMsg.isEmpty() ? unmountMsg.append(" ") : "").arg("You can now unplug the external drive from the computer."));
    }
    else
    {
      QLOG_ERROR() << "Could not unmount external hard drive";
      QMessageBox::warning(this, tr("Finished"), QString("%1%2").arg(!unmountMsg.isEmpty() ? unmountMsg.append(" ") : "").arg("Could not disconnect hard drive! Wait a couple minutes and then unplug the external drive from the computer."));
    }

    busy = false;

    selectedImportExportMovieChangeSets.clear();
    pendingImportExportMovieChangeSets.clear();
  }
}

void MovieLibraryWidget::copyNextMovieChangeSet()
{
  if (!pendingImportExportMovieChangeSets.isEmpty())
  {
    QString srcPath;
    QString destPath;

    currentImportExportMovieChangeSet = pendingImportExportMovieChangeSets.takeFirst();

    if (driveState == Export)
    {
      // Local drive
      srcPath = QString("%1/%2/%3").arg(settings->getValue(VIDEO_PATH).toString())
                .arg(currentImportExportMovieChangeSet.Year())
                .arg(currentImportExportMovieChangeSet.WeekNum());

      // External drive
      destPath = QString("%1/%2/%3").arg(externalDriveMountPath)
                 .arg(currentImportExportMovieChangeSet.Year())
                 .arg(currentImportExportMovieChangeSet.WeekNum());
    }
    else if (driveState == Import)
    {
      // External drive
      srcPath = QString("%1/%2/%3").arg(externalDriveMountPath)
                .arg(currentImportExportMovieChangeSet.Year())
                .arg(currentImportExportMovieChangeSet.WeekNum());

      // Local drive
      destPath = QString("%1/%2/%3").arg(settings->getValue(VIDEO_PATH).toString())
                 .arg(currentImportExportMovieChangeSet.Year())
                 .arg(currentImportExportMovieChangeSet.WeekNum());
    }
    else
    {
      QLOG_ERROR() << "A request was made to copy a movie change set but the current state is unknown";
      QMessageBox::warning(this, tr("Unknown State"), tr("A request was made to copy a movie change set but it was not expected. Contact tech support."));
    }

    // Create the target directory
    QDir destDir;
    destDir.mkpath(destPath);

    // Copy the movie change set to the appropriate location based on driveState
    fileCopier->startCopy(srcPath, destPath);

  }
  else
  {
    QLOG_ERROR() << "A request was made to copy a movie change set but there is nothing in the list";
    QMessageBox::warning(this, tr("Import/Export Error"), "A request was made to copy a movie change set but there is nothing in the list!");
  }
}

void MovieLibraryWidget::refreshMovieChangeSetsView()
{
  // Get list of dvd copy tasks that are finished
  QList<DvdCopyTask> finishedTasks = dbMgr->getDvdCopyTasks(DvdCopyTask::Finished);

 // QLOG_DEBUG() << QString("finishedTasks has %1 items").arg(finishedTasks.count());

  // Get list of movie change sets in movie change queue
  QList<MovieChangeInfo> movieChangeQueue = dbMgr->getMovieChangeQueue(settings->getValue(MAX_CHANNELS).toInt());

  // Clear the movie change sets datagrid
  clearMovieChangeSets();

  // Add each DVD copy task that is finished to the movie change sets datagrid
  // as long as they pass some rules
  foreach (DvdCopyTask dvdCopyTask, finishedTasks)
  {
    bool exclude = false;

    // If movie is part of a movie change set that is in the queue then exclude
    if (movieChangeQueue.count() > 0)
    {
     // QLOG_DEBUG() << QString("Movie change queue has %1 items").arg(movieChangeQueue.count());

      foreach (MovieChangeInfo m, movieChangeQueue)
      {
        if (dvdCopyTask.WeekNum() == m.WeekNum() && dvdCopyTask.Year() == m.Year())
        {
        //  QLOG_DEBUG() << QString("Excluding item year: %1, week #: %2 since its in the queue").arg(m.WeekNum()).arg(m.Year());

          exclude = true;
          break;
        }
      }

    } // endif movie change queue has items

    // If DVD copy job is in progress and the item has not been flagged yet
    // for exclusion
    if (movieCopyJobModel->rowCount() > 0 && !exclude)
    {
      for (int i = 0; i < movieCopyJobModel->rowCount(); ++i)
      {
        QStandardItem *objectField = movieCopyJobModel->item(i, 9);
        DvdCopyTask task = objectField->data().value<DvdCopyTask>();

        // DVD copy job tasks are stored in one table whether they are currently being copied or are finished
        // This works fine until the datagrid has to be updated (refreshMovieChangeSetsView) and then
        // we need to be careful not to add a movie to the datagrid that is already in the Current DVD
        // Copy Job datagrid
        // TODO: Ideally there should be another field in the table to indicate if a movie (dvd copy job task) is
        // currently part of a copy job or not
        if (task.WeekNum() == dvdCopyTask.WeekNum() && task.Year() == dvdCopyTask.Year())
        {
        //  QLOG_DEBUG() << QString("Excluding item year: %1, week #: %2 since its part of a DVD copy job").arg(task.WeekNum()).arg(task.Year());

          exclude = true;
          break;
        }
      }
    }

    if (!exclude)
    {
    //  QLOG_DEBUG() << QString("Adding movie change set item year: %1, week #: %2 since it passed tests").arg(dvdCopyTask.WeekNum()).arg(dvdCopyTask.Year());

      insertMovieChangeSetRow(dvdCopyTask);
    }

  } // end foreach dvd copy task
}

void MovieLibraryWidget::downloadMetadataClicked()
{
  QLOG_DEBUG() << "User clicked Download Metadata button";

  QMessageBox::information(this, tr("Download Metadata"), tr("When metadata is needed for videos, the %1 software automatically checks in with a server every %2 minute(s) and downloads this data when it is available. Clicking the \"Download Metadata\" button is a way of manually checking.")
                           .arg(SOFTWARE_NAME)
                           .arg(settings->getValue(UPC_LOOKUP_INTERVAL).toInt() / 1000 / 60)); // Convert milliseconds to minutes

  userRequestedMetadata = true;

  checkUPCs();
}

void MovieLibraryWidget::cancelJobClicked()
{
  QLOG_DEBUG() << "User clicked Cancel Job button";

  if (QMessageBox::question(this, tr("Cancel DVD Copy Job"), tr("Canceling the DVD copy job will stop copying the DVD(s) in the DVD autoloader. If you cancel the job, please be patient while the current operation (loading, unloading or copying a DVD) is terminated. Are you sure you want to continue?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
  {
    QLOG_DEBUG() << "User confirmed canceling the copy job";

    userCanceledJob = true;
    btnCancelJob->setEnabled(false);

    if (dvdCopier->isCopying())
    {
      QLOG_DEBUG() << "DVD is currently being copied";

      // Terminating the copy process will emit the finishedCopying signal from it which
      // in turn will unload the disc and then end the copy job since the userCanceledJob flag is set
      dvdCopier->terminateCopying();
    }
    else
    {
      QLOG_DEBUG() << "DVD is currently being loaded or unloaded, waiting for operation to finish";
    }
  }
  else
  {
    QLOG_DEBUG() << "User chose not to cancel the copy job";
  }
}

void MovieLibraryWidget::checkDeleteUPCs()
{
  // If there are any UPCs to delete in the database then send to the web service
  QStringList upcList;

  upcList = dbMgr->getDeleteUPCs();

  QLOG_DEBUG() << QString("Found %1 UPC(s) that need to be deleted from lookup queue").arg(upcList.count());
  if (upcList.count() > 0)
  {
    busy = true;
    lookupService->startDeleteVideoLookup(upcList);
  }
  else
  {
    // No need to keep checking if the table is empty
    // The timer will be started next time a UPC is deleted
    deleteUpcTimer->stop();
  }
}

void MovieLibraryWidget::deleteVideoLookupResults(bool success, const QString &message, const QStringList &upcList)
{
  if (success)
  {
    dbMgr->deleteUPCs(upcList);
  }
  else
  {
    QLOG_ERROR() << QString("Could not delete UPCs from queue on server, will try again...");
  }

  busy = false;
}

void MovieLibraryWidget::insertMovieChangeSetRow(DvdCopyTask task)
{
  // Week #, Complete, UPC, Title, Producer, Category, Subcategory, File Size, Year, DvdCopyTask
  QStandardItem *weekNumField, *upcField, *titleField, *producerField, *categoryField, *subcategoryField, *fileSizeField, *completeField, *transcodeField, *yearField, *objectField;

  weekNumField = new QStandardItem(QString("%1").arg(task.WeekNum()));
  weekNumField->setData(task.WeekNum());

  completeField = new QStandardItem(QString("%1").arg(task.Complete() ? "True" : "False"));
  completeField->setData(task.Complete());

  upcField = new QStandardItem(task.UPC());
  upcField->setData(task.UPC());

  titleField = new QStandardItem(task.Title());
  titleField->setData(task.Title());

  producerField = new QStandardItem(task.Producer());
  producerField->setData(task.Producer());

  categoryField = new QStandardItem(task.Category());
  categoryField->setData(task.Category());

  subcategoryField = new QStandardItem(task.Subcategory());
  subcategoryField->setData(task.Subcategory());

  fileSizeField = new QStandardItem(QString("%1 GB").arg(qreal(task.FileLength()) / (1024 * 1024 * 1024), 0, 'f', 2));
  fileSizeField->setData(int(task.FileLength()));

  transcodeField = new QStandardItem(task.TranscodeStatusToString());
  transcodeField->setData(task.TranscodeStatus());

  yearField = new QStandardItem(QString("%1").arg(task.Year()));
  yearField->setData(task.Year());

  QVariant v;
  v.setValue(task);
  objectField = new QStandardItem;
  objectField->setData(v);

  int row = movieChangeSetsModel->rowCount();

  movieChangeSetsModel->setItem(row, 0, weekNumField);
  movieChangeSetsModel->setItem(row, 1, upcField);
  movieChangeSetsModel->setItem(row, 2, titleField);
  movieChangeSetsModel->setItem(row, 3, producerField);
  movieChangeSetsModel->setItem(row, 4, categoryField);
  movieChangeSetsModel->setItem(row, 5, subcategoryField);
  movieChangeSetsModel->setItem(row, 6, fileSizeField);
  movieChangeSetsModel->setItem(row, 7, transcodeField);
  movieChangeSetsModel->setItem(row, 8, completeField);
  movieChangeSetsModel->setItem(row, 9, yearField);
  movieChangeSetsModel->setItem(row, 10, objectField);
}

void MovieLibraryWidget::insertDvdCopyJobRow(DvdCopyTask task)
{
  // Week #, UPC, Title, Producer, Category, Subcategory, Status, Year, DvdCopyTask
  QStandardItem *weekNumField, *upcField, *titleField, *producerField, *categoryField, *subcategoryField, *statusField, *transcodedField, *yearField, *objectField;

  // Add to current job table
  weekNumField = new QStandardItem(QString("%1").arg(task.WeekNum()));
  weekNumField->setData(task.WeekNum());

  upcField = new QStandardItem(task.UPC());
  upcField->setData(task.UPC());

  titleField = new QStandardItem(task.Title());
  titleField->setData(task.Title());

  producerField = new QStandardItem(task.Producer());
  producerField->setData(task.Producer());

  categoryField = new QStandardItem(task.Category());
  categoryField->setData(task.Category());

  subcategoryField = new QStandardItem(task.Subcategory());
  subcategoryField->setData(task.Subcategory());

  statusField = new QStandardItem(task.TaskStatusToString());
  statusField->setData(int(task.TaskStatus()));

  transcodedField = new QStandardItem(task.TranscodeStatusToString());
  transcodedField->setData(task.TranscodeStatus());

  yearField = new QStandardItem(QString("%1").arg(task.Year()));
  yearField->setData(task.Year());

  QVariant v;
  v.setValue(task);
  objectField = new QStandardItem;
  objectField->setData(v);

  int row = movieCopyJobModel->rowCount();

  movieCopyJobModel->setItem(row, 0, weekNumField);
  movieCopyJobModel->setItem(row, 1, upcField);
  movieCopyJobModel->setItem(row, 2, titleField);
  movieCopyJobModel->setItem(row, 3, producerField);
  movieCopyJobModel->setItem(row, 4, categoryField);
  movieCopyJobModel->setItem(row, 5, subcategoryField);
  movieCopyJobModel->setItem(row, 6, statusField);
  movieCopyJobModel->setItem(row, 7, transcodedField);
  movieCopyJobModel->setItem(row, 8, yearField);
  movieCopyJobModel->setItem(row, 9, objectField);
}

void MovieLibraryWidget::insertVideoRow(Movie video)
{
  // UPC, Title, Producer, Category, Subcategory, Date Added
  QStandardItem *channelField, *upcField, *titleField, *producerField, *categoryField, *subcategoryField, *dateAddedField;

  channelField = new QStandardItem(QString("%1").arg(video.ChannelNum()));
  channelField->setData(video.ChannelNum());

  upcField = new QStandardItem(video.UPC());
  upcField->setData(video.UPC());

  titleField = new QStandardItem(video.Title());
  titleField->setData(video.Title());

  producerField = new QStandardItem(video.Producer());
  producerField->setData(video.Producer());

  categoryField = new QStandardItem(video.Category());
  categoryField->setData(video.Category());

  subcategoryField = new QStandardItem(video.Subcategory());
  subcategoryField->setData(video.Subcategory());

  dateAddedField = new QStandardItem(video.DateIn().toString("MM/dd/yyyy"));
  dateAddedField->setData(video.DateIn());

  int row = libraryModel->rowCount();

  libraryModel->setItem(row, 0, channelField);
  libraryModel->setItem(row, 1, upcField);
  libraryModel->setItem(row, 2, titleField);
  libraryModel->setItem(row, 3, producerField);
  libraryModel->setItem(row, 4, categoryField);
  libraryModel->setItem(row, 5, subcategoryField);
  libraryModel->setItem(row, 6, dateAddedField);
}

int MovieLibraryWidget::findMovieChangeSetRow(QString upc)
{
  int row = -1;

  // Look in column 2 for a matching UPC
  QList<QStandardItem *> resultList = movieChangeSetsModel->findItems(upc, Qt::MatchExactly, 1);

  if (resultList.count() > 0)
    row = resultList.first()->row();

  return row;
}

int MovieLibraryWidget::findDvdCopyJobRow(QString upc)
{
  int row = -1;

  // Look in column 1 for a matching UPC
  QList<QStandardItem *> resultList = movieCopyJobModel->findItems(upc, Qt::MatchExactly, 1);

  if (resultList.count() > 0)
    row = resultList.first()->row();

  return row;
}

void MovieLibraryWidget::clearMovieChangeSets()
{
  if (movieChangeSetsModel->rowCount() > 0)
    movieChangeSetsModel->removeRows(0, movieChangeSetsModel->rowCount());
}

void MovieLibraryWidget::clearMovieCopyJobs()
{
  if (movieCopyJobModel->rowCount() > 0)
    movieCopyJobModel->removeRows(0, movieCopyJobModel->rowCount());
}

void MovieLibraryWidget::clearMovieLibrary()
{
  if (libraryModel->rowCount() > 0)
    libraryModel->removeRows(0, libraryModel->rowCount());
}

bool MovieLibraryWidget::videoFilesComplete(QString path)
{
  QDir dir(path);

  if (dir.exists())
  {
    Movie movie(dir.absoluteFilePath("mediaInfo.xml"), 0);

    if (movie.isValid())
    {
      QFile tsFile(dir.absoluteFilePath(movie.VideoFile()));
      QFile frontCoverFile(dir.absoluteFilePath(movie.FrontCover()));
      QFile backCoverFile(dir.absoluteFilePath(movie.BackCover()));

      return (tsFile.exists() && frontCoverFile.exists() && backCoverFile.exists());
    }

  } // endif download directory exists in parent directory

  return false;
}

void MovieLibraryWidget::finishCopyJob()
{
  QLOG_DEBUG() << QString("DVD copy job finished");

  int row = 0;
  int numFinished = 0, numFailed = 0;

  while (row < movieCopyJobModel->rowCount())
  {
    QStandardItem *objectField = movieCopyJobModel->item(row, movieCopyJobModel->columnCount() - 1);
    DvdCopyTask task = objectField->data().value<DvdCopyTask>();

    if (task.TaskStatus() == DvdCopyTask::Finished)
    {
      QLOG_DEBUG() << QString("Moving UPC: %1 to movie change set model").arg(task.UPC());

      movieCopyJobModel->removeRow(row);

      insertMovieChangeSetRow(task);

      numFinished++;
    }
    else
    {
      QLOG_DEBUG() << QString("UPC: %1 will not be moved since it failed").arg(task.UPC());
      row++;
      numFailed++;
    }
  }

  // Reset index into model data
  currentDvdIndex = 0;

  if (numFailed == 0)
  {
    QLOG_DEBUG() << QString("Successfully copied all %1 DVDs").arg(numFinished);
    QMessageBox::information(this, tr("Finished"), tr("DVD copy job has finished and all %1 DVDs were copied successfully.").arg(numFinished));
  }
  else
  {
    // Delete any orphan files in the /var/cas-mgr/share/videos/<year>/<week_num>/
    QStringList searchList;
    searchList << "*.VOB" << "*.IFO";
    int maxDepth = 3;

    QStringList orphans = Global::find(settings->getValue(VIDEO_PATH).toString(), searchList, maxDepth);

    if (orphans.count() > 0)
    {
      QLOG_DEBUG() << "Orphaned video files: " << orphans;

      foreach (QString file, orphans)
      {
        if (QFile::remove(file))
          QLOG_DEBUG() << "Deleted: " << file;
        else
          QLOG_ERROR() << "Could not delete: " << file;
      }
    }
    else
      QLOG_DEBUG() << "No orphaned video files found";

    QLOG_ERROR() << QString("DVD copy job finished but only %1/%2 DVDs were copied.").arg(numFinished).arg(numFailed + numFinished);

    if (numFinished == 0)
      QMessageBox::warning(this, tr("Failed"), tr("None of the DVDs were copied. Use the Retry Copy Job button to try again or the Delete Job to remove one or more DVDs from the job."));
    else
      QMessageBox::warning(this, tr("Finished with errors"), tr("DVD copy job finished -- but only %1 out of %2 DVDs were copied. Use the Retry Copy Job button to try again or the Delete Job to remove one or more DVDs from the job.").arg(numFinished).arg(numFailed + numFinished));

    btnRetryJob->setEnabled(true);
    btnDeleteJob->setEnabled(true);
  }

  btnCancelJob->setEnabled(false);
  btnNewJob->setEnabled(true);
  btnShowAutoloaderCP->setEnabled(true);
}

void MovieLibraryWidget::viewVideoLibraryEntry(const QModelIndex &index)
{
  QLOG_DEBUG() << QString("User double-clicked video in Current Video Library datagrid");

  if (!videoInfoWin->isVisible())
  {
    QStandardItem *upcObject = libraryModel->item(index.row(), 1);

    DvdCopyTask archivedTask = dbMgr->getArchivedDvdCopyTask(upcObject->data().toString());

    if (archivedTask.UPC().isEmpty())
    {
      QLOG_ERROR() << QString("Could not find UPC in archived DVD copy jobs to show front/back covers: %1").arg(upcObject->data().toString());
      Global::sendAlert(QString("Could not find UPC in archived DVD copy jobs to show front/back covers: %1").arg(upcObject->data().toString()));

      QMessageBox::warning(this, tr("Not Found"), tr("Could not find this video by UPC in the database. Please contact tech support."));

      return;
    }

    videoInfoWin->populateForm(archivedTask);
    videoInfoWin->show();
  }
  else
  {
    videoInfoWin->show();
    videoInfoWin->raise();
    videoInfoWin->activateWindow();
  }
}

int MovieLibraryWidget::findVideoRow(QString upc)
{
  int row = -1;

  // Look in column 1 for a matching UPC
  QList<QStandardItem *> resultList = libraryModel->findItems(upc, Qt::MatchExactly, 1);

  if (resultList.count() > 0)
    row = resultList.first()->row();

  return row;
}

void MovieLibraryWidget::deleteVideoLibraryEntry()
{
  QModelIndexList selectedRows = currentLibraryTable->selectionModel()->selectedRows(0);

  if (selectedRows.count() == 0)
  {
    QLOG_DEBUG() << "User pressed Delete key/clicked Delete Video button without row selected in current video library table";
    QMessageBox::warning(this, tr("Delete Video"), tr("Select one or more rows in the table to delete."));
  }
  else
  {
    QLOG_DEBUG() << "User selected" << selectedRows.count() << "videos to delete from the arcade";

    // ask user if he wants to delete the selected videos from the arcade
    if (QMessageBox::question(this, tr("Delete Video"), tr("Do you want to delete the selected video(s) from all booths?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
    {
      QLOG_DEBUG() << "User chose to delete video(s) from the arcade";

      if (dbMgr->getMovieChangeQueue(0).count() == 0)
      {
        bool ok = false;
        int deleteMovieYear = dbMgr->getMaxMovieChangeYear(&ok);

        if (ok)
        {
          if (deleteMovieYear < settings->getValue(DELETE_MOVIE_YEAR).toInt())
            deleteMovieYear = settings->getValue(DELETE_MOVIE_YEAR).toInt();
          else
            ++deleteMovieYear;

          QDateTime scheduledDate = QDateTime::currentDateTime();

          MovieChangeInfo movieChange;
          movieChange.setYear(deleteMovieYear);
          movieChange.setWeekNum(scheduledDate.date().weekNumber());
          movieChange.setNumVideos(selectedRows.count());
          movieChange.setMaxChannels(0);
          movieChange.setOverrideViewtimes(false);
          movieChange.setFirstChannel(0);
          movieChange.setLastChannel(0);

          QList<DvdCopyTask> dvdCopyTaskList;

          for (int i = 0; i < selectedRows.count(); ++i)
          {
            QModelIndex index = selectedRows.at(i);

            QLOG_DEBUG() << "Channel #:" << index.data().toInt();

            Movie currentMovie;
            currentMovie.setChannelNum(index.data().toInt());
            currentMovie.setUPC(libraryModel->item(index.row(), 1)->data().toString());
            currentMovie.setTitle(libraryModel->item(index.row(), 2)->data().toString());
            currentMovie.setProducer(libraryModel->item(index.row(), 3)->data().toString());
            currentMovie.setCategory(libraryModel->item(index.row(), 4)->data().toString());
            currentMovie.setSubcategory(libraryModel->item(index.row(), 5)->data().toString());

            dbMgr->insertMovieChange(deleteMovieYear, scheduledDate.date().weekNumber(), NULL, currentMovie.ChannelNum(), true, scheduledDate, currentMovie);

            DvdCopyTask task;
            task.setUPC(currentMovie.UPC());
            task.setYear(deleteMovieYear);
            task.setWeekNum(scheduledDate.date().weekNumber());

            dvdCopyTaskList.append(task);
          }

          movieChange.setDvdCopyTaskList(dvdCopyTaskList);

          dbMgr->insertMovieChangeQueue(movieChange);

          // Take user to the movie change tab
          emit queueDeleteVideo(movieChange);
        }
        else
        {
          QMessageBox::warning(this, tr("Delete Video"), tr("Could not retrieve information from the database. Please contact tech suppport."));
        }
      }
      else
      {
        QMessageBox::information(this, tr("Delete Video"), tr("Cannot delete videos from booths while a movie change is in progress. Please wait for it to finish."));
      }
    }
    else
      QLOG_DEBUG() << "User chose not to delete video(s) from the arcade";
  }
}
