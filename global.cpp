#include "global.h"
#include "alerter.h"
#include <QProcess>
#include <QFileInfo>
#include <QDir>
#include <QObject>

Global::Global()
{
}

QString Global::getIpAddress()
{
  QString prog = "/bin/sh";
  QStringList arguments;
  arguments << "-c" << "ifconfig eth0 | grep 'inet ' | awk '{print $2}' | sed 's/addr://'";
  QProcess process;
  process.start(prog, arguments);

  QString address;
  if (!process.waitForFinished(10000))
    address = "ERROR";
  else
    address = process.readAll();

  return address;
}

QString Global::getHostname()
{
  QProcess process;
  process.start("hostname");

  QString hostname;
  if (!process.waitForFinished(10000))
    hostname = "ERROR";
  else
    hostname = process.readAll();

  return hostname.trimmed();
}

QString Global::getUptime()
{
  //QString prog = "/bin/sh";
  //QStringList arguments;
  //arguments << "-c" << "uptime";
  QProcess process;
  process.start("uptime");

  QString uptime;
  if (!process.waitForFinished(10000))
    uptime = "ERROR";
  else
    uptime = process.readAll();

  return uptime;
}

QString Global::secondsToHMS(int seconds)
{
  QString res;
  int sec = (int)(seconds % 60);
  seconds /= 60;
  int min = (int)(seconds % 60);
  seconds /= 60;

  int hours = 0;
  if (seconds >= 24)
    hours = seconds;
  else
    hours = (int)(seconds % 24);

  return res.sprintf("%02d:%02d:%02d", hours, min, sec);
}

QStringList Global::grep(QString search, QString filename)
{
  QProcess process;
  process.start(QString("grep \"%1\" \"%2\"").arg(search).arg(filename));

  QStringList result;
  if (process.waitForFinished())
  {
    result = QString(process.readAll()).split("\n", QString::SkipEmptyParts);
  }

  return result;
}

bool Global::isProcessRunning(QString procName)
{
  // If a zero is returned it means the process was found in memory
  return QProcess::execute(QString("sh -c \"pgrep %1\"").arg(procName)) == 0;
}

bool Global::killProcess(QString procName)
{
  return QProcess::execute(QString("sh -c \"killall %1\"").arg(procName)) == 0;
}

bool Global::isValidEmail(QString emailAddress)
{
  QRegExp mailREX("\\b[A-Z0-9._%+-]+@[A-Z0-9.-]+\\.[A-Z]{2,4}\\b");
  mailREX.setCaseSensitivity(Qt::CaseInsensitive);
  mailREX.setPatternSyntax(QRegExp::RegExp);

  return mailREX.exactMatch(emailAddress);
}

// Runs the find command with support for searching multiple patterns and specifying a maximum search depth:
// find /var/cas-mgr/share/videos -maxdepth 3 -name "*.VOB" -o -name "*.IFO"
QStringList Global::find(QString path, QStringList searchList, int maxDepth)
{
  QString maxDepthParam = "";
  if (maxDepth > 0)
    maxDepthParam = QString("-maxdepth %1").arg(maxDepth);

  QString nameParam = "";
  foreach (QString s, searchList)
  {
    if (!nameParam.isEmpty())
      nameParam += QString(" -o -name \"%1\"").arg(s);
    else
      nameParam += QString("-name \"%1\"").arg(s);
  }

  QString findCommand = QString("find \"%1\" %2 %3").arg(path).arg(maxDepthParam).arg(nameParam);

  QProcess process;
  process.start(findCommand);

  QStringList result;
  if (process.waitForFinished())
  {
    result = QString(process.readAll()).split("\n", QString::SkipEmptyParts);
  }

  return result;
}

// Return the available bytes on the specified filesystem
// Example: diskFreeSpace("/dev/sda4")
qint64 Global::diskFreeSpace(QString filesystem)
{
  qint64 availableBytes = -1;

  // The 4th column in the df output shows the available blocks. By default 1 block = 1024 bytes
  // command: df | grep /dev/sda4 | awk '{print $4}'
  QString prog = "/bin/sh";
  QStringList arguments;
  arguments << "-c" << QString("df | grep %1 | awk '{print $4}'").arg(filesystem);
  QProcess process;
  process.start(prog, arguments);

  QString availableBlocks;
  if (process.waitForFinished(5000))
    availableBlocks = process.readAll();

  if (!availableBlocks.isEmpty())
  {
    qint64 numAvailableBlocks = availableBlocks.toLongLong();
    availableBytes = numAvailableBlocks * 1024;
  }

  return availableBytes;
}

// Return the maximum capacity in bytes of the specified filesystem
// Example: maxDiskCapacity("/dev/sda4")
qint64 Global::maxDiskCapacity(QString filesystem)
{
  qint64 maxCapacityBytes = -1;

  // The 2nd column in the df output shows the maximum capacity in blocks. By default 1 block = 1024 bytes
  // command: df | grep /dev/sda4 | awk '{print $2}'
  QString prog = "/bin/sh";
  QStringList arguments;
  arguments << "-c" << QString("df | grep %1 | awk '{print $2}'").arg(filesystem);
  QProcess process;
  process.start(prog, arguments);

  QString maxCapacityBlocks;
  if (process.waitForFinished(5000))
    maxCapacityBlocks = process.readAll();

  if (!maxCapacityBlocks.isEmpty())
  {
    qint64 numMaxCapacityBlocks = maxCapacityBlocks.toLongLong();
    maxCapacityBytes = numMaxCapacityBlocks * 1024;
  }

  return maxCapacityBytes;
}

// Return the total bytes used by the directory or file
// Example: diskUsage("/var/cas-mgr/share/videos/2015/1")
qint64 Global::diskUsage(QString path)
{
  qint64 diskUsageBytes = -1;

  // The 1st column in the du output shows the usage in blocks. By default 1 block = 1024 bytes
  // The -s option displays a summary instead of every subdirectory and file within specified path
  // command: du -s /path/to/dir | awk '{print $1}'
  QString prog = "/bin/sh";
  QStringList arguments;
  arguments << "-c" << QString("du -s %1 | awk '{print $1}'").arg(path);
  QProcess process;
  process.start(prog, arguments);

  QString diskUsageBlocks;
  if (process.waitForFinished(5000))
    diskUsageBlocks = process.readAll();

  if (!diskUsageBlocks.isEmpty())
  {
    qint64 numDiskUsageBlocks = diskUsageBlocks.toLongLong();
    diskUsageBytes = numDiskUsageBlocks * 1024;
  }

  return diskUsageBytes;
}

void Global::sendAlert(QString message)
{
  Alerter *alerter = new Alerter(WEB_SERVICE_URL);
  QObject::connect(alerter, SIGNAL(finishedSending()), alerter, SLOT(deleteLater()));

  alerter->sendAlert(SOFTWARE_NAME, SOFTWARE_VERSION, message);
}

QString Global::randomString(int length)
{
  static QString possibleCharacters("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");

  QString rndString;

  for (int i = 0; i < length; ++i)
  {
    int index = qrand() % possibleCharacters.length();
    QChar nextChar = possibleCharacters.at(index);
    rndString.append(nextChar);
  }

  return rndString;
}

bool Global::unmountFilesystem(QString filesystem)
{
  return QProcess::execute(QString("umount \"%1\"").arg(filesystem)) == 0;
}
