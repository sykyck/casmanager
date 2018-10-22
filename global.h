#ifndef GLOBAL_H
#define GLOBAL_H

#include <QString>
#include <QStringList>

const QString SOFTWARE_NAME = "US Arcades CAS Manager";
const QString SOFTWARE_VERSION = "1.1.29"; // TODO: Always check version number before releasing!!!
const QString WEB_SERVICE_URL = "https://ws.usarcades.com/ws";

class Global
{
public:
  Global();

  static QString getIpAddress();
  static QString getHostname();
  static QString getUptime();
  static QString secondsToHMS(int seconds);
  static QStringList grep(QString search, QString filename);
  static bool isProcessRunning(QString procName);
  static bool killProcess(QString procName);
  static bool isValidEmail(QString emailAddress);
  static QStringList find(QString path, QStringList searchList, int maxDepth = 0);
  static qint64 diskFreeSpace(QString filesystem);
  static bool unmountFilesystem(QString filesystem);
  static qint64 maxDiskCapacity(QString filesystem);
  static qint64 diskUsage(QString path);
  static void sendAlert(QString message);
  static QString randomString(int length);
};

#endif // GLOBAL_H
