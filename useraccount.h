#ifndef USERACCOUNT_H
#define USERACCOUNT_H

#include <QString>
#include <QDateTime>
#include "qjson-backport/qjsonobject.h"

class UserAccount
{
public:
  UserAccount();
  UserAccount(const QJsonObject &object);

  QString getUsername() const;
  void setUsername(const QString &value);

  QString getPassword() const;
  void setPassword(const QString &value);

  QString getRevID() const;
  void setRevID(const QString &value);

  QDateTime getActivated() const;
  void setActivated(const QDateTime &value);

  QDateTime getLastActive() const;
  void setLastActive(const QDateTime &value);

  QJsonObject toQJsonObject();

private:
  QString username;
  QString password;
  QString revID;
  QDateTime activated;
  QDateTime lastActive;
};

#endif // USERACCOUNT_H
