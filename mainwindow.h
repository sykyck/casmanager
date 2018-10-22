#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMenuBar>
#include <QAction>
#include <QTableWidget>
#include <QCloseEvent>
#include <QTimer>
#include <QTime>
#include <QProgressBar>

#include "movielibrarywidget.h"
#include "moviechangecontainerwidget.h"
#include "viewtimesreportswidget.h"
#include "boothsettingswidget.h"
#include "cardswidget.h"
#include "collectioncontainerwidget.h"
#include "boothstatus.h"
#include "databasemgr.h"
#include "casserver.h"
#include "settings.h"
#include "updater.h"
#include "videotranscoder.h"
#include "arcadecard.h"
#include "helpcenterwidget.h"
#include "clerksessionsreportingwidget.h"

class MainWindow : public QMainWindow
{
  Q_OBJECT
  
public:
  MainWindow(QWidget *parent = 0);  
  ~MainWindow();
  void initialize();
  bool isBusy();

protected:
  void closeEvent(QCloseEvent *event);
  void showEvent(QShowEvent *);
  void changeEvent(QEvent *e);

private slots:
  void exitProgram();
  void showOptions();
  void checkForUpdate();
  void manuallyCheckForUpdate();
  void checkForUpdateFinished(QString response);
  void checkRestartSchedule();
  void checkTranscodingQueue();
  void transcodingFinished(bool success, QString errorMessage = QString());
  void showHelpcenter();
  void showAbout();
  void cleanup();
  void initializeFileCopyStatus(int totalFiles);
  void updateFileCopyStatus(int numFiles, const QString &overallStatus);
  void cleanupFileCopyStatus();
  void startDriveWaitStatus();
  void cleanupDriveWaitStatus();
  void helpCenterClosed();
  void verifyCouchDbFinished(QString errorMessage, bool ok);
  void getExpiredCardsFinished(QList<ArcadeCard> &cards, bool ok);
  void bulkDeleteCardsFinished(int numCards, bool ok);
  void queueMovieDelete(MovieChangeInfo &movieChangeInfo);

private:
  void createActions();
  void createMenus();

  QMenu *fileMenu;
  QMenu *toolsMenu;
  QMenu *helpMenu;
  QAction *optionsAction;
  QAction *softwareUpdateAction;
  QAction *restartDevicesAction;
  QAction *exitAction;
  QAction *helpcenterAction;
  QAction *aboutAction;

  QTabWidget *tabs;
  CardsWidget *cardsWidget;
  CollectionContainerWidget *collectionContainerWidget;
  MovieLibraryWidget *movieLibWidget;
  MovieChangeContainerWidget *movieChangeContainerWidget;
  ViewTimesReportsWidget *viewtimesReportsWidget;
  ClerkSessionsReportingWidget *clerkSessionsReportingWidget;
  BoothStatus *boothStatusWidget;
  BoothSettingsWidget *boothSettingsWidget;
  QProgressBar *progressBar;
  HelpCenterWidget *helpCenterWidget;

  DatabaseMgr *dbMgr;
  CasServer *casServer;
  Settings *settings;
  Updater *softwareUpdater;
  QTimer *checkForUpdateTimer;
  QTimer *restartDeviceTimer;
  bool manualUpdateCheck;

  VideoTranscoder *videoTranscoder;
  QTimer *videoTranscodeTimer;
  DvdCopyTask currentVideoTranscoding;
  QTime transcodingTime;
  SharedCardServices *sharedCardService;

  QLabel *lblProgressDescription;
};

#endif // MAINWINDOW_H
