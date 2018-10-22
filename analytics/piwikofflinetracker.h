#ifndef PIWIKOFFLINETRACKER_H
#define PIWIKOFFLINETRACKER_H

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

#define PIWIK_OFFLINEDB "/media/cas/piwik.sqlite"

class DatabaseMgr;


class PiwikOfflineTracker : public QObject
{
    Q_OBJECT

public:
    PiwikOfflineTracker(DatabaseMgr *mgr);
    void sendPiwikRequest();

public slots:
    void onNetworkAccessibilityChanged(QNetworkAccessManager::NetworkAccessibility);
    void replyFinished(QNetworkReply * reply);
private:
     QNetworkAccessManager networkAccessManager;
     DatabaseMgr *_databaseMgr;
};

#endif // PIWIKOFFLINETRACKER_H
