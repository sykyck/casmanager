#include "discautoloader.h"
#include "filehelper.h"
#include "qslog/QsLog.h"
#include <QFileInfo>

DiscAutoLoader::DiscAutoLoader(QString cdromDrivePath, QString autoloaderProg, QString autoloaderLogFile, QString driveStatusLogFile, int dvdMountTimeout, QObject *parent) : QObject(parent)
{
  // FIXME: The autoloader executable seems to crash when calling it for the first time after a system restart. Don't know if it's because I'm not calling INIT first, need to test more when we get another autoloader

  this->cdromDrivePath = cdromDrivePath;
  this->autoloaderProg = autoloaderProg;
  this->autoloaderLogFile = autoloaderLogFile;
  this->driveStatusLogFile = driveStatusLogFile;

  currentState = IDLE;

  QFileInfo workingDir(autoloaderProg);
  autoloaderProc = new QProcess;
  autoloaderProc->setWorkingDirectory(workingDir.path());
  connect(autoloaderProc, SIGNAL(finished(int)), this, SLOT(evaluateState(int)));

  ejectProc = new QProcess;
  connect(ejectProc, SIGNAL(finished(int)), this, SLOT(evaluateState(int)));

  QFileInfo statusFile(driveStatusLogFile);
  QStringList fileFilter;
  fileFilter <<  statusFile.fileName();
  dirWatcher = new DirectoryWatcher;
  dirWatcher->addPath(statusFile.path(), fileFilter, dvdMountTimeout);
  connect(dirWatcher, SIGNAL(fileFound(QString)), this, SLOT(foundDriveLogFile(QString)));
  connect(dirWatcher, SIGNAL(timeout()), this, SLOT(driveLogTimeout()));

  initialize();
}

void DiscAutoLoader::loadDisc()
{
  QLOG_DEBUG() << "Loading disc with autoloader";

  // 1. Ejects the DVD drive tray
  // 2. Commands autoloader to load the disc onto the tray
  // 3. Closes the DVD drive tray

  currentState = EJECT_EMPTY_TRAY;

  // Eject empty DVD drive tray
  ejectCdromDrive(cdromDrivePath);
}

void DiscAutoLoader::unloadDisc(bool rejectDisc)
{
  // 1. Ejects the DVD drive tray
  // 2. Commands autoloader to pick up disc from tray
  // 3. Closes the DVD drive tray
  // 4. Commands autoloader to release the DVD down the front or back slide
  this->rejectDisc = rejectDisc;

  currentState = EJECT_TRAY_WITH_DISC;

  // Eject DVD drive tray with disc
  ejectCdromDrive(cdromDrivePath);
}

void DiscAutoLoader::loadCdromDrive(QString drivePath)
{
  QLOG_DEBUG() << "Load CDROM tray: " << drivePath;

  QStringList arguments;
  arguments << "-t" << drivePath;
  ejectProc->start("eject", arguments);
}

void DiscAutoLoader::ejectCdromDrive(QString drivePath)
{
  QLOG_DEBUG() << "Eject CDROM tray: " << drivePath;

  QStringList arguments;
  arguments << drivePath;
  ejectProc->start("eject", arguments);
}

void DiscAutoLoader::evaluateState(int exitCode)
{
  switch (currentState)
  {
    case IDLE:
      QLOG_DEBUG() << "Current state: IDLE";
      break;

    case EJECT_EMPTY_TRAY:
      QLOG_DEBUG() << "Current state: EJECT_EMPTY_TRAY";

      currentState = LOAD_DISC_TO_TRAY;

      sendAutoloaderCmd(LOAD);
      break;

    case LOAD_DISC_TO_TRAY:
    {
      QLOG_DEBUG() << "Current state: LOAD_DISC_TO_TRAY";

      // Check response from autoloader
      QString response = FileHelper::readTextFile(autoloaderLogFile);

      if (response.contains("error", Qt::CaseInsensitive))
      {
        QLOG_DEBUG() << "Autoloader returned error in response file: " << response;

        // Autoloader encountered an error, emit error message
        // Do not continue the load disc procedure
        QString errorMessage = "Cannot load disc due to an unknown autoloader error.";

        if (response.contains("TOTRAY NOT OPEN", Qt::CaseInsensitive))
          errorMessage = "Cannot load disc because the DVD drive tray is not open. Try unplugging the USB cable from the back of the autoloader (blue cable) and plugging it back in. If that does not help then turn off/on the autoloader.";
        else if (response.contains("FROMTRAY NO DISC", Qt::CaseInsensitive))
          errorMessage = "Cannot load disc because the autoloader is empty. Make sure there are discs in the autoloader and that they are laying flat. Sometimes it helps to gently push down on the stack of discs.";
        else if (response.contains("TOTRAY HAS DISC", Qt::CaseInsensitive))
          errorMessage = "Cannot load disc because there is already a disc in the DVD drive tray. Remove any discs from the autoloader and carefully remove the disk from the DVD drive tray. It helps to lift the disc out at an angle. Make sure to stack the discs in the correct order when returning them to the autoloader.";

        emit finishedLoadingDisc(false, errorMessage);
      }
      else
      {
        QLOG_DEBUG() << "Autoloader successfully loaded disc into tray";

        // Delete drive status log file before closing drive tray
        // The file will be created when udev detects the drive state change
        if (!QFile::remove(driveStatusLogFile))
        {
          QLOG_DEBUG() << "Could not delete file: " << driveStatusLogFile;
        }

        currentState = CLOSE_TRAY_WITH_DISC;

        loadCdromDrive(cdromDrivePath);
      }
    }
      break;

    case CLOSE_TRAY_WITH_DISC:
      QLOG_DEBUG() << "Current state: CLOSE_TRAY_WITH_DISC";

      dirWatcher->startWatching();

      break;

    case COPYING_DISC:
      QLOG_DEBUG() << "Current state: COPYING_DISC";
      break;

    case EJECT_TRAY_WITH_DISC:
      QLOG_DEBUG() << "Current state: EJECT_TRAY_WITH_DISC";

      currentState = PICK_DISC_FROM_TRAY;

      sendAutoloaderCmd(PICK);
      break;

    case PICK_DISC_FROM_TRAY:
    {
      QLOG_DEBUG() << "Current state: PICK_DISC_FROM_TRAY";

      // Check response from autoloader
      QString response = FileHelper::readTextFile(autoloaderLogFile);

      if (response.contains("error", Qt::CaseInsensitive))
      {
        QLOG_DEBUG() << "Autoloader returned error in response file: " << response;

        // Autoloader encountered an error, emit error message
        // Do not continue the unload disc procedure
        QString errorMessage = "Cannot remove disc because of an unknown autoloader error.";

        if (response.contains("FROMTRAY NOT OPEN", Qt::CaseInsensitive))
          errorMessage = "Cannot remove disc because the DVD drive tray is not open. Try unplugging the USB cable from the back of the autoloader (blue cable) and plugging it back in. If that does not help then turn off/on the autoloader.";
        if (response.contains("FROMTRAY NO DISC", Qt::CaseInsensitive))
          errorMessage = "Cannot remove disc because the DVD drive tray is empty.";
        else if (response.contains("Unknown", Qt::CaseInsensitive))
          errorMessage = "The autoloader is in an unexpected state, cannot remove the disc.";

        emit finishedUnloadingDisc(false, errorMessage);
      }
      else
      {
        QLOG_DEBUG() << "Autoloader successfully picked disc from tray";

        currentState = CLOSE_EMPTY_TRAY;

        // Close empty DVD drive tray
        loadCdromDrive(cdromDrivePath);
      }
    }
      break;

    case CLOSE_EMPTY_TRAY:
      QLOG_DEBUG() << "Current state: CLOSE_EMPTY_TRAY";

      if (rejectDisc)
      {
        currentState = REJECT_DISC;
        sendAutoloaderCmd(REJECT);
      }
      else
      {
        currentState = UNLOAD_DISC;
        sendAutoloaderCmd(UNLOAD);
      }
      break;

    case UNLOAD_DISC:
    {
      QLOG_DEBUG() << "Current state: UNLOAD_DISC";

      // Check response from autoloader
      QString response = FileHelper::readTextFile(autoloaderLogFile);

      if (response.contains("error", Qt::CaseInsensitive))
      {
        QLOG_DEBUG() << "Autoloader returned error in response file: " << response;

        // Autoloader encountered an error, emit error message
        // Do not continue the unload disc procedure
        QString errorMessage = "Cannot unload disc because of an unknown autoloader error.";

        if (response.contains("FROMTRAY NOT CLOSED", Qt::CaseInsensitive))
          errorMessage = "Cannot unload the disc because the DVD drive tray is not closed. Try unplugging the USB cable from the back of the autoloader (blue cable) and plugging it back in. If that does not help then turn off/on the autoloader.";
        else if (response.contains("Unknown", Qt::CaseInsensitive))
          errorMessage = "The autoloader is in an unexpected state, cannot unload the disc.";

        emit finishedUnloadingDisc(false, errorMessage);
      }
      else
      {
        QLOG_DEBUG() << "Autoloader successfully unloaded disc";

        emit finishedUnloadingDisc(true);
      }
    }
      break;

    case REJECT_DISC:
    {
      QLOG_DEBUG() << "Current state: REJECT_DISC";

      // Check response from autoloader
      QString response = FileHelper::readTextFile(autoloaderLogFile);

      if (response.contains("error", Qt::CaseInsensitive))
      {
        QLOG_DEBUG() << "Autoloader returned error in response file: " << response;

        // Autoloader encountered an error, emit error message
        // Do not continue the unload disc procedure
        QString errorMessage = "Cannot reject disc because of an unknown autoloader error.";

        if (response.contains("FROMTRAY NOT CLOSED", Qt::CaseInsensitive))
          errorMessage = "Cannot reject the disc because the DVD drive tray is not closed. Try unplugging the USB cable from the back of the autoloader (blue cable) and plugging it back in. If that does not help then turn off/on the autoloader.";
        else if (response.contains("Unknown", Qt::CaseInsensitive))
          errorMessage = "The autoloader is in an unexpected state, cannot reject the disc.";

        emit finishedUnloadingDisc(false, errorMessage);
      }
      else
      {
        QLOG_DEBUG() << "Autoloader successfully rejected disc";

        emit finishedUnloadingDisc(true);
      }
    }
      break;
  }
}

void DiscAutoLoader::foundDriveLogFile(QString logfile)
{
  dirWatcher->stopWatching();

  QString response = FileHelper::readTextFile(logfile);

  // Doesn't really matter what is in the file, changed so the file is only created with a disc is in the drive
  if (response.contains("disc", Qt::CaseInsensitive))
    emit finishedLoadingDisc(true);
  else    
    emit finishedLoadingDisc(false, "Remove the discs from the autoloader that were not copied and stack them in the autoloader in the correct order. Start the copy job when you are ready to try again.");
}

void DiscAutoLoader::driveLogTimeout()
{
  dirWatcher->stopWatching();

  QLOG_DEBUG() << "Timed out waiting for DVD drive to detect the presence of the disc";

  emit finishedLoadingDisc(false, "Timed out waiting for DVD drive to detect the presence of the disc. Remove the discs from the autoloader that were not copied and stack them in the autoloader in the correct order. Start the copy job when you are ready to try again.");
}

void DiscAutoLoader::sendAutoloaderCmd(COMMAND cmd)
{
  QStringList arguments;

  switch (cmd)
  {
    case INIT:
      QLOG_DEBUG() << "Sending Autoloader command: INIT";
      arguments << "INIT";
      break;

    case LOAD:
      QLOG_DEBUG() << "Sending Autoloader command: LOAD";
      arguments << "LOAD";
      break;

    case PICK:
      QLOG_DEBUG() << "Sending Autoloader command: PICK";
      arguments << "PICK";
      break;

    case UNLOAD:
      QLOG_DEBUG() << "Sending Autoloader command: UNLOAD";
      arguments << "UNLOAD";
      break;

    case REJECT:
      QLOG_DEBUG() << "Sending Autoloader command: REJECT";
      arguments << "REJECT";
      break;

    default:
      break;
  }

  autoloaderProc->start(autoloaderProg, arguments);
}

QString DiscAutoLoader::getAutoloaderLogFile() const
{
  return autoloaderLogFile;
}

void DiscAutoLoader::setAutoloaderLogFile(const QString &value)
{
  autoloaderLogFile = value;
}

QString DiscAutoLoader::getAutoloaderProg() const
{
  return autoloaderProg;
}

void DiscAutoLoader::setAutoloaderProg(const QString &value)
{
  autoloaderProg = value;

  QFileInfo workingDir(autoloaderProg);
  autoloaderProc->setWorkingDirectory(workingDir.path());
}

QString DiscAutoLoader::getCdromDrivePath() const
{
  return cdromDrivePath;
}

void DiscAutoLoader::setCdromDrivePath(const QString &value)
{
  cdromDrivePath = value;
}


void DiscAutoLoader::initialize()
{
  QLOG_DEBUG() << "Initializing autoloader";

  currentState = IDLE;

  sendAutoloaderCmd(INIT);
}
