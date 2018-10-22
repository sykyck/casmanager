#ifndef DVDCOPIER_H
#define DVDCOPIER_H

#include <QObject>
#include <QTimer>
#include <QProcess>
#include <QString>
#include <QStringList>
#include "directorywatcher.h"

// Uses external program to make a copy of the DVD
// Copied files are placed in a directory specified by caller
// Provides feedback of when copy process starts and ends
// Can have a timeout set to terminate copy process if it's taking too long
class DvdCopier : public QObject
{
  Q_OBJECT
public:
  explicit DvdCopier(QString dvdCopyProg, QString dvdCopyLogFile, QString dvdCopyProcName, QObject *parent = 0);
  bool isCopying();

signals:
  void startedCopying();
  void finishedCopying(bool success, QString errorMessage = QString());
  
public slots:
  void copyDvd(QString srcDriveLetter, QString srcDevPath, QString destPath, int timeout = 0);
  void terminateCopying();
  
private slots:
  void copyingTimedOut();
  void dvdCopyProcFinished(int exitCode);

private:
  QTimer *copyingTimeout;
  int timeoutMinutes;
  QProcess *dvdCopyProc;
  QString dvdCopyProg;
  QString dvdCopyLogFile;
  QString dvdCopyProcName;
  QString srcDevPath;
  QString srcDriveLetter;
  QString destPath;
  DirectoryWatcher *dirWatcher;
};

#endif // DVDCOPIER_H
