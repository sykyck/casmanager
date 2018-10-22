#ifndef ARCADECARD_H
#define ARCADECARD_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QMetaType>
#include <QMetaEnum>
#include <QVariant>
#include "qjson-backport/qjsonobject.h"

class ArcadeCard : public QObject
{
  Q_OBJECT
public:
  enum CvsCardTypes
  {
    ARCADE,
    PREVIEW,
    TECH = 14
  };

  Q_ENUMS(CvsCardTypes)

  explicit ArcadeCard(QObject *parent = 0);
  ArcadeCard(const ArcadeCard &other, QObject *parent = 0);
  ArcadeCard &operator =(const ArcadeCard &other);
  ArcadeCard(const QJsonObject &object, QObject *parent = 0);
  ~ArcadeCard();
  void clear();

  void setCardNum(QString cardNum);
  QString getCardNum() const;

  void setRevID(QString revID);
  QString getRevID() const;

  void setMadeBy(QString madeBy);
  QString getMadeBy() const;

  void setCredits(int credits);
  int getCredits() const;

  void setInUse(bool inUse);
  bool getInUse() const;

  void setDeviceNum(int deviceNum);
  int getDeviceNum() const;

  void setUPC(QString upc);
  QString getUPC() const;

  void setCategory(QString category);
  QString getCategory() const;

  void setActivated(QDateTime activated);
  QDateTime getActivated() const;

  void setLastActive(QDateTime lastActive);
  QDateTime getLastActive() const;

  bool getTechAccess() const;

  qreal getCashValue() const;

  QJsonObject toQJsonObject();

  void setCardType(CvsCardTypes cardType);
  CvsCardTypes getCardType() const;
  QString cardTypeToString();

private:
  // Card # (CouchDB _id field)
  QString cardNum;

  // Revision ID (CouchDB _rev field)
  QString revID;

  QString madeBy;

  // Number of credits available on card
  int credits;

  // Set when the card is being used at a device
  bool inUse;

  // Last device address the card was used at
  int deviceNum;

  // Last video UPC watched
  QString upc;

  // Last category watched
  QString category;

  // When the card was first entered into the system
  QDateTime activated;

  // Last time the card was used
  QDateTime lastActive;

  // When set the user has access to administration area of software
  bool techAccess;

  CvsCardTypes cardType;
};

// Makes the custom data type known to QMetaType so it can be stored in a QVariant
// The class requires a public default constructor, public copy constructor and public destructor
Q_DECLARE_METATYPE(ArcadeCard)

#endif // ARCADECARD_H
