#include "mainwindow.h"
#include "qslog/QsLog.h"
#include "global.h"
#include "appsettingswidget.h"
#include <QDir>
#include <QSettings>
#include <QCoreApplication>
#include <QMessageBox>
#include <QApplication>
#include <QDesktopWidget>
#include <QDesktopServices>
#include <QDebug>
#include <QStatusBar>
#include <QDesktopWidget>
#include "analytics/piwiktracker.h"

#include "moviechangesetindex.h"

// TODO: Use qjson-backport class for all JSON objects instead of the more complicated qjson
// TODO: Does a modal dialog halt all execution? If so need to create a new dialog box that will allow
// the UI to continue working and timers. Especially because a dialog may remain open all night before
// a user gets to it
// FIXME: Use lsscsi to find DVD auto loader: sudo apt-get install lsscsi
// Sample output (autoloader is /dev/sr1 in this example)
/*
[0:0:0:0]    disk    ATA      ST2000DM001-1ER1 CC25  /dev/sda
[1:0:0:0]    cd/dvd  ATAPI    iHAS124   E      4L07  /dev/sr0
[6:0:0:0]    cd/dvd  PIONEER  DVD-RW  DVR-221L 1.00  /dev/sr1
*/
// FIXME: Need to verify convert is available which is part of imagemagick: sudo apt-get install imagemagick
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
  movieChangeContainerWidget = 0;
  viewtimesReportsWidget = 0;
  clerkSessionsReportingWidget = 0;
  boothStatusWidget = 0;
  boothSettingsWidget = 0;
  collectionContainerWidget = 0;
  restartDeviceTimer = 0;
  helpCenterWidget = 0;

  progressBar = new QProgressBar;

  lblProgressDescription = new QLabel;

  // Override the cleanlook theme's tooltip color since it's white text on a yellow background which is unreadable
  qApp->setStyleSheet("QToolTip {color:#000080; background-color:#FFF8DC; padding:0px;}");

  // Make the status bar at the bottom of the window visible
  this->statusBar()->show();

  // Get the path our executable is located in, this is used
  // for building paths to other resources
  QDir appDir(QCoreApplication::applicationDirPath());

  QLOG_DEBUG() << QString("%1 v%2 starting...").arg(SOFTWARE_NAME).arg(SOFTWARE_VERSION);

  if (QDir::setCurrent(QCoreApplication::applicationDirPath()))
    QLOG_DEBUG() << QString("Changed working directory to: %1").arg(QCoreApplication::applicationDirPath());
  else
    QLOG_ERROR() << QString("Could not change working directory to: %1").arg(QCoreApplication::applicationDirPath());

  dbMgr = new DatabaseMgr;
  dbMgr->openDB(appDir.absoluteFilePath("cas-mgr.sqlite"));
  if (!dbMgr->verifyCasMgrDb())
  {
    QLOG_ERROR() << QString("Verifying our database failed");
    Global::sendAlert(QString("Verifying our database failed"));

    QMessageBox::warning(this, tr("Database Error"), tr("Problem with database, the software cannot continue. Contact tech support."));
  }

  // Load JSON-formatted settings from database
  bool ok = false;
  QString tempSettings = dbMgr->getValue("all_settings", "", &ok);

  // The Settings class expects a JSON-formatted string containing all the settings
  // If the string is empty then there are no settings in the beginning
  settings = new Settings(tempSettings);

  initialize();

  // Need to set the data path after we know it from the settings
  dbMgr->setDataPath(settings->getValue(DATA_PATH).toString());
  if (!dbMgr->verifySharedDBs())
  {
    QLOG_ERROR() << QString("Verifying shared databases failed");
    Global::sendAlert(QString("Verifying shared databases failed"));

    QMessageBox::warning(this, tr("Database Error"), tr("Problem with databases, software cannot continue. Contact tech support."));
  }

  // Verify we can connect to the CouchDB server and that the database exists, create database if not found
  QLOG_DEBUG() << "Verifying database server...";
  connect(dbMgr, SIGNAL(verifyCouchDbFinished(QString,bool)), this, SLOT(verifyCouchDbFinished(QString,bool)));
  connect(dbMgr, SIGNAL(getExpiredCardsFinished(QList<ArcadeCard>&,bool)), this, SLOT(getExpiredCardsFinished(QList<ArcadeCard>&,bool)));
  connect(dbMgr, SIGNAL(bulkDeleteCardsFinished(int,bool)), this, SLOT(bulkDeleteCardsFinished(int,bool)));
  dbMgr->verifyCouchDB();

  casServer = new CasServer(dbMgr, settings);

  this->setWindowTitle(SOFTWARE_NAME + " v" + SOFTWARE_VERSION);

  createActions();
  createMenus();

  manualUpdateCheck = false;

  tabs = new QTabWidget;

  videoTranscoder = new VideoTranscoder(settings->getValue(TRANSCODER_PROG_FILE).toString());
  connect(videoTranscoder, SIGNAL(transcodingFinished(bool,QString)), this, SLOT(transcodingFinished(bool,QString)));  

  videoTranscodeTimer = new QTimer;
  videoTranscodeTimer->setInterval(60000);
  connect(videoTranscodeTimer, SIGNAL(timeout()), this, SLOT(checkTranscodingQueue()));
  videoTranscodeTimer->start();
  transcodingTime.setHMS(QTime::currentTime().hour(), QTime::currentTime().minute(), QTime::currentTime().second(), QTime::currentTime().msec());

  this->setCentralWidget(tabs);

  softwareUpdater = new Updater(WEB_SERVICE_URL);
  checkForUpdateTimer = new QTimer;
  checkForUpdateTimer->setInterval(settings->getValue(SOFTWARE_UPDATE_INTERVAL).toInt());
  connect(checkForUpdateTimer, SIGNAL(timeout()), this, SLOT(checkForUpdate()));
  connect(softwareUpdater, SIGNAL(finishedChecking(QString)), this, SLOT(checkForUpdateFinished(QString)));
  checkForUpdateTimer->start();  

  helpCenterWidget = new HelpCenterWidget;
  helpCenterWidget->setFixedSize(650, 550);
  helpCenterWidget->setGeometry(QStyle::alignedRect(Qt::LeftToRight,
                                                    Qt::AlignCenter,
                                                    helpCenterWidget->size(),
                                                    qApp->desktop()->availableGeometry()));
  helpCenterWidget->setShowOnStartup(settings->getValue(SHOW_HELP_CENTER_ON_STARTUP).toBool());

  // FIXME: This doesn't seem to work when only SHOW_CHANGELOG_ON_STARTUP is true
  bool openHelpCenter = settings->getValue(SHOW_HELP_CENTER_ON_STARTUP).toBool() || settings->getValue(SHOW_CHANGELOG_ON_STARTUP).toBool();

  // Normally we show the Tip of the Day tab first but if the changelog flag is set then
  // set this instead
  if (settings->getValue(SHOW_CHANGELOG_ON_STARTUP).toBool())
  {
    helpCenterWidget->setCurrentTab(HelpCenterWidget::Changelog);

    // The show changelog flag is cleared so it doesn't show up on subsequent executions
    settings->setValue(SHOW_CHANGELOG_ON_STARTUP, false);
  }
  else
    helpCenterWidget->setCurrentTab(HelpCenterWidget::TipOfTheDay);

  helpCenterWidget->setHtmlTips(dbMgr->getTipsOfTheDay());

  // Each time the program executes, increment the tip of the day index so the user sees a different one
  helpCenterWidget->setCurrentTip(settings->getValue(TIP_OF_THE_DAY_INDEX).toInt());

  int tipIndex = settings->getValue(TIP_OF_THE_DAY_INDEX).toInt();
  if (++tipIndex >= helpCenterWidget->countTips())
  {
    tipIndex = 0;
  }

  settings->setValue(TIP_OF_THE_DAY_INDEX, tipIndex);

  // Save the change to the tip of the day index and possibly the show changelog flag if it changed
  dbMgr->setValue("all_settings", settings->getSettings());

  helpCenterWidget->setHtmlDocumentation(dbMgr->getDocumentation());
  helpCenterWidget->setHtmlChangelog(dbMgr->getChangelog());

  connect(helpCenterWidget, SIGNAL(accepted()), this, SLOT(helpCenterClosed()));

  // Check for update, cleanup and check transcoding queue after event loop goes idle
  QTimer::singleShot(0, this, SLOT(cleanup()));
  QTimer::singleShot(0, this, SLOT(checkForUpdate()));
  QTimer::singleShot(0, this, SLOT(checkTranscodingQueue()));

  if (openHelpCenter)
  {
    QTimer::singleShot(0, helpCenterWidget, SLOT(show()));
  }

  sharedCardService = new SharedCardServices(this);
}

MainWindow::~MainWindow()
{
  fileMenu->deleteLater();
  toolsMenu->deleteLater();
  helpMenu->deleteLater();
  optionsAction->deleteLater();
 // restartDevicesAction->deleteLater();
  exitAction->deleteLater();
  softwareUpdateAction->deleteLater();
  helpcenterAction->deleteLater();
  aboutAction->deleteLater();
  tabs->deleteLater();
  dbMgr->deleteLater();
  casServer->deleteLater();
  settings->deleteLater();
  videoTranscoder->deleteLater();
  softwareUpdater->deleteLater();
  checkForUpdateTimer->deleteLater();
  videoTranscodeTimer->deleteLater();
  if (restartDeviceTimer)
    restartDeviceTimer->deleteLater();

  if (!progressBar->parent())
    progressBar->deleteLater();

  if (!lblProgressDescription->parent())
    lblProgressDescription->deleteLater();

  if (helpCenterWidget)
    helpCenterWidget->deleteLater();
  if(sharedCardService)
      sharedCardService->deleteLater();
}

void MainWindow::initialize()
{
  QDir appDir(QCoreApplication::applicationDirPath());

  // Load all settings and set to default values if they don't exist yet
  settings->getValue(ALLOW_MOVIE_CHANGE, DEFAULT_ALLOW_MOVIE_CHANGE);
  settings->getValue(ARCADE_SUBNET, DEFAULT_ARCADE_SUBNET);
  settings->getValue(AUTOLOADER_DVD_DRIVE_MOUNT, DEFAULT_AUTOLOADER_DVD_DRIVE_MOUNT);
  settings->getValue(AUTOLOADER_PROG_FILE, DEFAULT_AUTOLOADER_PROG_FILE);
  settings->getValue(AUTOLOADER_RESPONSE_FILE, DEFAULT_AUTOLOADER_RESPONSE_FILE);
  settings->getValue(AUTOLOADER_DVD_DRIVE_STATUS_FILE, DEFAULT_AUTOLOADER_DVD_DRIVE_STATUS_FILE);
  settings->getValue(BOOTH_STATUS_INTERVAL, DEFAULT_BOOTH_STATUS_INTERVAL);
  settings->getValue(COLLECTION_REPORT_RECIPIENTS, DEFAULT_COLLECTION_REPORT_RECIPIENTS);
  settings->getValue(CUSTOMER_PASSWORD, DEFAULT_CUSTOMER_PASSWORD);
  settings->getValue(DAILY_METERS_REPORT_RECIPIENTS, "");
  settings->getValue(DAILY_METERS_TIME, DEFAULT_DAILY_METERS_TIME);
  settings->getValue(DAILY_METERS_INTERVAL, DEFAULT_DAILY_METERS_INTERVAL);
  settings->getValue(DATA_PATH, appDir.absoluteFilePath(DEFAULT_DATA_PATH));
  settings->getValue(DEVICE_LIST, QStringList());
  settings->getValue(DVD_COPY_PROG, DEFAULT_DVD_COPY_PROG);
  settings->getValue(DVD_COPIER_LOG_FILE, DEFAULT_DVD_COPIER_LOG_FILE);
  settings->getValue(DVD_COPIER_PROC_NAME, DEFAULT_DVD_COPIER_PROC_NAME);
  settings->getValue(DVD_MOUNT_TIMEOUT, DEFAULT_DVD_MOUNT_TIMEOUT);
  settings->getValue(DVD_DRIVE_MOUNT, DEFAULT_DVD_DRIVE_MOUNT);
  settings->getValue(DVD_COPIER_TIMEOUT, DEFAULT_DVD_COPIER_TIMEOUT);
  settings->getValue(ENABLE_AUTO_LOADER, DEFAULT_ENABLE_AUTO_LOADER);
  settings->getValue(ENABLE_NO_BACK_COVER, DEFAULT_ENABLE_NO_BACK_COVER);
  settings->getValue(ENABLE_COLLECTION_REPORT, DEFAULT_ENABLE_COLLECTION_REPORT);
  settings->getValue(ENABLE_DAILY_METERS, DEFAULT_ENABLE_DAILY_METERS);
  settings->getValue(FILE_SERVER_PROG_FILE, DEFAULT_FILE_SERVER_PROG_FILE);
  settings->getValue(FILE_SERVER_LOG_FILE, DEFAULT_FILE_SERVER_LOG_FILE);
  settings->getValue(FILE_SERVER_STATUS_FILE, DEFAULT_FILE_SERVER_STATUS_FILE);
  settings->getValue(FILE_SERVER_LIST_FILE, DEFAULT_FILE_SERVER_LIST_FILE);
  // Create a default time the devices were last restarted using today's date and the time of day
  // devices should be restarted. This is used in case this value doesn't exist yet
  settings->getValue(RESTART_DEVICE_TIME, DEFAULT_RESTART_DEVICE_TIME);
  QDateTime defaultLastRestart = QDateTime::currentDateTime();
  defaultLastRestart.setTime(settings->getValue(RESTART_DEVICE_TIME).toTime());
  settings->getValue(LAST_RESTART_DEVICE_DATE, defaultLastRestart);
  defaultLastRestart.setTime(DEFAULT_DAILY_METERS_TIME);
  settings->getValue(LAST_DAILY_METERS_DATE, defaultLastRestart);
  settings->getValue(LAST_EXPIRED_CARDS_CHECK_DATE, defaultLastRestart.addDays(-1));
  settings->getValue(ENABLE_COLLECTION_SNAPSHOT, DEFAULT_ENABLE_COLLECTION_SNAPSHOT);
  settings->getValue(COLLECTION_SNAPSHOT_REPORT_RECIPIENTS, DEFAULT_COLLECTION_SNAPSHOT_REPORT_RECIPIENTS);
  settings->getValue(COLLECTION_SNAPSHOT_INTERVAL, DEFAULT_COLLECTION_SNAPSHOT_INTERVAL);
  settings->getValue(COLLECTION_SNAPSHOT_SCHEDULE_DAY, DEFAULT_COLLECTION_SNAPSHOT_SCHEDULE_DAY);
  settings->getValue(LAST_COLLECTION_SNAPSHOT_DATE, DEFAULT_LAST_COLLECTION_SNAPSHOT_DATE);
  settings->getValue(MIN_FREE_DISKSPACE, DEFAULT_MIN_FREE_DISKSPACE);
  settings->getValue(MIN_ADDRESS, DEFAULT_MIN_ADDRESS);
  settings->getValue(MAX_ADDRESS, DEFAULT_MAX_ADDRESS);
  settings->getValue(MAX_CHANNELS, DEFAULT_MAX_CHANNELS);
  settings->getValue(MAX_TRANSCODED_VIDEO_FILE_SIZE, DEFAULT_MAX_TRANSCODED_VIDEO_FILE_SIZE);
  settings->getValue(MAX_MOVIE_CHANGE_SET_SIZE, DEFAULT_MAX_MOVIE_CHANGE_SET_SIZE);
  settings->getValue(MOVIE_CHANGE_STATUS_INTERVAL, DEFAULT_MOVIE_CHANGE_STATUS_INTERVAL);
  settings->getValue(MOVIE_CHANGE_SCHEDULE_TIME_ADJUST, DEFAULT_MOVIE_CHANGE_SCHEDULE_TIME_ADJUST);
  settings->getValue(NETWORK_PASSPHRASE, DEFAULT_NETWORK_PASSPHRASE);
  //settings->getValue(RESTART_DEVICE_INTERVAL, DEFAULT_RESTART_DEVICE_INTERVAL);
  settings->getValue(SHOW_EXTRA_ADD_VIDEO_FIELDS, DEFAULT_SHOW_EXTRA_ADD_VIDEO_FIELDS);
  settings->getValue(SHOW_NO_BACKCOVER_OPTION, DEFAULT_SHOW_NO_BACKCOVER_OPTION);
  settings->getValue(SOFTWARE_UPDATE_INTERVAL, DEFAULT_SOFTWARE_UPDATE_INTERVAL);
  settings->getValue(STORE_NAME, "");
  settings->getValue(TRANSCODER_PROG_FILE, DEFAULT_TRANSCODER_PROG_FILE);
  settings->getValue(TRANSCODER_LOG_FILE, DEFAULT_TRANSCODER_LOG_FILE);
  settings->getValue(TRANSMISSION_SPEED, DEFAULT_TRANSMISSION_SPEED);
  settings->getValue(UPC_LOOKUP_INTERVAL, DEFAULT_UPC_LOOKUP_INTERVAL);
  settings->getValue(UPLOAD_MOVIE_METADATA, DEFAULT_UPLOAD_MOVIE_METADATA);
  settings->getValue(UPLOAD_VIEW_TIMES, DEFAULT_UPLOAD_VIEW_TIMES);
  settings->getValue(VIDEO_METADATA_PATH, appDir.absoluteFilePath(DEFAULT_VIDEO_METADATA_PATH));
  settings->getValue(VIDEO_PATH, appDir.absoluteFilePath(DEFAULT_VIDEO_PATH));
  settings->getValue(WINE_AUTOLOADER_DVD_DRIVE_LETTER, DEFAULT_WINE_AUTOLOADER_DVD_DRIVE_LETTER);
  settings->getValue(WINE_DVD_COPY_DEST_DRIVE_LETTER, DEFAULT_WINE_DVD_COPY_DEST_DRIVE_LETTER);
  settings->getValue(WINE_DVD_DRIVE_LETTER, DEFAULT_WINE_DVD_DRIVE_LETTER);
  settings->getValue(APP_MODE, DEFAULT_APP_MODE);
  settings->getValue(EXT_HD_MOUNT_PATH, DEFAULT_EXT_HD_MOUNT_PATH);
  settings->getValue(EXT_HD_MOUNT_TIMEOUT, DEFAULT_EXT_HD_MOUNT_TIMEOUT);
  settings->getValue(INTERNAL_DRIVE_PATH, DEFAULT_INTERNAL_DRIVE_PATH);
  settings->getValue(DELETE_UPC_INTERVAL, DEFAULT_DELETE_UPC_INTERVAL);
  settings->getValue(FIRST_RUN, DEFAULT_FIRST_RUN);
  settings->getValue(SHOW_HELP_CENTER_ON_STARTUP, DEFAULT_SHOW_HELP_CENTER_ON_STARTUP);
  settings->getValue(SHOW_CHANGELOG_ON_STARTUP, DEFAULT_SHOW_CHANGELOG_ON_STARTUP);
  settings->getValue(TIP_OF_THE_DAY_INDEX, DEFAULT_TIP_OF_THE_DAY_INDEX);
  settings->getValue(VIEWTIME_COLLECTION_SCHEDULE_DAY, DEFAULT_VIEWTIME_COLLECTION_SCHEDULE_DAY);
  settings->getValue(VIEWTIME_COLLECTION_SCHEDULE_TIME, DEFAULT_VIEWTIME_COLLECTION_SCHEDULE_TIME);
  settings->getValue(LAST_VIEWTIME_COLLECTION, DEFAULT_LAST_VIEWTIME_COLLECTION);
  settings->getValue(LAST_MOVIE_CHANGE, "");
  settings->getValue(BOOTHS_DOWN, QStringList());
  settings->getValue(MOVIE_CHANGE_DURATION_THRESHOLD, DEFAULT_MOVIE_CHANGE_DURATION_THRESHOLD);
  settings->getValue(SHOW_NEW_MOVIE_CHANGE_FEATURE, DEFAULT_SHOW_NEW_MOVIE_CHANGE_FEATURE);
  settings->getValue(BOOTH_NO_RESPONSE_THRESHOLD, DEFAULT_BOOTH_NO_RESPONSE_THRESHOLD);
  settings->getValue(MIN_NUM_WEEKS_MOVIECHANGE, DEFAULT_MIN_NUM_WEEKS_MOVIECHANGE);
  settings->getValue(MAX_MOVIECHANGE_RETRIES, DEFAULT_MAX_MOVIECHANGE_RETRIES);
  settings->getValue(MOVIECHANGE_RETRY_TIMEOUT, DEFAULT_MOVIECHANGE_RETRY_TIMEOUT);
  settings->getValue(UPDATE_CARDS_INTERVAL, DEFAULT_UPDATE_CARDS_INTERVAL);
  settings->getValue(AUTH_INTERVAL, DEFAULT_AUTH_INTERVAL);
  settings->getValue(DELETE_MOVIE_YEAR, DEFAULT_DELETE_MOVIE_YEAR);
  settings->getValue(AUTHORIZE_NET_MARKET_TYPES, DEFAULT_AUTHORIZE_NET_MARKET_TYPES);
  settings->getValue(AUTHORIZE_NET_DEVICE_TYPES, DEFAULT_AUTHORIZE_NET_DEVICE_TYPES);
  settings->getValue(USE_SHARE_CARDS, DEFAULT_USE_SHARE_CARDS);

  // Save settings
  dbMgr->setValue("all_settings", settings->getSettings());


  gPiwikTracker->setDatabaseMgr(dbMgr);
  QLOG_DEBUG() << "Store Name:" << settings->getValue(STORE_NAME, "");
  gPiwikTracker->fetchSiteId(settings->getValue(STORE_NAME, "").toString());
}

bool MainWindow::isBusy()
{
  return movieLibWidget->isBusy() || (movieChangeContainerWidget && movieChangeContainerWidget->isBusy()) || (viewtimesReportsWidget && viewtimesReportsWidget->isBusy()) ||
         (boothStatusWidget && boothStatusWidget->isBusy()) || (boothSettingsWidget && boothSettingsWidget->isBusy()) ||
         (collectionContainerWidget && collectionContainerWidget->isBusy()) || videoTranscoder->isTranscoding();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
  // If there is a DVD copy process or something else running, tell the user the program can't be closed
  if (isBusy())
  {
    QLOG_DEBUG() << QString("Cannot close because program is busy");

    QMessageBox::warning(this, tr("Busy"), tr("You cannot exit the program until it finishes the current operation. Please wait for it to finish."));
    event->ignore();
  }
  else
  {
    if (helpCenterWidget)
      helpCenterWidget->close();

    QLOG_DEBUG() << QString("%1 v%2 closing").arg(SOFTWARE_NAME).arg(SOFTWARE_VERSION);
  }
}

void MainWindow::showEvent(QShowEvent *)
{
  this->setWindowState(Qt::WindowMaximized);

  // If this is the first time the software has been run (no settings exist) then
  if (settings->getValue(FIRST_RUN).toBool())
  {
    QLOG_DEBUG() << "First run of software, showing settings window";

    QMessageBox::information(this, SOFTWARE_NAME, tr("The software must be configured before use. On the next screen please fill out the required fields."));

    // show setup window loading default settings into it
    showOptions();
  }

  // When in DVD Copying Only app mode, only the movie library tab is visible
  if (((Settings::App_Mode)settings->getValue(APP_MODE).toInt()) == Settings::DVD_Copying_Only)
  {
    QLOG_DEBUG() << "Configured for DVD Copying Only";

    movieLibWidget = new MovieLibraryWidget(dbMgr, casServer, settings);

    tabs->addTab(movieLibWidget, tr("Movie Library"));
  }
  else
  {
    // The other app modes include all the tabs but individual tabs such as the movie library might be
    // further customized based on mode

    movieLibWidget = new MovieLibraryWidget(dbMgr, casServer, settings);
    movieChangeContainerWidget = new MovieChangeContainerWidget(dbMgr, casServer, settings);
    viewtimesReportsWidget = new ViewTimesReportsWidget(dbMgr, casServer, settings);
    clerkSessionsReportingWidget = new ClerkSessionsReportingWidget(dbMgr, casServer, settings);
    boothStatusWidget = new BoothStatus(dbMgr, casServer, settings);
    boothSettingsWidget = new BoothSettingsWidget(dbMgr, casServer, settings);
    cardsWidget = new CardsWidget(dbMgr, casServer, settings);
    collectionContainerWidget = new CollectionContainerWidget(dbMgr, casServer, settings);

    connect(boothSettingsWidget, SIGNAL(updatingBooth(bool)), tabs, SLOT(setDisabled(bool)));
    connect(boothSettingsWidget, SIGNAL(initializeSettingsUpdateProgress(int)), this, SLOT(initializeFileCopyStatus(int)));
    connect(boothSettingsWidget, SIGNAL(updateSettingsUpdateProgress(int,QString)), this, SLOT(updateFileCopyStatus(int, QString)));
    connect(boothSettingsWidget, SIGNAL(cleanupSettingsUpdateProgress()), this, SLOT(cleanupFileCopyStatus()));

    connect(movieChangeContainerWidget, SIGNAL(finishedMovieChange()), boothStatusWidget, SLOT(updateBoothStatusAfterMovieChange()));
    connect(boothStatusWidget, SIGNAL(refreshVideoLibrary()), movieLibWidget, SLOT(populateVideoLibrary()));
    connect(boothStatusWidget, SIGNAL(refreshViewtimeDates()), viewtimesReportsWidget, SLOT(setUpdateViewTimeDates()));
    connect(boothStatusWidget, SIGNAL(updateBoothStatusFinished()), movieChangeContainerWidget, SLOT(updateBoothStatusFinished()));

    tabs->addTab(boothStatusWidget, tr("Booth Status"));
    tabs->addTab(boothSettingsWidget, tr("Device Settings"));
    tabs->addTab(cardsWidget, tr("Cards"));
    tabs->addTab(collectionContainerWidget, tr("Collections"));
    tabs->addTab(movieLibWidget, tr("Movie Library"));
    tabs->addTab(viewtimesReportsWidget, tr("View Times"));
    tabs->addTab(movieChangeContainerWidget, tr("Movie Change"));
    tabs->addTab(clerkSessionsReportingWidget, tr("Clerk Station"));

    restartDeviceTimer = new QTimer;
    restartDeviceTimer->setInterval(60 * 1000); // 1 minute
    connect(restartDeviceTimer, SIGNAL(timeout()), this, SLOT(checkRestartSchedule()));
    restartDeviceTimer->start();
  }

  connect(movieLibWidget, SIGNAL(abortTranscoding()), videoTranscoder, SLOT(abortTranscoding()));
  connect(movieLibWidget, SIGNAL(queueDeleteVideo(MovieChangeInfo&)), this, SLOT(queueMovieDelete(MovieChangeInfo&)));

  // If not mode All_Functions then listen to additional signals from widget
  if (((Settings::App_Mode)settings->getValue(APP_MODE).toInt()) != Settings::All_Functions)
  {
    connect(movieLibWidget, SIGNAL(initializeCopyProgress(int)), this, SLOT(initializeFileCopyStatus(int)));
    connect(movieLibWidget, SIGNAL(updateCopyProgress(int, QString)), this, SLOT(updateFileCopyStatus(int, QString)));
    connect(movieLibWidget, SIGNAL(cleanupCopyProgress()), this, SLOT(cleanupFileCopyStatus()));
    connect(movieLibWidget, SIGNAL(waitingForExternalDrive()), this, SLOT(startDriveWaitStatus()));
    connect(movieLibWidget, SIGNAL(finishedWaitingForExternalDrive()), this, SLOT(cleanupDriveWaitStatus()));
  }
}

void MainWindow::changeEvent(QEvent *e)
{
  if (e->type() == QEvent::WindowStateChange)
  {
    // If no window state then it means the maximize button was clicked
    // Normally this will toggle between a maximized window and a smaller window (no state)
    // so override this behavior by returning it to maximized so the user can't reduce the size
    if (this->windowState() == Qt::WindowNoState)
      this->setWindowState(Qt::WindowMaximized);
  }
}

void MainWindow::exitProgram()
{
  this->close();
}

void MainWindow::showOptions()
{
  // Set address range of booths
  // Build list of IP Addresses
  // Allow user to add/remove individual addresses from list
  // Scan booths
  QLOG_DEBUG() << QString("Showing Settings window");
  AppSettingsWidget appSettings(dbMgr, casServer, settings, sharedCardService, settings->getValue(FIRST_RUN).toBool());
  if (appSettings.exec() == QDialog::Accepted)
  {
    bool restartRequired = false;

    // If this is not the first run and the app mode or the min/max address were changed then
    // the software must be closed and reopened
    if (!settings->getValue(FIRST_RUN).toBool() && appSettings.restartRequired())
    {
      QLOG_DEBUG() << "Changes made to settings that require restart";

      restartRequired = true;
    }
    else
    {
      settings->setValue(FIRST_RUN, false);
    }

    QLOG_DEBUG() << QString("User updated settings, now saving");
    dbMgr->setValue("all_settings", settings->getSettings());

    if (restartRequired)
    {
      QLOG_DEBUG() << QString("User made changes that require restarting the software");

      QMessageBox::information(this, tr("Settings Changed"), tr("The software needs to be restarted in order for the changes to take effect. Once the main window closes, click the %1 icon to start it again.").arg(SOFTWARE_NAME));

      exitProgram();
    }
  }
  else
    QLOG_DEBUG() << QString("User canceled updating settings");
}

void MainWindow::checkForUpdate()
{
  // Make sure we're not busy before checking for update
  if (!isBusy())
  {
    if (checkForUpdateTimer->interval() != settings->getValue(SOFTWARE_UPDATE_INTERVAL).toInt())
    {
      QLOG_DEBUG() << "Changing software update interval back to default value";
      checkForUpdateTimer->setInterval(settings->getValue(SOFTWARE_UPDATE_INTERVAL).toInt());
    }

    QLOG_DEBUG() << "Checking for software update...";
    softwareUpdater->checkForUpdate(SOFTWARE_NAME, SOFTWARE_VERSION);
  }
  else
  {        
    QLOG_DEBUG() << "Skipping software update check because we're busy";

    if (checkForUpdateTimer->interval() == settings->getValue(SOFTWARE_UPDATE_INTERVAL).toInt())
    {
      QLOG_DEBUG() << "Reducing software update interval until software is no longer busy";
      checkForUpdateTimer->setInterval(60000 * 5); // 5 minutes
    }

    if (manualUpdateCheck)
      QMessageBox::warning(this, tr("Busy"), tr("Cannot check for software update while another operation is in progress. Please wait for it to finish."));

    manualUpdateCheck = false;
  }
}

void MainWindow::manuallyCheckForUpdate()
{
  manualUpdateCheck = true;
  checkForUpdate();
}

void MainWindow::checkForUpdateFinished(QString response)
{
  if (manualUpdateCheck)
  {
    QMessageBox::information(this, SOFTWARE_NAME, response);
    manualUpdateCheck = false;
  }
}

void MainWindow::checkRestartSchedule()
{
  QDateTime lastRestarted = settings->getValue(LAST_RESTART_DEVICE_DATE).toDateTime();

  if (lastRestarted.date() != QDate::currentDate())
  {
    QDateTime nextRestartTime = QDateTime::currentDateTime();
    nextRestartTime.setTime(settings->getValue(RESTART_DEVICE_TIME).toTime());

    // If it's time or past time to restart devices
    if (nextRestartTime <= QDateTime::currentDateTime())
    {
      if (!casServer->isBusy())
      {
        QLOG_DEBUG() << "Time to restart all CAS devices";

        // Do not show results of restarting all devices since this is a scheduled event
        boothStatusWidget->setSilentRestart(true);

        // send restart command to all devices
        // If there is a movie change or session in progress then the device will set its
        // pending reboot flag instead of interrupting
        casServer->startRestartDevices();

        // update last reboot time
        settings->setValue(LAST_RESTART_DEVICE_DATE, nextRestartTime);

        // Save settings
        dbMgr->setValue("all_settings", settings->getSettings());
      }
      else
      {
        QLOG_DEBUG() << "Time to restart CAS devices but casServer has another thread running, will try later";
      }
    }
  }

  QDateTime lastExpiredCardsCheck = settings->getValue(LAST_EXPIRED_CARDS_CHECK_DATE).toDateTime();

  if (lastExpiredCardsCheck.date() != QDate::currentDate())
  {
    if (!casServer->isBusy())
    {
      QLOG_DEBUG() << "Time to delete expired cards";

      dbMgr->getExpiredCards();

      // update last reboot time
      settings->setValue(LAST_EXPIRED_CARDS_CHECK_DATE, QDateTime::currentDateTime());

      // Save settings
      dbMgr->setValue("all_settings", settings->getSettings());
    }
    else
    {
      QLOG_DEBUG() << "Time to delete expired cards but casServer has another thread running, will try later";
    }
  }
}

void MainWindow::checkTranscodingQueue()
{
  // if not currently transcoding a video then
  //   if there are any videos that need to be transcoded then
  //     start transcoding first video returned

  // Look for videos in database that need to be transcoded
  // If any videos found then start encoding
  // Set start time of encoding
  // Notify Movie Library tab of update
  // After a video is finished encoding then
  //   Delete original VOB file
  //   Set end time of encoding
  //   look in database for videos that need to be transcoded
  //
  if (!videoTranscoder->isTranscoding())
  {
    QList<DvdCopyTask> videosToEncode = dbMgr->getVideosToTranscode();

    if (videosToEncode.count() > 0)
    {
      QLOG_DEBUG() << "Found" << videosToEncode.count() << "videos to transcode";

      currentVideoTranscoding = videosToEncode.first();

      QString srcVideo = QString("%1/%2/%3/%4/%5.VOB").arg(settings->getValue(VIDEO_PATH).toString()).arg(currentVideoTranscoding.Year()).arg(currentVideoTranscoding.WeekNum()).arg(currentVideoTranscoding.UPC()).arg(currentVideoTranscoding.UPC());
      QString destVideo = QString("%1/%2/%3/%4/%5.ts").arg(settings->getValue(VIDEO_PATH).toString()).arg(currentVideoTranscoding.Year()).arg(currentVideoTranscoding.WeekNum()).arg(currentVideoTranscoding.UPC()).arg(currentVideoTranscoding.UPC());

      QLOG_DEBUG() << "Transcoding video with UPC:" << currentVideoTranscoding.UPC() << ", source:" << srcVideo << ", destination:" << destVideo;

      videoTranscoder->startTranscoding(srcVideo, destVideo, settings->getValue(TRANSCODER_LOG_FILE).toString());

      // Set start time of when transcoding started, just used for logging purposes
      transcodingTime.restart();

      // Update the transcode status
      currentVideoTranscoding.setTranscodeStatus(DvdCopyTask::Working);
      dbMgr->updateDvdCopyTask(currentVideoTranscoding);

      // Update status in UI
      movieLibWidget->updateEncodeStatus(currentVideoTranscoding.UPC(), currentVideoTranscoding.TranscodeStatus());
    }
  }
}

void MainWindow::transcodingFinished(bool success, QString errorMessage)
{
  int elapsedTime = transcodingTime.elapsed();

  // The metadata could've been modified between the time we started transcoding and when we finished
  // so load the latest version from the database
  DvdCopyTask latestVideoMetadata = dbMgr->getDvdCopyTask(currentVideoTranscoding.UPC());

  if (latestVideoMetadata.UPC() == currentVideoTranscoding.UPC())
  {
    currentVideoTranscoding = latestVideoMetadata;
  }
  else
  {
    QLOG_ERROR() << "The video we transcoded is no longer in the database:" << currentVideoTranscoding.UPC();
  }

  if (success)
  {
    QLOG_DEBUG() << "Finished trancoding video with UPC:" << currentVideoTranscoding.UPC() << "elapsed time:" << ((qreal)elapsedTime / 1000 / 60) << "minutes";

    QFileInfo originalVideoFile(QString("%1/%2/%3/%4/%5.VOB").arg(settings->getValue(VIDEO_PATH).toString()).arg(currentVideoTranscoding.Year()).arg(currentVideoTranscoding.WeekNum()).arg(currentVideoTranscoding.UPC()).arg(currentVideoTranscoding.UPC()));
    QFileInfo transcodedVideoFile(QString("%1/%2/%3/%4/%5.ts").arg(settings->getValue(VIDEO_PATH).toString()).arg(currentVideoTranscoding.Year()).arg(currentVideoTranscoding.WeekNum()).arg(currentVideoTranscoding.UPC()).arg(currentVideoTranscoding.UPC()));
    if (transcodedVideoFile.exists())
    {
      QLOG_DEBUG() << "Original file size:" << originalVideoFile.size() << ", transcoded file size:" << transcodedVideoFile.size();

      // if transcoded file size larger than max size then set status as too large, else set finished status
      if (transcodedVideoFile.size() > settings->getValue(MAX_TRANSCODED_VIDEO_FILE_SIZE).toLongLong())
      {
        QLOG_ERROR() << "Transcoded file size is greater than the maximum size, cannot use file in arcade";
        currentVideoTranscoding.setTranscodeStatus(DvdCopyTask::Video_Too_Large);
      }
      else
      {
        QLOG_DEBUG() << "Transcoded file size is acceptable";
        currentVideoTranscoding.setTranscodeStatus(DvdCopyTask::Finished);
      }

      currentVideoTranscoding.setFileLength(transcodedVideoFile.size());
      dbMgr->updateDvdCopyTask(currentVideoTranscoding);

      // Update status and new file size in UI
      movieLibWidget->updateEncodeStatus(currentVideoTranscoding.UPC(), currentVideoTranscoding.TranscodeStatus(), currentVideoTranscoding.FileLength());

      // If mediaInfo.xml exists then update the video file size
      QString mediaInfoFile = QString("%1/%2/%3/%4/mediaInfo.xml").arg(settings->getValue(VIDEO_PATH).toString()).arg(currentVideoTranscoding.Year()).arg(currentVideoTranscoding.WeekNum()).arg(currentVideoTranscoding.UPC());
      if (QFile::exists(mediaInfoFile))
      {
        Movie movie(mediaInfoFile, 0);

        movie.setFileLength(transcodedVideoFile.size());

        if (movie.writeXml(mediaInfoFile))
        {
          QLOG_DEBUG() << QString("Updated file size in mediaInfo.xml file: %1").arg(mediaInfoFile);
        }
        else
        {
          QLOG_ERROR() << QString("Could not update file size in mediaInfo.xml file: %1").arg(mediaInfoFile);
        }
      }
      else
      {
        QLOG_DEBUG() << QString("mediaInfo.xml file: %1 does not exist, skipping updating video file size").arg(mediaInfoFile);
      }      

      // Delete the VOB file since transcoding was successful
      QString srcVideo = QString("%1/%2/%3/%4/%5.VOB").arg(settings->getValue(VIDEO_PATH).toString()).arg(currentVideoTranscoding.Year()).arg(currentVideoTranscoding.WeekNum()).arg(currentVideoTranscoding.UPC()).arg(currentVideoTranscoding.UPC());
      if (QFile::remove(srcVideo))
      {
        QLOG_DEBUG() << "Deleted original video file:" << srcVideo;
      }
      else
      {
        QLOG_ERROR() << "Could not delete original video file:" << srcVideo;
      }
    }
    else
    {
      QLOG_ERROR() << "Transcoded video file does not exist:" << transcodedVideoFile.filePath();
    }
  }
  else
  {
    QLOG_ERROR() << "Could not trancode video with UPC:" << currentVideoTranscoding.UPC() << errorMessage << "elapsed time:" << ((qreal)elapsedTime / 1000 / 60) << "minutes";

    // Update the transcode status
    currentVideoTranscoding.setTranscodeStatus(DvdCopyTask::Failed);
    dbMgr->updateDvdCopyTask(currentVideoTranscoding);

    // Update status in UI
    movieLibWidget->updateEncodeStatus(currentVideoTranscoding.UPC(), currentVideoTranscoding.TranscodeStatus());

    QString newLogFile = QString("/var/cas-mgr/transcode-video-%1.log").arg(currentVideoTranscoding.UPC());

    if (QFile::rename(settings->getValue(TRANSCODER_LOG_FILE).toString(), newLogFile))
    {
      QLOG_DEBUG() << "Moved transcode log file to:" << newLogFile;
    }
    else
    {
      QLOG_ERROR() << "Could not move transcode log file to:" << newLogFile;
    }
  }

  currentVideoTranscoding = DvdCopyTask();

  checkTranscodingQueue();
}

void MainWindow::showHelpcenter()
{
  //QString url = "https://usarcades.com/documentation-and-videos";
  //QDesktopServices::openUrl(QUrl(url));

  QLOG_DEBUG() << QString("User clicked the Help Center menu item");
  helpCenterWidget->setCurrentTab(HelpCenterWidget::TipOfTheDay);
  helpCenterWidget->show();
}

void MainWindow::showAbout()
{
  // In case system date is wrong, just use 2015
  int currentYear = QDate::currentDate().year();

  if (currentYear < 2015)
    currentYear = 2015;

  QMessageBox::about(this, tr("About"), tr("%1 v%2\n\nCopyright 2012-%3 US Arcades, Inc.\nAll Rights Reserved\n\nThe %4 software is used to perform various functions on the booths in your arcade. Please see the Help Center for more information.")
                     .arg(SOFTWARE_NAME)
                     .arg(SOFTWARE_VERSION)
                     .arg(currentYear)
                     .arg(SOFTWARE_NAME));
}

void MainWindow::cleanup()
{
  // Find .VOB and .IFO files that may have been left in /var/cas-mgr/share/videos/<year>/<week_num>/
  // and delete if found. These files could be left behind if user interrupted the DVD copy process.
  // Limiting the depth to 3 is done so it doesn't look in actual UPC subdirectories where a VOB
  // might exist due to the transcoding process getting interrupted which will get resumed in the next section.

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

  // If any videos were being transcoded then change back to the pending state so the process
  // can start again. Transcoding could get interrupted if the user interrupted it by resetting the computer
  QList<DvdCopyTask> interruptedVideos = dbMgr->getVideosBeingTranscoded();

  if (interruptedVideos.count() > 0)
  {
    QLOG_DEBUG() << QString("Found %1 video(s) interrupted during transcoding").arg(interruptedVideos.count());

    foreach (DvdCopyTask dt, interruptedVideos)
    {
      QLOG_DEBUG() << QString("Changing transcode status of Year: %1, Week #: %2, UPC: %3 back to Pending").arg(dt.Year()).arg(dt.WeekNum()).arg(dt.UPC());

      dt.setTranscodeStatus(DvdCopyTask::Pending);
      dbMgr->updateDvdCopyTask(dt);
    }
  }
  else
  {
    QLOG_DEBUG() << "No videos found that were interrupted while being transcoded";
  }
}

void MainWindow::initializeFileCopyStatus(int totalFiles)
{
  progressBar->setTextVisible(true);
  progressBar->setMinimum(0);
  progressBar->setMaximum(totalFiles);    

  this->statusBar()->addWidget(progressBar, 1);
  this->statusBar()->addWidget(lblProgressDescription, 3);

  lblProgressDescription->show();
  progressBar->show();
}

void MainWindow::updateFileCopyStatus(int numFiles, const QString &overallStatus)
{
  progressBar->setValue(numFiles);
  lblProgressDescription->setText(overallStatus);
}

void MainWindow::cleanupFileCopyStatus()
{
  this->statusBar()->removeWidget(progressBar);
  this->statusBar()->removeWidget(lblProgressDescription);

  lblProgressDescription->clear();
  progressBar->reset();
}

void MainWindow::startDriveWaitStatus()
{
  progressBar->setMinimum(0);
  progressBar->setMaximum(0);
  progressBar->setTextVisible(false);

  lblProgressDescription->setText(tr("Waiting for external hard drive..."));

  this->statusBar()->addWidget(progressBar, 1);
  this->statusBar()->addWidget(lblProgressDescription, 3);

  lblProgressDescription->show();
  progressBar->show();
}

void MainWindow::cleanupDriveWaitStatus()
{
  this->statusBar()->removeWidget(progressBar);
  this->statusBar()->removeWidget(lblProgressDescription);

  lblProgressDescription->clear();
  progressBar->reset();
}

void MainWindow::helpCenterClosed()
{
  QLOG_DEBUG() << "Help Center window closed";

  if (settings->getValue(SHOW_HELP_CENTER_ON_STARTUP).toBool() != helpCenterWidget->getShowOnStartup())
  {
    QLOG_DEBUG() << "Help Center - show on startup checkbox changed to:" << helpCenterWidget->getShowOnStartup();

    settings->setValue(SHOW_HELP_CENTER_ON_STARTUP, helpCenterWidget->getShowOnStartup());
    dbMgr->setValue("all_settings", settings->getSettings());
  }

  // Move to the next tip of the day on close so
  // when the widget is next shown, it is displaying a new tip
  // This is done on close instead of on show because if a specific
  // tip is selected to be shown we don't want to change it
  helpCenterWidget->gotoNextTip();
}

void MainWindow::verifyCouchDbFinished(QString errorMessage, bool ok)
{
  if (ok)
    QLOG_DEBUG() << "Database server is online and database exists";
  else
  {
    QLOG_ERROR() << QString("Verifying CouchDB server failed: %1").arg(errorMessage);
    Global::sendAlert(QString("Verifying CouchDB server failed: %1").arg(errorMessage));

    QMessageBox::warning(this, tr("Database Server Error"), tr("Problem with database server. You will not be able to manage cards for the arcade. Contact tech support."));
  }
}

void MainWindow::getExpiredCardsFinished(QList<ArcadeCard> &cards, bool ok)
{
  if (ok)
  {
    QLOG_DEBUG() << QString("Found %1 expired cards to delete").arg(cards.length());

    if (cards.length() > 0)
    {
      dbMgr->bulkDeleteCards(cards);
    }
  }
  else
  {
    QLOG_ERROR() << "There was a problem getting the expired cards";
  }
}

void MainWindow::bulkDeleteCardsFinished(int numCards, bool ok)
{
  if (ok)
  {
    QLOG_DEBUG() << QString("Deleted %1 expired card(s) from database").arg(numCards);
  }
  else
  {
    QLOG_ERROR() << "Could not delete expired card(s) from database";
  }
}

void MainWindow::queueMovieDelete(MovieChangeInfo &movieChangeInfo)
{
  movieChangeContainerWidget->queueMovieDelete(movieChangeInfo);
  tabs->setCurrentWidget(movieChangeContainerWidget);
}

void MainWindow::createActions()
{
  exitAction = new QAction(tr("E&xit"), this);
  exitAction->setShortcut(QKeySequence::Quit);
  exitAction->setStatusTip(tr("Close the application"));
  connect(exitAction, SIGNAL(triggered()), this, SLOT(exitProgram()));

  optionsAction = new QAction(tr("S&ettings..."), this);
  optionsAction->setShortcut(Qt::ALT + Qt::Key_E);
  optionsAction->setStatusTip(tr("Settings"));
  //optionsAction->setEnabled(false);
  connect(optionsAction, SIGNAL(triggered()), this, SLOT(showOptions()));

  softwareUpdateAction = new QAction(tr("Check for &updates..."), this);
  softwareUpdateAction->setShortcut(Qt::ALT + Qt::Key_U);
  softwareUpdateAction->setStatusTip(tr("Check for updates to this software"));
  connect(softwareUpdateAction, SIGNAL(triggered()), this, SLOT(manuallyCheckForUpdate()));

  helpcenterAction = new QAction(tr("Help Center"), this);
  helpcenterAction->setShortcut(Qt::Key_F1);
  helpcenterAction->setStatusTip(tr("View tips of the day, documentation, videos and more."));
  connect(helpcenterAction, SIGNAL(triggered()), this, SLOT(showHelpcenter()));

  aboutAction = new QAction(tr("&About CAS Manager..."), this);
  aboutAction->setShortcut(Qt::ALT + Qt::Key_A);
  aboutAction->setStatusTip(tr("Information about the CAS Manager software."));
  connect(aboutAction, SIGNAL(triggered()), this, SLOT(showAbout()));

  //restartDevicesAction = new QAction(tr("Restart Devices..."), this);
  //connect(restartDevicesAction, SIGNAL(triggered()), this, SLOT(checkRestartSchedule()));
}

void MainWindow::createMenus()
{
  fileMenu = menuBar()->addMenu(tr("&File"));
  fileMenu->addAction(exitAction);

  toolsMenu = menuBar()->addMenu(tr("&Tools"));
  toolsMenu->addAction(optionsAction);
  toolsMenu->addAction(softwareUpdateAction);
  //toolsMenu->addAction(restartDevicesAction);

  helpMenu = menuBar()->addMenu(tr("&Help"));
  helpMenu->addAction(helpcenterAction);
  helpMenu->addSeparator();
  helpMenu->addAction(aboutAction);
}
