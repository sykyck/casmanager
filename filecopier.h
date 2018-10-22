#ifndef FILECOPIER_H
#define FILECOPIER_H

#include <QObject>
#include <QString>
#include <QFutureWatcher>

class FileCopier : public QObject
{
  Q_OBJECT
public:
  explicit FileCopier(QObject *parent = 0);
  ~FileCopier();  
  bool isBusy();
  bool isSuccess();
  
signals:
  void filesToCopy(int totalFiles);
  void copyProgress(int copiedFiles);
  void finished();

public slots:
  void startCopy(const QString &srcPath, const QString &destPath);
  
private slots:
  void copy(const QString &srcPath, const QString &destPath);
  bool copyRecursively(const QString &srcPath, const QString &destPath);

private:
  int copiedFiles;
  int totalFiles;

  QFutureWatcher<void> qfwCopyFiles;
};

#endif // FILECOPIER_H
