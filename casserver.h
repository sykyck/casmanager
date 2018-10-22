#ifndef CASSERVER_H
#define CASSERVER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QUrl>
#include <QNetworkReply>
#include <QFutureWatcher>
#include "settings.h"
#include "databasemgr.h"
#include "movie.h"
#include "moviechangeinfo.h"

const char KEY[] = {0x7c, 0x78, 0x7f, 0x49, 0x42, 0x7f, 0x43, 0x42,
                    0x6d, 0x14, 0x63, 0x42, 0x68, 0x70, 0x54, 0x76,
                    0x70, 0x56, 0x44, 0x7c, 0x7d, 0x78, 0x6b, 0x58,
                    0x74, 0x43, 0x5f, 0x47, 0x22, 0x23, 0x2a, 0x77,
                    0};

const quint8 COMMAND_PING                          = 1;
const quint8 COMMAND_VIEW_TIME_COLLECTION          = 2;
const quint8 COMMAND_CLEAR_VIEW_TIMES              = 3;
const quint8 COMMAND_MOVIE_CHANGE                  = 4;
const quint8 COMMAND_RELOAD_VIDEOS                 = 5;
const quint8 COMMAND_ENABLE_BUDDY_BOOTH            = 6; // Not used in this program
const quint8 COMMAND_DISABLE_BUDDY_BOOTH           = 7; // Not used in this program
const quint8 COMMAND_RESTART_DEVICE                = 8;
const quint8 COMMAND_SEND_DATABASE                 = 9;
const quint8 COMMAND_UPDATE_SETTINGS               = 10;
const quint8 COMMAND_COLLECT_BOOTH                 = 11;
const quint8 COMMAND_DISABLE_ALARM                 = 12;
const quint8 COMMAND_ENABLE_ALARM                  = 13;
const quint8 COMMAND_MOVIE_CHANGE_RETRY            = 14;
const quint8 COMMAND_MOVIE_CHANGE_CANCEL           = 15;
const quint8 COMMAND_MOVIE_CHANGE_CANCEL_NO_DELETE = 16;
const quint8 COMMAND_CLEAR_CLERKSTATION            = 17;

const quint8 ERROR_CODE_SUCCESS             = 0;
const quint8 ERROR_CODE_WRONG_PASSPHRASE    = 1;
const quint8 ERROR_CODE_DB_ERROR            = 2;
const quint8 ERROR_CODE_IN_SESSION          = 3;
const quint8 ERROR_CODE_INCOMPLETE_DL       = 4;
const quint8 ERROR_CODE_INCOMPLETE_MC       = 5;
const quint8 ERROR_CODE_IDLE_STATE          = 6;
const quint8 ERROR_CODE_IN_SESSION_STATE    = 7;
const quint8 ERROR_CODE_DB_COPY_ERROR       = 8;
const quint8 ERROR_CODE_SETTINGS_COPY_ERROR = 9;
const quint8 ERROR_CODE_UNKNOWN_COMMAND     = 99;


const quint16 TCP_PORT = 45124;

class CasServer : public QObject
{
  Q_OBJECT
public:
  explicit CasServer(DatabaseMgr *dbMgr, Settings *settings, QObject *parent = 0);
  ~CasServer();
  void startScanDevices();
  void startCheckDevices();
  void startGetBoothDatabases();
  void startViewTimeCollection(QStringList ipAddressList);
  void startClearViewTimes();
  void startMovieChangePrepare(MovieChangeInfo movieChangeInfo, QStringList boothAddresses, bool force = false, bool deleteOnly = false);
  void startMovieChangeClients(QStringList ipAddressList);
  void startMovieChangeRetry(QStringList ipAddressRetryList);
  void startMovieChangeCancel(QStringList ipAddressList, int year, int weekNum, bool cleanup);
  void startUpdateSettings(QStringList ipAddressList);
  void startRestartDevices();
  void reloadVideos();
  void startCollection();
  void startToggleAlarmState(bool enableAlarm);

  QList<Movie> getMovieDownloads(QString moviePath);
  int findMovieDownloadInCategory(QList<Movie> movieList, QString category);
  QString ipAddressToDeviceNum(QString ipAddress);
  QStringList ipAddressesToDeviceNums(QStringList ipAddressList);
  QString findLargestVideoCategory(QMap<QString, QList<Movie> > viewTimesByCategory);  
  void sendCollection(QDate collectionDate, QVariantList deviceMeters);
  void sendDailyMeters(QDateTime metersCaptured, QVariantList deviceMeters);
  void sendCollectionSnapshot(QDateTime startTime, QDateTime endTime, QVariantList deviceMeters);

  void userAuthorization(QString password);

  // Returns list of device addresses based on deviceList property
  // Takes just the last octet of each device IP address
  QStringList getDeviceAddresses();

  // Returns true if any threads are currently running
  bool isBusy();

  bool sendMessage(quint8 command, QString ipAddress, quint16 port, quint8 *returnCode = 0);

  void startClearClerkstations();

signals:
  void scanningDevice(QString ipAddress);
  void scanDeviceSuccess(QString ipAddress);
  void scanDeviceFailed(QString ipAddress);
  void scanDevicesFinished();

  void checkDeviceSuccess(QString ipAddress, bool inSession);
  void checkDeviceFailed(QString ipAddress);
  void checkDevicesFinished();

  void collectedViewTimesSuccess(QString ipAddress);
  void collectedViewTimesFailed(QString ipAddress);
  void mergeViewTimesSuccess(QString ipAddress);
  void mergeViewTimesFailed(QString ipAddress);
  void viewTimeCollectionFinished();

  void getBoothDatabaseSuccess(QString ipAddress);
  void getBoothDatabaseFailed(QString ipAddress);
  void getBoothDatabasesFinished();

  void clearViewTimesSuccess(QString ipAddress);
  void clearViewTimesInSession(QString ipAddress);
  void clearViewTimesFailed(QString ipAddress);
  void clearViewTimesFinished();

  void movieChangePrepareError(QString message);
  void movieChangePreparePossibleError(QString message);
  void movieChangePrepareFinished();

  void movieChangeStartSuccess(QString ipAddress);
  void movieChangeStartFailed(QString ipAddress);
  void movieChangeClientsFinished();

  void movieChangeExcluded(QString ipAddress);

  void movieChangeRetrySuccess(QString ipAddress);
  void movieChangeRetryFailed(QString ipAddress);
  void movieChangeRetryFinished();

  void movieChangeCancelSuccess(QString ipAddress);
  void movieChangeCancelFailed(QString ipAddress);
  void movieChangeCancelFinished();

  void updateSettingsSuccess(QString ipAddress);
  void updateSettingsInSession(QString ipAddress);
  void updateSettingsFailed(QString ipAddress);
  void updateSettingsFinished();

  void restartDeviceSuccess(QString ipAddress);
  void restartDeviceInSession(QString ipAddress);
  void restartDeviceFailed(QString ipAddress);
  void restartDevicesFinished();

  void collectionSuccess(QString ipAddress);
  void collectionFailed(QString ipAddress);
  void collectionFinished();

  void toggleAlarmSuccess(QString ipAddress);
  void toggleAlarmFailed(QString ipAddress);
  void toggleAlarmFinished();

  void sendCollectionSuccess(QString message);
  void sendCollectionFailed(QString message);
  void sendCollectionFinished();

  void sendDailyMetersSuccess(QString message);
  void sendDailyMetersFailed(QString message);
  void sendDailyMetersFinished();

  void sendCollectionSnapshotSuccess(QString message);
  void sendCollectionSnapshotFailed(QString message);
  void sendCollectionSnapshotFinished();

  void userAuthorizationReceived(bool success, QString response);

  void clearClerkstationsSuccess(QString ipAddress);
  void clearClerkstationsFailed(QString ipAddress);
  void clearClerkstationsFinished();

public slots:
  QStringList getBoothsOutOfSync(bool *ok);

private slots:  
  void scanDevices();
  void checkDevices();
  void viewTimeCollection(QStringList ipAddressList);
  void getBoothDatabases();
  void clearViewTimes();
  void movieChangePrepare(MovieChangeInfo movieChangeInfo, QStringList boothAddresses, bool force, bool deleteOnly);
  void movieChangeClients(QStringList ipAddressList);
  void movieChangeRetry(QStringList ipAddressRetryList);
  void movieChangeCancel(QStringList ipAddressList, int year, int weekNum, bool cleanup);
  void updateSettings(QStringList ipAddressList);
  void restartDevices();
  void collectDevices();
  void toggleAlarmState(bool enableAlarm);

  void finishedSendingViewTimes(bool success);
  void receivedViewTimesResponse();
  void sendingViewTimesError(QNetworkReply::NetworkError error);
  void abortConnection();
  void resetTimeout(qint64,qint64);

  void sendingCollectionError(QNetworkReply::NetworkError error);
  void receivedCollectionsResponse();
  void finishedSendingCollectionReport(bool success);

  void sendingDailyMetersError(QNetworkReply::NetworkError error);
  void receivedDailyMetersResponse();
  void finishedSendingDailyMetersReport(bool success);

  void sendingCollectionSnapshotError(QNetworkReply::NetworkError error);
  void receivedCollectionSnapshotResponse();
  void finishedSendingCollectionSnapshotReport(bool success);

  void userAuthorizationError(QNetworkReply::NetworkError error);
  void receivedUserAuthorizationResponse();
  void finishedUserAuthorization(bool success, QString response);

  void clearClerkstations();

private:

  // Number of video downloadeds to expect in download directory
  int numVideoDownloads;

  QNetworkAccessManager *netMgr;
  QNetworkReply *netReplySendViews;
  QNetworkReply *netReplySendCollection;
  QNetworkReply *netReplySendDailyMeters;
  QNetworkReply *netReplySendCollectionSnapshot;
  QNetworkReply *netReplyUserAuth;
  QTimer *connectionTimer;

  // Counts the number of times we retry to contact server
  int numRetries;

  DatabaseMgr *dbMgr;
  Settings *settings;

  QFutureWatcher<void> qfwScanDevices;
  QFutureWatcher<void> qfwCheckDevices;
  QFutureWatcher<void> qfwCollectViewTimes;
  QFutureWatcher<void> qfwGetBoothDatabases;
  QFutureWatcher<void> qfwClearViewTimes;
  QFutureWatcher<void> qfwMovieChangePrepare;
  QFutureWatcher<void> qfwMovieChangeClients;
  QFutureWatcher<void> qfwMovieChangeRetry;
  QFutureWatcher<void> qfwMovieChangeCancel;
  QFutureWatcher<void> qfwUpdateSettings;
  QFutureWatcher<void> qfwRestartDevices;
  QFutureWatcher<void> qfwCollection;
  QFutureWatcher<void> qfwToggleAlarmState;
  QFutureWatcher<void> qfwCollectDailyViewTimes;
  QFutureWatcher<void> qfwClearClerkstations;
};

#endif // CASSERVER_H
