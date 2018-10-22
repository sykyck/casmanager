#ifndef UFTPSERVER_H
#define UFTPSERVER_H

#include <QObject>
#include <QStringList>
#include <QProcess>

class UftpServer : public QObject
{
  Q_OBJECT
public:
  enum UftpStatusCode
  {
    Success,               // All clients finished downloading
    Fail_Restart_Possible, // One or more clients failed to download and restart file exists
    Fail_Cannot_Restart,   // One or more clients faild but no restart file
    Exit_With_Error,       // UFTP returned an error code
    Unknown_State          // Log and status files are missing, state is unknown
  };

  explicit UftpServer(QString uftpProg, QString uftpLogFile, QString statusFile, QString listFile, QObject *parent = 0);
  ~UftpServer();
  bool isRunning();

signals:
  void startedCopying();
  void finishedCopying(UftpServer::UftpStatusCode statusCode, QString errorMessage = QString(), QStringList failureList = QStringList(), QStringList successList = QStringList());
  void processError();

public slots:
  void start(int transmissionSpeed, QStringList dirList, bool useRestartFile = false);
  void stop();
  
private slots:
  void uftpProcStarted();
  void uftpProcFinished(int exitCode);
  void uftpProcError(QProcess::ProcessError errorCode);

private:
  QStringList getSessionClientList();
  bool isSuccessfulSession(QString clientID);
  QProcess *uftpProc;
  QString uftpProg;
  QString uftpLogFile;
  QString statusFile;
  QString listFile;
  QString restartFile;
  int transmissionSpeed;
  bool enableSessionRestart;
  QString sessionGroupID;
};

#endif // UFTPSERVER_H
