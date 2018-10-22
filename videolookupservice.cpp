#include "videolookupservice.h"
#include <QStringList>
#include <QFileInfo>
#include "qjson-backport/qjsondocument.h"
#include "qjson-backport/qjsonobject.h"
#include "qjson/serializer.h"
#include "qjson/parser.h"
#include "global.h"

// Maximum number of times we retry to contact server
const int MAX_RETRIES = 3;

// Maximum time in msec to wait for a network connection
const int CONNECTION_TIMEOUT = 15000;

VideoLookupService::VideoLookupService(QString webServiceUrl, QString downloadPath, QString password, QObject *parent) : QObject(parent)
{
  this->webServiceUrl = QUrl(webServiceUrl);
  this->downloadPath = QDir(downloadPath);
  this->password = password;
  currentVideoCoverUrl.clear();

  formPost = new FormPostPlugin;

  netReplyVideoLookup = 0;
  netReplyDownload = 0;  
  netReplyDeleteVideoLookup = 0;
  netReplyVideoUpdate = 0;
  numRetries = 0;

  netMgr = new QNetworkAccessManager;
  //connect(netMgr, SIGNAL(finished(QNetworkReply*)), this, SLOT(networkActivityFinished(QNetworkReply*)));

  timer = new QTimer;
  timer->setSingleShot(true);
  timer->setInterval(1 + (qrand() % 30000));
  connect(timer, SIGNAL(timeout()), this, SLOT(videoLookup()));

  connectionTimer = new QTimer;
  connectionTimer->setInterval(CONNECTION_TIMEOUT);
  connectionTimer->setSingleShot(true);
  connect(connectionTimer, SIGNAL(timeout()), this, SLOT(abortConnection()));

  QDir path(downloadPath);
  if (!path.exists())
    path.mkpath(downloadPath);
}

VideoLookupService::~VideoLookupService()
{
  timer->deleteLater();
  netMgr->deleteLater();

  if (netReplyVideoLookup)
    netReplyVideoLookup->deleteLater();

  if (netReplyDownload)
    netReplyDownload->deleteLater();

  if (netReplyVideoUpdate)
    netReplyVideoUpdate->deleteLater();

  if (netReplyDeleteVideoLookup)
    netReplyDeleteVideoLookup->deleteLater();

  connectionTimer->deleteLater();

  formPost->deleteLater();
}

void VideoLookupService::startVideoLookup(QStringList upcList)
{
  upcLookupList = upcList;

  videoLookup();
}

void VideoLookupService::startVideoUpdate(const Movie &movie, const QString &frontCover, const QString &backCover)
{
  QVariantMap jsonRequest;
  jsonRequest["action"] = "updateUPC";
  jsonRequest["upc"] = movie.UPC();
  jsonRequest["title"] = movie.Title();
  jsonRequest["producer"] = movie.Producer();
  jsonRequest["category"] = movie.Category();
  jsonRequest["subcategory"] = movie.Subcategory();
  jsonRequest["passphrase"] = password;

  // If the video cover files were specified then include "covers" flag to indicate
  // to server that we are including the files
  if (!frontCover.isEmpty())
  {
    jsonRequest["covers"] = "1";
    formPost->addFile("front_cover", frontCover, "application/octet-stream");
    formPost->addFile("back_cover", backCover, "application/octet-stream");
  }

  QJson::Serializer serializer;
  formPost->addField("msg", QString(serializer.serialize(jsonRequest)));

  netReplyVideoUpdate = formPost->postData(webServiceUrl.toString());
  //netReplyVideoUpload->setProperty("ActionType", VIDEO_UPDATE);

  connect(netReplyVideoUpdate, SIGNAL(finished()), this, SLOT(videoUpdateFinished()));
  //connect(netReplyVideoUpdate, SIGNAL(uploadProgress(qint64,qint64)), this, SLOT(uploadProgress(qint64,qint64)));
}

void VideoLookupService::startDeleteVideoLookup(QStringList upcList)
{
  deleteUpcList = upcList;

  QLOG_DEBUG() << QString("Now deleting the following UPCs from the video lookup queue: %1").arg(deleteUpcList.join(", "));

  QVariantMap jsonRequest;
  jsonRequest["action"] = "deleteUpcQueue";
  jsonRequest["upc_list"] = deleteUpcList;
  jsonRequest["passphrase"] = password;

  //qDebug(qPrintable(data));
  QNetworkRequest request(webServiceUrl);

  QJson::Serializer serializer;

  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  netReplyDeleteVideoLookup = netMgr->post(request, serializer.serialize(jsonRequest));

  connect(netReplyDeleteVideoLookup, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(deleteVideoLookupError(QNetworkReply::NetworkError)));
  connect(netReplyDeleteVideoLookup, SIGNAL(finished()), this, SLOT(receivedDeleteVideoLookupResponse()));
  connect(netReplyDeleteVideoLookup, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(resetTimeout(qint64,qint64)));

  // Start timer which will abort the connection when the time expires
  // Connecting the downloadProgress signal to the resetTimeout slot
  // allows resetting the timer everytime the progress changes
  connectionTimer->start();
}

void VideoLookupService::startProducerListing()
{
  QVariantMap jsonRequest;
  jsonRequest["action"] = "getProducerListing";
  jsonRequest["passphrase"] = password;

  QNetworkRequest request(webServiceUrl);

  QJson::Serializer serializer;

  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  netReplyProducerListing = netMgr->post(request, serializer.serialize(jsonRequest));

  connect(netReplyProducerListing, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(producerListingError(QNetworkReply::NetworkError)));
  connect(netReplyProducerListing, SIGNAL(finished()), this, SLOT(receivedProducerListingResponse()));
  connect(netReplyProducerListing, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(resetTimeout(qint64,qint64)));

  // Start timer which will abort the connection when the time expires
  // Connecting the downloadProgress signal to the resetTimeout slot
  // allows resetting the timer everytime the progress changes
  connectionTimer->start();
}

void VideoLookupService::videoLookup()
{
  QLOG_DEBUG() << QString("Now looking up the following UPCs: %1").arg(upcLookupList.join(", "));

  QVariantMap jsonRequest;
  jsonRequest["action"] = "lookupUPCs";
  jsonRequest["upc_list"] = upcLookupList;
  jsonRequest["passphrase"] = password;

  //qDebug(qPrintable(data));
  QNetworkRequest request(webServiceUrl);

  QJson::Serializer serializer;
  //QByteArray convertedData = serializer.serialize(jsonRequest);

  //QString postRequest = QString("msg=%1").arg(serializer.serialize(jsonRequest).constData());
  //QLOG_DEBUG() << QString("Sending: %1").arg(serializer.serialize(jsonRequest).constData());
  //QByteArray convertedData(qPrintable(postRequest));

  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  netReplyVideoLookup = netMgr->post(request, serializer.serialize(jsonRequest));
  //reply->setProperty("ActionType", VIDEO_LOOKUP);

  connect(netReplyVideoLookup, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(videoLookupError(QNetworkReply::NetworkError)));
  connect(netReplyVideoLookup, SIGNAL(finished()), this, SLOT(receivedVideoLookupResponse()));
  connect(netReplyVideoLookup, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(resetTimeout(qint64,qint64)));

  // Start timer which will abort the connection when the time expires
  // Connecting the downloadProgress signal to the resetTimeout slot
  // allows resetting the timer everytime the progress changes
  connectionTimer->start();
}

void VideoLookupService::finishedVideoLookup(bool success)
{
  if (success || ++numRetries > MAX_RETRIES)
  {
    if (!success && numRetries > MAX_RETRIES)
    {
      QLOG_ERROR() << QString("Giving up on looking up videos, reached maximum retries");

      Global::sendAlert("Giving up on looking up videos, reached maximum retries");
    }

    // Done with UPCs that were submitted by caller
    upcLookupList.clear();

    // Reset counter for next operations
    numRetries = 0;

    // Start processing video lookup results, this will be called each time
    // until there are no more video covers to download
    processVideoLookupResults();
  }
  else
  {
    disconnect(timer, 0, 0, 0);
    connect(timer, SIGNAL(timeout()), this, SLOT(videoLookup()));

    timer->setInterval(1 + (qrand() % 30000));
    QLOG_DEBUG() << QString("Retrying to lookup videos on server in %1 ms. Retry: %2").arg(timer->interval()).arg(numRetries);
    timer->start();
  }
}

void VideoLookupService::receivedVideoLookupResponse()
{
  connectionTimer->stop();

  bool success = false;

  // Clear lists of any items from the last lookup
  videoList.clear();
  videoCoverList.clear();
  videoCoverUpcList.clear();

  // The finished signal is always emitted, even if there was a network error
  if (netReplyVideoLookup->error() == QNetworkReply::NoError)
  {
    QByteArray json = netReplyVideoLookup->readAll();

    // Expected response is in JSON
    // [ {"upc":"", "title":"", "producer":"", "category":"", "subcategory":"", "front_cover":"", "back_cover":""},
    //   {"upc":"", "title":"", "producer":"", "category":"", "subcategory":"", "front_cover":"", "back_cover":""}, ... ]

    // If the server has a problem with what was sent, the response will be:
    // { "error" : "description" }

    QString response(json.constData());

    bool ok = false;
    QJson::Parser parser;
    QVariant var = parser.parse(json, &ok);

    if (ok)
    {
      QLOG_DEBUG() << QString("Received valid JSON response: %1").arg(response);

      if (var.toMap().contains("error"))
      {
        QLOG_ERROR() << QString("Server returned an error: %1").arg(var.toMap()["error"].toString());
      }
      else
      {
        // Build list of URLs to the video cover files to download
        // and store each record in list to emit to caller after
        // file downloads are complete
        foreach (QVariant v, var.toList())
        {
          QVariantMap map = v.toMap();          

          Movie video;
          video.setUPC(map["upc"].toString());
          video.setTitle(map["title"].toString());
          video.setProducer(map["producer"].toString());
          video.setCategory(map["category"].toString());
          video.setSubcategory(map["subcategory"].toString());

          // Check to see if cover files are available
          if (map["front_cover"].toString().length() > 0)
          {
            videoCoverList.append(map["front_cover"].toString());

            // Replace the front and back cover URLs with <upc>.jpg and <upc>b.jpg
            video.setFrontCover(QString("%1.jpg").arg(map["upc"].toString()));

            // Put UPC associated with this cover in a list in case we need to delete
            // the metadata from the videoList later because the cover could not be downloaded.
            // There's a more elegant way of doing this but I don't have time...
            videoCoverUpcList.append(map["upc"].toString());
          }
          else
            QLOG_DEBUG() << "No front cover available for UPC";

          if (map["back_cover"].toString().length() > 0)
          {
            videoCoverList.append(map["back_cover"].toString());
            video.setBackCover(QString("%1b.jpg").arg(map["upc"].toString()));

            // Put UPC associated with this cover in a list in case we need to delete
            // the metadata from the videoList later because the cover could not be downloaded.
            // There's a more elegant way of doing this but I don't have time...
            videoCoverUpcList.append(map["upc"].toString());
          }
          else
            QLOG_DEBUG() << "No back cover available for UPC";

          videoList.append(video);
        }

        QLOG_DEBUG() << "UPCs identified:" << videoList.count() << ", number of video covers:" << videoCoverList.count();
      }

      success = true;
    }
    else
    {
      QLOG_ERROR() << QString("Did not receive valid JSON response: %1").arg(response);
    }
  }

  netReplyVideoLookup->deleteLater();
  netReplyVideoLookup = 0;

  finishedVideoLookup(success);
}

void VideoLookupService::videoLookupError(QNetworkReply::NetworkError error)
{
  Q_UNUSED(error);

  QLOG_ERROR() << QString("Network error during video lookup: %1").arg(netReplyVideoLookup->errorString());
}

void VideoLookupService::videoUpdateFinished()
{
  if (netReplyVideoUpdate->error() == QNetworkReply::NoError)
  {
    emit videoUpdateResults(true, QString(formPost->response().constData()));
  }
  else
  {
    emit videoUpdateResults(false, QString(formPost->response().constData()));
  }

  netReplyVideoUpdate->deleteLater();
  netReplyVideoUpdate = 0;
}

void VideoLookupService::receivedDeleteVideoLookupResponse()
{
  connectionTimer->stop();

  if (netReplyDeleteVideoLookup->error() == QNetworkReply::NoError)
  {
    QByteArray json = netReplyDeleteVideoLookup->readAll();

    // Expected response is in JSON
    // { "result" : "description" }

    // If the server has a problem with what was sent, the response will be:
    // { "error" : "description" }

    QString response(json.constData());

    bool ok = false;
    QJson::Parser parser;
    QVariant var = parser.parse(json, &ok);

    if (ok)
    {
      QLOG_DEBUG() << QString("Received valid JSON response: %1").arg(response);

      if (var.toMap().contains("error"))
      {
        QLOG_ERROR() << QString("Server returned an error: %1").arg(var.toMap()["error"].toString());
        emit deleteVideoLookupResults(false, var.toMap()["error"].toString());
      }
      else if (var.toMap().contains("result"))
      {
        emit deleteVideoLookupResults(true, var.toMap()["result"].toString(), deleteUpcList);
      }
      else
      {
        QLOG_ERROR() << QString("Server returned an error: %1").arg(var.toMap()["error"].toString());
        emit deleteVideoLookupResults(false, "Response from server did not contain expected layout.");
      }
    }
    else
    {
      QLOG_ERROR() << "Server did not return a valid JSON response";
      emit deleteVideoLookupResults(false, "Invalid response from server.");
    }
  }
  else
  {
    emit deleteVideoLookupResults(false, netReplyDeleteVideoLookup->errorString());
  }

  netReplyDeleteVideoLookup->deleteLater();
  netReplyDeleteVideoLookup = 0;
}

void VideoLookupService::deleteVideoLookupError(QNetworkReply::NetworkError error)
{
  Q_UNUSED(error);

  QLOG_ERROR() << QString("Network error during delete video lookup: %1").arg(netReplyDeleteVideoLookup->errorString());
}

void VideoLookupService::receivedProducerListingResponse()
{
  connectionTimer->stop();

  bool success = false;
  QStringList producerList;

  // The finished signal is always emitted, even if there was a network error
  if (netReplyProducerListing->error() == QNetworkReply::NoError)
  {
    QByteArray json = netReplyProducerListing->readAll();

    // Expected response is in JSON
    // [ "producer1", producer2", ... ]

    // If the server has a problem with what was sent, the response will be:
    // { "error" : "description" }

    QString response(json.constData());

    bool ok = false;
    QJson::Parser parser;
    QVariant var = parser.parse(json, &ok);

    if (ok)
    {
      QLOG_DEBUG() << QString("Received valid JSON response: %1").arg(response);

      if (var.toMap().contains("error"))
      {
        QLOG_ERROR() << QString("Server returned an error: %1").arg(var.toMap()["error"].toString());
      }
      else
      {
        producerList = var.toStringList();
      }

      success = true;
    }
    else
    {
      QLOG_ERROR() << QString("Did not receive valid JSON response: %1").arg(response);
    }
  }

  emit producerListingResults(producerList);

  netReplyProducerListing->deleteLater();
  netReplyProducerListing = 0;
}

void VideoLookupService::producerListingError(QNetworkReply::NetworkError error)
{
  Q_UNUSED(error);

  QLOG_ERROR() << QString("Network error during producer listing: %1").arg(netReplyProducerListing->errorString());
}

void VideoLookupService::downloadFileReceived()
{
  connectionTimer->stop();

  // The finished signal is always emitted, even if there was a network error
  if (netReplyDownload->error() == QNetworkReply::NoError)
  {
    QLOG_DEBUG() << "Download video cover file finished";

    QByteArray ba = netReplyDownload->readAll();

    QFileInfo f(netReplyDownload->url().toString());

    QFile localFile(downloadPath.absoluteFilePath(f.fileName()));
    if (!localFile.open(QIODevice::WriteOnly))
    {
      QLOG_ERROR() << QString("Could not open: %1 for writing").arg(downloadPath.absoluteFilePath(f.fileName()));

      // Send error alert
      /*if (!alerter)
        alerter = new Alerter(DEFAULT_ALERT_SERVICE_URL);

      alerter->sendAlert(SOFTWARE_NAME, SOFTWARE_VERSION, QString("Could not open: %1 for writing").arg(localFile.fileName()));*/
    }
    else
    {
      localFile.write(ba);
      localFile.close();

      // Successfully downloaded, remove URL from list
      videoCoverList.removeFirst();
      videoCoverUpcList.removeFirst();

    } // endif file open for writing
  }

  netReplyDownload->deleteLater();
  netReplyDownload = 0;

  processVideoLookupResults();
}

void VideoLookupService::downloadFileError(QNetworkReply::NetworkError error)
{
  Q_UNUSED(error);

  QLOG_ERROR() << QString("Network error during video cover download: %1").arg(netReplyDownload->errorString());
}

void VideoLookupService::resetTimeout(qint64, qint64)
{
  // Restart timer
  connectionTimer->start();
}

void VideoLookupService::abortConnection()
{
  QLOG_DEBUG() << "Aborting network connection for video lookup due to timeout";

  if (netReplyVideoLookup)
    netReplyVideoLookup->abort();
  else if (netReplyDownload)
    netReplyDownload->abort();
  else if (netReplyVideoUpdate)
    netReplyVideoUpdate->abort();
  else if (netReplyDeleteVideoLookup)
    netReplyDeleteVideoLookup->abort();
  else
    QLOG_ERROR() << "No network connection appears to be valid, not aborting";
}

void VideoLookupService::networkActivityFinished(QNetworkReply *reply)
{
  /*
  switch (reply->property("ReplyType").toInt())
  {
    case VIDEO_LOOKUP:
    {
      qDebug() << "VIDEO_LOOKUP";

      if (reply->error() == QNetworkReply::NoError)
      {

      }
      else
      {
        QLOG_ERROR() << QString("Network error during video lookup: %1").arg(reply->errorString());
      }
    }
      break;

    case VIDEO_UPDATE:
    {
      qDebug() << "VIDEO_UPDATE";

      if (reply->error() == QNetworkReply::NoError)
      {

      }
      else
      {
        QLOG_ERROR() << QString("Network error during video lookup: %1").arg(reply->errorString());
      }
    }
      break;

    case VIDEO_ATTRIBUTES:
    {
      qDebug() << "VIDEO_ATTRIBUTES";

      if (reply->error() == QNetworkReply::NoError)
      {

      }
      else
      {
        QLOG_ERROR() << QString("Network error during video lookup: %1").arg(reply->errorString());
      }
    }
      break;

    default:
      qDebug() << "Unknown type!!";
  }

  // Important to release the QNetworkReply object since it's our responsibility!
  reply->deleteLater();
  */
}

void VideoLookupService::downloadFile(QString url)
{
  QUrl fileURL(url);
  QNetworkRequest request(fileURL);

  QLOG_DEBUG() << QString("Downloading: %1").arg(url);

  //request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
  netReplyDownload = netMgr->get(request);
  connect(netReplyDownload, SIGNAL(finished()), this, SLOT(downloadFileReceived()));
  connect(netReplyDownload, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(downloadFileError(QNetworkReply::NetworkError)));
  connect(netReplyDownload, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(resetTimeout(qint64,qint64)));

  // Start timer which will abort the connection when the time expires
  // Connecting the downloadProgress signal to the resetTimeout slot
  // allows resetting the timer everytime the progress changes
  connectionTimer->start();
}

void VideoLookupService::processVideoLookupResults()
{
  bool finished = true;

  if (!videoCoverList.isEmpty())
  {
    // If the current video is the same as the video at the head of the list then
    // we must be trying to download the file again, increment the retry counter
    // to keep track of the iterations
    if (currentVideoCoverUrl == videoCoverList.first())
    {
      if (++numRetries > MAX_RETRIES)
      {
        QLOG_ERROR() << QString("Giving up on downloading video cover: %1, reached maximum retries").arg(currentVideoCoverUrl);

        QString upc = videoCoverUpcList.first();
        videoCoverUpcList.removeFirst();

        // Find and remove the Movie object so the rest of the metadata isn't used. If we were
        // to allow it to stay in the list then the caller would assume the metadata is complete with both covers
        QList<Movie>::iterator m;
        for (m = videoList.begin(); m != videoList.end(); ++m)
        {
          if (((Movie)*m).UPC() == upc)
          {
            //QLOG_DEBUG() << "Found UPC:" << upc << "in videoList, now removing since the cover couldn't be downloaded";

            videoList.removeOne((Movie)*m);
            /*
            if (videoList.removeOne((Movie)*m))
              QLOG_DEBUG() << "Removed from videoList";
            else
              QLOG_ERROR() << "Could not remove from videoList!!!";
            */

            break;
          }
        }

        // Send error alert
        /*if (!alerter)
          alerter = new Alerter(DEFAULT_ALERT_SERVICE_URL);

        alerter->sendAlert(SOFTWARE_NAME, SOFTWARE_VERSION, QString("Giving up on downloading package for task id: %1, reached maximum retries").arg(currentTask.getTaskID()));
        */

        // Remove the video cover URL and move on to the next
        videoCoverList.removeFirst();

        // If there are more URLs then get the next one otherwise we're done
        if (videoCoverList.count() > 0)
        {
          currentVideoCoverUrl = videoCoverList.first();
          finished = false;
        }

        // Reset counter
        numRetries = 0;
      }
      else
      {
        // Retry current download after waiting a random amount of time
        disconnect(timer, 0, 0, 0);
        connect(timer, SIGNAL(timeout()), this, SLOT(processVideoLookupResults()));

        timer->setInterval(1 + (qrand() % 30000));
        QLOG_DEBUG() << QString("Retrying to download video cover: %1 in %2 ms. Retry: %3")
                        .arg(currentVideoCoverUrl)
                        .arg(timer->interval())
                        .arg(numRetries);
        timer->start();

        //finished = false;
        return;
      }
    }
    else
    {
      currentVideoCoverUrl = videoCoverList.first();
      finished = false;
    }

    // Make sure we still need to process current task
    if (!finished)
    {
      QLOG_DEBUG() << QString("Downloading video cover file: %1").arg(currentVideoCoverUrl);

      // Download video cover file
      downloadFile(currentVideoCoverUrl);
    }

  } // endif video cover URL list not empty

  if (finished)
  {
    QLOG_DEBUG() << QString("Finished downloading video cover files");
    emit videoLookupResults(videoList);
  }
}
