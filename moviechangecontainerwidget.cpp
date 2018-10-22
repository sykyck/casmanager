#include "moviechangecontainerwidget.h"

MovieChangeContainerWidget::MovieChangeContainerWidget(DatabaseMgr *dbMgr, CasServer *casServer, Settings *settings, QWidget *parent) : QWidget(parent)
{
  tabs = new QTabWidget;
  movieChangeWidget = new MovieChangeWidget(dbMgr, casServer, settings, tabs);
  movieChangeHistoryWidget = new MovieChangeHistoryWidget(dbMgr, casServer, settings);
  forceMovieChangeWidget = new ForceMovieChangeWidget(dbMgr, casServer, settings, movieChangeWidget, this);

  connect(casServer, SIGNAL(movieChangePrepareError(QString)), forceMovieChangeWidget, SLOT(onMovieChangeError(QString)));
  connect(movieChangeWidget, SIGNAL(finishedMovieChange()), this, SIGNAL(finishedMovieChange()));

  tabs->addTab(movieChangeWidget, tr("Current"));
  tabs->addTab(movieChangeHistoryWidget, tr("History"));
  tabs->addTab(forceMovieChangeWidget, tr("Force Movie Change"));

  verticalLayout = new QVBoxLayout;
  verticalLayout->addWidget(tabs);
  this->setLayout(verticalLayout);
}

MovieChangeContainerWidget::~MovieChangeContainerWidget()
{
}

bool MovieChangeContainerWidget::isBusy()
{
  return movieChangeWidget->isBusy() || movieChangeHistoryWidget->isBusy();
}


void MovieChangeContainerWidget::updateBoothStatusFinished()
{
  movieChangeWidget->updateMovieChangeStatus();
}

void MovieChangeContainerWidget::queueMovieDelete(MovieChangeInfo &movieChangeInfo)
{
  movieChangeWidget->queueMovieDelete(movieChangeInfo);
  tabs->setCurrentWidget(movieChangeWidget);
}
