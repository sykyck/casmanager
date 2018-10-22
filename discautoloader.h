#ifndef DISCAUTOLOADER_H
#define DISCAUTOLOADER_H

#include <QObject>
#include <QProcess>
#include "directorywatcher.h"

//   Eject DVD drive tray
//   Evaluate state: what is current state, move to next state, execute appropriate action
//   Autoloader: LOAD
//   Close DVD drive tray
//   Copy disc
//   Eject DVD drive tray
//   Autoloader: PICK
//   Close DVD drive tray
//   Autoloader: UNLOAD

class DiscAutoLoader : public QObject
{
  Q_OBJECT
public:
  explicit DiscAutoLoader(QString cdromDrivePath, QString autoloaderProg, QString autoloaderLogFile, QString driveStatusLogFile, int dvdMountTimeout, QObject *parent = 0);

  enum STATE
  {
    IDLE,
    EJECT_EMPTY_TRAY,
    LOAD_DISC_TO_TRAY,
    CLOSE_TRAY_WITH_DISC,
    COPYING_DISC,
    EJECT_TRAY_WITH_DISC,
    PICK_DISC_FROM_TRAY,
    CLOSE_EMPTY_TRAY,
    UNLOAD_DISC,
    REJECT_DISC
  };

  enum COMMAND
  {
    INIT,
    LOAD,
    PICK,
    UNLOAD,
    REJECT
  };

  QString getCdromDrivePath() const;
  void setCdromDrivePath(const QString &value);

  QString getAutoloaderProg() const;
  void setAutoloaderProg(const QString &value);

  QString getAutoloaderLogFile() const;
  void setAutoloaderLogFile(const QString &value);

signals:
  void finishedLoadingDisc(bool success, QString errorMsg = QString());
  void finishedUnloadingDisc(bool success, QString errorMsg = QString());
  
public slots:
  // Initialize the autoloader
  void initialize();

  // 1. Ejects the DVD drive tray
  // 2. Commands autoloader to load the disc onto the tray
  // 3. Closes the DVD drive tray
  void loadDisc();

  // 1. Ejects the DVD drive tray
  // 2. Commands autoloader to pick up disc from tray
  // 3. Closes the DVD drive tray
  // 4. Commands autoloader to release the DVD down the front or back slide
  void unloadDisc(bool rejectDisc = false);

private slots:
  void evaluateState(int exitCode);
  void foundDriveLogFile(QString logfile);
  void driveLogTimeout();
  
private:
  QString cdromDrivePath;
  QString autoloaderProg;
  QString autoloaderLogFile;
  QString driveStatusLogFile;

  void loadCdromDrive(QString drivePath);
  void ejectCdromDrive(QString drivePath);
  void sendAutoloaderCmd(COMMAND cmd);
  QString readTextFile(QString filename);

  QProcess *autoloaderProc;
  QProcess *ejectProc;

  DirectoryWatcher *dirWatcher;

  STATE currentState;

  // Flag set if disc should be rejected instead of unloading
  bool rejectDisc;
};

#endif // DISCAUTOLOADER_H
