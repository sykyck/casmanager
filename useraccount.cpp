#include "useraccount.h"

UserAccount::UserAccount()
{
  username.clear();
  password.clear();
}

UserAccount::UserAccount(const QJsonObject &object)
{
  username = object["_id"].toString();
  revID = object["_rev"].toString();
  password = object["password"].toString();
  activated = QDateTime::fromString(object["activated"].toString(), "yyyy-MM-dd hh:mm:ss");
  lastActive = QDateTime::fromString(object["last_active"].toString(), "yyyy-MM-dd hh:mm:ss");
}

QString UserAccount::getPassword() const
{
  return password;
}

void UserAccount::setPassword(const QString &value)
{
  password = value;
}

QString UserAccount::getUsername() const
{
  return username;
}

void UserAccount::setUsername(const QString &value)
{
  username = value;
}

QDateTime UserAccount::getLastActive() const
{
  return lastActive;
}

void UserAccount::setLastActive(const QDateTime &value)
{
  lastActive = value;
}

QDateTime UserAccount::getActivated() const
{
  return activated;
}

void UserAccount::setActivated(const QDateTime &value)
{
  activated = value;
}

QString UserAccount::getRevID() const
{
  return revID;
}

void UserAccount::setRevID(const QString &value)
{
  revID = value;
}

QJsonObject UserAccount::toQJsonObject()
{
  QJsonObject object;

  object["_id"] = username;

  if (!revID.isEmpty())
    object["_rev"] = revID;

  object["password"] = password;
  object["activated"] = activated.toString("yyyy-MM-dd hh:mm:ss");
  object["last_active"] = (lastActive.isValid() ? lastActive.toString("yyyy-MM-dd hh:mm:ss") : NULL);
  object["type"] = QString("user");

  return object;
}
