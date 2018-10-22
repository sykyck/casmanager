#ifndef SHAREDCARDOPERATIONS_H
#define SHAREDCARDOPERATIONS_H

#include "arcadecard.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include "qjson/serializer.h"
#include "qjson/parser.h"
#include "qjson-backport/qjsondocument.h"
#include "qjson-backport/qjsonobject.h"
#include "qjson-backport/qjsonvalue.h"
#include "qjson-backport/qjsonarray.h"
#include "formpost/formpost.h"
#include "settings.h"
#include "global.h"

class SharedCardServices:public QObject
{
    Q_OBJECT
public:
    enum ReqType {
        AddCard = 1,
        DeleteCard,
        UpdateCard,
        GetCards,
        CheckCardPresent,
        ReplicateDb,
        NoRequest
    };
    Q_ENUMS(ReqType)

    SharedCardServices(QObject *parent = 0);
    ~SharedCardServices();

    void addCard(ArcadeCard card);
    void checkCardPresent(ArcadeCard card);
    void deleteCard(ArcadeCard card);
    void updateCard(ArcadeCard card);
    void getCards(int page);
    void replicateDatabase(QString srcdb, QString destDb, bool isSecond);
    void addCardQuery(QVariantMap map);
    void checkCardPresentQuery(QVariantMap map);
    void deleteCardQuery(QVariantMap map);
    void updateCardQuery(QVariantMap map);
    void getCardsQuery(QVariantMap map);
    void replicateDatabaseQuery(QVariantMap map);

protected:
    void sendServiceCall(QVariantMap &jsonRequest, QString attr);

signals:
    void serviceError(QString);
    void addCardFinished(ArcadeCard &, bool);
    void deleteCardFinished(QString, bool);
    void updateCardFinished(ArcadeCard &, bool);
    void getCardsFinished(QList<ArcadeCard> &, bool);
    void getCardFinished(ArcadeCard &, bool);
    void sendPageInfo(int,int);
    void oneWayReplicated();
    void twoWayReplicated();
    void errorInReplication();

public slots:
    void sendCallFinished();
    void sendCallError(QNetworkReply::NetworkError error);

private:
    QNetworkReply *netReplyCheck;
    QNetworkAccessManager *netMgr;
    FormPostPlugin *formPost;
    Settings *settings;
    bool isSecond;
};

#endif // SHAREDCARDOPERATIONS_H
