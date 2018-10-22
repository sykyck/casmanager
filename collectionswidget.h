#ifndef COLLECTIONSWIDGET_H
#define COLLECTIONSWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QTableView>
#include <QStandardItemModel>
#include <QTimer>
#include <QProcess>
#include <QTabWidget>
#include "databasemgr.h"
#include "casserver.h"
#include "settings.h"

class CollectionsWidget : public QWidget
{
  Q_OBJECT
public:
  explicit CollectionsWidget(DatabaseMgr *dbMgr, CasServer *casServer, Settings *settings, QTabWidget *parentTab = 0,  QWidget *parent = 0);
  ~CollectionsWidget();
  bool isBusy();

protected:
  void showEvent(QShowEvent *);

signals:
  
public slots:
  
private slots:
  void collectBoothsClicked();
  void toggleAlarmClicked();
  void clearCollectionClicked();

  void collectionSuccess(QString ipAddress);
  void collectionFailed(QString ipAddress);
  void collectionFinished();

  void toggleAlarmSuccess(QString ipAddress);
  void toggleAlarmFailed(QString ipAddress);
  void toggleAlarmFinished();

  void sendCollectionSuccess(QString message);
  void sendCollectionFailed(QString message);
  void sendCollectionFinished();

private:
  void insertBoothCollection(QString deviceAlias, QString ipAddress, QString deviceType, int credits, qreal cash, int programmed, qreal ccCharges, QDateTime lastUsed, QDateTime lastCollection, QString collectionStatus);
  void updateBoothCollection(QString ipAddress, int credits, qreal cash, int programmed, qreal ccCharges, QDateTime lastUsed, QDateTime lastCollection, QString collectionStatus);
  int findDevice(QString ipAddress);

  QStandardItemModel *collectionStatusModel;
  QTableView *collectionStatusView;
  QLabel *lblCollectionStatus;

  QLabel *lblTotalCash;
  QLineEdit *txtTotalCash;

  QPushButton *btnCollectBooths;
  QPushButton *btnToggleAlarm;
  QPushButton *btnClearCollection;
  QVBoxLayout *verticalLayout;
  QHBoxLayout *buttonLayout;
  QHBoxLayout *totalLayout;

  bool busy;
  DatabaseMgr *dbMgr;
  CasServer *casServer;
  Settings *settings;
  QTabWidget *parentTab;

  // Current state of alarm in devices
  bool alarmEnabled;

  QStringList collectSuccessList;
  QStringList collectFailList;
  QStringList toggleAlarmSuccessList;
  QStringList toggleAlarmFailList;
};

#endif // COLLECTIONSWIDGET_H
