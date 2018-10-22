#ifndef FILESYSTEMWATCHER_H
#define FILESYSTEMWATCHER_H

#include <QObject>
#include <QStringList>
#include <QTimer>

class FileSystemWatcher : public QObject
{
  Q_OBJECT
public:
  explicit FileSystemWatcher(QObject *parent = 0);
  ~FileSystemWatcher();
  void addMountPoint(const QString &path, int timeout = 0);
  
signals:
  void mounted(const QString &path);
  void timeout();

public slots:
  void startWatching();
  void stopWatching();
  
private slots:
  void checkMountPoints();
  void watchTimedOut();

private:
  QTimer *timer;
  QTimer *monitorTimeout;
  QStringList mountPointsMonitorList;
};

#endif // FILESYSTEMWATCHER_H
