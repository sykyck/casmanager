#ifndef CLERKSESSIONSREPORTINGWIDGET_H
#define CLERKSESSIONSREPORTINGWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTableView>
#include <QStandardItemModel>
#include <QTimer>
#include <QProcess>
#include <QComboBox>
#include "settings.h"
#include "databasemgr.h"
#include "casserver.h"
#include <QDateEdit>
#include <QStringList>

class ClerkSessionsReportingWidget : public QWidget
{
  Q_OBJECT
public:
  explicit ClerkSessionsReportingWidget(DatabaseMgr *dbMgr, CasServer *casServer, Settings *settings, QWidget *parent = 0);
  ~ClerkSessionsReportingWidget();
  bool isBusy();

protected:
  void showEvent(QShowEvent *);

signals:

public slots:

private slots:
  void clerkSessionDateChanged(QDate);
  void refreshSessionsClicked();
  void clearClerkstationsClicked();
  void clearClerkstationsSuccess(QString ipAddress);
  void clearClerkstationsFailed(QString ipAddress);
  void clearClerkstationsFinished();
  void userAuthorizationReceived(bool success, QString response);

private:
  void insertClerkSession(QString user, QString deviceNumber, QDateTime startTime, QDateTime endTime, double amount, double arcade_amount, double preview_amount);

  QStandardItemModel *clerkSessionsModel;
  QTableView *clerkSessionsTable;
  QLabel *lblViewTimeDate;
  QDateEdit *cmbViewTimeDate;
  QHBoxLayout *reportCriteriaLayout;
  QVBoxLayout *verticalLayout;
  bool firstLoad;
  bool busy;
  bool updateSessionDates;
  QPushButton *btnRefreshSessions;
  QPushButton *btnClearClerkstations;

  DatabaseMgr *dbMgr;
  CasServer *casServer;
  Settings *settings;

  QStringList clearClerkstationsSuccessList;
  QStringList clearClerkstationsFailureList;
};

#endif // CLERKSESSIONSREPORTINGWIDGET_H
