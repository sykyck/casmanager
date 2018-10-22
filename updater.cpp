#include <QNetworkRequest>
#include <QProcess>
#include <QFile>
#include <QFileInfo>
#include <QCoreApplication>
#include "qjson/serializer.h"
#include "qjson/parser.h"
#include "updater.h"
#include "qslog/QsLog.h"
#include "encdec.h"
#include "global.h"

char key[] = {0x54, 0x61, 0x17, 0x18, 0x1e, 0x18, 0x74,
              0x7e, 0x1d, 0x13, 0x4e, 0x13, 0x7a, 0x55,
              0x41, 0x17, 0x57, 0x6a, 0x40, 0x25, 0x4e,
              0x73, 0x6f, 0x73, 0x3c, 0x7a, 0x77, 0x30,
              0x3e, 0x79, 0x70, 0x7f, 0};

Updater::Updater(QString serverURL, int timeout, QObject *parent) : QObject(parent)
{
  netReplyCheck = 0;
  netReplyDownload = 0;  

  netMgr = new QNetworkAccessManager;
  this->serverURL = QUrl(serverURL);

  connectionTimer = new QTimer;
  connectionTimer->setInterval(timeout);
  connectionTimer->setSingleShot(true);
  connect(connectionTimer, SIGNAL(timeout()), this, SLOT(abortConnection()));
}

Updater::~Updater()
{
  netMgr->deleteLater();
  connectionTimer->deleteLater();
}

void Updater::checkForUpdate(QString programName, QString version)
{
  QVariantMap jsonRequest;
  jsonRequest["action"] = "updater";
  jsonRequest["software_name"] = programName;
  jsonRequest["version"] = version;
  jsonRequest["os"] = "Linux";

  QNetworkRequest request(serverURL);

  QJson::Serializer serializer;

  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  netReplyCheck = netMgr->post(request, serializer.serialize(jsonRequest));
  connect(netReplyCheck, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(checkForUpdateError(QNetworkReply::NetworkError)));
  connect(netReplyCheck, SIGNAL(finished()), this, SLOT(checkForUpdateFinished()));
  connect(netReplyCheck, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(resetTimeout(qint64,qint64)));

  // Start timer which will abort the connection when the time expires
  // Connecting the downloadProgress signal to the resetTimeout slot
  // allows resetting the timer everytime the progress changes
  connectionTimer->start();
}

void Updater::downloadFile(QString url)
{
  QUrl fileURL(url);
  QNetworkRequest request(fileURL);

  QLOG_DEBUG() << QString("Downloading: %1").arg(url);

  //request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
  netReplyDownload = netMgr->get(request);
  connect(netReplyDownload, SIGNAL(finished()), this, SLOT(downloadFileFinished()));
  connect(netReplyDownload, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(downloadFileError(QNetworkReply::NetworkError)));
  connect(netReplyDownload, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(resetTimeout(qint64,qint64)));

  // Start timer which will abort the connection when the time expires
  // Connecting the downloadProgress signal to the resetTimeout slot
  // allows resetting the timer everytime the progress changes
  connectionTimer->start();
}

void Updater::checkForUpdateFinished()
{
  connectionTimer->stop();

  // The finished signal is always emitted, even if there was a network error
  if (netReplyCheck->error() == QNetworkReply::NoError)
  {
    QByteArray ba = netReplyCheck->readAll();

    QString response(ba.constData());

    // Response: version_num|url
    if (response.indexOf("|") > 0)
    {
      QStringList updateInfo = response.split("|", QString::SkipEmptyParts);

      if (updateInfo.count() == 2)
      {
        QLOG_DEBUG() << QString("Version %1 available").arg(updateInfo.at(0));
        version = updateInfo.at(0);
        downloadFile(updateInfo.at(1));
      }
      else
      {
        QLOG_ERROR() << QString("Unexpected response: %1").arg(response);
        emit finishedChecking("Received unexpected response from update server.");
      }
    }
    else
    {
      QLOG_DEBUG() << QString("Update server response: %1").arg(ba.constData());
      emit finishedChecking(QString(ba.constData()));
    }
  }
  else
    emit finishedChecking(netReplyCheck->errorString());

  netReplyCheck->deleteLater();
  netReplyCheck = 0;
}

void Updater::checkForUpdateError(QNetworkReply::NetworkError error)
{
  Q_UNUSED(error);

  QLOG_ERROR() << QString("Network error during update check: %1").arg(netReplyCheck->errorString());
}

void Updater::downloadFileFinished()
{
  connectionTimer->stop();

  // The finished signal is always emitted, even if there was a network error
  if (netReplyDownload->error() == QNetworkReply::NoError)
  {
    QLOG_DEBUG() << "Download finished";

    QByteArray ba = netReplyDownload->readAll();

    QFileInfo f(netReplyDownload->url().toString());

    QFile localFile("./" + f.fileName());
    if (!localFile.open(QIODevice::WriteOnly))
    {
      QLOG_ERROR() << QString("Could not open: %1 for writing").arg(localFile.fileName());
    }
    else
    {
      localFile.write(ba);
      localFile.close();

      QString decoded;
      QByteArray password(key);

      //QLOG_DEBUG() << QString("key Length: %1").arg(strlen(key));
      //QLOG_DEBUG() << QString("password Length: " + QString::number(password.length()));

      for (int i = 0; i < password.length(); ++i)
      {
        password[i] = password[i] ^ ((password.length() - i) + 16);

        decoded.append(QChar(password[i]));
      }
      //QLOG_DEBUG() << QString("Decoded password: " + decoded);
      //QLOG_DEBUG() << QString("Decoded password length: " + QString::number(decoded.length()));

      QString decryptedFilename = QString(f.fileName() + ".tar.gz");
      if (EncDec::aesDecryptFile(f.fileName(), decryptedFilename, decoded))
      {
        // Untar files
        QString prog = "tar";
        QStringList arguments;
        arguments << "xvf" << decryptedFilename;
        QProcess process;
        process.start(prog, arguments);

        if (!process.waitForFinished())
          QLOG_ERROR() << "Error extracting files";
        else
          QLOG_DEBUG() << "Extracted files";

        // Starts process and detaches from it so it continues to
        // live even if the calling process exits
        arguments.clear();
        if (!QProcess::startDetached("./install.sh", arguments, QCoreApplication::applicationDirPath()))
        {
          QLOG_ERROR() << "Could not execute script";
        }
        else
        {
          QLOG_DEBUG() << "Executed script";

          // Emit new version number so it can be saved to database
          emit newVersionNum(version);
        }
      }
      else
      {
        QLOG_ERROR() << "Decryption failed!";
      }

    } // endif file open for writing
  }

  emit finishedChecking("Installing software update...");

  netReplyDownload->deleteLater();
  netReplyDownload = 0;
}

void Updater::downloadFileError(QNetworkReply::NetworkError error)
{
  Q_UNUSED(error);

  QLOG_ERROR() << QString("Network error during download: %1").arg(netReplyDownload->errorString());
}

void Updater::resetTimeout(qint64, qint64)
{
  // Restart timer
  connectionTimer->start();
}

void Updater::abortConnection()
{
  QLOG_DEBUG() << "Aborting network connection in Updater due to timeout";

  if (netReplyCheck)
    netReplyCheck->abort();
  else if (netReplyDownload)
    netReplyDownload->abort();
  else
    QLOG_ERROR() << "No network connection appears to be valid, not aborting";
}
