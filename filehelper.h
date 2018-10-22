#ifndef FILEHELPER_H
#define FILEHELPER_H

#include <QString>

class FileHelper
{
public:
  static QString readTextFile(QString filename);
  static bool writeTextFile(QString filename, QString data);
  static bool recursiveRmdir(QString path);
};

#endif // FILEHELPER_H
