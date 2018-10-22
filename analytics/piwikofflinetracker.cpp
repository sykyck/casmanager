#include "piwikofflinetracker.h"
#include "databasemgr.h"
#include "qslog/QsLog.h"
#include "databasemgr.h"

PiwikOfflineTracker::PiwikOfflineTracker(DatabaseMgr *mgr) : networkAccessManager(this), _databaseMgr(mgr)
{
   connect(&networkAccessManager, SIGNAL(networkAccessibleChanged(QNetworkAccessManager::NetworkAccessibility)), this, SLOT(onNetworkAccessibilityChanged(QNetworkAccessManager::NetworkAccessibility)));
   connect(&networkAccessManager,SIGNAL(finished(QNetworkReply *)),this,SLOT(replyFinished(QNetworkReply *)));
   sendPiwikRequest();
}

void PiwikOfflineTracker::onNetworkAccessibilityChanged(QNetworkAccessManager::NetworkAccessibility status)
{
    QLOG_DEBUG() << "Piwik Offline Tracker Status = "<< status;

    if(status == QNetworkAccessManager::Accessible)
    {
       QLOG_DEBUG() << "Piwik Offline Tracker NetworkAccessibilityChanged to Accessible";
       sendPiwikRequest();
    }
}

void PiwikOfflineTracker::replyFinished(QNetworkReply * reply)
{
    QLOG_DEBUG() << "Piwik Offline tracker Reply Received From NetworkAccessManger " << reply->url().toString();

    //After success deletes that record
//    _databaseMgr->removePiwikUrl(PIWIK_OFFLINEDB,reply->url().toString());
//    //If network is accessible get the next record from table and send the piwik request
//    if(networkAccessManager.networkAccessible())
//    {
//        QString receivedString;

//        receivedString = _databaseMgr->getPiwikUrl(PIWIK_OFFLINEDB);
//        if(!receivedString.isEmpty())
//        {
//           QUrl url(receivedString);

//           networkAccessManager.get(QNetworkRequest(url));
//        }
//    }
}

void PiwikOfflineTracker::sendPiwikRequest()
{
    QString receivedString;
    //gets the first record
//    receivedString = _databaseMgr->getPiwikUrl(PIWIK_OFFLINEDB);
//    if(!receivedString.isEmpty())
//    {
//       QUrl url(receivedString);

//       networkAccessManager.get(QNetworkRequest(url));
//    }
}
