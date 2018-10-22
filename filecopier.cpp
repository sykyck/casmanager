#include "filecopier.h"
#include "qslog/QsLog.h"
#include "filehelper.h"
#include <QDir>
#include <QDirIterator>
#include <QtCore>

FileCopier::FileCopier(QObject *parent) : QObject(parent)
{
  copiedFiles = 0;
  totalFiles = 0;

  connect(&qfwCopyFiles, SIGNAL(finished()), this, SIGNAL(finished()));
}

FileCopier::~FileCopier()
{

}

bool FileCopier::isBusy()
{
  return qfwCopyFiles.isRunning();
}

bool FileCopier::isSuccess()
{
  // Copying is considered successful if the number of files copied = the total files
  QLOG_DEBUG() << QString("copiedFiles = %1, totalFiles = %2").arg(copiedFiles).arg(totalFiles);

  return copiedFiles == totalFiles;
}

void FileCopier::copy(const QString &srcPath, const QString &destPath)
{
  copyRecursively(srcPath, destPath);
}

void FileCopier::startCopy(const QString &srcPath, const QString &destPath)
{
  copiedFiles = 0;
  totalFiles = 0;

  QLOG_DEBUG() << QString("Source path: %1, Dest path: %2: . Copying the following files:").arg(srcPath).arg(destPath);

  // Determine number of files that will be copied
  QDirIterator it(srcPath, QDir::Files, QDirIterator::Subdirectories);
  while (it.hasNext())
  {
      QLOG_DEBUG() << it.next();
      ++totalFiles;
  }

  emit filesToCopy(totalFiles);

  qfwCopyFiles.setFuture(QtConcurrent::run(this, &FileCopier::copy, srcPath, destPath));
}

bool FileCopier::copyRecursively(const QString &srcPath, const QString &destPath)
{
    QFileInfo srcFileInfo(srcPath);
    if (srcFileInfo.isDir())
    {
        QDir targetDir(destPath);
        targetDir.cdUp();
        if (!targetDir.mkpath(QFileInfo(destPath).fileName()))
          return false;

        QDir sourceDir(srcPath);
        QStringList fileNames = sourceDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
        foreach (const QString &fileName, fileNames)
        {
            const QString newSrcFilePath = srcPath + QLatin1Char('/') + fileName;
            const QString newTgtFilePath = destPath + QLatin1Char('/') + fileName;

            if (!this->copyRecursively(newSrcFilePath, newTgtFilePath))
              return false;
        }
    }
    else
    {
      if (!QFile::copy(srcPath, destPath))
        return false;
      else
      {
        emit copyProgress(++copiedFiles);
      }
    }    

    return true;
}


