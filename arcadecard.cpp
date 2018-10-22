#include "arcadecard.h"
#include <QDebug>


ArcadeCard::ArcadeCard(QObject *parent) : QObject(parent)
{
  clear();
}

ArcadeCard::ArcadeCard(const ArcadeCard &other, QObject *parent) : QObject(parent)
{
  cardNum = other.getCardNum();
  revID = other.getRevID();
  madeBy = other.getMadeBy();
  credits = other.getCredits();
  inUse = other.getInUse();
  deviceNum = other.getDeviceNum();
  upc = other.getUPC();
  category = other.getCategory();
  activated = other.getActivated();
  lastActive = other.getLastActive();
  techAccess = other.getTechAccess();
  cardType = other.getCardType();
}

ArcadeCard &ArcadeCard::operator =(const ArcadeCard &other)
{
  cardNum = other.getCardNum();
  revID = other.getRevID();
  credits = other.getCredits();
  inUse = other.getInUse();
  deviceNum = other.getDeviceNum();
  upc = other.getUPC();
  category = other.getCategory();
  activated = other.getActivated();
  lastActive = other.getLastActive();
  techAccess = other.getTechAccess();
  cardType = other.getCardType();
  madeBy = other.getMadeBy();

  return *this;
}

ArcadeCard::ArcadeCard(const QJsonObject &object, QObject *parent) : QObject(parent)
{
  cardNum = object["_id"].toString();
  revID = object["_rev"].toString();
  credits = (int)object["credits"].toDouble();
  inUse = object["in_use"].toBool();
  deviceNum = object["last_device_num"].toDouble();
  if(object.contains("made_by"))
      madeBy = object["made_by"].toString();
  //upc = map.value("last_upc").toString();
  //category = map.value("last_category").toString();
  activated = QDateTime::fromString(object["activated"].toString(), "yyyy-MM-dd hh:mm:ss");
  lastActive = QDateTime::fromString(object["last_active"].toString(), "yyyy-MM-dd hh:mm:ss");
  cardType = ArcadeCard::CvsCardTypes((int)object["card_type"].toDouble());
}

ArcadeCard::~ArcadeCard()
{
}

void ArcadeCard::clear()
{
  cardNum.clear();
  revID.clear();
  madeBy.clear();
  credits = 0;
  inUse = false;
  deviceNum = 0;
  upc.clear();
  category.clear();
  activated = QDateTime();
  lastActive = QDateTime();
  techAccess = false;
  cardType = ARCADE;
}

void ArcadeCard::setCardNum(QString cardNum)
{
  this->cardNum = cardNum;
}

QString ArcadeCard::getCardNum() const
{
  return cardNum;
}

void ArcadeCard::setRevID(QString revID)
{
  this->revID = revID;
}

QString ArcadeCard::getRevID() const
{
  return revID;
}

void ArcadeCard::setMadeBy(QString madeBy)
{
    this->madeBy = madeBy;
}

QString ArcadeCard::getMadeBy() const
{
    return madeBy;
}

void ArcadeCard::setCredits(int credits)
{
  this->credits = credits;
}

int ArcadeCard::getCredits() const
{
  return credits;
}

void ArcadeCard::setInUse(bool inUse)
{
  this->inUse = inUse;
}

bool ArcadeCard::getInUse() const
{
  return inUse;
}

void ArcadeCard::setDeviceNum(int deviceNum)
{
  this->deviceNum = deviceNum;
}

int ArcadeCard::getDeviceNum() const
{
  return deviceNum;
}

void ArcadeCard::setUPC(QString upc)
{
  this->upc = upc;
}

QString ArcadeCard::getUPC() const
{
  return upc;
}

void ArcadeCard::setCategory(QString category)
{
  this->category = category;
}

QString ArcadeCard::getCategory() const
{
  return category;
}

void ArcadeCard::setActivated(QDateTime activated)
{
  this->activated = activated;
}

QDateTime ArcadeCard::getActivated() const
{
  return activated;
}

void ArcadeCard::setLastActive(QDateTime lastActive)
{
  this->lastActive = lastActive;
}

QDateTime ArcadeCard::getLastActive() const
{
  return lastActive;
}

bool ArcadeCard::getTechAccess() const
{
  return cardType == ArcadeCard::TECH;
}

qreal ArcadeCard::getCashValue() const
{
  return (qreal)credits / 4;
}

QJsonObject ArcadeCard::toQJsonObject()
{
  QJsonObject object;

  object["_id"] = cardNum;

  if (!revID.isEmpty())
    object["_rev"] = revID;

  object["credits"] = credits;
  object["in_use"] = inUse;
  object["last_device_num"] = deviceNum;
  //j["last_upc"] = upc;
  //j["last_category"] = category;
  object["activated"] = activated.toString("yyyy-MM-dd hh:mm:ss");
  object["last_active"] = (lastActive.isValid() ? lastActive.toString("yyyy-MM-dd hh:mm:ss") : NULL);
  object["card_type"] = cardType;
  object["type"] = QString("card");
  object["made_by"] = madeBy;

  return object;
}

void ArcadeCard::setCardType(ArcadeCard::CvsCardTypes cardType)
{
  this->cardType = cardType;
}

ArcadeCard::CvsCardTypes ArcadeCard::getCardType() const
{
  return cardType;
}

QString ArcadeCard::cardTypeToString()
{
  const QMetaObject &mo = ArcadeCard::staticMetaObject;
  int index = mo.indexOfEnumerator("CvsCardTypes");
  QMetaEnum metaEnum = mo.enumerator(index);

  return QString(metaEnum.valueToKey(cardType));
}
