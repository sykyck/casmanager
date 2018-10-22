#ifndef DIRECTORYWATCHER_H
#define DIRECTORYWATCHER_H

#include <QObject>
#include <QStringList>
#include <QTimer>

class DirectoryWatcher : public QObject
{
  Q_OBJECT
public:
  explicit DirectoryWatcher(QObject *parent = 0);
  ~DirectoryWatcher();
  void addPath(const QString &path, const QStringList &filterList, int timeout = 0);
  
signals:
  void fileFound(const QString &file);
  void timeout();

public slots:
  void startWatching();
  void stopWatching();
  
private slots:
  void checkDirectories();
  void watchTimedOut();

private:
  QTimer *timer;
  QTimer *monitorTimeout;
  QStringList dirMonitorList;
  QStringList filterList;
};

#endif // DIRECTORYWATCHER_H
