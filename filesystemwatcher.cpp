#include "filesystemwatcher.h"
#include "qslog/QsLog.h"
#include "filehelper.h"

FileSystemWatcher::FileSystemWatcher(QObject *parent) : QObject(parent)
{
  timer = new QTimer;
  timer->setInterval(500);
  connect(timer, SIGNAL(timeout()), this, SLOT(checkMountPoints()));

  monitorTimeout = new QTimer;
  monitorTimeout->setSingleShot(true);
  connect(monitorTimeout, SIGNAL(timeout()), this, SLOT(watchTimedOut()));
}

FileSystemWatcher::~FileSystemWatcher()
{
  stopWatching();
  timer->deleteLater();
}

void FileSystemWatcher::addMountPoint(const QString &path, int timeout)
{
  mountPointsMonitorList.append(path);

  QLOG_DEBUG() << "Setting filesystem watcher timeout to:" << timeout << "ms";
  monitorTimeout->setInterval(timeout);
}

void FileSystemWatcher::checkMountPoints()
{
  // Get directory contents using filter of each path in the list
  // If any files found then return the first one in the list and stop looking
  foreach (QString path, mountPointsMonitorList)
  {
    QString contents = FileHelper::readTextFile("/proc/mounts");

    if (contents.contains(path))
    {
      if (monitorTimeout->isActive())
        monitorTimeout->stop();

      QLOG_DEBUG() << QString("File system was mounted at: %1").arg(path);
      emit mounted(path);
      break;
    }
  }
}

void FileSystemWatcher::watchTimedOut()
{
  QLOG_DEBUG() << "FileSystemWatcher timed out";
  stopWatching();
  emit timeout();
}


void FileSystemWatcher::startWatching()
{
  QLOG_DEBUG() << QString("Now watching mount points: %1").arg(mountPointsMonitorList.join(", "));
  timer->start();

  if (monitorTimeout->interval() > 0)
  {
    QLOG_DEBUG() << "Monitor timeout set for FileSystemWatcher";
    monitorTimeout->start();
  }
}

void FileSystemWatcher::stopWatching()
{
  if (timer->isActive())
  {
    QLOG_DEBUG() << "Stopped watching directories";
    timer->stop();
  }
}
