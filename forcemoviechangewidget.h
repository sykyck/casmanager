#ifndef FORCEMOVIECHANGEWIDGET_H
#define FORCEMOVIECHANGEWIDGET_H

#include <QWidget>
#include <QTreeWidget>
#include <QHBoxLayout>
#include <QObject>
#include <QListWidget>
#include <QLineEdit>
#include "moviechangewidget.h"
#include "databasemgr.h"
#include "casserver.h"
#include "settings.h"

class ForceMovieChangeWidget:public QWidget
{
    Q_OBJECT
public:
    ForceMovieChangeWidget(DatabaseMgr *dbMgr, CasServer *casServer, Settings *settings, MovieChangeWidget *movieChangeWidget, QWidget *parent = 0);

protected:
    void showEvent(QShowEvent *);

public slots:
    void onMovieChangeError(QString error);
    void onForceMovieChange();
    void onProcessStarted();
    void onProcessErrorOccur(QProcess::ProcessError error);
    void onProcessFinished(int exitCode);

private:
    void clearChildNodes();
    void populateDeviceTree();
    void startScriptCopyProcess(QTreeWidgetItem *item);

    DatabaseMgr *dbMgr;
    CasServer *casServer;
    Settings *settings;
    MovieChangeWidget *movieChangeWidget;
    QTreeWidget *deviceTree;
    QHBoxLayout *horizontalLayout;
    QVBoxLayout *verticalLayout;
};

#endif // FORCEMOVIECHANGEWIDGET_H
