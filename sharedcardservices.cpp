#include "sharedcardservices.h"
#include "QsLog.h"
#include <QSslConfiguration>
#include <QSslCertificate>
#include <QSslSocket>
#include <QSslKey>

SharedCardServices::SharedCardServices(QObject *parent):QObject(parent)
{
    formPost = new FormPostPlugin;
    netReplyCheck = 0;
    netMgr = 0;
}

SharedCardServices::~SharedCardServices()
{
    if (formPost)
        formPost->deleteLater();

    if (netReplyCheck)
        netReplyCheck->deleteLater();

    if (netMgr)
        netMgr->deleteLater();
}

void SharedCardServices::sendServiceCall(QVariantMap &jsonRequest, QString attr)
{
    const char KEY[] = {0x7c, 0x78, 0x7f, 0x49, 0x42, 0x7f, 0x43, 0x42,
                        0x6d, 0x14, 0x63, 0x42, 0x68, 0x70, 0x54, 0x76,
                        0x70, 0x56, 0x44, 0x7c, 0x7d, 0x78, 0x6b, 0x58,
                        0x74, 0x43, 0x5f, 0x47, 0x22, 0x23, 0x2a, 0x77,
                        0};

    QString decoded;
    QByteArray password(KEY);

    for (int i = 0; i < password.length(); ++i)
    {
        password[i] = password[i] ^ ((password.length() - i) + 16);

        decoded.append(QChar(password[i]));
    }
    jsonRequest["hostname"] = Global::getHostname();
    jsonRequest["ip_address"] = Global::getIpAddress();
    jsonRequest["passphrase"] = decoded;

    QJson::Serializer serializer;
    formPost->addField("msg", QString(serializer.serialize(jsonRequest)));
    formPost->setUserAttribute(attr);
    netReplyCheck = formPost->postData(WEB_SERVICE_URL);

    connect(netReplyCheck, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(sendCallError(QNetworkReply::NetworkError)));
    connect(netReplyCheck, SIGNAL(finished()), this, SLOT(sendCallFinished()));
}

void SharedCardServices::sendCallFinished()
{
    QByteArray response(formPost->response());
    QLOG_DEBUG()<<"Response Received = "<<response;
    // The finished signal is always emitted, even if there was a network error
    if (netReplyCheck->error() == QNetworkReply::NoError) {

        QJson::Parser parser;
        QVariant var = parser.parse(response);
        QVariantMap map = var.toMap();
        if (!map.contains("error")) {
            QString responseType = netReplyCheck->request().attribute(QNetworkRequest::User).toString();
            const QMetaObject &mo = SharedCardServices::staticMetaObject;
            int index = mo.indexOfEnumerator("ReqType");
            QMetaEnum metaEnum = mo.enumerator(index);

            switch (metaEnum.keyToValue(responseType.toStdString().c_str())) {
            case AddCard:
                this->addCardQuery(map);
                break;
            case DeleteCard:
                this->deleteCardQuery(map);
                break;
            case UpdateCard:
                this->updateCardQuery(map);
                break;
            case GetCards:
                this->getCardsQuery(map);
                break;
            case CheckCardPresent:
                this->checkCardPresentQuery(map);
                break;
            case ReplicateDb:
                this->replicateDatabaseQuery(map);
                break;
            }
        }
    }
    else {
        QLOG_ERROR() << "Could not send service call to server." << response;
    }
}

void SharedCardServices::sendCallError(QNetworkReply::NetworkError error)
{
    Q_UNUSED(error);
    QLOG_ERROR() << QString("Network error while sending call to server: %1").arg(netReplyCheck->errorString());
    emit serviceError(netReplyCheck->errorString());
}

void SharedCardServices::addCard(ArcadeCard card)
{
    QLOG_DEBUG() << "Adding shared card " << card.getCardNum();
    QVariantMap jsonRequest;

    jsonRequest["action"] = "createCard";
    jsonRequest["cardNumber"] = card.getCardNum();
    jsonRequest["cardCredits"] = card.getCredits();
    jsonRequest["techAccessType"] = card.getTechAccess();
    jsonRequest["cardType"] = card.getCardType();
    jsonRequest["revId"] = card.getRevID();
    jsonRequest["deviceNum"] = card.getDeviceNum();
    jsonRequest["inUse"] = card.getInUse();

    this->sendServiceCall(jsonRequest,"AddCard");
}

void SharedCardServices::addCardQuery(QVariantMap map)
{
    QLOG_DEBUG()<<"response received IN addCardQuery = "<<map;

    ArcadeCard obj;
    if (!map.contains("error")) {
        QVariantMap innerMap = map.find("data").value().toMap();
        obj.setCredits(innerMap["cardCredits"].toInt());
        obj.setCardNum(innerMap["cardNumber"].toString());
        obj.setCardType(ArcadeCard::CvsCardTypes(innerMap["cardType"].toInt()));
        obj.setDeviceNum(innerMap["deviceNum"].toInt());
        obj.setInUse(innerMap["inUse"].toBool());
        obj.setLastActive(innerMap["lastActive"].toDateTime());
        obj.setRevID(innerMap["revId"].toString());
        obj.setMadeBy("CAS Manager");
        emit addCardFinished(obj, true);
    } else {
        emit addCardFinished(obj, false);
    }

}

void SharedCardServices::checkCardPresent(ArcadeCard card)
{
    QLOG_DEBUG() << "Checking shared card Present" << card.getCardNum();
    QVariantMap jsonRequest;

    jsonRequest["action"] = "checkCard";
    jsonRequest["cardNumber"] = card.getCardNum();

    this->sendServiceCall(jsonRequest,"CheckCardPresent");
}

void SharedCardServices::checkCardPresentQuery(QVariantMap map)
{
    QLOG_DEBUG()<<"response received IN checkCardPresentQuery = "<<map;

    ArcadeCard obj;
    if (map["result"].toBool()) {
        QVariantMap innerMap = map.find("data").value().toMap();
        obj.setCredits(innerMap["cardCredits"].toInt());
        obj.setCardNum(innerMap["cardNumber"].toString());
        obj.setCardType(ArcadeCard::CvsCardTypes(innerMap["cardType"].toInt()));
        obj.setDeviceNum(innerMap["deviceNum"].toInt());
        obj.setInUse(innerMap["inUse"].toBool());
        obj.setLastActive(innerMap["lastActive"].toDateTime());
        obj.setRevID(innerMap["revId"].toString());
        obj.setActivated(innerMap["activated"].toDateTime());
        obj.setMadeBy("CAS Manager");
        emit getCardFinished(obj, true);
    } else {
        emit getCardFinished(obj, false);
    }
}

void SharedCardServices::deleteCard(ArcadeCard card)
{
    QVariantMap jsonRequest;
    jsonRequest["action"] = "deleteCard";
    jsonRequest["cardNumber"] = card.getCardNum();

    this->sendServiceCall(jsonRequest, "DeleteCard");
}

void SharedCardServices::deleteCardQuery(QVariantMap map)
{
    QLOG_DEBUG() << "response received in deleteCardQuery = " << map;

    if (!map.contains("error")) {
        emit deleteCardFinished(map["result"].toString(), true);
    } else {
        emit deleteCardFinished(map["result"].toString(), false);
    }
}

void SharedCardServices::updateCard(ArcadeCard card)
{
    QVariantMap jsonRequest;

    jsonRequest["action"] = "updateCard";
    jsonRequest["cardNumber"] = card.getCardNum();
    jsonRequest["cardCredits"] = card.getCredits();
    jsonRequest["cardType"] = card.getCardType();
    jsonRequest["inUse"] = card.getInUse();
    jsonRequest["lastActive"] = QVariant(QDateTime::currentDateTime());
    jsonRequest["techAccessType"] = card.getTechAccess();
    jsonRequest["deviceNum"] = card.getDeviceNum();
    jsonRequest["revId"] = card.getRevID();

    this->sendServiceCall(jsonRequest, "UpdateCard");
}

void SharedCardServices::updateCardQuery(QVariantMap map)
{
    QLOG_DEBUG()<<"response received IN updateCardQuery = "<<map;
    ArcadeCard obj;
    if (!map.contains("error")) {
        QVariantMap innerMap = map.find("data").value().toMap();
        obj.setCredits(innerMap["cardCredits"].toInt());
        obj.setCardNum(innerMap["cardNumber"].toString());
        obj.setCardType(ArcadeCard::CvsCardTypes(innerMap["cardType"].toInt()));
        obj.setDeviceNum(innerMap["deviceNum"].toInt());
        obj.setInUse(innerMap["inUse"].toBool());
        obj.setLastActive(innerMap["lastActive"].toDateTime());
        obj.setRevID(innerMap["revId"].toString());
        obj.setActivated(innerMap["activated"].toDateTime());

        emit updateCardFinished(obj,true);
    } else {
        emit updateCardFinished(obj,false);
    }
}

void SharedCardServices::getCards(int page)
{
    QVariantMap jsonRequest;

    jsonRequest["action"] = "getCards";
    jsonRequest["page"] = page;

    this->sendServiceCall(jsonRequest, "GetCards");
}

void SharedCardServices::getCardsQuery(QVariantMap map)
{
    QLOG_DEBUG()<<"response received IN  getCardsQuery = "<<map;
    QList<ArcadeCard> arcadeCardList;
    if (!map.contains("error")) {
        QVariantMap innerMap = map.find("result").value().toMap();
        QVariant var = innerMap["subset"];
        QList<QVariant> innerMapList = var.toList();
        QList<QVariant>::iterator iter;
        for(iter = innerMapList.begin(); iter != innerMapList.end(); ++iter) {
            ArcadeCard obj;
            QVariant card = *iter;
            QVariantMap cardMap = card.toMap();
            obj.setCredits(cardMap["cardCredits"].toInt());
            obj.setCardNum(cardMap["cardNumber"].toString());
            obj.setCardType(ArcadeCard::CvsCardTypes(cardMap["cardType"].toInt()));
            obj.setDeviceNum(cardMap["deviceNum"].toInt());
            obj.setInUse(cardMap["inUse"].toBool());
            obj.setLastActive(cardMap["lastActive"].toDateTime());
            obj.setRevID(cardMap["revId"].toString());
            obj.setActivated(cardMap["activated"].toDateTime());
            obj.setMadeBy("CAS Manager");
            arcadeCardList.append(obj);
        }
        emit getCardsFinished(arcadeCardList, true);
        emit sendPageInfo(innerMap["pos"].toInt(),innerMap["count"].toInt());
    } else {
        emit getCardsFinished(arcadeCardList, false);
    }
}

void SharedCardServices::replicateDatabase(QString srcDb, QString destDb, bool isSecond)
{
    this->isSecond = isSecond;
    netMgr = new QNetworkAccessManager(this);
    QString url = "https://cas-server:6984/_replicate/";
    QString concatenated = "cas_admin:7B445jh8axVFL2tAMoQtLBlg";
    QByteArray data = concatenated.toLocal8Bit().toBase64();
    QString headerData = "Basic " + data;
//    QSslConfiguration config;
//    QList<QSslCertificate> list;
//    list.append(QSslCertificate::fromPath("/etc/couchdb/cert/couchdb.pem").first());
//    QLOG_DEBUG()<<QSslCertificate::fromPath("/etc/couchdb/cert/couchdb.pem");
//    config.setCaCertificates(list);

    QSslError error(QSslError::HostNameMismatch, QSslCertificate::fromPath("/etc/couchdb/cert/couchdb.pem").first());
    QList<QSslError> expectedSslErrors;
    expectedSslErrors.append(error);
    QSslError error1(QSslError::SelfSignedCertificate, QSslCertificate::fromPath("/etc/couchdb/cert/couchdb.pem").first());
    expectedSslErrors.append(error1);

    QUrl Url = QUrl(url);
    QNetworkRequest request;
    request.setUrl(Url);
//    request.setSslConfiguration(config);
    request.setRawHeader("Authorization", headerData.toLocal8Bit());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setAttribute(QNetworkRequest::User, (QString)QString("ReplicateDb"));
    request.setRawHeader(QString("Referer").toUtf8(), url.toUtf8());

    QJsonObject jsonObj;

    jsonObj.insert("source", srcDb);
    jsonObj.insert("target", destDb);
    jsonObj.insert("continuous", true);

    QJsonDocument jsonDoc(jsonObj);
    QByteArray postData = jsonDoc.toJson();

    QLOG_DEBUG()<<"VALUE OF POSTdATA = "<< jsonDoc;
    netReplyCheck = netMgr->post(request, postData);
    netReplyCheck->ignoreSslErrors(expectedSslErrors);
    connect(netReplyCheck, SIGNAL(finished()), this, SLOT(sendCallFinished()));
    connect(netReplyCheck, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(sendCallError(QNetworkReply::NetworkError)));
}

void SharedCardServices::replicateDatabaseQuery(QVariantMap map)
{
   if (map["ok"].toBool()) {
       QLOG_DEBUG()<<"Replicate Database";
       if(this->isSecond)
       {
           emit twoWayReplicated();
       }
       else
       {
           emit oneWayReplicated();
       }
   } else {
       QLOG_DEBUG()<<"Could not Replicate Database";
       emit errorInReplication();
   }
}
