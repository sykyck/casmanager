#include "moviechangewidget.h"
#include "moviechangeweeknumwidget.h"
#include "qslog/QsLog.h"
#include "global.h"
#include "filehelper.h"
#include <QSortFilterProxyModel>
#include <QHeaderView>
#include <QMessageBox>
#include <QDir>
#include "analytics/piwiktracker.h"


// FIXME: There is the chance that when showing this widget, the booth status job is in progress and a database might not exist because the booth is deleting and copying it
// FIXME: Operating on 30 databases is slow, when tab is first shown and it needs to archive records, the app will appear to hang and the tab doesn't show until it finishes
// BUG: If tab is never clicked then movie change won't continue if it has to recover from an unexpected crash or reboot

MovieChangeWidget::MovieChangeWidget(DatabaseMgr *dbMgr, CasServer *casServer, Settings *settings, QTabWidget *parentTab, QWidget *parent) : QWidget(parent)
{
  this->dbMgr = dbMgr;
  this->casServer = casServer;
  this->settings = settings;
  this->parentTab = parentTab;
  firstLoad = true;
  busy = false;
  errorMsg.clear();
  possibleErrorMsg.clear();
  uftpServer = 0;
  currentMovieChangeIndex = 0;
  sentMovieChangeStuckAlert = false;
  msgWidget = 0;
  numMovieChangeRetries = 0;
  forceMovieChange = false;
  movieChangeCancelCleanup = false;
  enableUpdateMovieChangeStatus = false;

  movieChangeRetryTimer = new QTimer;
  movieChangeRetryTimer->setInterval(settings->getValue(MOVIECHANGE_RETRY_TIMEOUT).toInt());
  movieChangeRetryTimer->setSingleShot(true);
  connect(movieChangeRetryTimer, SIGNAL(timeout()), this, SLOT(retryMovieChange()));

  uftpServer = new UftpServer(settings->getValue(FILE_SERVER_PROG_FILE).toString(),
                              settings->getValue(FILE_SERVER_LOG_FILE).toString(),
                              settings->getValue(FILE_SERVER_STATUS_FILE).toString(),
                              settings->getValue(FILE_SERVER_LIST_FILE).toString());
  connect(uftpServer, SIGNAL(startedCopying()), this, SLOT(startedUftp()));
  connect(uftpServer, SIGNAL(finishedCopying(UftpServer::UftpStatusCode,QString,QStringList,QStringList)), this, SLOT(finishedUftp(UftpServer::UftpStatusCode,QString,QStringList,QStringList)));
  connect(uftpServer, SIGNAL(processError()), this, SLOT(uftpProcessError()));

  connect(gPiwikTracker, SIGNAL(receivedLeastViewedMovies(QList<Movie>,bool)), this, SLOT(leastViewedMoviesResponse(QList<Movie>,bool)));

  // Signals/slots for preparing the movie change which includes determining which booths should be included based
  // on online status, being up-to-date on movie changes, assigning channels to the new movies and determining if any
  // movies need to be deleted to make room
  connect(casServer, SIGNAL(movieChangePrepareError(QString)), this, SLOT(movieChangePrepareError(QString)));
  connect(casServer, SIGNAL(movieChangePreparePossibleError(QString)), this, SLOT(movieChangePreparePossibleError(QString)));
  connect(casServer, SIGNAL(movieChangePrepareFinished()), this, SLOT(movieChangePrepareFinished()));

  // Signals/slots for starting the movie change process on the client side
  connect(casServer, SIGNAL(movieChangeStartSuccess(QString)), this, SLOT(movieChangeStartSuccess(QString)));
  connect(casServer, SIGNAL(movieChangeStartFailed(QString)), this, SLOT(movieChangeStartFailed(QString)));
  connect(casServer, SIGNAL(movieChangeClientsFinished()), this, SLOT(movieChangeClientsFinished()));

  // Signal/slot used when both preparing the movie change and starting the movie change process
  connect(casServer, SIGNAL(movieChangeExcluded(QString)), this, SLOT(movieChangeExcluded(QString)));

  // Signals/slots used when starting a movie change retry with booths that did not finish the uftp session
  connect(casServer, SIGNAL(movieChangeRetryFailed(QString)), this, SLOT(movieChangeRetryFailed(QString)));
  connect(casServer, SIGNAL(movieChangeRetrySuccess(QString)), this, SLOT(movieChangeRetrySuccess(QString)));
  connect(casServer, SIGNAL(movieChangeRetryFinished()), this, SLOT(movieChangeRetryFinished()));

  // Signals/slots used when canceling the movie change process with booths
  connect(casServer, SIGNAL(movieChangeCancelFailed(QString)), this, SLOT(movieChangeCancelFailed(QString)));
  connect(casServer, SIGNAL(movieChangeCancelSuccess(QString)), this, SLOT(movieChangeCancelSuccess(QString)));
  connect(casServer, SIGNAL(movieChangeCancelFinished()), this, SLOT(movieChangeCancelFinished()));

  verticalLayout = new QVBoxLayout;
  buttonLayout = new QHBoxLayout;

  // # Movies, Status, Year, Week # (last 2 hidden columns)
  movieChangeQueueModel = new QStandardItemModel(0, 4);
  movieChangeQueueModel->setHorizontalHeaderItem(Num_Movies, new QStandardItem(QString("# Movies")));
  movieChangeQueueModel->setHorizontalHeaderItem(Status, new QStandardItem(QString("Status")));
  movieChangeQueueModel->setHorizontalHeaderItem(Queue_Year, new QStandardItem(QString("Year")));
  movieChangeQueueModel->setHorizontalHeaderItem(Queue_Week_Num, new QStandardItem(QString("Week #")));

  // Data is from the movie_changes table (moviechanges.sqlite) and joined with the dvd_copy_jobs_history table (cas-mgr.sqlite)
  // A new movie change cannot be scheduled while data exists in the movie_changes table

  // Scheduled Date, Week #, UPC, Title, Category, Channel #, Previous Movie, Year (hidden column)
  pendingMovieChangeModel = new QStandardItemModel(0, 8);
  // TODO: Do we really need to have Scheduled_Date in the data model anymore?
  pendingMovieChangeModel->setHorizontalHeaderItem(Scheduled_Date, new QStandardItem(QString("Scheduled Date")));
  pendingMovieChangeModel->setHorizontalHeaderItem(Week_Num, new QStandardItem(QString("Week #")));
  pendingMovieChangeModel->setHorizontalHeaderItem(UPC, new QStandardItem(QString("UPC")));
  pendingMovieChangeModel->setHorizontalHeaderItem(Title, new QStandardItem(QString("Title")));
  pendingMovieChangeModel->setHorizontalHeaderItem(Category, new QStandardItem(QString("Category")));
  pendingMovieChangeModel->setHorizontalHeaderItem(Channel_Num, new QStandardItem(QString("Channel #")));
  pendingMovieChangeModel->setHorizontalHeaderItem(Previous_Movie, new QStandardItem(QString("Previous Movie")));
  pendingMovieChangeModel->setHorizontalHeaderItem(Year, new QStandardItem(QString("Year")));

  // If data exist in movie_changes table (moviechanges.sqlite) then this is populated with data
  // from all the moviechanges_<address>.sqlite, viewtimes_<address>.sqlite, casplayer_<address>.sqlite databases

  // Name, Type, Address, Free Space, # Channels, Scheduled, Download Started, Download Finished, Movies Changed
  movieChangeDetailModel = new QStandardItemModel(0, 10);
  movieChangeDetailModel->setHorizontalHeaderItem(Name, new QStandardItem(QString("Name")));
  movieChangeDetailModel->setHorizontalHeaderItem(Booth_Type, new QStandardItem(QString("Type")));
  movieChangeDetailModel->setHorizontalHeaderItem(Address, new QStandardItem(QString("Address")));
  movieChangeDetailModel->setHorizontalHeaderItem(Free_Space, new QStandardItem(QString("Free Space")));
  movieChangeDetailModel->setHorizontalHeaderItem(Total_Channels, new QStandardItem(QString("# Channels")));
  movieChangeDetailModel->setHorizontalHeaderItem(Scheduled, new QStandardItem(QString("Scheduled"))); // Pending, True, Failed / True, False
  movieChangeDetailModel->setHorizontalHeaderItem(Download_Started, new QStandardItem(QString("Download Started")));
  // Hidden field, used for keeping track of booths that were successful according to uftp
  movieChangeDetailModel->setHorizontalHeaderItem(UFTP_Finished, new QStandardItem(QString("UFTP Finished")));
  movieChangeDetailModel->setHorizontalHeaderItem(Download_Finished, new QStandardItem(QString("Download Finished")));
  movieChangeDetailModel->setHorizontalHeaderItem(Movies_Changed, new QStandardItem(QString("Movies Changed")));

  movieChangeQueueView = new QTableView;
  pendingMovieChangeView = new QTableView;
  movieChangeDetailView = new QTableView;

  lblMovieChangeQueue = new QLabel(tr("Movie Change Queue"));
  lblPendingMovieChange = new QLabel(tr("Movie Change Set Detail"));
  lblMovieChangeDetail = new QLabel(tr("Movie Change Status"));

  btnNewMovieChange = new QPushButton(tr("Add Movie Change"));
  connect(btnNewMovieChange, SIGNAL(clicked()), this, SLOT(addNewMovieChange()));

  btnCancelMovieChange = new QPushButton(tr("Cancel Movie Change"));
  btnCancelMovieChange->setEnabled(false);
  connect(btnCancelMovieChange, SIGNAL(clicked()), this, SLOT(cancelMovieChangeClicked()));

  movieChangeQueueView->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
  movieChangeQueueView->horizontalHeader()->setStretchLastSection(true);
  movieChangeQueueView->horizontalHeader()->setStyleSheet("font:bold Arial;");
  movieChangeQueueView->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
  movieChangeQueueView->setSelectionMode(QAbstractItemView::SingleSelection);
  movieChangeQueueView->setSelectionBehavior(QAbstractItemView::SelectRows);
  movieChangeQueueView->setWordWrap(true);
  movieChangeQueueView->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
  movieChangeQueueView->verticalHeader()->hide();
  movieChangeQueueView->setAlternatingRowColors(true);
  movieChangeQueueView->setModel(movieChangeQueueModel);
  movieChangeQueueView->setColumnHidden(Queue_Year, true);
  movieChangeQueueView->setColumnHidden(Queue_Week_Num, true);
  movieChangeQueueView->setEditTriggers(QAbstractItemView::NoEditTriggers);

  pendingMovieChangeView->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
  pendingMovieChangeView->horizontalHeader()->setStretchLastSection(true);
  pendingMovieChangeView->horizontalHeader()->setStyleSheet("font:bold Arial;");
  pendingMovieChangeView->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
  pendingMovieChangeView->setSelectionMode(QAbstractItemView::SingleSelection);
  pendingMovieChangeView->setSelectionBehavior(QAbstractItemView::SelectRows);
  pendingMovieChangeView->setWordWrap(true);
  pendingMovieChangeView->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
  pendingMovieChangeView->verticalHeader()->hide();
  pendingMovieChangeView->setModel(pendingMovieChangeModel);
  pendingMovieChangeView->setColumnHidden(Scheduled_Date, true);
  pendingMovieChangeView->setColumnHidden(Week_Num, true);
  pendingMovieChangeView->setColumnHidden(Year, true);
  pendingMovieChangeView->setAlternatingRowColors(true);
  // Make table read-only
  pendingMovieChangeView->setEditTriggers(QAbstractItemView::NoEditTriggers);

  QSortFilterProxyModel *sortFilter = new QSortFilterProxyModel;
  sortFilter->setSourceModel(movieChangeDetailModel);
  sortFilter->sort(Address, Qt::AscendingOrder);

  movieChangeDetailView->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
  movieChangeDetailView->horizontalHeader()->setStretchLastSection(true);
  movieChangeDetailView->horizontalHeader()->setStyleSheet("font:bold Arial;");
  movieChangeDetailView->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
  movieChangeDetailView->setSelectionMode(QAbstractItemView::SingleSelection);
  movieChangeDetailView->setSelectionBehavior(QAbstractItemView::SelectRows);
  movieChangeDetailView->setWordWrap(true);
  movieChangeDetailView->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
  movieChangeDetailView->verticalHeader()->hide();
  movieChangeDetailView->setModel(sortFilter);
  movieChangeDetailView->setColumnHidden(UFTP_Finished, true);
  movieChangeDetailView->setSortingEnabled(true);
  movieChangeDetailView->setAlternatingRowColors(true);
  movieChangeDetailView->setEditTriggers(QAbstractItemView::NoEditTriggers);

  buttonLayout->addWidget(btnNewMovieChange, 0, Qt::AlignLeft);
  buttonLayout->addWidget(btnCancelMovieChange, 0, Qt::AlignLeft);
  buttonLayout->addStretch(1);

  verticalLayout->addWidget(lblMovieChangeQueue, 0, Qt::AlignLeft);
  verticalLayout->addWidget(movieChangeQueueView, 0); // Shorter than other 2 datagrids
  verticalLayout->addWidget(lblPendingMovieChange, 0, Qt::AlignLeft);
  verticalLayout->addWidget(pendingMovieChangeView, 3);
  verticalLayout->addWidget(lblMovieChangeDetail, 0, Qt::AlignLeft);
  verticalLayout->addWidget(movieChangeDetailView, 3);
  verticalLayout->addLayout(buttonLayout);

  this->setLayout(verticalLayout);
}

MovieChangeWidget::~MovieChangeWidget()
{
  uftpServer->deleteLater();

  if (msgWidget)
    msgWidget->deleteLater();

  movieChangeRetryTimer->deleteLater();
}

bool MovieChangeWidget::isBusy()
{
  return busy;
}

bool MovieChangeWidget::isMovieChangeInProgress()
{
  return (pendingMovieChangeModel->rowCount() > 0);
}

// FIXME: Preview booths are still getting movie changes
void MovieChangeWidget::showEvent(QShowEvent *)
{
  QLOG_DEBUG() << QString("Show Movie Change tab");

  if (firstLoad)
  {
    int year = 0, weekNum = 0;

    QLOG_DEBUG() << "Looking for items in movie change queue";

    // Load movie change queue
    QList<MovieChangeInfo> previousMovieChangeQueue = dbMgr->getMovieChangeQueue(settings->getValue(MAX_CHANNELS).toInt());

    // Only continue if movies found in queue but none have been added to the movie change queue model
    // This is to handle the scenario where this tab is being shown for the first time but we came from the Movie Library tab to delete movies
    if (previousMovieChangeQueue.count() > 0 && movieChangeQueueModel->rowCount() == 0)
    {
      QLOG_DEBUG() << QString("Movie change process was interrupted, loaded %1 movie change set(s) from movie change queue table").arg(previousMovieChangeQueue.count());
      Global::sendAlert(QString("Movie change process was interrupted, loaded %1 movie change set(s) from movie change queue table").arg(previousMovieChangeQueue.count()));

      QStringList movieChangeQueueErrors;

      // Populate movie change queue datagrid
      for (int i = 0; i < previousMovieChangeQueue.count(); ++i)
      {
        MovieChangeInfo movieChange = previousMovieChangeQueue.at(i);

        insertMovieChangeQueue(movieChange);

        // If any movie change set after the first one does not have a status of Pending then we have a problem
        if (i > 0 && movieChange.Status() != MovieChangeInfo::Pending)
        {
          movieChangeQueueErrors.append(QString("Movie change set is at position %1 in movie change queue but it has an unexpected status of: %2. Year: %3, Week #: %4")
                                        .arg(i)
                                        .arg(MovieChangeInfo::MovieChangeStatusToString(movieChange.Status()))
                                        .arg(movieChange.Year())
                                        .arg(movieChange.WeekNum()));

          // Delete from queue and ignore since this is in an unexpected state
          QLOG_ERROR() << QString("Deleting movie change set from queue since it has an unexpected status");
          dbMgr->deleteMovieChangeQueue(movieChange);
        }
        else
        {
          movieChangeQueue.append(movieChange);
        }
      }

      if (movieChangeQueueErrors.count() > 0)
      {
        Global::sendAlert(movieChangeQueueErrors.join("\n"));
        QLOG_ERROR() << movieChangeQueueErrors.join("\n");
      }

      if (movieChangeQueue.count() > 0)
      {
        MovieChangeInfo interruptedMovieChange = movieChangeQueue.at(0);
        QLOG_DEBUG() << QString("Movie change set year: %1, week #: %2 was in the %3 state").arg(interruptedMovieChange.Year()).arg(interruptedMovieChange.WeekNum()).arg(MovieChangeInfo::MovieChangeStatusToString(interruptedMovieChange.Status()));

        switch (interruptedMovieChange.Status())
        {
          case MovieChangeInfo::Pending:
            QTimer::singleShot(0, this, SLOT(checkMovieChangeQueue()));
            break;

          case MovieChangeInfo::Processing:
            QLOG_DEBUG() << "Changing movie change set status to Pending";
            updateMovieChangeQueueStatus(MovieChangeInfo::Pending);
            QTimer::singleShot(0, this, SLOT(checkMovieChangeQueue()));
            break;

          case MovieChangeInfo::Copying:
            // TODO: Tell all booths to delete previous files downloaded for this movie change set and start uftpd at each booth
            // TODO: populate movie change set detail and status datagrids
            QTimer::singleShot(0, this, SLOT(startCopyingMovieChangeSet()));
            break;

          case MovieChangeInfo::Waiting_For_Booths:
            // It finished running uftp and was waiting for the booths to perform the actual movie change
            // TODO: populate movie change set detail and status datagrids
            // Start check status timer
            // Call
            //checkStatusTimer->start();
            QTimer::singleShot(0, this, SLOT(updateMovieChangeStatus()));
            break;

          case MovieChangeInfo::Finished:
            // Must have gotten interrupted right before the movie change set was archived
            QTimer::singleShot(0, this, SLOT(finalizeMovieChange()));
            break;

          case MovieChangeInfo::Failed:
            // ??????
            break;

          case MovieChangeInfo::Retrying:
            // It was running uftp for the booths that failed to download all files
            // populate movie change set detail datagrid
            break;

          default:
            Global::sendAlert(QString("Movie change set from queue has an unknown status: %1").arg(interruptedMovieChange.Status()));
            QLOG_ERROR() << QString("Movie change set from queue has an unknown status: %1").arg(interruptedMovieChange.Status());
            // TODO: Cannot resume movie change because this is in an unknown state
            break;
        }

        busy = true;
      }
      else
      {
        QLOG_ERROR() << "The movie change queue is now empty after encountering errors";
      }
    }
    else
    {
      QLOG_DEBUG() << "The movie change queue is empty, nothing to resume";
    }

    QVariantList pendingMovieChanges = dbMgr->getPendingMovieChange();

    foreach (QVariant v, pendingMovieChanges)
    {
      QVariantMap movieChange = v.toMap();

      DvdCopyTask task;
      task.setUPC(movieChange["upc"].toString());
      task.setTitle(movieChange["title"].toString());
      task.setCategory(movieChange["category"].toString());
      task.setYear(movieChange["year"].toInt());
      task.setWeekNum(movieChange["week_num"].toInt());

      year = task.Year();
      weekNum = task.WeekNum();

      insertMovieChangeScheduled(movieChange["scheduled_date"].toDateTime(), task, movieChange["channel_num"].toInt(), movieChange["previous_movie"].toString());

      // Get the scheduled date to start downloading movies so we can compare it to the dates
      // retrieved from the casplayer_*.sqlite databases. The date will be the same for each loop iteration
      scheduledDate = movieChange["scheduled_date"].toDateTime();
    }

    if (year > 0 && weekNum > 0)
    {
      // Adjust scheduled date/time by adding back the seconds that were subtracted for the booths
      // if this is not done then there is the remote possibly that the scheduled time falls before
      // the last movie change finishing (if this movie change was started right after the last one)
      // AND the casplayer_*.sqlite databases have not been updated by the booths since the new movie
      // change started so it would appear the movie change already finished
      scheduledDate = scheduledDate.addSecs(settings->getValue(MOVIE_CHANGE_SCHEDULE_TIME_ADJUST).toInt());

      QLOG_DEBUG() << "A movie change was found and scheduled for: " << scheduledDate;

      QVariantList boothList = dbMgr->getOnlyBoothInfo(casServer->getDeviceAddresses());

      int numFinishedDownloading = 0;
      foreach (QVariant v, boothList)
      {
        QVariantMap booth = v.toMap();

        QString scheduled = "Failed";
        QDateTime downloadStart;
        QDateTime downloadFinished;
        QDateTime movieChanged;

        if (booth.contains("scheduled"))
        {
          // TODO: This will always be True because it is no longer based on whether a date is found
          // it seems unnecessary to show this information
          if (booth["scheduled"].toBool())
            scheduled = "True";

          if (booth["download_started"].toDateTime().isValid() && scheduledDate <= booth["download_started"].toDateTime())
            downloadStart = booth["download_started"].toDateTime();

          if (booth["download_finished"].toDateTime().isValid() && scheduledDate <= booth["download_finished"].toDateTime())
          {
            downloadFinished = booth["download_finished"].toDateTime();
            ++numFinishedDownloading;
          }

          if (booth["movies_changed"].toDateTime().isValid() && scheduledDate <= booth["movies_changed"].toDateTime())
            movieChanged = booth["movies_changed"].toDateTime();
        }

        insertMovieChangeBoothStatus(booth["device_alias"].toString(), booth["device_type"].toString(), settings->getValue(ARCADE_SUBNET).toString() + booth["device_num"].toString(), booth["free_space_gb"].toDouble(), booth["num_channels"].toInt(), scheduled, downloadStart, downloadFinished, movieChanged);
      }

      // Start the status update timer since we found a pending movie change
      //checkStatusTimer->start();
      updateMovieChangeStatus();

      if (numFinishedDownloading != boothList.count())
      {
        QLOG_DEBUG() << QString("Starting uftp since it looks like it was interrupted and %1 out of %2 booths have finished downloading").arg(numFinishedDownloading).arg(boothList.count());
        uftpServer->start(settings->getValue(TRANSMISSION_SPEED).toInt(), getVideoDirectoryListing(year, weekNum));
      }
      else
      {
        QLOG_DEBUG() << "Looks like the software did not shutdown cleanly but all booths finished downloading the current movie change";
      }
    }

    firstLoad = false;
  }
}

void MovieChangeWidget::addNewMovieChange()
{
  errorMsg.clear();
  possibleErrorMsg.clear();

  QLOG_DEBUG() << QString("User clicked the Add Movie Change button");

  if (settings->getValue(SHOW_NEW_MOVIE_CHANGE_FEATURE).toBool())
  {
    if (!msgWidget)
    {
      msgWidget = new MessageWidget(tr("You can now queue multiple movie change sets to send to the booths. This means no longer having to wait for one movie change to finish before starting another. Click the \"Add Movie Change\" button at <b>ANY TIME</b> to send another movie change set.<br><br>For more information, go to the Help Center, which is accessible by pressing the F1 key or by clicking Help from the main menu."),
                                    tr("New Feature"),
                                    tr("OK"),
                                    "",
                                    "",
                                    "",
                                    tr("Do not show this message again."));
    }

    msgWidget->exec();

    // If checked then no longer show this information
    if (msgWidget->isChecked())
    {
      settings->setValue(SHOW_NEW_MOVIE_CHANGE_FEATURE, false);

      // Save settings
      dbMgr->setValue("all_settings", settings->getSettings());
    }
  }

  // Get list of unique dvd copy jobs by year and week num and how many videos in the set
  // Only include the week num if all records with a matching year and week num have the complete flag = 1
  // Exclude movie change sets that have already been added to the queue because these could still be returned
  // if they haven't been processed yet
  QList<MovieChangeInfo> movieChangeSets = dbMgr->getAvailableMovieChangeSets(false, movieChangeQueue);

  if (movieChangeSets.count() > 0)
  {
    QLOG_DEBUG() << QString("Found %1 movie change sets available").arg(movieChangeSets.count());

    // Identify the most recent casplayer_<address>.sqlite file, replace our videos table (cas-mgr.sqlite) with it and return the highest channel #
    // The highest channel # is used in the movie change set selection widget to limit the user from assigning a channel range that
    // extends past the last channel.
    int highestChannelNum = dbMgr->getVideos().count();
    MovieChangeWeekNumWidget changeSetWeekNum(movieChangeSets, highestChannelNum);
    changeSetWeekNum.setFirstChannel(1);

    if (changeSetWeekNum.exec() == QDialog::Accepted)
    {
      foreach (MovieChangeInfo movieChange, changeSetWeekNum.getSelectedMovieSet())
      {
        movieChange.setMaxChannels(settings->getValue(MAX_CHANNELS).toInt());

        QLOG_DEBUG() << QString("User selected movie change set: Year: %1, Week #: %2, Override viewtimes: %3, No. Videos: %4, First Ch #: %5, Last Ch #: %6, Max Channels: %7")
                        .arg(movieChange.Year())
                        .arg(movieChange.WeekNum())
                        .arg(movieChange.OverrideViewtimes() ? "True" : "False")
                        .arg(movieChange.NumVideos())
                        .arg(movieChange.FirstChannel())
                        .arg(movieChange.LastChannel())
                        .arg(movieChange.MaxChannels());

        // Add movie change set to queue
        movieChangeQueue.append(movieChange);

        // Add the movie change queue datagrid
        insertMovieChangeQueue(movieChange);

        // Keep track of movie change in database
        dbMgr->insertMovieChangeQueue(movieChange);
      }

      // If no movie change currently in progress then check movie change queue now
      // otherwise the queue will be checked when the current one finishes
      if (!isMovieChangeInProgress())
      {
        // Since this is the first movie change in the queue, reset the index to the beginning
        currentMovieChangeIndex = 0;

        // Clear flag so user is required to answer the question again if a possible error is encountered
        // this flag only gets cleared when adding the first movie change to the queue
        forceMovieChange = false;

        checkMovieChangeQueue();
      }

      busy = true;
    }
    else
      QLOG_DEBUG() << QString("User canceled selecting a movie change set");
  }
  else
  {
    QLOG_DEBUG() << QString("No movie change sets available");
    QMessageBox::information(this, tr("New Movie Change"), tr("There are no movie change sets available or no complete movie change sets found."));
  }
}

void MovieChangeWidget::checkMovieChangeQueue()
{
  if (currentMovieChangeIndex < movieChangeQueue.count())
  {
    sentMovieChangeStuckAlert = false;
    numMovieChangeRetries = 0;

    QLOG_DEBUG() << QString("Now processing movie change index: %1, year: %2, week #: %3 from the queue")
                    .arg(currentMovieChangeIndex)
                    .arg(movieChangeQueue[currentMovieChangeIndex].Year())
                    .arg(movieChangeQueue[currentMovieChangeIndex].WeekNum());

    // Automatically set forceMovieChange flag if this is not the first movie change set in the queue.
    // This is because we assume the user wants to continue with the movie changes even if some booths
    // are behind on movie changes. If this wasn't done, there is the possibility that the software will
    // be waiting for days or weeks before the user sees the prompt about whether the movie change should
    // continue. The forceMovieChange flag is still ignored for any serious error
    if (currentMovieChangeIndex > 0 && !forceMovieChange)
    {
      QLOG_DEBUG() << QString("Finished the first movie change in the queue, now assuming user wants to continue with the movie change if any error is encountered");
      forceMovieChange = true;
    }

    // This is just the time the process started and not when uftp starts since there might be a delay while
    // we're getting view times (if we don't already have recent view times) and the time that is stored in the
    // database is MOVIE_CHANGE_SCHEDULE_TIME_ADJUST (default 5 minutes) earlier so uftpd starts on the booths before the server
    QDateTime dateScheduled = QDateTime::currentDateTime();

    // Populate data models with selected movie set and booth details
    if (movieChangeQueue[currentMovieChangeIndex].Year() < settings->getValue(DELETE_MOVIE_YEAR).toInt())
    {
      foreach (DvdCopyTask task, movieChangeQueue[currentMovieChangeIndex].DvdCopyTaskList())
        insertMovieChangeScheduled(dateScheduled, task);
    }

    onlyBoothAddresses.clear();
    foreach (QVariant v, dbMgr->getOnlyBoothInfo(casServer->getDeviceAddresses()))
    {
      QVariantMap booth = v.toMap();
      onlyBoothAddresses.append(settings->getValue(ARCADE_SUBNET).toString() + booth["device_num"].toString());
      insertMovieChangeBoothStatus(booth["device_alias"].toString(),
          booth["device_type"].toString(),
          settings->getValue(ARCADE_SUBNET).toString() + booth["device_num"].toString(),
          booth["free_space_gb"].toDouble(),
          booth["num_channels"].toInt(),
          "Pending",
          QDateTime(),
          QDateTime(),
          QDateTime());
    }

    // Build path to location of movie change set: /var/cas-mgr/share/videos/<year>/<week_num>
    QDir videoPath(QString("%1/%2/%3")
                   .arg(settings->getValue(VIDEO_PATH).toString())
                   .arg(movieChangeQueue[currentMovieChangeIndex].Year())
                   .arg(movieChangeQueue[currentMovieChangeIndex].WeekNum()));

    movieChangeQueue[currentMovieChangeIndex].setVideoPath(videoPath.absolutePath());

    updateMovieChangeQueueStatus(MovieChangeInfo::Processing);

    //  Clear current least viewed movies from database
    if (dbMgr->deleteLeastViewedMovies())
    {
      bool ok = false;
      categoryQueue = dbMgr->getVideoCategories(&ok);

      if (ok)
      {
        QLOG_DEBUG() << "categoryQueue:" << categoryQueue;

        // Request least viewed movies in each category from web service
        requestLeastViewedMovies();
      }
    }
    else
    {
      // Cancel movie change
      QMessageBox::warning(this, tr("Movie Change"), tr("Could not prepare database for determining least viewed movies, movie change cannot continue. Contact tech support"));

      resetMovieChangeProcess();
    }

  } // endif movie change queue not empty
  else
  {
    QLOG_DEBUG() << "Finished processing all movie changes in the queue";

    // Finished processing movie change queue, clear the datagrids and reset widget state
    resetMovieChangeProcess();
  }
}

void MovieChangeWidget::movieChangePrepareError(QString message)
{
  errorMsg = message;
}

void MovieChangeWidget::movieChangePreparePossibleError(QString message)
{
  possibleErrorMsg = message;
}

void MovieChangeWidget::movieChangePrepareFinished()
{
  bool stillBusy = false;

  if (!possibleErrorMsg.isEmpty())
  {
    // Store currrent state then clear flag temporarily so while messagebox is being shown, status is not
    // updated since this can cause other problems. Note the flag will probably never be set at this point
    // but after having a problem with a messagebox showing 15+ times, I want to make sure it never happens
    bool lastState = enableUpdateMovieChangeStatus;
    enableUpdateMovieChangeStatus = false;

    Global::sendAlert(possibleErrorMsg);

    if (QMessageBox::question(this, tr("Movie Change"), possibleErrorMsg, QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
    {
      enableUpdateMovieChangeStatus = lastState;

      QLOG_DEBUG() << QString("User chose to ignore error and continue with movie change");

      forceMovieChange = true;

      casServer->startMovieChangePrepare(movieChangeQueue[currentMovieChangeIndex], onlyBoothAddresses, forceMovieChange, movieChangeQueue[currentMovieChangeIndex].Year() >= settings->getValue(DELETE_MOVIE_YEAR).toInt());

      stillBusy = true;
    }
    else
    {
      QLOG_DEBUG() << QString("User chose NOT to continue with movie change");

      dbMgr->deleteMovieChanges(movieChangeQueue[currentMovieChangeIndex].Year(), movieChangeQueue[currentMovieChangeIndex].WeekNum());

      resetMovieChangeProcess();      
    }

    possibleErrorMsg.clear();
  }
  else if (!errorMsg.isEmpty())
  {
    // Clear flag so while messagebox is being shown, status is not updated since this can cause other problems
    enableUpdateMovieChangeStatus = false;

    Global::sendAlert(errorMsg);

    QMessageBox::warning(this, tr("Movie Change"), errorMsg);

    errorMsg.clear();

    dbMgr->deleteMovieChanges(movieChangeQueue[currentMovieChangeIndex].Year(), movieChangeQueue[currentMovieChangeIndex].WeekNum());

    resetMovieChangeProcess();
  }
  else
  {
    // Reset flag
    movieChangeCancelCleanup = false;

    // If we made it this far, at least one booth online and current on movie changes
    // Get list of booth IP addresses that are not excluded and
    // contact this list of booths to start the movie change
    casServer->startMovieChangeClients(getBoothsNotExcluded());

    if (movieChangeQueue[currentMovieChangeIndex].Year() >= settings->getValue(DELETE_MOVIE_YEAR).toInt())
    {
      QMessageBox::information(this, tr("Delete Video"), tr("Please note that the selected channels/videos will not be removed from the list under the Movie Library tab until ALL booths have finished deleting these."));
    }

    stillBusy = true;
  }

  busy = stillBusy;
}

void MovieChangeWidget::movieChangeStartSuccess(QString ipAddress)
{
  // Booth successfully started the movie change process
  QLOG_DEBUG() << QString("Started movie change on booth %1").arg(ipAddress);

  int row = findMovieChangeDetail(ipAddress);

  if (row > -1)
  {
    QStandardItem *scheduledStatusField = movieChangeDetailModel->item(row, Scheduled);
    QStandardItem *downloadFinishedField = movieChangeDetailModel->item(row, Download_Finished);
    QStandardItem *moviesChangedField = movieChangeDetailModel->item(row, Movies_Changed);

    scheduledStatusField->setText("True");
    scheduledStatusField->setData("True");

    // These fields are cleared in case uftp is being started again for booths that failed during the initial uftp session
    downloadFinishedField->setText("");
    downloadFinishedField->setData(0);

    moviesChangedField->setText("");
    moviesChangedField->setData(0);
  }
  else
  {
    QLOG_ERROR() << QString("Received movie change start success from unexpected IP address: %1").arg(ipAddress);
  }
}

void MovieChangeWidget::movieChangeStartFailed(QString ipAddress)
{
  // A booth failing to start a movie change would be one that *did* respond to the ping command and was
  // up to date on movie changes, so the unit must've been reset, a network error or database issue
  // This will later be changed to Excluded in movieChangeExcluded
  QLOG_DEBUG() << QString("Failed to start movie change on booth %1").arg(ipAddress);

  int row = findMovieChangeDetail(ipAddress);

  if (row > -1)
  {
    QStandardItem *scheduledStatusField = movieChangeDetailModel->item(row, Scheduled);
    QStandardItem *downloadFinishedField = movieChangeDetailModel->item(row, Download_Finished);
    QStandardItem *moviesChangedField = movieChangeDetailModel->item(row, Movies_Changed);

    scheduledStatusField->setText("Failed");
    scheduledStatusField->setData("Failed");

    // These fields are cleared in case uftp is being started again for booths that failed during the initial uftp session
    downloadFinishedField->setText("");
    downloadFinishedField->setData(0);

    moviesChangedField->setText("");
    moviesChangedField->setData(0);
  }
  else
  {
    QLOG_ERROR() << QString("Received movie change start failure from unexpected IP address: %1").arg(ipAddress);
  }
}

void MovieChangeWidget::movieChangeClientsFinished()
{
  // If at least one booth has True set for Scheduled field then start the uftp server
  if (getBoothsStartedMovieChange().count() > 0)
  {
    QLOG_DEBUG() << "One or more booths started the movie change process, ready to start the copy process on our end";

    // Exclude from the movie change any booth that failed to start uftp client
    if (getBoothsStartFailed().count() > 0)
    {
      QLOG_ERROR() << QString("The following booths did not start the uftp client: %1").arg(getBoothsStartFailed().join(", "));
      Global::sendAlert(QString("The following booths did not start the uftp client: %1").arg(getBoothsStartFailed().join(", ")));

      foreach (QString ipAddress, getBoothsStartFailed())
        movieChangeExcluded(ipAddress);
    }

    startCopyingMovieChangeSet();
  }
  else
  {
    if (getBoothsFinishedUftp().count() == 0)
    {
      QLOG_ERROR() << QString("No booth started the movie change process, canceling movie change now");
      Global::sendAlert(QString("No booth started the movie change process, canceling movie change now"));

      updateMovieChangeQueueStatus(MovieChangeInfo::Failed);

      // No booth finished the uftp session, send cancel signal and delete movie change records from all databases
      // when this finishes it will call the reset movie change method
      movieChangeCancelCleanup = true;
      casServer->startMovieChangeCancel(getBoothsStartFailed(), movieChangeQueue[currentMovieChangeIndex].Year(), movieChangeQueue[currentMovieChangeIndex].WeekNum(), movieChangeCancelCleanup);

      QMessageBox::warning(this, tr("Movie Change"), tr("None of the booths could start the movie change process. Please contact tech support."));
    }
    else
    {
      QLOG_DEBUG() << QString("Could not start the movie change process in booths: %1, but waiting for the following booths to finish the movie change process: %2").arg(getBoothsStartFailed().join(", ")).arg(getBoothsFinishedUftp().join(", "));
      Global::sendAlert(QString("Could not start the movie change process in booths: %1, but waiting for the following booths to finish the movie change process: %2").arg(getBoothsStartFailed().join(", ")).arg(getBoothsFinishedUftp().join(", ")));

      // This was a retry and one or more booths already finished downloading files via uftp so exclude the ones that failed and wait for the other booths
      updateMovieChangeQueueStatus(MovieChangeInfo::Waiting_For_Booths);

      foreach (QString ipAddress, getBoothsStartFailed())
        movieChangeExcluded(ipAddress);

      // Stop movie change process only at these booths but do not delete /media/download/temp
      // The reasoning behind this is that these booths may have started the copy process so we might
      // as well keep any files around to speed up the sync process later
      movieChangeCancelCleanup = false;
      casServer->startMovieChangeCancel(getBoothsStartFailed(), movieChangeQueue[currentMovieChangeIndex].Year(), movieChangeQueue[currentMovieChangeIndex].WeekNum(), movieChangeCancelCleanup);

      // No need to start the checkStatusTimer timer since it was already started the first time uftp finished
    }

  } // endif no booth started movie change process
}

void MovieChangeWidget::movieChangeExcluded(QString ipAddress)
{
  // An excluded booth is one that did not respond to the ping command, behind on movie changes, did not respond to
  // a movie change retry command or reached max movie change retries
  QLOG_DEBUG() << QString("Excluding booth: %1 from movie change").arg(ipAddress);

  int row = findMovieChangeDetail(ipAddress);

  if (row > -1)
  {
    QStandardItem *scheduledStatusField = movieChangeDetailModel->item(row, Scheduled);
    scheduledStatusField->setText("Excluded");
    scheduledStatusField->setData("Excluded");
  }
  else
  {
    QLOG_ERROR() << QString("Received movie change exclusion from unexpected IP address: %1").arg(ipAddress);
  }
}

void MovieChangeWidget::startCopyingMovieChangeSet()
{
  // Archive dvd copy tasks
  if (dbMgr->archiveDvdCopyTasks(movieChangeQueue[currentMovieChangeIndex].Year(), movieChangeQueue[currentMovieChangeIndex].WeekNum()))
    QLOG_DEBUG() << QString("Archived dvd copy tasks for year: %1, week #: %2").arg(movieChangeQueue[currentMovieChangeIndex].Year()).arg(movieChangeQueue[currentMovieChangeIndex].WeekNum());
  else
  {
    QLOG_ERROR() << QString("Could not archive dvd copy tasks for year: %1, week #: %2").arg(movieChangeQueue[currentMovieChangeIndex].Year()).arg(movieChangeQueue[currentMovieChangeIndex].WeekNum());
    Global::sendAlert(QString("Could not archive dvd copy tasks for year: %1, week #: %2").arg(movieChangeQueue[currentMovieChangeIndex].Year()).arg(movieChangeQueue[currentMovieChangeIndex].WeekNum()));
  }

  // Show the channel # assigned to each of the movies in the movie change set and what movie (if any) was previous assigned
  QVariantList pendingMovieChanges = dbMgr->getPendingMovieChange(movieChangeQueue[currentMovieChangeIndex].Year() >= settings->getValue(DELETE_MOVIE_YEAR).toInt());

  foreach (QVariant v, pendingMovieChanges)
  {
    QVariantMap movieChange = v.toMap();

    if (movieChangeQueue[currentMovieChangeIndex].Year() < settings->getValue(DELETE_MOVIE_YEAR).toInt())
    {
      int row = findPendingMovieChange(movieChange["upc"].toString());

      if (row > -1)
      {
        QStandardItem *channelField = pendingMovieChangeModel->item(row, Channel_Num);
        channelField->setText(movieChange["channel_num"].toString());
        channelField->setData(movieChange["channel_num"].toInt());

        QStandardItem *previousMovieField = pendingMovieChangeModel->item(row, Previous_Movie);
        previousMovieField->setText(movieChange["previous_movie"].toString());
        previousMovieField->setData(movieChange["previous_movie"].toString());
      }
      else
      {
        QLOG_ERROR() << QString("Could not find UPC: %1 among the pending movie changes so channel # cannot be displayed").arg(movieChange["upc"].toString());
      }
    }
    else
    {
      DvdCopyTask task;
      task.setUPC(movieChange["upc"].toString());
      task.setTitle(movieChange["title"].toString());
      task.setCategory(movieChange["category"].toString());
      task.setYear(movieChange["year"].toInt());
      task.setWeekNum(movieChange["week_num"].toInt());

      insertMovieChangeScheduled(movieChange["scheduled_date"].toDateTime(), task, movieChange["channel_num"].toInt(), movieChange["previous_movie"].toString());
    }

    // TODO: The scheduleDate should reflect when the copy process begins so when retrying to copy movies, we aren't using the
    // original time it started

    // Get the scheduled date that was stored in the moviechanges database, this is used later when
    // checking the movie change status. Each loop iteration returns the same value for scheduled_date
    scheduledDate = movieChange["scheduled_date"].toDateTime();
  }

  // Start uftp server if this is not a delete only movie change
  if (movieChangeQueue[currentMovieChangeIndex].Year() < settings->getValue(DELETE_MOVIE_YEAR).toInt())
  {
    QLOG_DEBUG() << "Starting uftp server";
    uftpServer->start(settings->getValue(TRANSMISSION_SPEED).toInt(), getVideoDirectoryListing(movieChangeQueue[currentMovieChangeIndex].Year(), movieChangeQueue[currentMovieChangeIndex].WeekNum()));

    // If current status is not Retrying then change to Copying
    if (movieChangeQueue[currentMovieChangeIndex].Status() != MovieChangeInfo::Retrying)
      updateMovieChangeQueueStatus(MovieChangeInfo::Copying);
  }
  else
  {
    // Jump straight to finished
    finishedUftp(UftpServer::Success, QString(), QStringList(), getBoothsNotExcluded());
  }

  // Movie change is in progress so movie change status can be shown
  enableUpdateMovieChangeStatus = true;
}

void MovieChangeWidget::deleteReplacedMoviesOnServer()
{
  QLOG_DEBUG() << QString("Checking movie change set that was sent to booths for movies that can be deleted from server");

  QStringList deleteUPCs;

  // Get movies in the movie change set that was sent out and check if any are replacing existing channels
  QVariantList pendingMovieChanges = dbMgr->getPendingMovieChange(movieChangeQueue[currentMovieChangeIndex].Year() >= settings->getValue(DELETE_MOVIE_YEAR).toInt());

  foreach (QVariant v, pendingMovieChanges)
  {
    QVariantMap movieChange = v.toMap();

    // If a movie was replaced at the booth then add to a list of videos to delete from the server
    if (!movieChange["previous_movie"].toString().isEmpty() && movieChange["previous_movie"].toString().at(0).isDigit())
    {
      // previous_movie field layout: UPC / Title / Producer / Category
      QStringList previousMovieFields = movieChange["previous_movie"].toString().split("/");

      if (previousMovieFields.count() > 0)
      {
        QString previousUPC = previousMovieFields.at(0).trimmed();

        QRegExp re("\\d*");  // a digit (\d), zero or more times (*)
        if (previousUPC.length() > 0 && re.exactMatch(previousUPC))
        {
          deleteUPCs.append(previousUPC);
        }
        else
        {
          QLOG_ERROR() << QString("Previous movie does not seem to have a valid UPC: %1").arg(movieChange["previous_movie"].toString());
          Global::sendAlert(QString("Previous movie does not seem to have a valid UPC: %1").arg(movieChange["previous_movie"].toString()));
        }
      }
      else
      {
        QLOG_ERROR() << QString("Could not determine UPC from previous movie field to delete it from server: %1").arg(movieChange["previous_movie"].toString());
        Global::sendAlert(QString("Could not determine UPC from previous movie field to delete it from server: %1").arg(movieChange["previous_movie"].toString()));
      }
    }
  }

  // If any current movie is getting deleted at the booth then delete the same movies from the server
  // to save disk space
  foreach (QString upc, deleteUPCs)
  {
    DvdCopyTask archivedTask = dbMgr->getArchivedDvdCopyTask(upc);

    if (!archivedTask.UPC().isEmpty())
    {
      // Build path to video directory to delete it
      QDir videoPath(QString("%1/%2/%3/%4").arg(settings->getValue(VIDEO_PATH).toString()).arg(archivedTask.Year()).arg(archivedTask.WeekNum()).arg(archivedTask.UPC()));

      // TODO: It would be nice to also delete the parent directory if no other UPCs exist
      // Also if no other week_num directories exist then delete the year directory
      if (FileHelper::recursiveRmdir(videoPath.absolutePath()))
      {
        QLOG_DEBUG() << QString("Deleted old movie from server since its being deleted from booths: %1").arg(videoPath.absolutePath());
      }
      else
      {
        QLOG_ERROR() << QString("Could not delete video from server: %1").arg(videoPath.absolutePath());

        Global::sendAlert(QString("Could not delete video from server: %1").arg(videoPath.absolutePath()));
      }
    }
    else
    {
      QLOG_ERROR() << QString("Could not find UPC in archived DVD copy jobs to delete from server: %1").arg(upc);

      Global::sendAlert(QString("Could not find UPC in archived DVD copy jobs to delete from server: %1").arg(upc));
    }
  }
}

void MovieChangeWidget::finalizeMovieChange()
{
  QLOG_DEBUG() << QString("Finalizing movie change");

  // Delete movie change from queue in database since we no longer need to track it
  if (!dbMgr->deleteMovieChangeQueue(movieChangeQueue[currentMovieChangeIndex]))
    Global::sendAlert(QString("Could not delete movie change queue item from database. Year: %1, Week #: %2").arg(movieChangeQueue[currentMovieChangeIndex].Year()).arg(movieChangeQueue[currentMovieChangeIndex].WeekNum()));

  // Update when the last movie change finished
  settings->setValue(LAST_MOVIE_CHANGE, QDateTime::currentDateTime());

  // Save settings
  dbMgr->setValue("all_settings", settings->getSettings());  

  deleteReplacedMoviesOnServer();

  // Convert IP addresses to device addresses that finished movie change
  QStringList deviceAddressList;
  foreach (QString address, getBoothsFinishedMovieChange())
    deviceAddressList.append(casServer->ipAddressToDeviceNum(address));

  MovieChangeInfo currentMovieChange = movieChangeQueue[currentMovieChangeIndex];

  if (dbMgr->archiveMovieChanges(currentMovieChange.Year(), currentMovieChange.WeekNum()))
  {
    QLOG_DEBUG() << QString("Archived movie change records in main database");

    // Only booths that finished the movie change have their records archived
    if (dbMgr->archiveBoothMovieChanges(deviceAddressList, currentMovieChange.Year(), currentMovieChange.WeekNum()))
    {
      QLOG_DEBUG() << QString("Archived movie change records in all booth databases");
    }
    else
    {
      QLOG_ERROR() << QString("Could not archive movie change records in moviechanges_*.sqlite. Year: %1, Week #: %2, Addresses: %3").arg(currentMovieChange.Year()).arg(currentMovieChange.WeekNum()).arg(casServer->getDeviceAddresses().join(", "));

      Global::sendAlert(QString("Could not archive movie change records in moviechanges_*.sqlite. Year: %1, Week #: %2, Addresses: %3").arg(currentMovieChange.Year()).arg(currentMovieChange.WeekNum()).arg(casServer->getDeviceAddresses().join(", ")));
    }

    // Update videos table with latest movie change
    if (!dbMgr->updateVideosFromMovieChange(currentMovieChange.Year(), currentMovieChange.WeekNum()))
    {
      QLOG_ERROR() << QString("Could not update videos table. Year: %1, Week #: %2").arg(currentMovieChange.Year()).arg(currentMovieChange.WeekNum());

      Global::sendAlert(QString("Could not update videos table. Year: %1, Week #: %2").arg(currentMovieChange.Year()).arg(currentMovieChange.WeekNum()));
    }
  }
  else
  {
    QLOG_ERROR() << QString("Could not archive movie change records in moviechanges.sqlite. Year: %1, Week #: %2").arg(currentMovieChange.Year()).arg(currentMovieChange.WeekNum());

    Global::sendAlert(QString("Could not archive movie change records in moviechanges.sqlite. Year: %1, Week #: %2").arg(currentMovieChange.Year()).arg(currentMovieChange.WeekNum()));
  }

  // Trigger databases to be downloaded from all booths and reload video library datagrid to
  // reflect the movie change set just sent to the booths
  emit finishedMovieChange();

  // Clear datagrids
  clearMovieChangeBoothStatus();
  clearMovieChangeScheduled();

  // if at the end of the queue then we don't need to wait to check it again
  if (++currentMovieChangeIndex >= movieChangeQueue.count())
  {
    checkMovieChangeQueue();
  }
  // else
  else
  {
    QLOG_DEBUG() << QString("Another movie change set is in the queue, waiting 120 seconds before starting...");

    // Set timer to call checkMovieChangeQueue in 2 minutes, hopefully all booths have restarted in that time

    // TODO: This will work but is not ideal because there is no guarantee that all booths will reboot immediately,
    // some could be in use for a long time. Need to instead check at some interval to determine when all booths
    // have actually restarted before beginning the next movie change in the queue
    QTimer::singleShot(120000, this, SLOT(checkMovieChangeQueue()));
  }
}

void MovieChangeWidget::movieChangeRetrySuccess(QString ipAddress)
{
  QLOG_DEBUG() << QString("Started movie change retry on booth %1").arg(ipAddress);

  movieChangeRetrySuccessList.append(ipAddress);

  int row = findMovieChangeDetail(ipAddress);

  if (row > -1)
  {
    QStandardItem *scheduledStatusField = movieChangeDetailModel->item(row, Scheduled);
    scheduledStatusField->setText("True");
    scheduledStatusField->setData("True");
  }
  else
  {
    QLOG_ERROR() << QString("Received movie change retry success from unexpected IP address: %1").arg(ipAddress);
  }
}

void MovieChangeWidget::movieChangeRetryFailed(QString ipAddress)
{
  QLOG_DEBUG() << QString("Failed to start movie change retry on booth %1").arg(ipAddress);

  movieChangeRetryFailureList.append(ipAddress);

  int row = findMovieChangeDetail(ipAddress);

  if (row > -1)
  {
    QStandardItem *scheduledStatusField = movieChangeDetailModel->item(row, Scheduled);
    scheduledStatusField->setText("Failed");
    scheduledStatusField->setData("Failed");
  }
  else
  {
    QLOG_ERROR() << QString("Received movie change retry failure from unexpected IP address: %1").arg(ipAddress);
  }
}

void MovieChangeWidget::movieChangeRetryFinished()
{
  QLOG_DEBUG() << QString("Finished sending movie change retry command to booth(s). Success list count: %1, Failure list count: %2")
                  .arg(movieChangeRetrySuccessList.count())
                  .arg(movieChangeRetryFailureList.count());

  if (movieChangeRetrySuccessList.count() > 0)
  {
    QLOG_DEBUG() << "Starting uftp server for movie change retry";

    uftpServer->start(settings->getValue(TRANSMISSION_SPEED).toInt(), QStringList(), true);

    if (movieChangeRetryFailureList.count() > 0)
    {
      foreach (QString ipAddress, movieChangeRetryFailureList)
        movieChangeExcluded(ipAddress);

      Global::sendAlert(QString("Movie change retry could not be started in the following booth(s): %1").arg(movieChangeRetryFailureList.join(", ")));
    }
  }
  else
  {
    QLOG_DEBUG() << "No booths in success list, not starting uftp server for movie change retry";

    // If haven't reached max retries then try again in a few minutes
    if (numMovieChangeRetries++ < settings->getValue(MAX_MOVIECHANGE_RETRIES).toInt())
    {
      QLOG_DEBUG() << QString("Scheduled movie change retry in %1 seconds").arg(movieChangeRetryTimer->interval() / 1000);

      movieChangeRetryTimer->start();
    }
    else
    {
      // give up uftp restart

      foreach (QString ipAddress, movieChangeRetryFailureList)
        movieChangeExcluded(ipAddress);

      QLOG_DEBUG() << QString("Reached maximum movie change retries, giving up on movie change");

      Global::sendAlert(QString("Movie change retry could not be started in these booths after multiple attempts: %1").arg(movieChangeRetryFailureList.join(", ")));
    }
  }
}

void MovieChangeWidget::movieChangeCancelSuccess(QString ipAddress)
{
  movieChangeCancelSuccessList.append(ipAddress);
}

void MovieChangeWidget::movieChangeCancelFailed(QString ipAddress)
{
  movieChangeCancelFailureList.append(ipAddress);
}

void MovieChangeWidget::movieChangeCancelFinished()
{
  QLOG_DEBUG() << QString("Finished canceling movie change");

  if (movieChangeCancelFailureList.count() > 0)
  {
    QLOG_ERROR() << QString("Could not cancel movie change in the following booth(s): %1. Successfully canceled the movie change in the following booth(s): %2").arg(movieChangeCancelFailureList.join(", ")).arg(movieChangeCancelSuccessList.join(", "));
    Global::sendAlert(QString("Could not cancel movie change in the following booth(s): %1. Successfully canceled the movie change in the following booth(s): %2").arg(movieChangeCancelFailureList.join(", ")).arg(movieChangeCancelSuccessList.join(", ")));
  }

  movieChangeCancelSuccessList.clear();
  movieChangeCancelFailureList.clear();

  // If flag is set then it means we contacted all booths to cancel the movie change and
  // continue with resetting movie change process
  if (movieChangeCancelCleanup)
  {
    // Restore archived records since the movie change is completely being canceled
    if (!dbMgr->restoreDvdCopyTasks(movieChangeQueue[currentMovieChangeIndex].Year(), movieChangeQueue[currentMovieChangeIndex].WeekNum()))
    {
      Global::sendAlert(QString("Could not restore dvd copy job records for year: %1, week #: %2").arg(movieChangeQueue[currentMovieChangeIndex].Year()).arg(movieChangeQueue[currentMovieChangeIndex].WeekNum()));
    }

    resetMovieChangeProcess();
  }
}

void MovieChangeWidget::insertMovieChangeQueue(MovieChangeInfo movieChange)
{ 
  // # Movies, Status, Year, Week #
  QStandardItem *numMoviesField, *statusField, *yearField, *weekNumField;

  numMoviesField = new QStandardItem(QString("%1").arg(movieChange.DvdCopyTaskList().count()));
  numMoviesField->setData(movieChange.DvdCopyTaskList().count());

  statusField = new QStandardItem(MovieChangeInfo::MovieChangeStatusToString(movieChange.Status()));
  statusField->setData(movieChange.Status());

  yearField = new QStandardItem(QString("%1").arg(movieChange.Year()));
  yearField->setData(movieChange.Year());

  weekNumField = new QStandardItem(QString("%1").arg(movieChange.WeekNum()));
  weekNumField->setData(movieChange.WeekNum());

  int row = movieChangeQueueModel->rowCount();

  movieChangeQueueModel->setItem(row, Num_Movies, numMoviesField);
  movieChangeQueueModel->setItem(row, Status, statusField);
  movieChangeQueueModel->setItem(row, Queue_Year, yearField);
  movieChangeQueueModel->setItem(row, Queue_Week_Num, weekNumField);
}

void MovieChangeWidget::insertMovieChangeScheduled(QDateTime scheduledDate, DvdCopyTask task, int channelNum, QString previousMovie)
{
  // Scheduled Date, Week #, UPC, Title, Category, Channel #, Previous Movie, Year
  QStandardItem *scheduledDateField, *weekNumField, *upcField, *titleField, *categoryField, *channelField, *yearField, *previousMovieField;

  scheduledDateField = new QStandardItem(scheduledDate.toString("MM/dd/yyyy h:mm AP"));
  scheduledDateField->setData(scheduledDate);

  weekNumField = new QStandardItem(QString("%1").arg(task.WeekNum()));
  weekNumField->setData(task.WeekNum());

  upcField = new QStandardItem(task.UPC());
  upcField->setData(task.UPC());

  titleField = new QStandardItem(task.Title());
  titleField->setData(task.Title());

  categoryField = new QStandardItem(task.Category());
  categoryField->setData(task.Category());

  QString channelNumText = "";
  if (channelNum > 0)
    channelNumText = QString("%1").arg(channelNum);

  channelField = new QStandardItem(channelNumText);
  channelField->setData(channelNum);

  previousMovieField = new QStandardItem(previousMovie.isEmpty() ? "" : previousMovie);
  previousMovieField->setData(previousMovie.isEmpty() ? "" : previousMovie);

  yearField = new QStandardItem(task.Year());
  yearField->setData(task.Year());

  int row = pendingMovieChangeModel->rowCount();

  pendingMovieChangeModel->setItem(row, Scheduled_Date, scheduledDateField);
  pendingMovieChangeModel->setItem(row, Week_Num, weekNumField);
  pendingMovieChangeModel->setItem(row, UPC, upcField);
  pendingMovieChangeModel->setItem(row, Title, titleField);
  pendingMovieChangeModel->setItem(row, Category, categoryField);
  pendingMovieChangeModel->setItem(row, Channel_Num, channelField);
  pendingMovieChangeModel->setItem(row, Previous_Movie, previousMovieField);
  pendingMovieChangeModel->setItem(row, Year, yearField);
}

void MovieChangeWidget::insertMovieChangeBoothStatus(QString deviceAlias, QString deviceType, QString ipAddress, qreal freeSpace, int numChannels, QString scheduled, QDateTime downloadStarted, QDateTime downloadFinished, QDateTime moviesChanged)
{
  // Name, Type, Address, Free Space, # Channels, Scheduled, UFTP Finished, Download Started, Download Finished, Movies Changed
  QStandardItem *deviceAliasField, *ipAddressField, *boothTypeField, *freeSpaceField, *numChannelsField, *scheduledStatusField, *downloadStartedField, *downloadFinishedField, *moviesChangedField, *uftpFinishedField;

  deviceAliasField = new QStandardItem(deviceAlias);
  deviceAliasField->setData(deviceAlias);

  ipAddressField = new QStandardItem(ipAddress);
  ipAddressField->setData(ipAddress);

  boothTypeField = new QStandardItem(deviceType);
  boothTypeField->setData(deviceType);

  freeSpaceField = new QStandardItem(QString("%1 GB").arg(freeSpace));
  freeSpaceField->setData(freeSpace);

  numChannelsField = new QStandardItem(QString("%1").arg(numChannels));
  numChannelsField->setData(numChannels);

  scheduledStatusField = new QStandardItem(scheduled);
  scheduledStatusField->setData(scheduled);

  downloadStartedField = new QStandardItem(downloadStarted.isValid() ? downloadStarted.toString("MM/dd/yyyy h:mm AP") : "");
  downloadStartedField->setData(downloadStarted);

  uftpFinishedField = new QStandardItem;

  downloadFinishedField = new QStandardItem(downloadFinished.isValid() ? downloadFinished.toString("MM/dd/yyyy h:mm AP") : "");
  downloadFinishedField->setData(downloadFinished);

  moviesChangedField = new QStandardItem(moviesChanged.isValid() ? moviesChanged.toString("MM/dd/yyyy h:mm AP") : "");
  moviesChangedField->setData(moviesChanged);

  int row = movieChangeDetailModel->rowCount();

  movieChangeDetailModel->setItem(row, Name, deviceAliasField);
  movieChangeDetailModel->setItem(row, Booth_Type, boothTypeField);
  movieChangeDetailModel->setItem(row, Address, ipAddressField);
  movieChangeDetailModel->setItem(row, Free_Space, freeSpaceField);
  movieChangeDetailModel->setItem(row, Total_Channels, numChannelsField);
  movieChangeDetailModel->setItem(row, Scheduled, scheduledStatusField);
  movieChangeDetailModel->setItem(row, Download_Started, downloadStartedField);
  movieChangeDetailModel->setItem(row, UFTP_Finished, uftpFinishedField);
  movieChangeDetailModel->setItem(row, Download_Finished, downloadFinishedField);
  movieChangeDetailModel->setItem(row, Movies_Changed, moviesChangedField);
}

void MovieChangeWidget::clearMovieChangeQueue()
{
  dbMgr->clearMovieChangeQueue();

  if (movieChangeQueueModel->rowCount() > 0)
    movieChangeQueueModel->removeRows(0, movieChangeQueueModel->rowCount());
}

void MovieChangeWidget::clearMovieChangeScheduled()
{
  if (pendingMovieChangeModel->rowCount() > 0)
    pendingMovieChangeModel->removeRows(0, pendingMovieChangeModel->rowCount());
}

void MovieChangeWidget::clearMovieChangeBoothStatus()
{
  if (movieChangeDetailModel->rowCount() > 0)
    movieChangeDetailModel->removeRows(0, movieChangeDetailModel->rowCount());
}

int MovieChangeWidget::findMovieChangeDetail(QString ipAddress)
{
  int row = -1;

  // Look for a matching address
  QList<QStandardItem *> resultList = movieChangeDetailModel->findItems(ipAddress, Qt::MatchExactly, Address);

  if (resultList.count() > 0)
    row = resultList.first()->row();

  return row;
}

int MovieChangeWidget::findPendingMovieChange(QString upc)
{
  int row = -1;

  // Look in UPC column for a match
  QList<QStandardItem *> resultList = pendingMovieChangeModel->findItems(upc, Qt::MatchExactly, UPC);

  if (resultList.count() > 0)
    row = resultList.first()->row();

  return row;
}

int MovieChangeWidget::findMovieChangeQueue(MovieChangeInfo movieChange)
{
  int row = -1;

  QList<QStandardItem *> resultList = movieChangeQueueModel->findItems(QString("%1").arg(movieChange.WeekNum()), Qt::MatchExactly, Queue_Week_Num);

  if (resultList.count() > 0)
  {
    // Further filter the result list using the year even though
    // the chances of the same week # from different years being in
    // the queue are highly unlikely
    foreach (QStandardItem *item, resultList)
    {
      if (movieChangeQueueModel->item(item->row(), Queue_Year)->data().toInt() == movieChange.Year())
      {
        row = item->row();
        break;
      }
    }
  }

  return row;
}

void MovieChangeWidget::updateMovieChangeStatus()
{
  bool stillBusy = false;
  bool enableButton = true;

  if (enableUpdateMovieChangeStatus)
  {
    enableUpdateMovieChangeStatus = false;

    if (pendingMovieChangeModel->rowCount() > 0)
    {
      QVariantList boothList = dbMgr->getOnlyBoothInfo(casServer->getDeviceAddresses());

      QStringList participatingBooths = getBoothsNotExcluded();
      foreach (QVariant v, boothList)
      {
        QVariantMap booth = v.toMap();

        // Only update fields for booth if it's not excluded from the movie change
        if (participatingBooths.contains(settings->getValue(ARCADE_SUBNET).toString() + booth["device_num"].toString()))
        {
          int row = findMovieChangeDetail(settings->getValue(ARCADE_SUBNET).toString() + booth["device_num"].toString());

          if (row > -1)
          {
            //QStandardItem *scheduledStatusField = movieChangeDetailModel->item(row, 5);
            QStandardItem *downloadStartedField = movieChangeDetailModel->item(row, Download_Started);
            QStandardItem *downloadFinishedField = movieChangeDetailModel->item(row, Download_Finished);
            QStandardItem *moviesChangedField = movieChangeDetailModel->item(row, Movies_Changed);

            if (booth.contains("scheduled"))
            {
              // Always compare scheduledDate to the various event times to make sure we aren't looking at an old
              // copy of the booth's database. If a booth has been offline then its database is not going to be current on the server
              if (booth["download_started"].toDateTime().isValid() && scheduledDate <= booth["download_started"].toDateTime())
              {
                if (downloadStartedField->text().isEmpty())
                {
                  QLOG_DEBUG() << QString("Booth: %1, Download Started: %2").arg(booth["device_num"].toString()).arg(booth["download_started"].toDateTime().toString("MM/dd/yyyy h:mm AP"));
                  downloadStartedField->setText(booth["download_started"].toDateTime().toString("MM/dd/yyyy h:mm AP"));
                  downloadStartedField->setData(booth["download_started"].toDateTime());
                }
              }

              if (booth["download_finished"].toDateTime().isValid() && scheduledDate <= booth["download_finished"].toDateTime())
              {
                if (downloadFinishedField->text().isEmpty())
                {
                  QLOG_DEBUG() << QString("Booth: %1, Download Finished: %2").arg(booth["device_num"].toString()).arg(booth["download_finished"].toDateTime().toString("MM/dd/yyyy h:mm AP"));
                  downloadFinishedField->setText(booth["download_finished"].toDateTime().toString("MM/dd/yyyy h:mm AP"));
                  downloadFinishedField->setData(booth["download_finished"].toDateTime());
                }
              }

              if (booth["movies_changed"].toDateTime().isValid() && scheduledDate <= booth["movies_changed"].toDateTime())
              {
                if (moviesChangedField->text().isEmpty())
                {
                  QLOG_DEBUG() << QString("Booth: %1, Movies Changed: %2").arg(booth["device_num"].toString()).arg(booth["movies_changed"].toDateTime().toString("MM/dd/yyyy h:mm AP"));
                  moviesChangedField->setText(booth["movies_changed"].toDateTime().toString("MM/dd/yyyy h:mm AP"));
                  moviesChangedField->setData(booth["movies_changed"].toDateTime());
                }
              }
            }
          }
          else
          {
            QLOG_ERROR() << QString("Could not find booth: %1 in the detail view for updating status").arg(booth["device_num"].toString());
          }

        } // endif booth is participating

      } // end for each booth

      QStringList boothsFinishedMovieChange = getBoothsFinishedMovieChange();

      QLOG_DEBUG() << QString("Booths participating: %1, Booths finished: %2").arg(participatingBooths.join(", ")).arg(boothsFinishedMovieChange.join(", "));

      // If all the booths that are participating in the movie change finished the movie change then
      // we can archive the movie change and clear the datagrids
      if (boothsFinishedMovieChange.count() == participatingBooths.count())
      {
        QLOG_DEBUG() << "All booths participating in the movie change have finished";

        updateMovieChangeQueueStatus(MovieChangeInfo::Finished);

        finalizeMovieChange();

        // Since we're done with the movie change, do not re-enable the Check Status button
        enableButton = false;

      } // endif all booths have finished movie change
      else
        stillBusy = true;

      // If the movie change process has been running longer than n seconds then send an alert
      if (QDateTime::currentDateTime().toTime_t() - scheduledDate.toTime_t() > settings->getValue(MOVIE_CHANGE_DURATION_THRESHOLD).toUInt() &&
          !sentMovieChangeStuckAlert)
      {
        sentMovieChangeStuckAlert = true;

        QLOG_ERROR() << QString("The movie change has been taking longer than %1 minutes").arg(settings->getValue(MOVIE_CHANGE_DURATION_THRESHOLD).toUInt() / 60);

        Global::sendAlert(QString("The movie change has been taking longer than %1 minutes").arg(settings->getValue(MOVIE_CHANGE_DURATION_THRESHOLD).toUInt() / 60));

        // FIXME: Need to end movie change and either move on to next item in queue, cancel movie change or just reset form
      }
    }
    else
    {
      QLOG_DEBUG() << "No movie change pending";
      //checkStatusTimer->stop();

      // Since there's no movie change, do not re-enable the Check Status button
      enableButton = false;
    }
  }

  enableUpdateMovieChangeStatus = enableButton;

  busy = stillBusy;
}

void MovieChangeWidget::queueMovieDelete(MovieChangeInfo &movieChangeInfo)
{
  QLOG_DEBUG() << "Queuing a delete only movie change";

  // This should be the first (and only) movie change in the queue, reset the index to the beginning
  currentMovieChangeIndex = 0;

  // Add movie change set to queue
  movieChangeQueue.append(movieChangeInfo);

  // Add the movie change queue datagrid
  insertMovieChangeQueue(movieChangeInfo);

  checkMovieChangeQueue();
}

void MovieChangeWidget::startedUftp()
{
  QLOG_DEBUG() << "uftp server started, enabling Cancel Movie Change button";

  btnCancelMovieChange->setEnabled(true);
}

void MovieChangeWidget::uftpProcessError()
{
  // There was some problem running the uftp program, it might not exist, crashed, etc.
  Global::sendAlert("An error occurred while trying to start uftp on the server, movie change was aborted");

  updateMovieChangeQueueStatus(MovieChangeInfo::Failed);

  // Build list of all booths (ip addresses) that have not finished the movie change yet
  // Theoretically no booth should have finished because a uftp error most likely occurred almost
  // immediately after starting the process
  QStringList ipAddressList;
  for (int i = 0; i < movieChangeDetailModel->rowCount(); ++i)
  {
    QStandardItem *ipAddressField = movieChangeDetailModel->item(i, Address);
    QStandardItem *scheduledStatusField = movieChangeDetailModel->item(i, Scheduled);
    QStandardItem *uftpFinishedField = movieChangeDetailModel->item(i, UFTP_Finished);

    // Build list of booth IP addresses that were not excluded and have not finished the movie change
    if (scheduledStatusField->data().toString() != "Excluded" && uftpFinishedField->data().isNull())
    {
      ipAddressList.append(ipAddressField->data().toString());
    }
  }

  // Send cancel signal to booths that are part of this movie change and delete movie change records from all databases
  // when this finishes it will call the reset movie change method
  movieChangeCancelCleanup = true;
  casServer->startMovieChangeCancel(ipAddressList, movieChangeQueue[currentMovieChangeIndex].Year(), movieChangeQueue[currentMovieChangeIndex].WeekNum(), movieChangeCancelCleanup);

  QMessageBox::warning(this, tr("Movie Change"), tr("The movie copy process could not be started. Please contact tech support."));
}

void MovieChangeWidget::finishedUftp(UftpServer::UftpStatusCode statusCode, QString errorMessage, QStringList failureList, QStringList successList)
{
  QLOG_DEBUG() << QString("uftp finished with status code: %1, error message: %2, success list size: %3, failure list size: %4")
                  .arg(statusCode)
                  .arg(errorMessage)
                  .arg(successList.count())
                  .arg(failureList.count());

  QLOG_DEBUG() << "Disabling Cancel Movie Change button";
  btnCancelMovieChange->setEnabled(false);

  // Set the UFTP_Finished field to the current time for any booth that is in the success list
  QDateTime uftpFinishedTime = QDateTime::currentDateTime();
  foreach (QString ipAddress, successList)
  {
    int row = findMovieChangeDetail(ipAddress);

    if (row > -1)
    {
      QLOG_DEBUG() << QString("Setting UFTP Finished time to %1 for booth with IP address: %2").arg(uftpFinishedTime.toString("MM/dd/yyyy h:mm AP")).arg(ipAddress);

      QStandardItem *uftpFinishedField = movieChangeDetailModel->item(row, UFTP_Finished);
      uftpFinishedField->setText(uftpFinishedTime.toString("MM/dd/yyyy h:mm AP"));
      uftpFinishedField->setData(uftpFinishedTime);
    }
    else
    {
      // FIXME: Program crashed when it couldn't find 10.0.0.21 in success list even though it appeared to be in the list
      //
      QLOG_ERROR() << QString("Could not find IP address: %1 from movie change success list in the detail datagrid").arg(ipAddress);
    }
  }

  // Seems pointless to set these fields since the retryMovieChange method immediately clears them
  // The only time it's useful is if we give up on retrying movie change
  foreach (QString ipAddress, failureList)
  {
    int row = findMovieChangeDetail(ipAddress);

    if (row > -1)
    {
      QLOG_DEBUG() << QString("Setting Download Finished to Failed for booth with IP address: %1").arg(ipAddress);

      QStandardItem *downloadFinishedField = movieChangeDetailModel->item(row, Download_Finished);
      downloadFinishedField->setText("Failed");
      downloadFinishedField->setData("Failed");

      QStandardItem *moviesChangedField = movieChangeDetailModel->item(row, Movies_Changed);
      moviesChangedField->setText("N/A");
      moviesChangedField->setData("N/A");
    }
    else
    {
      QLOG_ERROR() << QString("Could not find IP address: %1 from movie change failure list in the detail datagrid").arg(ipAddress);
    }
  }

  // Find booths that are in an unknown state, these are booths that were
  // not in the failure or success lists and have not already been excluded
  QStringList boothsUnknownState;

  for (int i = 0; i < movieChangeDetailModel->rowCount(); ++i)
  {
    QStandardItem *ipAddressField = movieChangeDetailModel->item(i, Address);
    QStandardItem *scheduledStatusField = movieChangeDetailModel->item(i, Scheduled);
    QStandardItem *uftpFinishedField = movieChangeDetailModel->item(i, UFTP_Finished);
    QStandardItem *downloadFinishedField = movieChangeDetailModel->item(i, Download_Finished);

    // Booth is in an unknown movie change state if the Scheduled field is currently set to True and
    // UFTP_Finished is empty and Download_Finished and Movies_Changed are empty
    if (scheduledStatusField->data().toString() == "True" && uftpFinishedField->data().isNull() && downloadFinishedField->data().isNull())
    {
      boothsUnknownState.append(ipAddressField->data().toString());
    }
  }

  // Booths in an unknown state will either be excluded if it is a subset of the booths participating or
  // the movie change will be canceled if all booths have an unknown state
  if (boothsUnknownState.count() > 0)
  {
    // If all booths that were not already excluded are in an unknown state then the list success and failure lists were empty
    // Canceling the movie change is the safest action since this might be a new scenario not considered and may need to be investigated
    if (boothsUnknownState.count() == getBoothsNotExcluded().count())
    {
      updateMovieChangeQueueStatus(MovieChangeInfo::Failed);

      QLOG_ERROR() << QString("All %1 booths participating in movie change are in an unknown state after uftp finished, canceling movie change: %2").arg(boothsUnknownState.count()).arg(boothsUnknownState.join(", "));
      Global::sendAlert(QString("All %1 booths participating in movie change are in an unknown state after uftp finished, canceling movie change: %2").arg(boothsUnknownState.count()).arg(boothsUnknownState.join(", ")));

      // Send cancel signal to booths that are part of this movie change and delete movie change records from all databases
      // when this finishes it will call the reset movie change method
      movieChangeCancelCleanup = true;
      casServer->startMovieChangeCancel(boothsUnknownState, movieChangeQueue[currentMovieChangeIndex].Year(), movieChangeQueue[currentMovieChangeIndex].WeekNum(), movieChangeCancelCleanup);

      QMessageBox::warning(this, tr("Movie Change"), tr("The movie copy process had an unknown error. Please contact tech support."));

      return;
    }
    else
    {
      // One or more booths have finishd the uftp session so instead of canceling the entire movie change, exclude the unknown state booths
      // and cancel the movie change process at these booths only

      updateMovieChangeQueueStatus(MovieChangeInfo::Waiting_For_Booths);

      QLOG_ERROR() << QString("Found %1 booth(s) in an unknown state after uftp finished and excluding now from the movie change: %2").arg(boothsUnknownState.count()).arg(boothsUnknownState.join(", "));
      Global::sendAlert(QString("Found %1 booth(s) in an unknown state after uftp finished and excluding now from the movie change: %2").arg(boothsUnknownState.count()).arg(boothsUnknownState.join(", ")));

      foreach (QString ipAddress, boothsUnknownState)
        movieChangeExcluded(ipAddress);

      // Stop movie change process only at these booths but do not delete /media/download/temp
      // The reasoning behind this is that these booths may have started the copy process so we might
      // as well keep any files around to speed up the sync process later
      movieChangeCancelCleanup = false;
      casServer->startMovieChangeCancel(boothsUnknownState, movieChangeQueue[currentMovieChangeIndex].Year(), movieChangeQueue[currentMovieChangeIndex].WeekNum(), movieChangeCancelCleanup);
    }
  }

  // All booths that participated in this uftp session *should* have finished downloading files
  if (statusCode == UftpServer::Success)
  {
    updateMovieChangeQueueStatus(MovieChangeInfo::Waiting_For_Booths);
  }
  else if (statusCode == UftpServer::Fail_Restart_Possible && failureList.count() > 0)
  {
    // Some or all booths that participated in this uftp session failed and the restart file exists so
    // booths in the failure list can resume where they left off if we restart the uftp server with this file

    QLOG_DEBUG() << QString("uftp server reported that we can restart the session with the booths that are in the failure list");

    if (numMovieChangeRetries++ < settings->getValue(MAX_MOVIECHANGE_RETRIES).toInt())
    {
      // Restart movie copy process for failure booths (booths resuming downloading where they left off instead of completely starting over)
      updateMovieChangeQueueStatus(MovieChangeInfo::Retrying);

      // The failure list is referenced in other methods
      movieChangeFailureList = failureList;

      QLOG_DEBUG() << QString("Starting movie change retry in %1 seconds for these booths: %2").arg(movieChangeRetryTimer->interval() / 1000).arg(movieChangeFailureList.join(", "));
      Global::sendAlert(QString("Starting movie change retry in %1 seconds for these booths: %2").arg(movieChangeRetryTimer->interval() / 1000).arg(movieChangeFailureList.join(", ")));

      movieChangeRetryTimer->start();
    }
    else
    {
      // Maximum retries reached, cancel the movie change if no booth has successfully finished the uftp session
      if (getBoothsFinishedUftp().count() == 0)
      {
        QLOG_ERROR() << QString("Reached maximum movie change retries, giving up on movie change");
        Global::sendAlert(QString("Movie change retry could not be started in these booths after multiple attempts: %1").arg(movieChangeRetryFailureList.join(", ")));

        updateMovieChangeQueueStatus(MovieChangeInfo::Failed);

        // No booth finished the uftp session, send cancel signal and delete movie change records from all databases
        // when this finishes it will call the reset movie change method
        movieChangeCancelCleanup = true;
        casServer->startMovieChangeCancel(getBoothsNotExcluded(), movieChangeQueue[currentMovieChangeIndex].Year(), movieChangeQueue[currentMovieChangeIndex].WeekNum(), movieChangeCancelCleanup);

        QMessageBox::warning(this, tr("Movie Change"), tr("The movie copy process had an unknown error. Please contact tech support."));

        return;
      }
      else
      {
        QLOG_DEBUG() << QString("Reached maximum movie change retries for booths: %1 but waiting for the following booths to finish the movie change process: %2").arg(failureList.join(", ")).arg(getBoothsFinishedUftp().join(", "));
        Global::sendAlert(QString("Reached maximum movie change retries for booths: %1 but waiting for the following booths to finish the movie change process: %2").arg(failureList.join(", ")).arg(getBoothsFinishedUftp().join(", ")));

        // One or more booths did finish downloading files via uftp so exclude the ones that failed and wait for the other booths
        updateMovieChangeQueueStatus(MovieChangeInfo::Waiting_For_Booths);

        foreach (QString ipAddress, failureList)
          movieChangeExcluded(ipAddress);

        // Stop movie change process only at these booths but do not delete /media/download/temp
        // The reasoning behind this is that these booths may have started the copy process so we might
        // as well keep any files around to speed up the sync process later
        movieChangeCancelCleanup = false;
        casServer->startMovieChangeCancel(failureList, movieChangeQueue[currentMovieChangeIndex].Year(), movieChangeQueue[currentMovieChangeIndex].WeekNum(), movieChangeCancelCleanup);
      }
    }
  } // endif status code = Fail_Restart_Possible
  else if (statusCode == UftpServer::Fail_Cannot_Restart && failureList.count() > 0)
  {
    QLOG_DEBUG() << QString("uftp server reported that we CANNOT restart the session with the booths that are in the failure list");

    if (numMovieChangeRetries++ < settings->getValue(MAX_MOVIECHANGE_RETRIES).toInt())
    {
      updateMovieChangeQueueStatus(MovieChangeInfo::Retrying);

      QLOG_DEBUG() << QString("Starting movie copy process over for the following booths: %1").arg(failureList.join(", "));
      Global::sendAlert(QString("Starting movie copy process over for the following booths: %1").arg(failureList.join(", ")));

      // start uftp at each booth in failure list, booths will delete /media/download/temp
      casServer->startMovieChangeClients(failureList);
    }
    else
    {
      // Maximum retries reached, cancel the movie change if no booth has successfully finished the uftp session
      if (getBoothsFinishedUftp().count() == 0)
      {
        QLOG_ERROR() << QString("Reached maximum movie change retries, giving up on movie change");
        Global::sendAlert(QString("Movie change could not be started in these booths after multiple attempts: %1").arg(failureList.join(", ")));

        updateMovieChangeQueueStatus(MovieChangeInfo::Failed);

        // No booth finished the uftp session, send cancel signal and delete movie change records from all databases
        // when this finishes it will call the reset movie change method
        movieChangeCancelCleanup = true;
        casServer->startMovieChangeCancel(getBoothsNotExcluded(), movieChangeQueue[currentMovieChangeIndex].Year(), movieChangeQueue[currentMovieChangeIndex].WeekNum(), movieChangeCancelCleanup);

        QMessageBox::warning(this, tr("Movie Change"), tr("The movie copy process had an unknown error. Please contact tech support."));

        return;
      }
      else
      {
        QLOG_DEBUG() << QString("Reached maximum movie change retries for booths: %1 but waiting for the following booths to finish the movie change process: %2").arg(movieChangeRetryFailureList.join(", ")).arg(getBoothsFinishedUftp().join(", "));
        Global::sendAlert(QString("Reached maximum movie change retries for booths: %1 but waiting for the following booths to finish the movie change process: %2").arg(movieChangeRetryFailureList.join(", ")).arg(getBoothsFinishedUftp().join(", ")));

        // One or more booths did finish downloading files via uftp so exclude the ones that failed and wait for the other booths
        updateMovieChangeQueueStatus(MovieChangeInfo::Waiting_For_Booths);

        foreach (QString ipAddress, failureList)
          movieChangeExcluded(ipAddress);

        // Stop movie change process only at these booths but do not delete /media/download/temp
        // The reasoning behind this is that these booths may have started the copy process so we might
        // as well keep any files around to speed up the sync process later
        movieChangeCancelCleanup = false;
        casServer->startMovieChangeCancel(failureList, movieChangeQueue[currentMovieChangeIndex].Year(), movieChangeQueue[currentMovieChangeIndex].WeekNum(), movieChangeCancelCleanup);
      }

    } // endif no more retries

  } // endif status code = Fail_Cannot_Restart
  else if (statusCode == UftpServer::Exit_With_Error || statusCode == UftpServer::Unknown_State)
  {
    // uftp server encountered some kind of an error, do not attempt to restart the process since it
    // will probably just make things worse

    if (QFile::exists(settings->getValue(FILE_SERVER_LOG_FILE).toString()))
    {
      // Some other kind of error was encountered, rename log file for later analysis
      QString newLogFilename = QString("/var/cas-mgr/uftp-%1-%2_%3.log").arg(movieChangeQueue[currentMovieChangeIndex].Year()).arg(movieChangeQueue[currentMovieChangeIndex].WeekNum()).arg(QDateTime::currentDateTime().toTime_t());

      if (QFile::rename(settings->getValue(FILE_SERVER_LOG_FILE).toString(), newLogFilename))
      {
        QLOG_DEBUG() << "Renamed uftp log to:" << newLogFilename;

        Global::sendAlert(QString("Check uftp log file: %1").arg(newLogFilename));
      }
      else
        QLOG_ERROR() << "Could not rename uftp log to:" << newLogFilename;
    }

    // if no booths have UFTP_Finished set then cancel movie change
    if (getBoothsFinishedUftp().count() == 0)
    {
      QLOG_ERROR() << QString("uftp returned an error or is in an unknown state, canceling movie change in all booths: %1").arg(getBoothsNotExcluded().join(", "));
      Global::sendAlert(QString("uftp returned an error or is in an unknown state, canceling movie change in all booths: %1").arg(getBoothsNotExcluded().join(", ")));

      updateMovieChangeQueueStatus(MovieChangeInfo::Failed);

      // No booth finished the uftp session, send cancel signal and delete movie change records from all databases
      // when this finishes it will call the reset movie change method
      movieChangeCancelCleanup = true;
      casServer->startMovieChangeCancel(getBoothsNotExcluded(), movieChangeQueue[currentMovieChangeIndex].Year(), movieChangeQueue[currentMovieChangeIndex].WeekNum(), movieChangeCancelCleanup);

      QMessageBox::warning(this, tr("Movie Change"), tr("The movie copy process had an unknown error. Please contact tech support."));

      return;
    }
    else
    {
      QLOG_DEBUG() << QString("uftp returned an error or is in an unknown state but we cannot cancel the movie change since the following booths have finished downloading the movies: %1").arg(getBoothsFinishedUftp().join(", "));
      Global::sendAlert(QString("uftp returned an error or is in an unknown state but we cannot cancel the movie change since the following booths have finished downloading the movies: %1").arg(getBoothsFinishedUftp().join(", ")));

      // One or more booths did finish downloading files via uftp so exclude the ones that failed and wait for the other booths
      updateMovieChangeQueueStatus(MovieChangeInfo::Waiting_For_Booths);

      foreach (QString ipAddress, getBoothsInUftpSession())
        movieChangeExcluded(ipAddress);

      // Stop movie change process only at these booths but do not delete /media/download/temp
      // The reasoning behind this is that these booths may have started the copy process so we might
      // as well keep any files around to speed up the sync process later
      movieChangeCancelCleanup = false;
      casServer->startMovieChangeCancel(getBoothsInUftpSession(), movieChangeQueue[currentMovieChangeIndex].Year(), movieChangeQueue[currentMovieChangeIndex].WeekNum(), movieChangeCancelCleanup);
    }

  } // endif uftp error or uftp unknown state

  //updateMovieChangeStatus();
}

void MovieChangeWidget::retryMovieChange()
{
  QLOG_DEBUG() << "Retrying movie change";

  // Clear lists used for keeping track of which booths succeeded/failed to execute the movie change retry command
  movieChangeRetrySuccessList.clear();
  movieChangeRetryFailureList.clear();

  // Clear download finished and movies changed fields for failed booths
  foreach (QString ipAddress, movieChangeFailureList)
  {
    int row = findMovieChangeDetail(ipAddress);

    if (row > -1)
    {
      QStandardItem *downloadFinishedField = movieChangeDetailModel->item(row, Download_Finished);
      downloadFinishedField->setText("");
      downloadFinishedField->setData(0);

      QStandardItem *moviesChangedField = movieChangeDetailModel->item(row, Movies_Changed);
      moviesChangedField->setText("");
      moviesChangedField->setData(0);
    }
    else
    {
      QLOG_ERROR() << QString("Could not find IP address: %1 from movie change failure list in the detail datagrid").arg(ipAddress);
    }
  }

  casServer->startMovieChangeRetry(movieChangeFailureList);
}

void MovieChangeWidget::resetMovieChangeProcess()
{
  // Clear datagrids
  // Clear queue
  // stop update status timer
  // return buttons to original state

  movieChangeQueue.clear();
  clearMovieChangeQueue();
  clearMovieChangeBoothStatus();
  clearMovieChangeScheduled();

  //checkStatusTimer->stop();

  enableUpdateMovieChangeStatus = false;
  btnCancelMovieChange->setEnabled(false);

  busy = false;
}

void MovieChangeWidget::cancelMovieChangeClicked()
{
  QLOG_DEBUG() << "User clicked Cancel Movie Change button";

  if (uftpServer->isRunning())
  {
    QString prompt = tr("If you cancel the movie change then the booths will not receive any of the movies in the set. If there are multiple movie change sets in the queue then they will be canceled as well.\n\nAre you sure you want to continue?");

    // If there are some booths that finished the uftp session then canceling only affects the booths performing a retry and doesn't delete
    // any files already downloaded by booths since some booths already have the movies
    if (getBoothsFinishedUftp().count() > 0)
    {
      prompt = tr("Currently retrying to send the movies to one or more booths. Because some booths have already received the movie change, canceling will only skip the booths still downloading the movies but will not reverse the movie change. The booths that do not have the movies yet will be excluded from subsequent movie changes until they get caught up.\n\nAre you sure you want to continue?");
    }

    if (QMessageBox::question(this, tr("Cancel Movie Change"), prompt, QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
    {
      QLOG_DEBUG() << "User chose to continue with canceling movie change";

      btnCancelMovieChange->setEnabled(false);

      // Verify the operation can still be canceled before continuing, it's possible the user waited too long
      // to answer the question
      if (uftpServer->isRunning())
      {
        // Only perform a cleanup (deleting movie change from databases and files downloaded at booths) if
        // no booth has finished uftp session
        if (getBoothsFinishedUftp().count() == 0)
        {
          movieChangeCancelCleanup = true;
        }
        else
          movieChangeCancelCleanup = false;

        // Stop uftp server, block signals so finishedCopying is not emitted
        uftpServer->blockSignals(true);
        uftpServer->stop();
        uftpServer->blockSignals(false);

        // Build list of booths to cancel movie change in, only ones that have not finished downloading the movies
        QStringList ipAddressList = getBoothsInUftpSession();

        // Send cancel signal to booths and delete all movie change records in databases
        // when this finishes it will call the reset movie change method
        casServer->startMovieChangeCancel(ipAddressList,
                                          movieChangeQueue[currentMovieChangeIndex].Year(),
                                          movieChangeQueue[currentMovieChangeIndex].WeekNum(),
                                          movieChangeCancelCleanup);

        // Should never encounter this scenario...
        if (ipAddressList.count() == 0)
        {
          QLOG_ERROR() << "Cancel Movie Change button was clicked but cannot find any booths in the expected uftp session state";
          Global::sendAlert("Cancel Movie Change button was clicked but cannot find any booths in the expected uftp session state");

          QMessageBox::warning(this, tr("Movie Change"), tr("The movie change is in an unexpected state while canceling. Contact tech support."));
        }

      } // endif uftp server running
      else
      {
        QLOG_DEBUG() << "Movie change can no longer be canceled because file copying has finished";

        // If movie change queue has any other movie change sets then clear them now
        for (int i = movieChangeQueue.count() - 1; i > currentMovieChangeIndex; --i)
        {
          QLOG_DEBUG() << QString("Removing movie change set item from queue and database. Year: %1, Week #: %2").arg(movieChangeQueue[i].Year()).arg(movieChangeQueue[i].WeekNum());

          if (!dbMgr->deleteMovieChangeQueue(movieChangeQueue[i]))
            Global::sendAlert(QString("Could not delete movie change queue item from database. Year: %1, Week #: %2").arg(movieChangeQueue[i].Year()).arg(movieChangeQueue[i].WeekNum()));

          movieChangeQueue.removeAt(i);
        }

        QMessageBox::warning(this, tr("Movie Change"), tr("The movie change can no longer be canceled because the server has finished sending the movies to the booths.\n\nAlthough, if there were any other movie change sets in the queue, these <b>have been canceled</b>."));
      }

    }
    else
    {
      QLOG_DEBUG() << "User chose not to cancel movie change";
    }
  }
  else
  {
    QLOG_ERROR() << "The Cancel Movie Change button should not be enabled. Not allowing user to cancel movie change";
    Global::sendAlert("The Cancel Movie Change button should not be enabled. Not allowing user to cancel movie change");

    QMessageBox::warning(this, tr("Movie Change"), tr("The movie change cannot be canceled at this point."));
  }
}

void MovieChangeWidget::updateMovieChangeQueueStatus(MovieChangeInfo::MovieChangeStatus status)
{
  // FIXME: Program crashed after logging this message, see other FIXME line about not finding 10.0.0.21 in success list since that happened right before this action
  QLOG_DEBUG() << QString("Updating movie change status to: %1").arg(MovieChangeInfo::MovieChangeStatusToString(status));

  movieChangeQueue[currentMovieChangeIndex].setStatus(status);

  if (!dbMgr->updateStatusMovieChangeQueue(movieChangeQueue[currentMovieChangeIndex]))
  {
    Global::sendAlert(QString("Could not update status of movie change queue item in database to %1. Year: %2, Week #: %3")
                      .arg(MovieChangeInfo::MovieChangeStatusToString(status))
                      .arg(movieChangeQueue[currentMovieChangeIndex].Year())
                      .arg(movieChangeQueue[currentMovieChangeIndex].WeekNum()));
  }

  // Set status of movie change in datagrid
  int row = findMovieChangeQueue(movieChangeQueue[currentMovieChangeIndex]);

  if (row > -1)
  {
    QStandardItem *statusField = movieChangeQueueModel->item(row, Status);
    statusField->setText(MovieChangeInfo::MovieChangeStatusToString(status));
    statusField->setData(status);
  }
  else
  {
    QLOG_ERROR() << QString("Could not find movie change queue item in datagrid to update status to %1. Year: %2, Week #: %3")
                            .arg(MovieChangeInfo::MovieChangeStatusToString(status))
                            .arg(movieChangeQueue[currentMovieChangeIndex].Year())
                            .arg(movieChangeQueue[currentMovieChangeIndex].WeekNum());
  }
}

QStringList MovieChangeWidget::getVideoDirectoryListing(int year, int weekNum)
{
  QStringList videoDirListing;

  // Get list of directories in specified year and week num: /path/to/videos/year/weekNum
  QDir videoDir(QString("%1/%2/%3").arg(settings->getValue(VIDEO_PATH).toString()).arg(year).arg(weekNum));

  foreach (QString dir, videoDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
  {
    videoDirListing << videoDir.absoluteFilePath(dir);
  }

  return videoDirListing;
}

QStringList MovieChangeWidget::getBoothsStartedMovieChange()
{
  QStringList ipAddressList;

  for (int i = 0; i < movieChangeDetailModel->rowCount(); ++i)
  {
    QStandardItem *ipAddressField = movieChangeDetailModel->item(i, Address);
    QStandardItem *scheduledStatusField = movieChangeDetailModel->item(i, Scheduled);

    if (!scheduledStatusField->data().isNull() && scheduledStatusField->data().toString() == "True")
    {
      ipAddressList.append(ipAddressField->data().toString());
    }
  }

  return ipAddressList;
}

QStringList MovieChangeWidget::getBoothsFinishedUftp()
{
  QStringList ipAddressList;

  for (int i = 0; i < movieChangeDetailModel->rowCount(); ++i)
  {
    QStandardItem *ipAddressField = movieChangeDetailModel->item(i, Address);
    QStandardItem *uftpFinishedField = movieChangeDetailModel->item(i, UFTP_Finished);

    if (!uftpFinishedField->data().isNull() && uftpFinishedField->data().toDateTime().isValid())
    {
      ipAddressList.append(ipAddressField->data().toString());
    }
  }

  return ipAddressList;
}

QStringList MovieChangeWidget::getBoothsFinishedMovieChange()
{
  QStringList ipAddressList;

  for (int i = 0; i < movieChangeDetailModel->rowCount(); ++i)
  {
    QStandardItem *ipAddressField = movieChangeDetailModel->item(i, Address);
    QStandardItem *moviesChangedField = movieChangeDetailModel->item(i, Movies_Changed);

    if (!moviesChangedField->data().isNull() && moviesChangedField->data().toDateTime().isValid())
    {
      ipAddressList.append(ipAddressField->data().toString());
    }
  }

  return ipAddressList;
}

QStringList MovieChangeWidget::getBoothsNotExcluded()
{
  QStringList ipAddressList;

  for (int i = 0; i < movieChangeDetailModel->rowCount(); ++i)
  {
    QStandardItem *ipAddressField = movieChangeDetailModel->item(i, Address);
    QStandardItem *scheduledStatusField = movieChangeDetailModel->item(i, Scheduled);

    if (!scheduledStatusField->data().isNull() && scheduledStatusField->data().toString() != "Excluded")
    {
      ipAddressList.append(ipAddressField->data().toString());
    }
  }

  return ipAddressList;
}

QStringList MovieChangeWidget::getBoothsInUftpSession()
{
  QStringList ipAddressList;

  for (int i = 0; i < movieChangeDetailModel->rowCount(); ++i)
  {
    QStandardItem *ipAddressField = movieChangeDetailModel->item(i, Address);
    QStandardItem *scheduledStatusField = movieChangeDetailModel->item(i, Scheduled);
    QStandardItem *uftpFinishedField = movieChangeDetailModel->item(i, UFTP_Finished);

    // Booth is in a uftp session if Scheduled field is "True" and UFTP Finished field is null
    if (!scheduledStatusField->data().isNull() &&
        scheduledStatusField->data().toString() == "True" &&
        uftpFinishedField->data().isNull())
    {
      ipAddressList.append(ipAddressField->data().toString());
    }
  }

  return ipAddressList;
}

QStringList MovieChangeWidget::getBoothsStartFailed()
{
  QStringList ipAddressList;

  for (int i = 0; i < movieChangeDetailModel->rowCount(); ++i)
  {
    QStandardItem *ipAddressField = movieChangeDetailModel->item(i, Address);
    QStandardItem *scheduledStatusField = movieChangeDetailModel->item(i, Scheduled);

    if (!scheduledStatusField->data().isNull() && scheduledStatusField->data().toString() == "Failed")
    {
      ipAddressList.append(ipAddressField->data().toString());
    }
  }

  return ipAddressList;
}

void MovieChangeWidget::requestLeastViewedMovies()
{
  if (categoryQueue.length() && movieChangeQueue[currentMovieChangeIndex].Year() < settings->getValue(DELETE_MOVIE_YEAR).toInt())
  {
    QString category = categoryQueue.first();
    categoryQueue.removeFirst();

    QLOG_DEBUG() << QString("Requesting least viewed movies from category: %1...").arg(category);
    gPiwikTracker->getLeastViewedMovies(category, 20);
  }
  else
  {
    casServer->startMovieChangePrepare(movieChangeQueue[currentMovieChangeIndex], onlyBoothAddresses, forceMovieChange, movieChangeQueue[currentMovieChangeIndex].Year() >= settings->getValue(DELETE_MOVIE_YEAR).toInt());
  }
}

void MovieChangeWidget::leastViewedMoviesResponse(QList<Movie> movies, bool error)
{
  if (!error)
  {
    QLOG_DEBUG() << QString("Received %1 movies for category %2").arg(movies.length()).arg(movies.length() > 0 ? movies.at(0).Category() : "(not available)");

    if (dbMgr->insertLeastViewedMovies(movies))
    {
      requestLeastViewedMovies();
    }
    else
    {
      QLOG_ERROR() << "Could not save least viewed movies to database, canceling movie change";

      Global::sendAlert("Could not save least viewed movies to database, canceling movie change");

      QMessageBox::warning(this, tr("Movie Change"), tr("Could not save least viewed movies to database, canceling movie change. Try again or contact tech support."));

      resetMovieChangeProcess();
    }
  }
  else
  {
    QLOG_ERROR() << "Could not get least viewed movies from web server, canceling movie change";

    Global::sendAlert("Could not get least viewed movies from web server, canceling movie change");

    QMessageBox::warning(this, tr("Movie Change"), tr("Could not get least viewed movies from web server, canceling movie change. Try again or contact tech support."));

    resetMovieChangeProcess();
  }
}
