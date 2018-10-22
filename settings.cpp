#include <QSqlQuery>
#include <QSettings>
#include <QFile>
#include <QDir>
#include <QCoreApplication>
#include <QTextStream>
#include "settings.h"
#include "encdec.h"
#include "qjson/serializer.h"
#include "qjson/parser.h"

char key2[] = {0x06, 0x18, 0x40, 0x75, 0x41, 0x1d, 0x40, 0x1b,
               0x10, 0x53, 0x76, 0x1d, 0x1c, 0x1b, 0x57, 0x51,
               0x71, 0x52, 0x6c, 0x6a, 0x69, 0x78, 0x63, 0x5b,
               0x2a, 0x74, 0x4c, 0x7b, 0x55, 0x2a, 0x54, 0x42,
               0};

Settings::Settings(QString settings, QObject *parent) : QObject(parent)
{
  loadSettings(settings);
}

QVariant Settings::getValue(QString keyName, QVariant defaultValue, bool decrypt)
{
  QVariant value;

  if (_settings.contains(keyName))
  {
    value = _settings[keyName];
  }
  else
  {
    if (defaultValue.isValid())
    {
      setValue(keyName, defaultValue);
      value = defaultValue;
    }
  }

  if (decrypt)
  {
    if (value.canConvert<QString>())
    {
      QString decoded;
      QByteArray password(key2);

      for (int i = 0; i < password.length(); ++i)
      {
        password[i] = password[i] ^ ((password.length() - i) + 16);

        decoded.append(QChar(password[i]));
      }

      bool ok = false;
      QString decodedValue = EncDec::aesDecryptString(value.toString(), decoded, &ok);

      if (ok)
        value = decodedValue;
      else
        value = QVariant();
    }
    else
      value = QVariant();
  }

  return value;
}

void Settings::setValue(QString keyName, QVariant value, bool encrypt)
{
  if (encrypt)
  {
    if (value.canConvert<QString>())
    {
      QString decoded;
      QByteArray password(key2);

      for (int i = 0; i < password.length(); ++i)
      {
        password[i] = password[i] ^ ((password.length() - i) + 16);

        decoded.append(QChar(password[i]));
      }

      bool ok = false;
      QString encodedValue = EncDec::aesEncryptString(value.toString(), decoded, &ok);

      if (ok)
        value = encodedValue;
      else
        value = QVariant();
    }
    else
      value = QVariant();
  }

  _settings[keyName] = value;
}

void Settings::loadSettings(QString settings)
{
  bool ok = false;
  QJson::Parser parser;
  QVariant var = parser.parse(settings.toAscii(), &ok);

  if (ok)
  {
    _settings = var.toMap();
  }
}

QString Settings::getSettings()
{
  QJson::Serializer serializer;
  QByteArray jsonData = serializer.serialize(_settings);

  return QString(jsonData.constData());
}
