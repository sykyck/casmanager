#ifndef UPDATER_H
#define UPDATER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QString>
#include <QUrl>
#include <QNetworkReply>
#include <QTimer>

class Updater : public QObject
{
  Q_OBJECT
public:
  // timeout is in ms, it is the amount of time to wait before aborting the network connection
  explicit Updater(QString serverURL, int timeout = 15000, QObject *parent = 0);
  ~Updater();

  void checkForUpdate(QString programName, QString version);
  void downloadFile(QString url);

signals:
  void finishedChecking(QString response);
  void newVersionNum(QString version);

public slots:

private slots:
  void checkForUpdateFinished();
  void checkForUpdateError(QNetworkReply::NetworkError error);
  void downloadFileFinished();
  void downloadFileError(QNetworkReply::NetworkError error);
  void resetTimeout(qint64,qint64);
  void abortConnection();

private:
  QNetworkAccessManager *netMgr;
  QNetworkReply *netReplyCheck;
  QNetworkReply *netReplyDownload;
  QUrl serverURL;
  QString version;

  QTimer *connectionTimer;
};

#endif // UPDATER_H
