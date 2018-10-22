#include "authorizenetservice.h"
#include "creditcard.h"
#include "qslog/QsLog.h"

static const QString APPROVED_RESPONSE_CODE = "1";
static const QString APPROVED_REASON_CODE = "1";

AuthorizeNetService::AuthorizeNetService(QString gatewayUrl, QString apiID, QString apiKey, QString transactionDescription, QObject *parent) : QObject(parent)
{
  this->gatewayUrl = QUrl(gatewayUrl);
  this->apiID = apiID;
  this->apiKey = apiKey;
  this->transactionDescription = transactionDescription;
  this->developmentMode = false;

  // Default settings
  relayResponse = false;
  transactionVersion = "3.1";
  delimitData = true;
  delimiter = '|';
  marketType = 2; // 2 = Retail (required for submitting track data)
  deviceType = 0; // 0 = exclude this field from request
  duplicateWindow = 30;

  netMgr = new QNetworkAccessManager;

  networkTimer = new QTimer;
  networkTimer->setSingleShot(true);
  networkTimer->setInterval(30000);
  connect(networkTimer, SIGNAL(timeout()), this, SLOT(networkTimeout()));

  demoTimer = new QTimer;
  demoTimer->setSingleShot(true);
  demoTimer->setInterval(5000);
  connect(demoTimer, SIGNAL(timeout()), this, SLOT(demoTimerFinished()));
}

void AuthorizeNetService::charge(QString trackData, qreal amount, bool testMode)
{
  /*
  Test accounts
  =============================================
  American Express	378282246310005
  American Express	371449635398431
  American Express Corporate	378734493671000
  Diners Club	38520000023237
  Discover	6011000990139424
  JCB	3530111333300000
  JCB	3566002020360505
  MasterCard	5555555555554444
  MasterCard	5105105105105100
  Visa	4111111111111111
  Visa	4012888888881881
  Visa	4222222222222
  */

  timedOut = false;
  this->amount = amount;

  bool demoMode = false;

  if (demoMode)
  {
    QLOG_DEBUG() << "Charging credit card in demo mode...";
    demoTimer->start();
  }
  else
  {
    QString data = QString("x_login=%1&x_tran_key=%2&x_type=%3&x_amount=%4&x_relay_response=%5&x_version=%6&x_delim_data=%7&x_market_type=%8&x_duplicate_window=%9")
                   .arg(apiID)
                   .arg(apiKey)
                   .arg("AUTH_CAPTURE")
                   .arg(amount)
                   .arg(relayResponse ? "T" : "F")
                   .arg(transactionVersion)
                   .arg(delimitData ? "T" : "F")
                   .arg(marketType)
                   .arg(duplicateWindow);

    if (deviceType > 0)
    {
      QLOG_DEBUG() << "Including x_device_type field";
      data.append(QString("&&x_device_type=%1").arg(deviceType));
    }

    data.append(QString("&x_delim_char=%1").arg(QUrl::toPercentEncoding(delimiter).data()));

    //trackData = "%B4111111111111111^DOE/JOHN           ^200550506070202203000110163000000?;4111111111111111=1605010087012181?";
    CreditCard cc(trackData);

    // If track1 found then use this, otherwise use track2
    if (!cc.Track1().isEmpty())
    {
      QLOG_DEBUG() << "Using track 1 data";
      data.append(QString("&x_track1=%1").arg(QUrl::toPercentEncoding(cc.Track1()).data()));
    }
    else
    {
      QLOG_DEBUG() << "Using track 2 data";
      data.append(QString("&x_track2=%1").arg(QUrl::toPercentEncoding(cc.Track2()).data()));
    }

    // If x_test_request is set to TRUE and the merchant account is NOT in test mode on Authorize.Net
    // then the transaction is treated as a test
    if (testMode)
    {
      QLOG_DEBUG() << "Setting test transaction flag";
      data.append("&x_test_request=T");
    }

    if (!transactionDescription.isEmpty())
      data.append(QString("&x_description=%1").arg(QUrl::toPercentEncoding(transactionDescription).data()));

    //QLOG_DEBUG() << "sending:" << data;

    // I was getting the error "SSL handshake failed" after updating the CA certificate bundle on the CAS system.
    // Looking at the SSL errors that are returned: "The issuer certificate of a locally looked up certificate could not be found",
    // it looks like the newer bundle is missing this certificate. I recreated the same problem using Qt's example SSL connection program in Ubuntu.
    // I then returned to the old CA certificate bundle and it worked fine, even on Ubuntu (by manually specifying the bundle file to load
    // with a simple modification to the sample program). I was able to figure out which certificate was missing:
    // Entrust
    //   CN = Entrust.net Secure Server Certification Authority
    //   SHA1 Fingerprint: 99:A6:9B:E6:1A:FE:88:6B:4D:2B:82:00:7C:B8:54:FC:31:7E:15:39
    /*
    -----BEGIN CERTIFICATE-----
    MIIE2DCCBEGgAwIBAgIEN0rSQzANBgkqhkiG9w0BAQUFADCBwzELMAkGA1UEBhMC
    VVMxFDASBgNVBAoTC0VudHJ1c3QubmV0MTswOQYDVQQLEzJ3d3cuZW50cnVzdC5u
    ZXQvQ1BTIGluY29ycC4gYnkgcmVmLiAobGltaXRzIGxpYWIuKTElMCMGA1UECxMc
    KGMpIDE5OTkgRW50cnVzdC5uZXQgTGltaXRlZDE6MDgGA1UEAxMxRW50cnVzdC5u
    ZXQgU2VjdXJlIFNlcnZlciBDZXJ0aWZpY2F0aW9uIEF1dGhvcml0eTAeFw05OTA1
    MjUxNjA5NDBaFw0xOTA1MjUxNjM5NDBaMIHDMQswCQYDVQQGEwJVUzEUMBIGA1UE
    ChMLRW50cnVzdC5uZXQxOzA5BgNVBAsTMnd3dy5lbnRydXN0Lm5ldC9DUFMgaW5j
    b3JwLiBieSByZWYuIChsaW1pdHMgbGlhYi4pMSUwIwYDVQQLExwoYykgMTk5OSBF
    bnRydXN0Lm5ldCBMaW1pdGVkMTowOAYDVQQDEzFFbnRydXN0Lm5ldCBTZWN1cmUg
    U2VydmVyIENlcnRpZmljYXRpb24gQXV0aG9yaXR5MIGdMA0GCSqGSIb3DQEBAQUA
    A4GLADCBhwKBgQDNKIM0VBuJ8w+vN5Ex/68xYMmo6LIQaO2f55M28Qpku0f1BBc/
    I0dNxScZgSYMVHINiC3ZH5oSn7yzcdOAGT9HZnuMNSjSuQrfJNqc1lB5gXpa0zf3
    wkrYKZImZNHkmGw6AIr1NJtl+O3jEP/9uElY3KDegjlrgbEWGWG5VLbmQwIBA6OC
    AdcwggHTMBEGCWCGSAGG+EIBAQQEAwIABzCCARkGA1UdHwSCARAwggEMMIHeoIHb
    oIHYpIHVMIHSMQswCQYDVQQGEwJVUzEUMBIGA1UEChMLRW50cnVzdC5uZXQxOzA5
    BgNVBAsTMnd3dy5lbnRydXN0Lm5ldC9DUFMgaW5jb3JwLiBieSByZWYuIChsaW1p
    dHMgbGlhYi4pMSUwIwYDVQQLExwoYykgMTk5OSBFbnRydXN0Lm5ldCBMaW1pdGVk
    MTowOAYDVQQDEzFFbnRydXN0Lm5ldCBTZWN1cmUgU2VydmVyIENlcnRpZmljYXRp
    b24gQXV0aG9yaXR5MQ0wCwYDVQQDEwRDUkwxMCmgJ6AlhiNodHRwOi8vd3d3LmVu
    dHJ1c3QubmV0L0NSTC9uZXQxLmNybDArBgNVHRAEJDAigA8xOTk5MDUyNTE2MDk0
    MFqBDzIwMTkwNTI1MTYwOTQwWjALBgNVHQ8EBAMCAQYwHwYDVR0jBBgwFoAU8Bdi
    E1U9s/8KAGv7UISX8+1i0BowHQYDVR0OBBYEFPAXYhNVPbP/CgBr+1CEl/PtYtAa
    MAwGA1UdEwQFMAMBAf8wGQYJKoZIhvZ9B0EABAwwChsEVjQuMAMCBJAwDQYJKoZI
    hvcNAQEFBQADgYEAkNwwAvpkdMKnCqV8IY00F6j7Rw7/JXyNEwr75Ji174z4xRAN
    95K+8cPV1ZVqBLssziY2ZcgxxufuP+NXdYR6Ee9GTxj005i7qIcyunL2POI9n9cd
    2cNgQ4xYDiKWL2KjLB+6rQXvqzJ4h6BUcxm1XAX5Uj5tLUUL9wqT6u0G+bI=
    -----END CERTIFICATE-----
     */
    // I then found an this post on Mozilla's site: https://blog.mozilla.org/security/2014/09/08/phasing-out-certificates-with-1024-bit-rsa-keys/
    // about them phasing out weaker certificates. So when I first created the CA bundle, this
    // certificate was on my Ubuntu development system. But later when I created a new version, it had been
    // deleted by Firefox. So until Authorize.net gets issued a new
    // certificate from Entrust, I'll have to keep this certificate in the bundle.
    // This page on Entrust's site talks about getting an old version of Java working which
    // includes the above certificate: http://www.entrust.net/knowledge-base/technote.cfm?tn=7044

    QByteArray convertedData(qPrintable(data));
    QNetworkRequest request(gatewayUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "text/plain");

    netReply = netMgr->post(request, convertedData);
    connect(netReply, SIGNAL(finished()), this, SLOT(replyFinished()));

    networkTimer->start();
  }
}

QString AuthorizeNetService::TransactionDescription() const
{
  return transactionDescription;
}

void AuthorizeNetService::setTransactionDescription(const QString &value)
{
  transactionDescription = value;
}

qint16 AuthorizeNetService::DuplicateWindow() const
{
  return duplicateWindow;
}

void AuthorizeNetService::setDuplicateWindow(const qint16 &value)
{
  duplicateWindow = value;
}

qint8 AuthorizeNetService::MarketType() const
{
  return marketType;
}

void AuthorizeNetService::setMarketType(const qint8 &value)
{
  marketType = value;
}

QChar AuthorizeNetService::Delimiter() const
{
  return delimiter;
}

void AuthorizeNetService::setDelimiter(const QChar &value)
{
  delimiter = value;
}

bool AuthorizeNetService::DelimitData() const
{
  return delimitData;
}

void AuthorizeNetService::setDelimitData(bool value)
{
  delimitData = value;
}

QString AuthorizeNetService::TransactionVersion() const
{
  return transactionVersion;
}

void AuthorizeNetService::setTransactionVersion(const QString &value)
{
  transactionVersion = value;
}

bool AuthorizeNetService::RelayResponse() const
{
  return relayResponse;
}

void AuthorizeNetService::setRelayResponse(bool value)
{
  relayResponse = value;
}


void AuthorizeNetService::replyFinished()
{
  networkTimer->stop();

  disconnect(netReply, 0, 0, 0);

  // Response example: 1.0|1|1|This transaction has been approved.|HWWK70|Y||2228158428|0EF0F6AAD7AD13021ADE497662AEAF84||||||||||||XXXX1111|Visa

  CcChargeResult result;
  result.setAmount(amount);

  QString msg = "";

  if (netReply->error() == QNetworkReply::NoError)
  {
    QByteArray ba = netReply->readAll();
    QString buffer = QString(ba.constData());

    if (buffer.length() > 0)
    {
      //QLOG_DEBUG() << "Payment gateway response:" << buffer;

      // Get the response code, reason code and message
      QStringList fields = buffer.split(delimiter);

      if (fields.count() >= RESPONSE_FIELD_COUNT)
      {
        // Response code and Reason code
        if (fields.at(RESPONSE_CODE) == APPROVED_RESPONSE_CODE &&
            fields.at(REASON_CODE) == APPROVED_REASON_CODE)
        {
          result.setCharged(true);
          result.setAuthCode(fields.at(AUTHORIZATION_CODE));
          result.setTransactionID(fields.at(TRANSACTION_ID));
        }
        else
        {
          QLOG_DEBUG() << QString("Response Code: %1, Reason Code: %2, Reason Text: %3")
                          .arg(fields.at(RESPONSE_CODE))
                          .arg(fields.at(REASON_CODE))
                          .arg(fields.at(REASON_TEXT));
        }

        result.setCardType(fields.at(CARD_TYPE));
        result.setMaskedCardNum(fields.at(MASKED_CARD_NUM));

        // Message
        msg = fields.at(REASON_TEXT);
      }
      else
      {
        QLOG_ERROR() << "Unexpected response from payment gateway: " << buffer;
        msg = "Unexpected response from payment gateway.";
      }
    }
    else
    {
      QLOG_ERROR() << "No network error but the response is empty!";
      msg = "Empty response from payment gateway.";
    }
  }
  else
  {
    QLOG_DEBUG() << "QNetworkReply error code:" << netReply->error() << "Error:" << netReply->errorString();

    if (timedOut)
      msg = "Connection timed out";
    else
      msg = netReply->errorString();
  }

  if (!result.isCharged())
    result.setErrorMessage(msg);

  emit finishedCharge(result);

  amount = 0;

  netReply->deleteLater();
  netReply = 0;

}

void AuthorizeNetService::networkTimeout()
{
  QLOG_ERROR() << "Network timeout while communicating with payment gateway.";

  timedOut = true;

  // When abort is called the network communication is immediately terminated
  // and the finished signal is emitted from netReply
  if (netReply)
    netReply->abort();
}

void AuthorizeNetService::sslErrors(QList<QSslError> errorList)
{
  foreach (QSslError error, errorList)
  {
    QLOG_ERROR() << "QSslError:" << error.errorString();
  }

  //netReply->ignoreSslErrors();
}

void AuthorizeNetService::demoTimerFinished()
{
  CcChargeResult result;
  result.setAmount(amount);
  result.setCharged(true);

  emit finishedCharge(result);
}

qint8 AuthorizeNetService::getDeviceType() const
{
  return deviceType;
}

void AuthorizeNetService::setDeviceType(const qint8 &value)
{
  deviceType = value;
}
