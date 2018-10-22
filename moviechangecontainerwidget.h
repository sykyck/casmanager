#ifndef MOVIECHANGECONTAINERWIDGET_H
#define MOVIECHANGECONTAINERWIDGET_H

#include <QWidget>
#include <QTabWidget>
#include <QVBoxLayout>
#include "moviechangewidget.h"
#include "moviechangehistorywidget.h"
#include "forcemoviechangewidget.h"

class MovieChangeContainerWidget : public QWidget
{
  Q_OBJECT
public:
  explicit MovieChangeContainerWidget(DatabaseMgr *dbMgr, CasServer *casServer, Settings *settings, QWidget *parent = 0);
  ~MovieChangeContainerWidget();
  bool isBusy();

signals:
  void finishedMovieChange();

public slots:
  void updateBoothStatusFinished();
  void queueMovieDelete(MovieChangeInfo &movieChangeInfo);
  
private:
  QTabWidget *tabs;
  MovieChangeWidget *movieChangeWidget;
  MovieChangeHistoryWidget *movieChangeHistoryWidget;
  QVBoxLayout *verticalLayout;
  ForceMovieChangeWidget *forceMovieChangeWidget;
};

#endif // MOVIECHANGECONTAINERWIDGET_H
