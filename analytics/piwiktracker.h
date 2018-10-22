/**
 * Copyright (c) 2014-2017 Patrizio Bekerle -- http://www.bekerle.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include "piwikofflinetracker.h"
#include "movie.h"

class DatabaseMgr;

#define PIWIK_SERVER_URL "https://reporting.usarcades.com"
const QString AUTH_TOKEN = "d7fbe0545fe310bc93cd76f4c39149c9";


class PiwikTracker : public QObject {
    Q_OBJECT

public:
    enum {
        kCard,
        kProgrammed,
        kCash,
        kCCCharge,
        kCreditsUsed
    };

    typedef int EventType;

    explicit PiwikTracker(QCoreApplication * parent,
                          int siteId = -1,
                          QString clientId = "");
    void sendVisit(QString path, QString actionName = "", QString userName = "");
    void sendPing(QString userName);

    void setUserName(QString user = "");
    void setDatabaseMgr(DatabaseMgr *mgr);

    void sendEvent(
            QString path,
            QString eventCategory,
            QString eventAction,
            PiwikTracker::EventType eventType,
            QString eventName = "",
            qreal eventValue = 0.0,
            QString userName = "");
    void sendMediaEvent(
            QString upc,
            QString producer,
            QString movieName,
            QString genre,
            QString category,
            QString viewType,
            int nSecs,
            int channel_num);

    void setCustomDimension(int id, QString value);
    void setCustomVisitVariables(QString key, QString value);

    void fetchSiteId(QString storeName = "");
    void setBoothAddress(QString boothAddress)
    {
        _boothAddress = boothAddress;
    }

    void resetClientId();

    void getLeastViewedMovies(QString category, int numResults);

private:

    mutable QNetworkAccessManager _networkAccessManager;
    QString _userName;
    QString _appName;
    QUrl _trackerUrl;
    int _siteId;
    QString _clientId;
    QString _screenResolution;
    QString _userAgent;
    QString _userLanguage;
    QString _boothAddress;
    QHash<int, QString> _customDimensions;
    QHash<QString, QString> _visitVariables;
    QUrl prepareUrlQuery(QString path);
    QString getVisitVariables();
    PiwikOfflineTracker *_piwikOfflineTracker;
    DatabaseMgr *_databaseMgr;

public slots:
    void sendPiwikEvent(QString path, QString eventCategory, QString eventAction, int eventType, QString eventName, qreal eventValue);
    void sendPiwikPlaytimeEvent(QString upc, QString producer, QString movieName, QString genre, QString category, QString viewType, int nSecs, int channel_num);

signals:
    void receivedLeastViewedMovies(QList<Movie> movies, bool error);

private Q_SLOTS:

    void replyFinished(QNetworkReply * reply);
    void replyError(QNetworkReply::NetworkError code);
};

extern PiwikTracker *gPiwikTracker;
