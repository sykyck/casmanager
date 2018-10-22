#include "encdec.h"
#include <QProcess>
#include <QFile>
#include "qslog/QsLog.h"
#include <QTemporaryFile>
#include <QTextStream>
#include <QDebug>

bool EncDec::aesEncryptFile(QString inFile, QString outFile, QString password)
{
  // Encrypt using AES256 CBC, salt it, base64 encode, use specified password
  // openssl enc -aes-256-cbc -salt -in casplayer-installer.sh -out casplayer-installer.bin -pass pass:mytoughpassword!

  QProcess openSSL;
  QStringList arguments;
  arguments << "enc" << "-aes-256-cbc" << "-salt" << "-in" << inFile << "-out" << outFile << "-pass" << QString("pass:%1").arg(password);

  openSSL.start("openssl", arguments);

  if (openSSL.waitForFinished())
  {
    return openSSL.exitCode() == 0;
  }
  else
    return false;
}

bool EncDec::aesDecryptFile(QString inFile, QString outFile, QString password)
{
  // Decrypt using AES256 CBC, base64 decode, use specified password
  // openssl enc -d -aes-256-cbc -a -in file.enc -out file.txt -pass pass:password

  QProcess openSSL;
  QStringList arguments;
  arguments << "enc" << "-d" << "-aes-256-cbc" << "-in" << inFile << "-out" << outFile << "-pass" << QString("pass:%1").arg(password);

  openSSL.start("openssl", arguments);

  if (openSSL.waitForFinished())
  {
    return openSSL.exitCode() == 0;
  }
  else
    return false;
}

QString EncDec::aesEncryptString(QString plainText, QString password, bool *ok)
{
  QTemporaryFile inFile;
 // inFile.setAutoRemove(false);
  QTemporaryFile outFile;
 // outFile.setAutoRemove(false);

  *ok = false;

  if (inFile.open() && outFile.open())
  {
   // QLOG_DEBUG() << QString("aesEncryptString inFile: %1, outFile: %2, plainText: %3").arg(inFile.fileName()).arg(outFile.fileName()).arg(plainText), true);

    QFile plainTextFile(inFile.fileName());

    if (plainTextFile.open(QIODevice::WriteOnly))
    {
      QTextStream out(&plainTextFile);

      out << plainText;

      plainTextFile.close();

      QProcess openSSL;
      QStringList arguments;
      arguments << "enc" << "-aes-256-cbc" << "-a" << "-salt" << "-in" << inFile.fileName() << "-out" << outFile.fileName() << "-pass" << QString("pass:%1").arg(password);
      // openssl enc -aes-256-cbc -a -salt -in plaintext.txt -out cyphertext.txt -pass pass:xxxxx

      openSSL.start("openssl", arguments);

      if (openSSL.waitForFinished())
      {
        if (openSSL.exitCode() == 0)
        {
          QFile cypherTextFile(outFile.fileName());
          if (cypherTextFile.open(QIODevice::ReadOnly))
          {
            QString cypherText;
            QTextStream in(&cypherTextFile);

            cypherText = in.readAll();

            cypherTextFile.close();

            *ok = true;
            return cypherText;
          }
        }
        else
          return "";
      }
      else
        return "";
    }
    return "";
  }
  else
    return "";
}

QString EncDec::aesDecryptString(QString cypherText, QString password, bool *ok)
{
  QTemporaryFile inFile;
 // inFile.setAutoRemove(false);
  QTemporaryFile outFile;
//  outFile.setAutoRemove(false);

  *ok = false;

  if (inFile.open() && outFile.open())
  {
  //  QLOG_DEBUG() << QString("aesDecryptString inFile: %1, outFile: %2, cypherText: %3").arg(inFile.fileName()).arg(outFile.fileName()).arg(cypherText), true);

    QFile cypherTextFile(inFile.fileName());

    if (cypherTextFile.open(QIODevice::WriteOnly))
    {
      QTextStream out(&cypherTextFile);
      out << cypherText;

      cypherTextFile.close();

      QProcess openSSL;
      QStringList arguments;
      arguments << "enc" << "-d" << "-aes-256-cbc" << "-a" << "-in" << inFile.fileName() << "-out" << outFile.fileName() << "-pass" << QString("pass:%1").arg(password);
      // openssl enc -d -aes-256-cbc -a -in cyphertext.txt -out plaintext2.txt -pass pass:xxxxx

      openSSL.start("openssl", arguments);

      if (openSSL.waitForFinished())
      {
        if (openSSL.exitCode() == 0)
        {
          QFile plainTextFile(outFile.fileName());
          if (plainTextFile.open(QIODevice::ReadOnly))
          {
            QString plainText;
            QTextStream in(&plainTextFile);

            plainText = in.readAll();

            plainTextFile.close();

            *ok = true;
            return plainText;
          }
        }
        else
          return "exit code not zero";
      }
      else
        return "process didn't finish";
    }
    return "could not open file to write cyphertext";
  }
  else
    return "could not open temp files";
}


