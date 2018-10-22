#ifndef ENCDEC_H
#define ENCDEC_H

#include <QString>
#include <QByteArray>

class EncDec
{
public:
  static bool aesEncryptFile(QString inFile, QString outFile, QString password);
  static bool aesDecryptFile(QString inFile, QString outFile, QString password);
  static QString aesEncryptString(QString plainText, QString password, bool *ok);
  static QString aesDecryptString(QString cypherText, QString password, bool *ok);

};

#endif // ENCDEC_H
