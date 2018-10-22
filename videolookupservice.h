#ifndef VIDEOLOOKUPSERVICE_H
#define VIDEOLOOKUPSERVICE_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QUrl>
#include <QNetworkReply>
#include <QTimer>
#include <QDir>

#include "qslog/QsLog.h"
#include "movie.h"
#include "formpost/formpost.h"

class VideoLookupService : public QObject
{
  Q_OBJECT
public:
  enum ActionType
  {
    VIDEO_LOOKUP,
    VIDEO_UPDATE,
    VIDEO_ATTRIBUTES
  };

  explicit VideoLookupService(QString webServiceUrl, QString downloadPath, QString password, QObject *parent = 0);
  ~VideoLookupService();
  void startVideoLookup(QStringList upcList);
  void startVideoUpdate(const Movie &movie, const QString &frontCover = QString(), const QString &backCover = QString());
  void startDeleteVideoLookup(QStringList upcList);
  void startProducerListing();

signals:
  void videoLookupResults(QList<Movie> videoList);
  void videoUpdateResults(bool success, const QString &message);
  void deleteVideoLookupResults(bool success, const QString &message, const QStringList &upcList = QStringList());
  void producerListingResults(QStringList producerList);

private slots:
  void videoLookup();
  void finishedVideoLookup(bool success);
  void receivedVideoLookupResponse();
  void videoLookupError(QNetworkReply::NetworkError error);
  void videoUpdateFinished();
  void receivedDeleteVideoLookupResponse();
  void deleteVideoLookupError(QNetworkReply::NetworkError error);

  void receivedProducerListingResponse();
  void producerListingError(QNetworkReply::NetworkError error);

  void downloadFileReceived();
  void downloadFileError(QNetworkReply::NetworkError error);

  void resetTimeout(qint64,qint64);
  void abortConnection();
  void networkActivityFinished(QNetworkReply *reply);
  void processVideoLookupResults();

private:
  void downloadFile(QString url);

  QTimer *timer;

  // Counts the number of times we retry to contact server
  int numRetries;

  QNetworkAccessManager *netMgr;
  QNetworkReply *netReplyVideoLookup;
  QNetworkReply *netReplyDownload;
  QNetworkReply *netReplyVideoUpdate;
  QNetworkReply *netReplyDeleteVideoLookup;
  QNetworkReply *netReplyProducerListing;
  QTimer *connectionTimer;
  QUrl webServiceUrl;
  QDir downloadPath;
  QList<Movie> videoList;
  QStringList videoCoverList;
  QStringList videoCoverUpcList;
  QStringList deleteUpcList;
  QString currentVideoCoverUrl;  
  QString password;
  QStringList upcLookupList;
  FormPostPlugin *formPost;
};

#endif // VIDEOLOOKUPSERVICE_H
