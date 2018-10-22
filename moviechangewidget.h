#ifndef MOVIECHANGEWIDGET_H
#define MOVIECHANGEWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTableView>
#include <QStandardItemModel>
#include <QTimer>
#include <QProcess>
#include "databasemgr.h"
#include "dvdcopytask.h"
#include "casserver.h"
#include "settings.h"
#include "uftpserver.h"
#include "messagewidget.h"

class MovieChangeWidget : public QWidget
{
  Q_OBJECT
public:
  explicit MovieChangeWidget(DatabaseMgr *dbMgr, CasServer *casServer, Settings *settings, QTabWidget *parentTab, QWidget *parent = 0);
  ~MovieChangeWidget();
  bool isBusy();
  bool isMovieChangeInProgress();

protected:
  void showEvent(QShowEvent *);

signals:
  void finishedMovieChange();
  
public slots:
  void updateMovieChangeStatus();
  void queueMovieDelete(MovieChangeInfo &movieChangeInfo);

private slots:
  void addNewMovieChange();
  void checkMovieChangeQueue();
  void movieChangePrepareError(QString message);
  void movieChangePreparePossibleError(QString message);
  void movieChangeStartSuccess(QString ipAddress);
  void movieChangeStartFailed(QString ipAddress);
  void movieChangeExcluded(QString ipAddress);
  void movieChangePrepareFinished();
  void startCopyingMovieChangeSet();
  void deleteReplacedMoviesOnServer();
  void finalizeMovieChange();

  void movieChangeRetrySuccess(QString ipAddress);
  void movieChangeRetryFailed(QString ipAddress);
  void movieChangeRetryFinished();

  void movieChangeCancelSuccess(QString ipAddress);
  void movieChangeCancelFailed(QString ipAddress);
  void movieChangeCancelFinished();

  void insertMovieChangeQueue(MovieChangeInfo movieChange);
  void insertMovieChangeScheduled(QDateTime scheduledDate, DvdCopyTask task, int channelNum = 0, QString previousMovie = QString());
  void insertMovieChangeBoothStatus(QString deviceAlias, QString deviceType, QString ipAddress, qreal freeSpace, int numChannels, QString scheduled, QDateTime downloadStarted, QDateTime downloadFinished, QDateTime moviesChanged);
  void clearMovieChangeQueue();
  void clearMovieChangeScheduled();
  void clearMovieChangeBoothStatus();
  int findMovieChangeDetail(QString ipAddress);
  int findPendingMovieChange(QString upc);
  int findMovieChangeQueue(MovieChangeInfo movieChange);

  void startedUftp();
  void finishedUftp(UftpServer::UftpStatusCode statusCode, QString errorMessage = QString(), QStringList failureList = QStringList(), QStringList successList = QStringList());
  void uftpProcessError();

  void retryMovieChange();

  void resetMovieChangeProcess();

  void cancelMovieChangeClicked();

  void updateMovieChangeQueueStatus(MovieChangeInfo::MovieChangeStatus status);

  void movieChangeClientsFinished();

  void requestLeastViewedMovies();
  void leastViewedMoviesResponse(QList<Movie> movies, bool error);

private:
  enum MovieChangeQueueColumns
  {
    Num_Movies,
    Status,
    Queue_Year,
    Queue_Week_Num
  };

  enum PendingMovieChangeColumns
  {
    Scheduled_Date,
    Week_Num,
    UPC,
    Title,
    Category,
    Channel_Num,
    Previous_Movie,
    Year
  };

  enum MovieChangeDetailColumns
  {
    Name,
    Booth_Type,
    Address,
    Free_Space,
    Total_Channels,
    Scheduled,
    Download_Started,
    UFTP_Finished,
    Download_Finished,
    Movies_Changed
  };

  QStringList getVideoDirectoryListing(int year, int weekNum);
  QStringList getBoothsFinishedUftp();
  QStringList getBoothsNotExcluded();
  QStringList getBoothsStartedMovieChange();
  QStringList getBoothsStartFailed();
  QStringList getBoothsInUftpSession();
  QStringList getBoothsFinishedMovieChange();

  QStandardItemModel *movieChangeQueueModel;
  QStandardItemModel *pendingMovieChangeModel;
  QStandardItemModel *movieChangeDetailModel;
  QTableView *movieChangeQueueView;
  QTableView *pendingMovieChangeView;
  QTableView *movieChangeDetailView;
  QLabel *lblMovieChangeQueue;
  QLabel *lblPendingMovieChange;
  QLabel *lblMovieChangeDetail;
  QPushButton *btnNewMovieChange;
  QPushButton *btnCancelMovieChange;
  QVBoxLayout *verticalLayout;
  QHBoxLayout *buttonLayout;
  DatabaseMgr *dbMgr;
  CasServer *casServer;
  Settings *settings;  
  QTabWidget *parentTab;
  bool firstLoad;
  bool busy;

  // When the current movie change should begin
  QDateTime scheduledDate;
  QString possibleErrorMsg;
  QString errorMsg;  
  UftpServer *uftpServer;

  // List of booth IP addresses that succeeded/failed to execute the movie change retry command
  QStringList movieChangeRetrySuccessList;
  QStringList movieChangeRetryFailureList;

  // List of booth IP addresses that did not finish the movie change
  QStringList movieChangeFailureList;

  // List of booth IP addresses that succeeded/failed to execute the movie change cancel command
  QStringList movieChangeCancelSuccessList;
  QStringList movieChangeCancelFailureList;

  // List of the movie change sets waiting to be sent to the booths
  QList<MovieChangeInfo> movieChangeQueue;

  // Current position in movieChangeQueue
  int currentMovieChangeIndex;

  bool sentMovieChangeStuckAlert;

  MessageWidget *msgWidget;

  // Counter of number of times we've retried to perform current movie change
  int numMovieChangeRetries;

  // Used when needing to retry a movie change after a set amount of time
  QTimer *movieChangeRetryTimer;

  // Flag set by user choosing to continue movie change even if some booths are offline and/or behind on movie changes
  bool forceMovieChange;

  // Flag set if canceled movie change in all participating booths
  bool movieChangeCancelCleanup;

  // Flag set when it is permitted to call the updateMovieChangeStatus slot
  bool enableUpdateMovieChangeStatus;

  // List of IP addresses of Arcade, Preview or Theater type booths (not clerk stations)
  QStringList onlyBoothAddresses;

  // List of movie categories to request least viewed movies
  QStringList categoryQueue;
};

#endif // MOVIECHANGEWIDGET_H
