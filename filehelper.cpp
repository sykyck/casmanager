#include "filehelper.h"
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QDebug>

QString FileHelper::readTextFile(QString filename)
{
  QString contents = QString();

  QFile file(filename);
  if (file.exists())
  {
    if (file.open(QIODevice::Text | QIODevice::ReadOnly))
    {
      //QLOG_DEBUG() << QString("Reading script file: %1").arg(nicSpeedScriptFile.fileName());
      QTextStream in(&file);
      contents = in.readAll();
      file.close();
    }
  }

  return contents;
}

bool FileHelper::writeTextFile(QString filename, QString data)
{
  QFile file(filename);

  if (file.open(QIODevice::Text | QIODevice::Truncate | QIODevice::WriteOnly))
  {
    QTextStream out(&file);

    out << data << endl;

    file.close();

    return true;
  }
  else
    return false;
}

bool FileHelper::recursiveRmdir(QString path)
{
  // get list of files and delete each one, directory must be empty to delete it
  QDir targetPath(path);

  if (targetPath.exists())
  {
    QFileInfoList fileList = targetPath.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden);

    // Delete each file and subdirectory found
    foreach (QFileInfo f, fileList)
    {
      if (f.isFile())
      {
        if (!targetPath.remove(f.absoluteFilePath()))
        {
          //qDebug("Failed to delete file: %s", qPrintable(f.absoluteFilePath()));
          return false;
        }
      }
      else
      {
        // If this fails then exit now, returning false to indicate failure
        if (!FileHelper::recursiveRmdir(f.absoluteFilePath()))
        {
          //qDebug("Failed to delete files/directories in: %s", qPrintable(f.absoluteFilePath()));
          return false;
        }
      }
    }

    // All files and subdirectories should now be deleted so delete directory
    if (!targetPath.rmdir(path))
    {
      //qDebug("Failed to delete empty directory: %s", qPrintable(path));
      return false;
    }
    else
      return true;
  }
  else
  {
    //qDebug("Path doesn't exist: %s", qPrintable(path));
    return false;
  }
}
