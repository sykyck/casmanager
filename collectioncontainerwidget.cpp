#include "collectioncontainerwidget.h"

CollectionContainerWidget::CollectionContainerWidget(DatabaseMgr *dbMgr, CasServer *casServer, Settings *settings, QWidget *parent) : QWidget(parent)
{
  tabs = new QTabWidget;
  collectionsWidget = new CollectionsWidget(dbMgr, casServer, settings, tabs);
  collectionReportsWidget = new CollectionReportsWidget(dbMgr, casServer, settings, tabs);

  tabs->addTab(collectionsWidget, tr("Collect"));
  tabs->addTab(collectionReportsWidget, tr("Reports"));

  verticalLayout = new QVBoxLayout;
  verticalLayout->addWidget(tabs);
  this->setLayout(verticalLayout);
}

CollectionContainerWidget::~CollectionContainerWidget()
{
}

bool CollectionContainerWidget::isBusy()
{
  return collectionsWidget->isBusy() || collectionReportsWidget->isBusy();
}
