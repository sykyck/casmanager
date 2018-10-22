#include "dvdcopier.h"
#include "filehelper.h"
#include "qslog/QsLog.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>

DvdCopier::DvdCopier(QString dvdCopyProg, QString dvdCopyLogFile, QString dvdCopyProcName, QObject *parent) : QObject(parent)
{
  this->dvdCopyProg = dvdCopyProg;
  this->dvdCopyLogFile = dvdCopyLogFile;
  this->dvdCopyProcName = dvdCopyProcName;

  timeoutMinutes = 0;

  copyingTimeout = new QTimer;
  copyingTimeout->setSingleShot(true);
  connect(copyingTimeout, SIGNAL(timeout()), this, SLOT(copyingTimedOut()));

  dvdCopyProc = new QProcess;
  connect(dvdCopyProc, SIGNAL(started()), this, SIGNAL(startedCopying()));
  connect(dvdCopyProc, SIGNAL(finished(int)), this, SLOT(dvdCopyProcFinished(int)));
}

bool DvdCopier::isCopying()
{
  return dvdCopyProc->state() != QProcess::NotRunning;
}

// srcDriveLetter = h
// srcDevPath = /dev/cdrom1
// destPath = r:\\2014\\20
void DvdCopier::copyDvd(QString srcDriveLetter, QString srcDevPath, QString destPath, int timeout)
{
  QLOG_DEBUG() << "copyDvd - srcDriveLetter: " << srcDriveLetter << ", srcDevPath: " << srcDevPath << ", destPath: " << destPath << ", timeout: " << timeout;

  timeoutMinutes = timeout;

  if (timeoutMinutes > 0)
  {
    // Timeout is in minutes, convert to milliseconds
    copyingTimeout->setInterval(timeoutMinutes * 60 * 1000);
    copyingTimeout->start();
  }

  // Delete last log file
  QFile::remove(dvdCopyLogFile);  

  QStringList arguments;
  arguments << srcDriveLetter << srcDevPath << destPath;
  dvdCopyProc->start(dvdCopyProg, arguments);
}

void DvdCopier::terminateCopying()
{
  QLOG_DEBUG() << "Terminating DVD copying process, triggering timeout";

  if (copyingTimeout->isActive())
    copyingTimeout->stop();

  copyingTimedOut();
}

void DvdCopier::copyingTimedOut()
{
  QLOG_DEBUG() << "DVD copy process timed out";

  // Kill the script process
  dvdCopyProc->kill();

  // Kill the DVD copying process as well
  QProcess p;
  p.start("pkill " + dvdCopyProcName);
  p.waitForFinished();

  // Not sure if dvdCopyProcFinished is still called by dvdCopyProc

  emit finishedCopying(false, tr("DVD copy process timed out after %1 minutes.").arg(timeoutMinutes));
}

void DvdCopier::dvdCopyProcFinished(int exitCode)
{
  QLOG_DEBUG() << "DVD copy process finished with exit code: " << exitCode;

  if (copyingTimeout->isActive())
    copyingTimeout->stop();

  // Check DVD copy log file
  // if dvd copy successfully then send signal true
  // else dvd copy failed, send sginal false
  QString log = FileHelper::readTextFile(dvdCopyLogFile);

  if (!log.isEmpty())
  {
    if (log.contains("Operation Successfully Completed", Qt::CaseInsensitive))
    {
      QLOG_DEBUG() << "DVD copied successfully according to log file: " << dvdCopyLogFile;

      emit finishedCopying(true);
    }
    else
    {
      QLOG_ERROR() << "DVD copy failed according to log file: " << dvdCopyLogFile;

      emit finishedCopying(false, "The DVD could not be copied.");
    }
  }
  else
  {
    QLOG_ERROR() << "Could not find the log file: " << dvdCopyLogFile;

    emit finishedCopying(false, "Could not find the log file from the DVD copy process.");
  }
}
