#ifndef VIEWTIMESREPORTSWIDGET_H
#define VIEWTIMESREPORTSWIDGET_H

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

class ViewTimesReportsWidget : public QWidget
{
  Q_OBJECT
public:
  explicit ViewTimesReportsWidget(DatabaseMgr *dbMgr, CasServer *casServer, Settings *settings, QWidget *parent = 0);
  ~ViewTimesReportsWidget();
  bool isBusy();

protected:
  void showEvent(QShowEvent *);

signals:

public slots:
  void setUpdateViewTimeDates(bool update = true);

private slots:
  void viewTimeDateChanged(int index);
  void boothChanged(int index);

private:
  void insertViewTime(QString upc, QString title, QString producer, QString category, QString subcategory, int channelNum, int playTime, int numWeeks);

  QStandardItemModel *viewTimesModel;
  QTableView *viewTimesTable;
  QLabel *lblViewTimeDate;
  QComboBox *cmbViewTimeDate;
  QLabel *lblBooth;
  QComboBox *cmbBooth;
  QHBoxLayout *reportCriteriaLayout;
  QVBoxLayout *verticalLayout;
  bool firstLoad;
  bool busy;
  bool updateViewTimeDates;

  DatabaseMgr *dbMgr;
  CasServer *casServer;
  Settings *settings;
};

#endif // VIEWTIMESREPORTSWIDGET_H
