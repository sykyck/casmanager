#ifndef CREDITCARD_H
#define CREDITCARD_H

#include <QObject>
#include <QString>

class CreditCard : public QObject
{
  Q_OBJECT
public:
  explicit CreditCard(QString trackData, QObject *parent = 0);
  ~CreditCard();
  
  QString Track1() const;
  void setTrack1(const QString &value);

  QString Track2() const;
  void setTrack2(const QString &value);

  QString AccountNum() const;
  void setAccountNum(const QString &value);

  qint16 ExpirationMonth() const;
  void setExpirationMonth(const qint16 &value);

  qint16 ExpirationYear() const;
  void setExpirationYear(const qint16 &value);

signals:
  
public slots:
  
private:
  void parseTrackData();

  QString trackData;
  QString track1;
  QString track2;
  QString accountNum;
  qint16 expirationMonth;
  qint16 expirationYear;
};

#endif // CREDITCARD_H
