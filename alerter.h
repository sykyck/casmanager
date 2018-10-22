#ifndef ALERTER_H
#define ALERTER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QString>
#include <QNetworkReply>
#include <QTimer>

class Alerter : public QObject
{
  Q_OBJECT
public:
  explicit Alerter(QString serverURL, QObject *parent = 0);
  ~Alerter();

  void sendAlert(QString programName, QString version, QString message);

signals:
  void finishedSending();

public slots:

private slots:
  void sendAlertFinished();
  void sendAlertError(QNetworkReply::NetworkError error);

  void resetTimeout(qint64,qint64);
  void abortConnection();

private:
  QNetworkAccessManager *netMgr;
  QNetworkReply *netReplyCheck;
  QString serverURL;
  QTimer *connectionTimer;
};

#endif // ALERTER_H
