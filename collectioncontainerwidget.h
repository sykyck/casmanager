#ifndef COLLECTIONCONTAINERWIDGET_H
#define COLLECTIONCONTAINERWIDGET_H

#include <QWidget>
#include <QTabWidget>
#include <QVBoxLayout>
#include "collectionswidget.h"
#include "collectionreportswidget.h"

class CollectionContainerWidget : public QWidget
{
  Q_OBJECT
public:
  explicit CollectionContainerWidget(DatabaseMgr *dbMgr, CasServer *casServer, Settings *settings, QWidget *parent = 0);
  ~CollectionContainerWidget();
  bool isBusy();

signals:

public slots:

private:
  QTabWidget *tabs;
  CollectionsWidget *collectionsWidget;
  CollectionReportsWidget *collectionReportsWidget;
  QVBoxLayout *verticalLayout;
};

#endif // COLLECTIONCONTAINERWIDGET_H
