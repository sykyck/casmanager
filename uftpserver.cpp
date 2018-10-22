#include "uftpserver.h"
#include "qslog/QsLog.h"
#include "filehelper.h"
#include "global.h"
#include <QFile>
#include <QHostAddress>
#include <QFileInfo>
#include <QDir>

UftpServer::UftpServer(QString uftpProg, QString fileServerLogFile, QString statusFile, QString listFile,  QObject *parent) : QObject(parent)
{
  this->uftpProg = uftpProg;
  this->uftpLogFile = fileServerLogFile;
  sessionGroupID.clear();
  restartFile.clear();

  this->statusFile = statusFile;
  this->listFile = listFile;

  uftpProc = new QProcess;

  // Get the path to the uftp program to use for the working directory
  QFileInfo f(uftpProg);
  uftpProc->setWorkingDirectory(f.absolutePath());

  connect(uftpProc, SIGNAL(started()), this, SLOT(uftpProcStarted()));
  connect(uftpProc, SIGNAL(error(QProcess::ProcessError)), this, SLOT(uftpProcError(QProcess::ProcessError)));
  connect(uftpProc, SIGNAL(finished(int)), this, SLOT(uftpProcFinished(int)));
}

UftpServer::~UftpServer()
{
  uftpProc->deleteLater();
}

bool UftpServer::isRunning()
{
  if (uftpProc->state() != QProcess::NotRunning)
    return true;
  else
    return false;
}

// If restartFile is not specified then a new session is assumed otherwise the restart file is used
// to resume the last session
void UftpServer::start(int transmissionSpeed, QStringList dirList, bool useRestartFile)
{
  // Stop uftp if it is currently running
  this->stop();

  // Delete the previous log file if it exists, otherwise it just gets concatenated to the existing
  if (QFile::exists(uftpLogFile))
  {
    QLOG_DEBUG() << "Deleting previous log file:" << uftpLogFile;

    if (!QFile::remove(uftpLogFile))
      QLOG_ERROR() << "Could not delete previous log file:" << uftpLogFile;
  }

  // Delete the previous status file if it exists, otherwise it just gets concatenated to the existing
  if (QFile::exists(statusFile))
  {
    QLOG_DEBUG() << "Deleting previous status file:" << statusFile;

    if (!QFile::remove(statusFile))
      QLOG_ERROR() << "Could not delete previous status file:" << statusFile;
  }

  if (!useRestartFile)
  {
    // Create file with the list of directories, one per line, this is needed instead of passing everything on the command line which could get truncated if too long
    if (FileHelper::writeTextFile(listFile, dirList.join("\n")))
    {
      QLOG_DEBUG() << "Wrote list of directories to file for uftp:" << listFile;
    }
    else
    {
      QLOG_ERROR() << "Could not write list of directories to file for uftp:" << listFile;

      // FIXME: Need to handle error if conf file not written for uftp
    }

    // Delete old "/var/cas-mgr/_group_*_restart" files which are created any time uftp is canceled in addition to other instances
    QDir dir(uftpProc->workingDirectory(), "_group_*_restart", QDir::Name | QDir::IgnoreCase, QDir::Files);

    foreach (QString restartFile, dir.entryList())
    {
      QLOG_DEBUG() << QString("Deleting old restart file: %1").arg(restartFile);
      if (!dir.remove(restartFile))
        QLOG_ERROR() << QString("Could not delete file: %1").arg(restartFile);
    }
  }

  // -R = transmission speed in kbps
  // -L = log file location
  // -S = status file location
  // -f = Restartable flag. If specified, and at least one client fails to receive all files,
  //      the server will write a restart file named "_group_{group ID}_restart" in the current directory
  //      to save the current state, which includes the group ID, list of files, and list  of failed clients.
  //      This file can then be passed to -F to restart the failed transfer.
  // -F = Specifies the name of a restart file to use to resume a failed transfer. If specified, -H may not be
  //      specified and all files listed to send will be ignored, since the restart file contains both of these.
  //      All other command line options specified on the first attempt are not automatically applied, so you
  //      can alter then for the next attempt if need be.
  // -i = file containing list of files/directories to copy, one per line
  // -r = init_grtt[:min_grtt:max_grtt] Specifies the initial value, and optionally the min and max values, of the Group Round Trip Time (GRTT) used in timing calculations.
  QStringList arguments;

  arguments << "-R" << QString("%1").arg(transmissionSpeed) << "-L" << uftpLogFile << "-S" << statusFile;
  if (!useRestartFile)
    arguments << "-f" << "-i" << listFile;
  else
    arguments << "-F" << restartFile;

  // Testing out new setting do deal with timing issue where a client is not responding within 0.5 second to the server
  // if this happens the server assumes the client has disconnected and ignores future communication
  arguments << "-r" << "0.5:0.1:15.0";

  QLOG_DEBUG() << "Starting uftp:" << uftpProg << arguments;

  uftpProc->start(uftpProg, arguments);
}

void UftpServer::stop()
{
  QFileInfo uftpProgramName(uftpProg);

  // Kill uftp if it is currently running
  if (Global::isProcessRunning(uftpProgramName.fileName()))
  {
    QLOG_DEBUG() << QString("%1 is currently running, now killing").arg(uftpProgramName.fileName());

    if (Global::killProcess(uftpProgramName.fileName()))
      QLOG_DEBUG() << QString("%1 killed").arg(uftpProgramName.fileName());
    else
      QLOG_ERROR() << QString("%1 could not be killed").arg(uftpProgramName.fileName());
  }
  else
    QLOG_DEBUG() << QString("%1 is not currently running").arg(uftpProgramName.fileName());
}

void UftpServer::uftpProcStarted()
{
  emit startedCopying();
}


void UftpServer::uftpProcFinished(int exitCode)
{
  QLOG_DEBUG() << "uftp process finished with exit code: " << exitCode;

  // exitCode 0 (success) = The file transfer session finished with at least one client receiving at least one file.
  if (exitCode > 0)
  {
    switch (exitCode)
    {
      case 1:
        QLOG_ERROR() << "An invalid command line parameter was specified.";
        break;

      case 2:
        QLOG_ERROR() << "An error occurred while attempting to initialize network connections.";
        break;

      case 3:
        QLOG_ERROR() << "An error occurred while reading or generating cryptographic key data.";
        break;

      case 4:
        QLOG_ERROR() << "An error occurred while opening or rolling the log file.";
        break;

      case 5:
        QLOG_ERROR() << "A memory allocation error occurred.";
        break;

      case 6:
        QLOG_DEBUG() << "The server was interrupted by the user.";

        // Exit without emitting finishedCopying since this was the user that canceled the movie change process
        return;
        break;

      case 7:
        QLOG_ERROR() << "No client responded to the ANNOUNCE message.";
        break;

      case 8:
        QLOG_ERROR() << "No client responded to a FILEINFO message.";
        break;

      case 9:
        QLOG_ERROR() << "All client either dropped out of the session or aborted. Also returned if one client drops out or aborts when -q is specified.";
        break;

      case 10:
        QLOG_ERROR() << "The session completed, but none of the specified files were received by any client.";
        break;

      default:
        QLOG_ERROR() << "Cannot identify uftp exit code.";
        break;
    }

    emit finishedCopying(Exit_With_Error);
    return;
  }

  restartFile.clear();
  sessionGroupID.clear();

  // The uftp log and status file are required to determine the outcome of the session
  if (QFile::exists(uftpLogFile) && QFile::exists(statusFile))
  {
    // Determine the Group ID for the session, this can be used later if the session has to be resumed
    QString searchTerm("Group ID: ");
    QStringList results = Global::grep(searchTerm, uftpLogFile);

    if (results.count() > 0)
    {
      int index = results.first().indexOf(searchTerm);
      sessionGroupID = results.first().mid(index + searchTerm.count());

      QLOG_DEBUG() << "Found group ID:" << sessionGroupID << "in log file:" << uftpLogFile;
    }
    else
    {
      QLOG_ERROR() << "No Group ID found in log file:" << uftpLogFile;
    }

    // Get list of all clients in the status file
    QStringList clients = getSessionClientList();
    QStringList clientErrors;
    QStringList clientsFinished;

    foreach (QString client, clients)
    {
      // Client addresses are in hex format, convert to IPv4 dot notation
      QHostAddress clientAddress(client.toInt(0, 16));

      if (!isSuccessfulSession(client))
      {        
        clientErrors << clientAddress.toString();
      }
      else
      {
        clientsFinished << clientAddress.toString();
      }
    }

    if (clientErrors.isEmpty())
    {
      if (clientsFinished.count() > 0)
      {
        QLOG_DEBUG() << "uftp process successful according to status file:" << statusFile;

        // If a restart file existed for this session group id then delete now
        QString oldRestartFile = QString("%1/_group_%2_restart").arg(uftpProc->workingDirectory()).arg(sessionGroupID);
        if (QFile::exists(oldRestartFile))
        {
          if (QFile::remove(oldRestartFile))
            QLOG_DEBUG() << "Deleted restart file:" << oldRestartFile << "since it is no longer needed";
          else
            QLOG_ERROR() << "Could not delete restart file:" << oldRestartFile;
        }

        emit finishedCopying(Success, QString(), QStringList(), clientsFinished);
      }
      else
      {
        QLOG_ERROR() << "uftp did not return any error code and the there are no clients that failed but no clients succeeded either";

        emit finishedCopying(Unknown_State, "uftp log and status files exist but could not determine the results of any client.");
      }
    }
    else
    {
      QLOG_DEBUG() << "uftp process failed according to status file:" << statusFile;

      restartFile = QString("%1/_group_%2_restart").arg(uftpProc->workingDirectory()).arg(sessionGroupID);
      if (!QFile::exists(restartFile))
      {
        restartFile.clear();
        QLOG_ERROR() << "Could not find expected restart file:" << restartFile;
        emit finishedCopying(Fail_Cannot_Restart, QString("Failed to copy videos to %1 booth(s).").arg(clientErrors.count()), clientErrors, clientsFinished);
      }
      else
      {
        QLOG_DEBUG() << "Found restart file:" << restartFile;
        emit finishedCopying(Fail_Restart_Possible, QString("Failed to copy videos to %1 booth(s).").arg(clientErrors.count()), clientErrors, clientsFinished);
      }      
    }
  }
  else
  {
    QLOG_ERROR() << "Could not find the log file:" << uftpLogFile << "and/or status file:" << statusFile;

    emit finishedCopying(Unknown_State, "Could not determine if all files were copied successfully because log files are missing.");
  }
}

void UftpServer::uftpProcError(QProcess::ProcessError errorCode)
{
  // This is only called if there was some problem executing the
  // program, it is not an error code returned from the process itself
  QLOG_ERROR() << QString("Error code returned when trying to start process: %1").arg(errorCode);

  emit processError();
}

QStringList UftpServer::getSessionClientList()
{
  QStringList results = Global::grep("CONNECT;", statusFile);

  QStringList clientList;
  foreach (QString line, results)
  {
    QStringList fields = line.split(";");

    if (fields.count() >= 3)
      clientList << fields[2].trimmed();
  }

  return clientList;
}

bool UftpServer::isSuccessfulSession(QString clientID)
{
  // Get count of files that were to be copied by counting number of RESULT entries
  // for this particular clientID
  QStringList results = Global::grep(QString("RESULT;%1;").arg(clientID), statusFile);

  int totalFiles = results.count();
  QStringList errorList;

  foreach (QString line, results)
  {
    QStringList fields = line.split(";");

    // Check status of copying each file
    if (fields.count() >= 6)
    {
      if (fields[4] != "copy")
        errorList << line;
    }
  }

  // Get session stats for clientID
  results = Global::grep(QString("STATS;%1;").arg(clientID), statusFile);

  if (results.count() > 0)
  {
    QString line = results.first();
    QStringList fields = line.split(";");

    // Get count of number of files actually copied
    if (fields.count() >= 8)
    {
      if (fields[2].toInt() != totalFiles)
        errorList << line;
    }
  }

  QLOG_DEBUG() << errorList;

  return errorList.isEmpty();
}
