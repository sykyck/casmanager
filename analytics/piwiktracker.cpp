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
 *
 *
 * To build PiwikTracker with a QtQuick application (QGuiApplication) instead
 * of Desktop, define PIWIK_TRACKER_QTQUICK in your .pro file like this:
 * `DEFINES += PIWIK_TRACKER_QTQUICK`
 * or in a cmake project:
 * `add_definitions(-DPIWIK_TRACKER_QTQUICK)`
 *
 * To enable debugging messages, `#define PIWIK_TRACKER_DEBUG 1` before
 * including the header file
 */

#include "piwiktracker.h"
#include <QSettings>
#include <QUuid>
#include "qjson-backport/qjsondocument.h"
#include "qjson-backport/qjsonobject.h"
#include "qjson-backport/qjsonvalue.h"
#include "qjson-backport/qjsonarray.h"
#include "qslog/QsLog.h"
#include "global.h"
#include "databasemgr.h"

#if defined(PIWIK_TRACKER_QTQUICK)
#include <QGuiApplication>
#include <QScreen>
#else
#ifdef QT_GUI_LIB
#include <QApplication>
#include <QDesktopWidget>
#endif
#endif

#ifndef PIWIK_TRACKER_DEBUG
#define PIWIK_TRACKER_DEBUG 1
#endif

enum {
    kSiteIdRequest,
    kLeastViewedMovies
};

QString kNetworkRequestIds[] = {
  "GETSITEID",
  "GETLEASTVIEWEDMOVIES"
};

PiwikTracker::PiwikTracker(QCoreApplication * parent,
                           int siteId,
                           QString clientId) :
        QObject(parent),
        _networkAccessManager(this),
        _trackerUrl(QUrl(PIWIK_SERVER_URL)),
        _siteId(siteId),
        _clientId(clientId),
        _piwikOfflineTracker(NULL),
        _databaseMgr(NULL)
{
    connect(
            &_networkAccessManager,
            SIGNAL(finished(QNetworkReply *)),
            this,
            SLOT(replyFinished(QNetworkReply *)));

    if (parent) {
        _appName = parent->applicationName();
    }

    // if no client id was set let's search in the settings
    if (!_clientId.size()) {
        this->resetClientId();
    }

    // get the screen resolution for gui apps
#if defined(PIWIK_TRACKER_QTQUICK)
    QScreen* screen = qApp->primaryScreen();
    _screenResolution = QString::number(screen->geometry().width())
        + "x" + QString::number(screen->geometry().height());
#else
#ifdef QT_GUI_LIB
    _screenResolution =
            QString::number(
                    QApplication::desktop()->screenGeometry().width()) + "x"
                    + QString::number(
                    QApplication::desktop()->screenGeometry().height());
#endif
#endif

    // try to get the operating system
    QString operatingSystem = "Other";

#ifdef Q_OS_LINUX
    operatingSystem = "Linux";
#endif

#ifdef Q_OS_MAC
    operatingSystem = "Macintosh";
#endif

#ifdef Q_OS_WIN32
    operatingSystem = "Windows";
#endif

    // for QT >= 5.4 we can use QSysInfo
    // Piwik doesn't recognize that on Mac OS X very well
#if (QT_VERSION >= QT_VERSION_CHECK(5, 4, 0))
#ifdef Q_OS_MAC
    operatingSystem = "Macintosh " + QSysInfo::prettyProductName();
#else
    operatingSystem = QSysInfo::prettyProductName() +", " +
            QSysInfo::currentCpuArchitecture();
#endif
#endif

    // get the locale
    QString locale = QLocale::system().name().toLower().replace("_", "-");

    // set the user agent
    _userAgent = "Mozilla/5.0 (" + operatingSystem + "; " + locale + ") "
                      "PiwikTracker/0.1 (Qt/" QT_VERSION_STR " )";

    // set the user language
    _userLanguage = locale;
}

void PiwikTracker::setDatabaseMgr(DatabaseMgr *mgr)
{
    _databaseMgr = mgr;
    _piwikOfflineTracker = new PiwikOfflineTracker(mgr);
}

void PiwikTracker::fetchSiteId(QString storeName)
{
    // Fetch the site id for this arcade or booth
    QUrl url = QUrl(_trackerUrl.toString() + "/index.php");

    url.addQueryItem("module", "API");
    url.addQueryItem("method", "UsArcade.getSiteId");
    url.addQueryItem("token_auth", AUTH_TOKEN);
    url.addQueryItem("format", "json");
    if (!storeName.isEmpty()) {
        url.addQueryItem("storeName", storeName);
    }

    QNetworkRequest siteIdRequest(url);

#if PIWIK_TRACKER_DEBUG
    QLOG_DEBUG() << kNetworkRequestIds[kSiteIdRequest];
#endif

    siteIdRequest.setAttribute(QNetworkRequest::User, kNetworkRequestIds[kSiteIdRequest]);

    // try to ensure the network is accessible
    _networkAccessManager.setNetworkAccessible(
            QNetworkAccessManager::Accessible);

    QNetworkReply *reply = _networkAccessManager.get(siteIdRequest);

    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)),
            this, SLOT(replyError(QNetworkReply::NetworkError)));

    // ignoring SSL errors
    connect(reply, SIGNAL(sslErrors(QList<QSslError>)), reply,
            SLOT(ignoreSslErrors()));
}

void PiwikTracker::resetClientId()
{
    QByteArray ba;
    ba.append(QUuid::createUuid().toString());

    // generate a random md5 hash
    QString md5Hash = QString(
            QCryptographicHash::hash(
                    ba,
                    QCryptographicHash::Md5).toHex());

    // the client id has to be a 16 character hex code
    _clientId = md5Hash.left(16);
}

void PiwikTracker::getLeastViewedMovies(QString category, int numResults)
{
  QUrl url = QUrl(_trackerUrl.toString() + "/index.php");

//  _siteId = 3;

  url.addQueryItem("module", "API");
  url.addQueryItem("method", "UsArcade.getLeastViewedMovies");
  url.addQueryItem("idsite", QString::number(_siteId));
  url.addQueryItem("limit", QString::number(numResults));
  url.addQueryItem("category", category);
  url.addQueryItem("token_auth", AUTH_TOKEN);
  url.addQueryItem("format", "json");

  QNetworkRequest request(url);

  request.setAttribute(QNetworkRequest::User, kNetworkRequestIds[kLeastViewedMovies]);

  // try to ensure the network is accessible
  _networkAccessManager.setNetworkAccessible(QNetworkAccessManager::Accessible);

  QNetworkReply *reply = _networkAccessManager.get(request);

  connect(reply, SIGNAL(error(QNetworkReply::NetworkError)),
          this, SLOT(replyError(QNetworkReply::NetworkError)));

  // ignoring SSL errors
  connect(reply, SIGNAL(sslErrors(QList<QSslError>)), reply,
          SLOT(ignoreSslErrors()));
}

void PiwikTracker::setUserName(QString user)
{
    // Set a random user name
    if (!user.isEmpty()) {
        this->_userName = user;
    }
    else {
        QByteArray ba;
        ba.append(QUuid::createUuid().toString());

        // generate a random md5 hash
        QString md5Hash = QString(
                QCryptographicHash::hash(
                        ba,
                        QCryptographicHash::Md5).toHex());

        // the client id has to be a 16 character hex code
        this->_userName = md5Hash.left(16);
    }
}

/**
 * Prepares the common query items for the tracking request
 */
QUrl PiwikTracker::prepareUrlQuery(QString path) {
    QUrl q;
    q.addQueryItem("idsite", QString::number(_siteId));
    // Using uid instead
    //q.addQueryItem("_id", _clientId);
    //q.addQueryItem("cid", _clientId);
    q.addQueryItem("url", _appName + "/" + path);

    // to record the request
    q.addQueryItem("rec", "1");

    // api version
    q.addQueryItem("apiv", "1");

    if (!_screenResolution.isEmpty()) {
        q.addQueryItem("res", _screenResolution);
    }

    if (!_userAgent.isEmpty()) {
        q.addQueryItem("ua", _userAgent);
    }

    if (!_userLanguage.isEmpty()) {
        q.addQueryItem("lang", _userLanguage);
    }

    if (_customDimensions.count() > 0) {
        QHash<int, QString>::iterator i;
        for (i = _customDimensions.begin(); i != _customDimensions.end(); ++i) {
            q.addQueryItem("dimension" + QString::number(i.key()), i.value());
        }
    }

    return q;
}

QString PiwikTracker::getVisitVariables()
{
    QString varString;
    /**
      * See spec at https://github.com/piwik/piwik/issues/2165
      * Need to pass in format {"1":["key1","value1"],"2":["key2","value2"]}
      */
    if( _visitVariables.count() > 0 ) {
        QHash<QString, QString>::iterator i;
        varString.append("{");
        int num=0;
        for (i = _visitVariables.begin(); i != _visitVariables.end(); ++i) {
            if( num != 0 )
            {
                varString.append(",");
            }
            QString thisvar=QString("\"%1\":[\"%2\",\"%3\"]").arg(num+1).arg(i.key()).arg(i.value());
            varString.append(thisvar);
            num++;
        }
        varString.append("}");
    }
    return varString;
}


/**
 * Sends a visit request with visit variables
 */
void PiwikTracker::sendVisit(QString path, QString actionName, QString userName) {
    if (_siteId < 1) {
        // Found no site id to log this visit to
        QLOG_DEBUG() << "Not able to log piwik visit";
        return;
    }
    // This is a new visit. So reset the client Id
    this->resetClientId();

    this->setUserName(userName);

    QUrl url = prepareUrlQuery(path);
    url.setUrl(_trackerUrl.toString() + "/piwik.php");
    QString visitVars=getVisitVariables();

    if( visitVars.size() != 0 )
    {
        url.addQueryItem("_cvar",visitVars);
    }
    if (!actionName.isEmpty()) {
        url.addQueryItem("action_name", actionName);
    }

    if (!userName.isEmpty()) {
        url.addQueryItem("uid", userName);
    }
    else if (!_userName.isEmpty()) {
        url.addQueryItem("uid", _userName);
    }

    if (!_boothAddress.isEmpty()) {
        url.addQueryItem("booth", _boothAddress);
    }
    else {
        url.addQueryItem("booth", "Unknown Booth");
    }

    // try to ensure the network is accessible
    _networkAccessManager.setNetworkAccessible(
            QNetworkAccessManager::Accessible);

    QNetworkReply *reply = _networkAccessManager.get(QNetworkRequest(url));

    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)),
            this, SLOT(replyError(QNetworkReply::NetworkError)));

    // ignoring SSL errors
    connect(reply, SIGNAL(sslErrors(QList<QSslError>)), reply,
            SLOT(ignoreSslErrors()));

#if PIWIK_TRACKER_DEBUG
    QLOG_DEBUG() << __func__ << " - 'url': " << url;
#endif
}

/**
 * Sends a ping request
 */
void PiwikTracker::sendPing(QString userName) {
    if (_siteId < 1) {
        // Found no site id to log this ping to
        QLOG_DEBUG() << "Not able to log piwik ping";
        return;
    }

    QUrl url = prepareUrlQuery("");
    url.setUrl(_trackerUrl.toString() + "/piwik.php");
    url.addQueryItem("ping", "1");
    if (!userName.isEmpty()) {
        url.addQueryItem("uid", userName);
    }
    else if (!_userName.isEmpty()) {
        url.addQueryItem("uid", _userName);
    }

    // try to ensure the network is accessible
    _networkAccessManager.setNetworkAccessible(
            QNetworkAccessManager::Accessible);

    QNetworkReply *reply = _networkAccessManager.get(QNetworkRequest(url));

    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)),
            this, SLOT(replyError(QNetworkReply::NetworkError)));

    // ignoring SSL errors
    connect(reply, SIGNAL(sslErrors(QList<QSslError>)), reply,
            SLOT(ignoreSslErrors()));

#if PIWIK_TRACKER_DEBUG
    QLOG_DEBUG() << __func__ << " - 'url': " << url;
#endif

    QLOG_DEBUG() << Q_FUNC_INFO << " Piwik sendVisit - 'url' " << url;
}

void PiwikTracker::sendPiwikPlaytimeEvent(QString upc, QString producer, QString movieName, QString genre, QString category, QString viewType, int nSecs, int channel_num)
{
    this->sendMediaEvent(upc, producer, movieName, genre, category, viewType, nSecs, channel_num);
}

void PiwikTracker::sendPiwikEvent(QString path, QString eventCategory, QString eventAction, int eventType, QString eventName, qreal eventValue)
{
    this->sendEvent(path, eventCategory, eventAction, eventType, eventName, eventValue);
}

void PiwikTracker::sendMediaEvent(
        QString upc,
        QString producer,
        QString movieName,
        QString genre,
        QString category,
        QString viewType,
        int nSecs,
        int channel_num)
{
    if (_siteId < 1) {
        // Found no site id to log this event to
        QLOG_DEBUG() << "Not able to log piwik media event";
        return;
    }

    QString path = "Movie Watch";
    QString eventCategory = "Movie View Time";
    QString eventAction = "Played Movie";
    QString eventName = "Play Time";

    QUrl url = prepareUrlQuery(path);
    url.setUrl(_trackerUrl.toString() + "/piwik.php");

    if (!eventCategory.isEmpty()) {
        url.addQueryItem("e_c", eventCategory);
    }

    if (!eventAction.isEmpty()) {
        url.addQueryItem("e_a", eventAction);
    }

    if (!eventName.isEmpty()) {
        url.addQueryItem("e_n", eventName);
    }

    if (!_userName.isEmpty()) {
        url.addQueryItem("uid", _userName);
    }

    if (!_boothAddress.isEmpty()) {
        url.addQueryItem("booth", _boothAddress);
    }
    else {
        url.addQueryItem("booth", "Unknown Booth");
    }

    // Add media information
    url.addQueryItem("upc", upc);
    url.addQueryItem("producer", producer);
    url.addQueryItem("moviename", movieName);
    url.addQueryItem("genre", genre);
    url.addQueryItem("category", category);
    url.addQueryItem("playtype", viewType);
    url.addQueryItem("playtime", QString::number(nSecs));
    url.addQueryItem("channelnum", QString::number(channel_num));

    // try to ensure the network is accessible
    _networkAccessManager.setNetworkAccessible(
            QNetworkAccessManager::Accessible);

    QNetworkReply *reply = _networkAccessManager.get(QNetworkRequest(url));

    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)),
            this, SLOT(replyError(QNetworkReply::NetworkError)));

    // ignoring SSL errors
    connect(reply, SIGNAL(sslErrors(QList<QSslError>)), reply,
            SLOT(ignoreSslErrors()));

#if PIWIK_TRACKER_DEBUG
    QLOG_DEBUG() << __func__ << " - 'url': " << url;
#endif

    QLOG_DEBUG() << Q_FUNC_INFO << " Piwik sendMediaEvent - 'url' " << url;


}

/**
 * Sends an event request
 */
void PiwikTracker::sendEvent(
        QString path,
        QString eventCategory,
        QString eventAction,
        PiwikTracker::EventType eventType,
        QString eventName,
        qreal eventValue,
        QString userName) {
    if (_siteId < 1) {
        // Found no site id to log this event to
        QLOG_DEBUG() << "Not able to log piwik event";
        return;
    }
    QUrl url = prepareUrlQuery(path);
    url.setUrl(_trackerUrl.toString() + "/piwik.php");

    if (!eventCategory.isEmpty()) {
        url.addQueryItem("e_c", eventCategory);
    }

    if (!eventAction.isEmpty()) {
        url.addQueryItem("e_a", eventAction);
    }

    if (!eventName.isEmpty()) {
        url.addQueryItem("e_n", eventName);
    }

    if (eventValue > 0) {
        bool sendEventValueAsRevenue = true;
        QString eventTypeStr;

        switch (eventType) {
        case kProgrammed:
            eventTypeStr = "programmed";
            // Dont add up to revenue
            sendEventValueAsRevenue = false;
            break;
        case kCard:
            eventTypeStr = "card";
            break;
        case kCash:
            eventTypeStr = "cash";
            break;
        case kCCCharge:
            eventTypeStr = "cccharge";
            break;
        case kCreditsUsed:
            eventTypeStr = "credits";
            sendEventValueAsRevenue = false;
            break;
        default:
            break;
        }
        if (sendEventValueAsRevenue) {
            // Log the revenue
            url.addQueryItem("e_v", QString::number((double)eventValue, 'g', 2));
        }
        url.addQueryItem(eventTypeStr, QString::number((double)eventValue, 'g', 2));
    }

    if (!userName.isEmpty()) {
        url.addQueryItem("uid", userName);
    }
    else if (!_userName.isEmpty()) {
        url.addQueryItem("uid", _userName);
    }

    if (!_boothAddress.isEmpty()) {
        url.addQueryItem("booth", _boothAddress);
    }
    else {
        url.addQueryItem("booth", "Unknown Booth");
    }

    // try to ensure the network is accessible
    _networkAccessManager.setNetworkAccessible(
            QNetworkAccessManager::Accessible);

    QNetworkReply *reply = _networkAccessManager.get(QNetworkRequest(url));

    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)),
            this, SLOT(replyError(QNetworkReply::NetworkError)));

    // ignoring SSL errors
    connect(reply, SIGNAL(sslErrors(QList<QSslError>)), reply,
            SLOT(ignoreSslErrors()));

#if PIWIK_TRACKER_DEBUG
    QLOG_DEBUG() << __func__ << " - 'url': " << url;
#endif

    QLOG_DEBUG() << Q_FUNC_INFO << " Piwik sendEvent - 'url' " << url;
}

/**
 * Sets a custom dimension
 */
void PiwikTracker::setCustomDimension(int id, QString value) {
    _customDimensions[id] = value;
}

/**
 * @brief PiwikTracker::setCustomVisitVariables
 * @param name The name of the custom variable to set (key)
 * @param value The value to set for this custom variable
 */
void PiwikTracker::setCustomVisitVariables(QString name, QString value) {
    _visitVariables[name]=value;
}

void PiwikTracker::replyFinished(QNetworkReply * reply)
{
  QByteArray replyData = reply->readAll();
#if PIWIK_TRACKER_DEBUG
  QLOG_DEBUG() << "Reply from " << reply->url().path();
  QLOG_DEBUG() << "Reply Data " << replyData;
  QLOG_DEBUG() << reply->attribute(QNetworkRequest::User).toString();
#endif

  if (reply->request().attribute(QNetworkRequest::User).toString() == kNetworkRequestIds[kSiteIdRequest])
  {
    if (reply->error() == QNetworkReply::NoError)
    {
      // Got back site id
      QJsonDocument doc = QJsonDocument::fromJson(replyData);
      QJsonArray responseArray = doc.array();

      QJsonObject siteIdObj = responseArray.at(0).toObject();

      if (siteIdObj.contains("idsite")) {

        _siteId = siteIdObj["idsite"].toString().toInt();

        QLOG_DEBUG() << "Successful response from piwik siteid = " << _siteId;
      }
      else {
        QLOG_DEBUG() << "Not found Piwik site id! Make sure you register it";
      }
    }
  }
  else if (reply->request().attribute(QNetworkRequest::User).toString() == kNetworkRequestIds[kLeastViewedMovies])
  {
    if (reply->error() == QNetworkReply::NoError)
    {
      // Got least viewed movies
      QList<Movie> movieList;

      QJsonDocument doc = QJsonDocument::fromJson(replyData);
      QJsonArray responseArray = doc.array();

      QLOG_DEBUG() << "responseArray length" << responseArray.count();

      foreach (QJsonValue jsonValue, responseArray)
      {
        QJsonObject jsonObj = jsonValue.toObject();

        Movie m;
        m.setUPC(jsonObj["movie_upc"].toString());
        m.setTitle(jsonObj["movie_name"].toString());
        m.setCategory(jsonObj["movie_category"].toString());
        m.setSubcategory(jsonObj["movie_genre"].toString());
        m.setProducer(jsonObj["movie_producer"].toString());
        m.setChannelNum(jsonObj["movie_channel_num"].toString().toInt());
        m.setTotalPlayTime(jsonObj["playtime"].toString().toInt());

        movieList.append(m);
      }

      emit receivedLeastViewedMovies(movieList, false);
    }
    else
    {
      emit receivedLeastViewedMovies(QList<Movie>(), true);
    }
  }
  else
  {
    QLOG_ERROR() << "Could not identify network reply";
  }
}

void PiwikTracker::replyError(QNetworkReply::NetworkError code) {
#if PIWIK_TRACKER_DEBUG
    QLOG_DEBUG() << "Network error code: " << code;
#else
    Q_UNUSED(code);
#endif
    QNetworkReply *reply = dynamic_cast<QNetworkReply *>(sender());

//    if (reply != NULL && _databaseMgr != NULL) {
//        QUrl url = reply->url();
//        _databaseMgr->addPiwikUrl(PIWIK_OFFLINEDB, url.toString());
//    }
    QLOG_DEBUG() << "Piwik Network error code: " << code;
}
