#include <QNetworkRequest>
#include <QUrl>
#include "alerter.h"
#include "qjson/serializer.h"
#include "qjson/parser.h"
#include "qslog/QsLog.h"
#include "global.h"

// Maximum time in msec to wait for a network connection
const int CONNECTION_TIMEOUT = 15000;

Alerter::Alerter(QString serverURL, QObject *parent) : QObject(parent)
{
  netReplyCheck = 0;

  netMgr = new QNetworkAccessManager;
  this->serverURL = serverURL;

  connectionTimer = new QTimer;
  connectionTimer->setInterval(CONNECTION_TIMEOUT);
  connectionTimer->setSingleShot(true);
}

Alerter::~Alerter()
{
  netMgr->deleteLater();
  connectionTimer->deleteLater();
}

void Alerter::sendAlert(QString programName, QString version, QString message)
{
  QLOG_DEBUG() << "Sending: " + programName + ", " + version + ", " + message;  

  QVariantMap jsonRequest;
  jsonRequest["action"] = "alert";
  jsonRequest["program_name"] = programName;
  jsonRequest["version"] = version;
  jsonRequest["ip_address"] = Global::getIpAddress();
  jsonRequest["hostname"] = Global::getHostname();
  jsonRequest["uptime"] = Global::getUptime();
  jsonRequest["message"] = message;

  QUrl url = QUrl(serverURL);
  QNetworkRequest request(url);

  QJson::Serializer serializer;

  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  netReplyCheck = netMgr->post(request, serializer.serialize(jsonRequest));
  connect(netReplyCheck, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(sendAlertError(QNetworkReply::NetworkError)));
  connect(netReplyCheck, SIGNAL(finished()), this, SLOT(sendAlertFinished()));
  connect(netReplyCheck, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(resetTimeout(qint64,qint64)));

  // Start timer which will abort the connection when the time expires
  // Connecting the downloadProgress signal to the resetTimeout slot
  // allows resetting the timer everytime the progress changes
  connect(connectionTimer, SIGNAL(timeout()), this, SLOT(abortConnection()));
  connectionTimer->start();
}

void Alerter::sendAlertFinished()
{
  connectionTimer->stop();

  // The finished signal is always emitted, even if there was a network error
  if (netReplyCheck->error() == QNetworkReply::NoError)
    QLOG_DEBUG() << "Alert was sent";

  emit finishedSending();

  netReplyCheck->deleteLater();
  netReplyCheck = 0;
}

void Alerter::sendAlertError(QNetworkReply::NetworkError error)
{
  Q_UNUSED(error);

  QLOG_ERROR() << QString("Network error while sending alert: %1").arg(netReplyCheck->errorString());
}

void Alerter::resetTimeout(qint64, qint64)
{
  // Restart timer
  connectionTimer->start();
}

void Alerter::abortConnection()
{
  QLOG_DEBUG() << "Aborting network connection in Alerter due to timeout";

  if (netReplyCheck)
    netReplyCheck->abort();
  else
    QLOG_ERROR() << "No network connection appears to be valid, not aborting";
}
