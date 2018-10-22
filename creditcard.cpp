#include "creditcard.h"
#include <QStringList>

CreditCard::CreditCard(QString trackData, QObject *parent) : QObject(parent)
{
  this->trackData = trackData;
  track1.clear();
  track2.clear();
  accountNum.clear();
  expirationMonth = 0;
  expirationYear = 0;

  parseTrackData();
}

CreditCard::~CreditCard()
{
  trackData.clear();
  track1.clear();
  track2.clear();
  accountNum.clear();
  expirationMonth = 0;
  expirationYear = 0;
}

void CreditCard::parseTrackData()
{
  bool track1Valid = false;
  bool track2Valid = false;

  QStringList tracks = trackData.split(";", QString::SkipEmptyParts);

  if (tracks.at(0).contains("^"))
    track1 = tracks.at(0);
  else
    track2 = tracks.at(0);

  if (tracks.count() > 1)
    track2 = tracks.at(1);

  //QLOG_DEBUG() << "Track 1:" << track1 << "Track 2:" << track2;

  // Track 1 layout
  // Start Sentinal (%), Format Code (B), PAN (Primary account number, 19 digits max), Field Separator (^), Additional Data (YYMM), Discretional data, End Sentinal (?)
  // Example: %B4111111111111111^DOE/JOHN           ^160550506070202203000110163000000?
  // Account #: 4111111111111111, Exp Date: 05/16

  // If track1 exists then extract the account number and expiration date
  if (track1.length() > 0)
  {
    QStringList fields = track1.split("^", QString::SkipEmptyParts);

    if (fields.count() > 2)
    {
      // Strip non-numeric characters from the Primary Account Number
      QString pan = fields.at(0);
      accountNum = pan.remove(QRegExp("[^0-9]"));

      if (fields.at(2).length() >= 4)
      {
        QString additionalData = fields.at(2);

        expirationYear = additionalData.left(2).toInt() + 2000;
        expirationMonth = additionalData.mid(2, 2).toInt();

        track1Valid = true;
      }
    }
  }

  // Track 2 layout
  // Start Sentinal (;), PAN (Primary account number, 19 digits max), Field Separator (=), Additional Data (YYMM), Discretional data, End Sentinal (?)
  // Example: ;4111111111111111=1605010087012181?
  // Account #: 4111111111111111, Exp Date: 05/16

  // If track 1 wasn't valid and there is a track 2 then
  // extract the account number and expiration date
  if (!track1Valid && track2.length() > 0)
  {
    QStringList fields = track2.split("=", QString::SkipEmptyParts);

    if (fields.count() > 1)
    {
      // Strip non-numeric characters from the Primary Account Number
      QString pan = fields.at(0);
      accountNum = pan.remove(QRegExp("[^0-9]"));

      if (fields.at(1).length() >= 4)
      {
        QString additionalData = fields.at(1);

        expirationYear = additionalData.left(2).toInt() + 2000;
        expirationMonth = additionalData.mid(2, 2).toInt();

        track2Valid = true;
      }
    }
  }

  //QLOG_DEBUG() << "accountNum: " + accountNum + ", expYear: " + QString::number(expirationYear) + ", expMonth: " + QString::number(expirationMonth);
}

qint16 CreditCard::ExpirationYear() const
{
  return expirationYear;
}

void CreditCard::setExpirationYear(const qint16 &value)
{
  expirationYear = value;
}

qint16 CreditCard::ExpirationMonth() const
{
  return expirationMonth;
}

void CreditCard::setExpirationMonth(const qint16 &value)
{
  expirationMonth = value;
}

QString CreditCard::AccountNum() const
{
  return accountNum;
}

void CreditCard::setAccountNum(const QString &value)
{
  accountNum = value;
}

QString CreditCard::Track2() const
{
  return track2;
}

void CreditCard::setTrack2(const QString &value)
{
  track2 = value;
}

QString CreditCard::Track1() const
{
  return track1;
}

void CreditCard::setTrack1(const QString &value)
{
  track1 = value;
}
