#ifndef BOOTHSTATUS_H
#define BOOTHSTATUS_H

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
#include "casserver.h"
#include "settings.h"
#include "qjson-backport/qjsondocument.h"
#include "qjson-backport/qjsonobject.h"
#include "qjson-backport/qjsonvalue.h"
#include "qjson-backport/qjsonarray.h"

class BoothStatus : public QWidget
{
  Q_OBJECT
public:
  explicit BoothStatus(DatabaseMgr *dbMgr, CasServer *casServer, Settings *settings, QWidget *parent = 0);
  ~BoothStatus();
  bool isBusy();

protected:
  void showEvent(QShowEvent *);

signals:
  void refreshVideoLibrary();
  void refreshViewtimeDates();
  void updateBoothStatusFinished();
  
public slots:
  void scanningDevice(QString ipAddress);
  void scanDeviceSuccess(QString ipAddress);
  void scanDeviceFailed(QString ipAddress);
  void scanDevicesFinished();

  void checkDeviceSuccess(QString ipAddress, bool inSession);
  void checkDeviceFailed(QString ipAddress);
  void checkDevicesFinished();

  void getBoothDatabaseSuccess(QString ipAddress);
  void getBoothDatabaseFailed(QString ipAddress);
  void getBoothDatabasesFinished();

  void restartDeviceSuccess(QString ipAddress);
  void restartDeviceInSession(QString ipAddress);
  void restartDeviceFailed(QString ipAddress);
  void restartDevicesFinished();

  void setSilentRestart(bool silent);

  void updateBoothStatusAfterMovieChange();
  
private slots:
  void checkDevicesClicked();  
  void clearBoothStatuses();
  void updateBoothStatus(bool forceUpdate = false);
  void restartDevicesClicked();

  void collectedViewTimesSuccess(QString ipAddress);
  void collectedViewTimesFailed(QString ipAddress);
  void mergeViewTimesSuccess(QString ipAddress);
  void mergeViewTimesFailed(QString ipAddress);
  void collectViewTimesFinished();

  void clearViewTimesSuccess(QString ipAddress);
  void clearViewTimesInSession(QString ipAddress);
  void clearViewTimesFailed(QString ipAddress);
  void clearViewTimesFinished();

  void getBoothStatusesFinished(QList<QJsonObject> &boothDocs, bool ok);
  void deleteBoothStatusFinished(QString id, bool ok);
  void addBoothStatusFinished(QString id, bool ok);

private:
  enum BoothStatusColumns
  {
    Name,
    Booth_Type,
    Address,
    Credits,
    Cash,
    Programmed,
    CC_Charges,
    Last_Used,
    Bill_Acceptor,
    Status
  };

  void insertBoothStatus(QString deviceAlias, QString deviceType, QString ipAddress, qreal credits, qreal cash, qreal programmed, qreal ccCharges, QDateTime lastUsed, QString billAcceptorStatus, QString status);
  int findBooth(QString ipAddress);
  void populateTable();

  DatabaseMgr *dbMgr;
  CasServer *casServer;
  Settings *settings;
  bool firstLoad;
  bool scanningNetwork;
  bool busy;
  bool silentRestart;
  bool reloadVideoLibrary;
  QVBoxLayout *verticalLayout;
  QHBoxLayout *buttonLayout;
  QPushButton *btnCheckDevices;
  QPushButton *btnRestartDevices;
  QTimer *statusTimer;
  QStandardItemModel *boothStatusModel;
  QTableView *boothStatusView;
  QLabel *lblCurrentBoothStatus;
  QStringList restartDeviceSuccessList;
  QStringList restartDeviceFailList;
  QStringList restartDeviceInSessionList;
  QStringList viewtimeCollectSuccessList;
  QStringList viewtimeCollectFailList;
  QStringList viewtimeMergeSuccessList;
  QStringList viewtimeMergeFailList;
  QStringList viewtimeClearSuccessList;
  QStringList viewtimeClearFailList;
  QStringList viewtimeClearInSessionList;
  quint8 numMinutes;
  QMap<QString, int> noResponseCount;
  QJsonObject boothStatuses;
};

#endif // BOOTHSTATUS_H
