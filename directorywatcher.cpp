#include "directorywatcher.h"
#include "qslog/QsLog.h"
#include <QDir>
#include <QFileInfo>
#include <QStringList>

DirectoryWatcher::DirectoryWatcher(QObject *parent) : QObject(parent)
{
  timer = new QTimer;
  timer->setInterval(100);
  connect(timer, SIGNAL(timeout()), this, SLOT(checkDirectories()));

  monitorTimeout = new QTimer;
  monitorTimeout->setSingleShot(true);
  connect(monitorTimeout, SIGNAL(timeout()), this, SLOT(watchTimedOut()));
}

DirectoryWatcher::~DirectoryWatcher()
{
  stopWatching();
  timer->deleteLater();
}

void DirectoryWatcher::addPath(const QString &path, const QStringList &filterList, int timeout)
{
  dirMonitorList.append(path);
  this->filterList = filterList;

  QLOG_DEBUG() << "Setting directory watcher timeout to:" << timeout << "ms";
  monitorTimeout->setInterval(timeout);
}

void DirectoryWatcher::checkDirectories()
{
  // Get directory contents using filter of each path in the list
  // If any files found then return the first one in the list and stop looking
  foreach (QString path, dirMonitorList)
  {
    QDir d = QDir(path);
    QStringList fileList = d.entryList(filterList, QDir::Files);

    if (fileList.count() > 0)
    {
      if (monitorTimeout->isActive())
        monitorTimeout->stop();

      // FIXME: Somehow this gets caught in an infinite loop on rare occassions even though the stopWatching slot
      // is called after this signal is received. Maybe try stopping timer before emitting signal???
      QLOG_DEBUG() << QString("File found: %1").arg(fileList.at(0));
      emit fileFound(d.absoluteFilePath(fileList.at(0)));
      break;
    }
  }
}

void DirectoryWatcher::watchTimedOut()
{
  QLOG_DEBUG() << "DirectoryWatcher timed out";
  stopWatching();
  emit timeout();
}


void DirectoryWatcher::startWatching()
{
  QLOG_DEBUG() << QString("Now watching directories: %1 with filters: %2").arg(dirMonitorList.join(", ")).arg(filterList.join(", "));
  timer->start();

  if (monitorTimeout->interval() > 0)
  {
    QLOG_DEBUG() << "Monitor timeout set for DirectoryWatcher";
    monitorTimeout->start();
  }
}

void DirectoryWatcher::stopWatching()
{
  if (timer->isActive())
  {
    QLOG_DEBUG() << "Stopped watching directories";
    timer->stop();
  }
}
