#ifndef MOVIECHANGEHISTORYWIDGET_H
#define MOVIECHANGEHISTORYWIDGET_H

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

class MovieChangeHistoryWidget : public QWidget
{
  Q_OBJECT
public:
  explicit MovieChangeHistoryWidget(DatabaseMgr *dbMgr, CasServer *casServer, Settings *settings, QWidget *parent = 0);
  ~MovieChangeHistoryWidget();
  bool isBusy();
  
protected:
  void showEvent(QShowEvent *);

signals:

public slots:

private slots:
  void movieChangeDateChanged(int index);

private:
  enum MovieChangeColumns
  {
    UPC,
    Title,
    Producer,
    Category,
    Subcategory,
    Channel_Num,
    Previous_Movie
  };

  void insertMovieChange(QString upc, QString title, QString producer, QString category, QString subcategory, int channelNum, QString previousMovie);

  QStandardItemModel *movieChangeModel;
  QTableView *movieChangeTable;
  QLabel *lblMovieChangeDate;
  QComboBox *cmbMovieChangeDate;
  QHBoxLayout *reportCriteriaLayout;
  QVBoxLayout *verticalLayout;
  bool firstLoad;
  bool busy;

  DatabaseMgr *dbMgr;
  CasServer *casServer;
  Settings *settings;
};

#endif // MOVIECHANGEHISTORYWIDGET_H
