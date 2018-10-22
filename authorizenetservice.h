#ifndef AUTHORIZENETSERVICE_H
#define AUTHORIZENETSERVICE_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QSslError>
#include "ccchargeresult.h"

class AuthorizeNetService : public QObject
{
  Q_OBJECT
public:
  explicit AuthorizeNetService(QString gatewayUrl, QString apiID, QString apiKey, QString transactionDescription = QString(), QObject *parent = 0);
  void charge(QString trackData, qreal amount, bool testMode = false);

  bool RelayResponse() const;
  void setRelayResponse(bool value);

  QString TransactionVersion() const;
  void setTransactionVersion(const QString &value);

  bool DelimitData() const;
  void setDelimitData(bool value);

  QChar Delimiter() const;
  void setDelimiter(const QChar &value);

  qint8 MarketType() const;
  void setMarketType(const qint8 &value);

  qint16 DuplicateWindow() const;
  void setDuplicateWindow(const qint16 &value);

  QString TransactionDescription() const;
  void setTransactionDescription(const QString &value);

  qint8 getDeviceType() const;
  void setDeviceType(const qint8 &value);

signals:
  void finishedCharge(CcChargeResult result);

public slots:

private slots:
  void replyFinished();
  void networkTimeout();
  void sslErrors(QList<QSslError> errorList);
  void demoTimerFinished();
  
private:
  static const int RESPONSE_FIELD_COUNT = 21;

  enum ResponseFields
  {
    // 0: Version - System version used to process the transaction
    VERSION,

    // 1: Response Code - 1 = Approved, 2 = Declined, 3 = Error, 4 = Held for Review
    RESPONSE_CODE,

    //2: Reason Code - A code representing more details about the result of the transaction
    REASON_CODE,

    //3: Reason Text - Brief description of a result, which corresponds with the Reason Code.
    REASON_TEXT,

    // 4: Authorization Code - Contains the six-digit alphanumeric approval code
    AUTHORIZATION_CODE,

    // 5: AVS Code - Indicates the result of Address Verification System (AVS) checks
    AVS_CODE,

    // 6: Card Code Response - Indicates the results of Card Code verification
    CARD_CODE_RESPONSE,

    // 7: Transaction ID - This number identifies the transaction in the system and can be used to submit a modification of this transaction at a later time, such as voiding, crediting or capturing the transaction
    TRANSACTION_ID,

    // 8: MD5 Hash - System-generated hash that can be validated by the merchant to authenticate a transaction response received from the gateway
    MD5_HASH,

    // 9: User Reference - Echoed by the system from the form input field x_user_ref
    USER_REF,

    // 10-19: Reserved

    // 20: Card number - The card number is returned as XXXX1234
    MASKED_CARD_NUM = 20,

    // 21: Card type - Visa, MasterCard, American Express, Discover, Diners Club, JCB
    CARD_TYPE,

    // The following fields are only provided if the merchant has enabled partial authorization transactions

    // 22: Split tender ID - The value that links the current authorization request to the original authorization request. This value is returned in the reply message from the original authorization request. This is only returned in the reply message for the first transaction that receives a partial authorization
    SPLIT_TENDER_ID,

    // 23: Requested amount - The amount requested in the original authorization
    REQUESTED_AMOUNT,

    // 24: Approved amount - The amount approved for this transaction
    APPROVED_AMOUNT,

    // 25: Remaining balance on card - Balance on the debit card or prepaid card. This has a value only if the current transaction is for a prepaid card
    REMAINING_BALANCE
  };

  QString apiID;
  QString apiKey;

  // Amount to charge credit card
  qreal amount;

  // False
  bool relayResponse;

  // 3.1
  QString transactionVersion;

  // True
  bool delimitData;

  // '|'
  QChar delimiter;

  // 2 = Retail
  qint8 marketType;

  // 3 = Self Service Terminal
  qint8 deviceType;

  // The period of time (seconds) after the submission of a transaction during which a duplicate transaction cannot be submitted
  // 0 - 28800
  qint16 duplicateWindow;

  // Optional transaction description. Up to 255 characters, no symbols
  QString transactionDescription;

  QUrl gatewayUrl;
  QNetworkAccessManager *netMgr;
  QTimer *networkTimer;
  bool timedOut;
  QNetworkReply *netReply;
  bool developmentMode;
  QTimer *demoTimer;
};

#endif // AUTHORIZENETSERVICE_H
