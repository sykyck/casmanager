#include "forcemoviechangewidget.h"
#include "moviechangecontainerwidget.h"
#include <QHeaderView>
#include <QMessageBox>
#include "qslog/QsLog.h"

const QString COPY_TO_DEVICE = "DEVICES";
const QString COPY_FROM_DEVICE = "COPY FROM DEVICE";
const QString PUSH_BUTTON = "PUSH BUTTON FOR CHANGES";
const QString ROOT_NODE = "DEVICES BEHIND MOVIE CHANGES";


ForceMovieChangeWidget::ForceMovieChangeWidget(DatabaseMgr *dbMgr, CasServer *casServer, Settings *settings, MovieChangeWidget *movieChangeWidget, QWidget *parent):QWidget(parent)
{
    this->dbMgr = dbMgr;
    this->settings = settings;
    this->casServer = casServer;
    this->movieChangeWidget = movieChangeWidget;

    deviceTree = new QTreeWidget(this);
    deviceTree->setColumnCount(3);
    deviceTree->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    deviceTree->header()->resizeSection(0, 100);
    deviceTree->header()->resizeSection(1, 400);
    deviceTree->setRootIsDecorated(true);

    QStringList headers;
    headers.append(COPY_TO_DEVICE);
    headers.append(COPY_FROM_DEVICE);
    headers.append(PUSH_BUTTON);
    deviceTree->setHeaderLabels(headers);

    QTreeWidgetItem *rootAllDevices = new QTreeWidgetItem(deviceTree);
    rootAllDevices->setText(0, ROOT_NODE);
    rootAllDevices->setData(0, Qt::DisplayRole, ROOT_NODE);
    rootAllDevices->setFirstColumnSpanned(true);

    horizontalLayout = new QHBoxLayout();
    horizontalLayout->addWidget(deviceTree);

    this->setLayout(horizontalLayout);
}

void ForceMovieChangeWidget::showEvent(QShowEvent *)
{
  QLOG_DEBUG() << QString("Showing ForceMovieChange tab");
  if(this->movieChangeWidget->isMovieChangeInProgress())
  {
      QMessageBox::information(this, tr("Busy"), tr("Movie Changes are happening Cannot Force Movie Changes Now!."));
      this->setDisabled(true);
  }
  else
  {
      this->setDisabled(false);
      populateDeviceTree();
  }
}

void ForceMovieChangeWidget::clearChildNodes()
{
  QTreeWidgetItem *root = deviceTree->topLevelItem(0);

  for (int i = 0; i < root->childCount(); ++i)
  {
    QTreeWidgetItem *parentNode = root->child(i);

    if (parentNode->childCount() > 0)
    {
      qDeleteAll(parentNode->takeChildren());
    }
  }

  qDeleteAll(root->takeChildren());
}

void ForceMovieChangeWidget::onMovieChangeError(QString error)
{
    Q_UNUSED(error);
    if(this->movieChangeWidget->isMovieChangeInProgress())
    {
        QMessageBox::information(this, tr("Busy"), tr("Movie Changes are happening Cannot Force Movie Changes Now!."));
        this->setDisabled(true);
    }
    else
    {
        this->setDisabled(false);
        populateDeviceTree();
    }
}

void ForceMovieChangeWidget::populateDeviceTree()
{
   clearChildNodes();

   bool ok = false;
   QStringList boothsOutOfSync = this->casServer->getBoothsOutOfSync(&ok);
   QLOG_DEBUG()<<boothsOutOfSync;

   boothsOutOfSync.append(QString("1"));
   boothsOutOfSync.append(QString("2"));
   boothsOutOfSync.append(QString("3"));
   boothsOutOfSync.append(QString("4"));
   boothsOutOfSync.append(QString("21"));

   foreach (QString booth, boothsOutOfSync)
   {
       QTreeWidgetItem *root = deviceTree->topLevelItem(0);

       // 1st level
       if (root->text(0) == ROOT_NODE)
       {
         QTreeWidgetItem *childNode = new QTreeWidgetItem(root);
         childNode->setText(0, booth);
         childNode->setData(0, Qt::UserRole, booth);
         QPushButton *forceMovieChangeButton = new QPushButton("Force Movie Change",deviceTree);
         deviceTree->setItemWidget(childNode, 2, forceMovieChangeButton);
         QLineEdit *deviceToCopyFrom = new QLineEdit(deviceTree);
         deviceTree->setItemWidget(childNode, 1, deviceToCopyFrom);
         connect(forceMovieChangeButton, SIGNAL(clicked()), this, SLOT(onForceMovieChange()));
       }
   }

   deviceTree->expandAll();
   deviceTree->setCurrentItem(deviceTree->topLevelItem(0));

}

void ForceMovieChangeWidget::onForceMovieChange()
{
    QLOG_DEBUG()<<"Inside slot onForceMovieChange";
    QTreeWidgetItem *root = deviceTree->topLevelItem(0);
    for (int i = 0; i < root->childCount(); ++i)
    {
      QTreeWidgetItem *childNode = root->child(i);
      if(deviceTree->itemWidget(childNode,2) == (QPushButton *)QObject::sender())
      {
          QLOG_DEBUG()<<"childNode selected = "<<childNode->text(0);
          startScriptCopyProcess(childNode);
          break;
      }
    }
}

void ForceMovieChangeWidget::startScriptCopyProcess(QTreeWidgetItem *item)
{
    QStringList copyProcessArgs;
    copyProcessArgs << "-p" << QString("4OBih8^RJ&O0W66y!900g0J%3IZXgLp*fl");
    copyProcessArgs << "scp" << "/var/cas-mgr/share/data/forcemoviescript.py" << QString("caspi@") + settings->getValue(ARCADE_SUBNET).toString() + item->text(0) + QString(":/media/cas");
    int exitCode = QProcess::execute("sshpass", copyProcessArgs);
    if(exitCode == 0)
    {
        QLOG_DEBUG()<<"Script Copied Successfully";
        QLineEdit *deviceToCopyFrom = (QLineEdit *)deviceTree->itemWidget(item, 1);
        if(!(deviceToCopyFrom->text().isEmpty()))
        {
           QProcess *monitorProgressProcess = new QProcess();
           connect(monitorProgressProcess, SIGNAL(started()), this, SLOT(onProcessStarted()));
           connect(monitorProgressProcess, SIGNAL(finished(int)), this, SLOT(onProcessFinished(int)));
           connect(monitorProgressProcess, SIGNAL(error(QProcess::ProcessError)), this, SLOT(onProcessErrorOccur(QProcess::ProcessError)));

           QStringList monitorProgressArgs;
           monitorProgressArgs.append(item->text(0));
           monitorProgressArgs.append(deviceToCopyFrom->text());
           monitorProgressArgs.append(settings->getValue(ARCADE_SUBNET).toString());

           monitorProgressProcess->start("/var/cas-mgr/share/data/ForceMovieChange", monitorProgressArgs);
        }
        else
        {
           QLOG_DEBUG()<<"Copy to Device is not entered";
           QMessageBox::warning(this, tr("Cannot Proceed With Changes"), tr("The Copy From Device field is Empty."));
        }
    }
    else
    {
       QLOG_DEBUG()<<"Could not Copy Script Process Exited with code ="<<exitCode;
       QMessageBox::warning(this, tr("Cannot Proceed With Changes"), tr("Script could not Be Copied To Device"));
    }
}

void ForceMovieChangeWidget::onProcessStarted()
{
    QLOG_DEBUG()<<"Movie Change Process Started";
}

void ForceMovieChangeWidget::onProcessErrorOccur(QProcess::ProcessError error)
{
    QLOG_DEBUG()<<"Error Occurred in Movie Change Process"<< error;
}

void ForceMovieChangeWidget::onProcessFinished(int exitCode)
{
    QLOG_DEBUG()<<"Movie Change Process Finished with exitCode = "<< exitCode;
}


