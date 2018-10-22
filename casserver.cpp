#include <QTextStream>
#include <QCoreApplication>
#include <QHostAddress>
#include <QFileInfo>
#include <QtNetwork>
#include <QDebug>

#include "casserver.h"
#include "global.h"
#include "qslog/QsLog.h"
#include "qjson/serializer.h"
#include "qjson/parser.h"

const int CONNECTION_TIMEOUT = 15000;

CasServer::CasServer(DatabaseMgr *dbMgr, Settings *settings, QObject *parent) : QObject(parent)
{
  this->dbMgr = dbMgr;
  this->settings = settings;

  connect(&qfwScanDevices, SIGNAL(finished()), this, SIGNAL(scanDevicesFinished()));
  connect(&qfwCheckDevices, SIGNAL(finished()), this, SIGNAL(checkDevicesFinished()));
  connect(&qfwCollectViewTimes, SIGNAL(finished()), this, SIGNAL(viewTimeCollectionFinished()));
  connect(&qfwGetBoothDatabases, SIGNAL(finished()), this, SIGNAL(getBoothDatabasesFinished()));
  connect(&qfwClearViewTimes, SIGNAL(finished()), this, SIGNAL(clearViewTimesFinished()));
  connect(&qfwMovieChangePrepare, SIGNAL(finished()), this, SIGNAL(movieChangePrepareFinished()));
  connect(&qfwMovieChangeClients, SIGNAL(finished()), this, SIGNAL(movieChangeClientsFinished()));
  connect(&qfwMovieChangeRetry, SIGNAL(finished()), this, SIGNAL(movieChangeRetryFinished()));
  connect(&qfwMovieChangeCancel, SIGNAL(finished()), this, SIGNAL(movieChangeCancelFinished()));
  connect(&qfwUpdateSettings, SIGNAL(finished()), this, SIGNAL(updateSettingsFinished()));
  connect(&qfwRestartDevices, SIGNAL(finished()), this, SIGNAL(restartDevicesFinished()));
  connect(&qfwCollection, SIGNAL(finished()), this, SIGNAL(collectionFinished()));
  connect(&qfwToggleAlarmState, SIGNAL(finished()), this, SIGNAL(toggleAlarmFinished()));
  connect(&qfwCollectDailyViewTimes, SIGNAL(finished()), this, SLOT(sendDailyViewTimesToServer()));
  connect(&qfwClearClerkstations, SIGNAL(finished()), this, SIGNAL(clearClerkstationsFinished()));


  netReplySendViews = 0;
  netReplySendCollection = 0;
  netReplyUserAuth = 0;
  netMgr = new QNetworkAccessManager;

  connectionTimer = new QTimer;
  connectionTimer->setInterval(CONNECTION_TIMEOUT);
  connectionTimer->setSingleShot(true);
  numRetries = 0;
}

CasServer::~CasServer()
{
  netMgr->deleteLater();
  connectionTimer->deleteLater();

  if (netReplySendViews)
    netReplySendViews->deleteLater();
}

/*
void CasServer::parseCommandline(QStringList arguments)
{
  if (arguments.count() >= 2 && arguments.count() <= 4)
  {
    if (arguments.contains("-s"))
    {
      scanDevices();
    }
    else if (arguments.contains("-vt"))
    {
      viewTimeCollection();
    }
    else if (arguments.contains("-svt"))
    {
      sendViewTimeCollection();
    }
    else if (arguments.contains("-cv"))
    {
      clearViewTimes();
    }
    else if (arguments.contains("-d"))
    {
      checkDevices();
    }
    else if (arguments.contains("-rv"))
    {
      reloadVideos();
    }
    else if (arguments.contains("-u"))
    {
      updateSettings();
    }
    else if (arguments.contains("-sd"))
    {
      sendDatabase();
    }
    else if (arguments.contains("-r"))
    {
      restartDevice();
    }
    else if (arguments.contains("-mc"))
    {
      int indexArgs = arguments.indexOf("-mc");
      if (indexArgs > 0 && indexArgs < arguments.count() - 1)
      {
        QString weekNum = arguments.at(indexArgs + 1);

        if (weekNum.toInt() > 0 && weekNum.toInt() < 54)
        {
          movieChange(weekNum.toInt());
        }
        else
        {
          //cout << "Week number must be 1 - 53.\n";
          startUpdateCheck();
        }
      }
      else
      {
        //cout << "Specify week number.\n";
        startUpdateCheck();
      }
    }
    else if (arguments.contains("-videodir"))
    {
      int indexDir = arguments.indexOf("-videodir");
      if (indexDir > 0 && indexDir < arguments.count() - 1)
      {
        QDir testDir(arguments.at(indexDir + 1));
        if (testDir.exists())
        {
          setDownloadsParentDir(testDir.absolutePath());
          //cout << "Video directory set to: " << testDir.absolutePath() << "\n";
          QLOG_DEBUG() << QString("Video directory set to: %1").arg(testDir.absolutePath()));
        }
        else
        {
          //cout << "Directory does not exist: " << testDir.absolutePath() << "\n";
        }
      }
      else
        //cout << "Specify a video directory.\n";

      startUpdateCheck();
    }
    else if (arguments.contains("-key"))
    {
      int indexKey = arguments.indexOf("-key");
      if (indexKey > 0 && indexKey < arguments.count() - 1)
      {
        setPassphrase(arguments.at(indexKey + 1));
        //cout << "Passphrase set to: " << arguments.at(indexKey + 1) << "\n";
        QLOG_DEBUG() << QString("Passphrase set to: %1").arg(arguments.at(indexKey + 1)));
      }
      else
        //cout << "Specify a key.\n";

      startUpdateCheck();
    }
    else if (arguments.contains("-iprange"))
    {
      int indexIP = arguments.indexOf("-iprange");
      if (indexIP > 0 && indexIP < arguments.count() - 2)
      {
        QString firstIpAddress = arguments.at(indexIP + 1);
        QString lastIpAddress = arguments.at(indexIP + 2);

        QHostAddress testFirstAddress;
        QHostAddress testLastAddress;

        if (testFirstAddress.setAddress(firstIpAddress) &&
            testLastAddress.setAddress(lastIpAddress))
        {
          if (testFirstAddress.toIPv4Address() > testLastAddress.toIPv4Address())
          {
            firstIpAddress = testLastAddress.toString();
            lastIpAddress = testFirstAddress.toString();
          }

          setIpAddressRange(firstIpAddress, lastIpAddress);
          //cout << "IP range set to: " << firstIpAddress << " - " << lastIpAddress << "\n";
          QLOG_DEBUG() << QString("IP range set to: %1 - %2")
                      .arg(firstIpAddress)
                      .arg(lastIpAddress));
        }
        else
          //cout << "Specify valid IP addresses such as: 10.0.0.21 10.0.0.30.\n";

      }
      else
        //cout << "Specify beginning and ending IP addresses for the range.\n";

      startUpdateCheck();
    }
    else
    {
      showUsage();
      startUpdateCheck();
    }

  }
  else
  {
    showUsage();
    startUpdateCheck();
  }
}
*/

void CasServer::startScanDevices()
{
  QLOG_DEBUG() << QString("Starting scan devices thread");
  qfwScanDevices.setFuture(QtConcurrent::run(this, &CasServer::scanDevices));
}

void CasServer::scanDevices()
{
  QStringList foundDevicesList;

  QString subnet = settings->getValue(ARCADE_SUBNET).toString();
  int currentAddress = settings->getValue(MIN_ADDRESS).toString().mid(settings->getValue(MIN_ADDRESS).toString().lastIndexOf(".") + 1).toInt();
  int numLastAddress = settings->getValue(MAX_ADDRESS).toString().mid(settings->getValue(MAX_ADDRESS).toString().lastIndexOf(".") + 1).toInt();

  // Contact each IP address in the range
  while (currentAddress <= numLastAddress)
  {
    QString ipAddress = subnet + QString::number(currentAddress);

    QLOG_DEBUG() << QString("Contacting %1...").arg(ipAddress);
    emit scanningDevice(ipAddress);

    if (sendMessage(COMMAND_PING, ipAddress, TCP_PORT))
    {
      foundDevicesList.append(ipAddress);
      emit scanDeviceSuccess(ipAddress);
    }
    else
      emit scanDeviceFailed(ipAddress);

    ++currentAddress;
  }

  if (foundDevicesList.count() > 0)
  {
    QLOG_DEBUG() << QString("Found %1 device(s)").arg(foundDevicesList.count());
    settings->setValue(DEVICE_LIST, foundDevicesList);
  }
  else
  {
    QLOG_DEBUG() << QString("No devices found in the IP range %1 - %2").arg(settings->getValue(MIN_ADDRESS).toString()).arg(settings->getValue(MAX_ADDRESS).toString());
    settings->setValue(DEVICE_LIST, foundDevicesList);
  }

  // Save settings
  dbMgr->setValue("all_settings", settings->getSettings());
}

void CasServer::startCheckDevices()
{
  QLOG_DEBUG() << QString("Starting check devices thread");
  qfwCheckDevices.setFuture(QtConcurrent::run(this, &CasServer::checkDevices));
}

void CasServer::checkDevices()
{
  if (settings->getValue(DEVICE_LIST).toStringList().count() > 0)
  {
    QStringList noReplyList;

    quint8 returnCode = 0;

    // Contact each IP address that was previously identified as running the CASplayer
    foreach (QString ipAddress, settings->getValue(DEVICE_LIST).toStringList())
    {
      QLOG_DEBUG() << QString("Contacting %1...").arg(ipAddress);

      if (!sendMessage(COMMAND_PING, ipAddress, TCP_PORT, &returnCode))
      {
        noReplyList.append(ipAddress);
        emit checkDeviceFailed(ipAddress);
      }
      else
      {
        emit checkDeviceSuccess(ipAddress, (returnCode == ERROR_CODE_IN_SESSION_STATE));
      }
    }

    if (noReplyList.count() > 0)
    {
      QLOG_DEBUG() << QString("%1 out of %2 device(s) did not respond").arg(noReplyList.count()).arg(settings->getValue(DEVICE_LIST).toStringList().count());
    }
    else
    {
      QLOG_DEBUG() << QString("All %1 devices responded").arg(settings->getValue(DEVICE_LIST).toStringList().count());
    }
  }
  else
    QLOG_ERROR() << "No devices are defined";
}

void CasServer::reloadVideos()
{
  if (settings->getValue(DEVICE_LIST).toStringList().count() > 0)
  {
    QStringList errorList;
    QStringList reloadedList;

    foreach (QString ipAddress, settings->getValue(DEVICE_LIST).toStringList())
    {
      //cout << "Requesting videos to be reloaded at: " << ipAddress << " ...\n";
      //cout.flush();

      if (sendMessage(COMMAND_RELOAD_VIDEOS, ipAddress, TCP_PORT))
        reloadedList.append(ipAddress);
      else
        errorList.append(ipAddress);

      //cout.flush();
    }

    if (errorList.count() > 0)
    {
      QLOG_DEBUG() << QString("Reloaded videos in %1 out of %2 device(s)")
                      .arg(reloadedList.count())
                      .arg(settings->getValue(DEVICE_LIST).toStringList().count());

      //cout << "Reloaded videos in " << reloadedList.count() << " out of " << settings->getValue(DEVICE_LIST).toStringList().count() << " device(s). The following failed:\n";
      for (int i = 0; i < errorList.count(); ++i)
      {
        // Display 4 IP addresses per line
        if (i > 0 && i % 4 == 0)
          ;//cout << "\n";
        //cout << errorList.at(i) << "\t";
      }
      //cout << "\n";
      //cout.flush();
    }
    else
    {
      QLOG_DEBUG() << QString("Reloaded videos in all %1 device(s)").arg(settings->getValue(DEVICE_LIST).toStringList().count());
      //cout << "Reloaded videos in all " << settings->getValue(DEVICE_LIST).toStringList().count() << " device(s)\n";
      //cout.flush();
    }
  }
  else
    QLOG_ERROR() << "No devices are defined";
}

void CasServer::startCollection()
{
  QLOG_DEBUG() << QString("Starting collection thread");
  qfwCollection.setFuture(QtConcurrent::run(this, &CasServer::collectDevices));
}

void CasServer::collectDevices()
{
  if (settings->getValue(DEVICE_LIST).toStringList().count() > 0)
  {
    QStringList errorList;
    QStringList collectionDeviceList;

    foreach (QString ipAddress, settings->getValue(DEVICE_LIST).toStringList())
    {
      QLOG_DEBUG() << QString("Collecting device %1").arg(ipAddress);

      if (sendMessage(COMMAND_COLLECT_BOOTH, ipAddress, TCP_PORT))
      {
        emit collectionSuccess(ipAddress);

        collectionDeviceList.append(ipAddress);
      }
      else
      {
        emit collectionFailed(ipAddress);

        errorList.append(ipAddress);
      }
    }

    if (errorList.count() > 0)
    {
      QLOG_DEBUG() << QString("Collected %1 out of %2 device(s)")
                      .arg(collectionDeviceList.count())
                      .arg(settings->getValue(DEVICE_LIST).toStringList().count());
    }
    else
    {
      QLOG_DEBUG() << QString("Collected all %1 device(s)").arg(settings->getValue(DEVICE_LIST).toStringList().count());
    }
  }
  else
    QLOG_ERROR() << "No devices are defined";
}

void CasServer::startToggleAlarmState(bool enableAlarm)
{
  QLOG_DEBUG() << QString("Starting alarm toggle thread");
  qfwToggleAlarmState.setFuture(QtConcurrent::run(this, &CasServer::toggleAlarmState, enableAlarm));
}

void CasServer::toggleAlarmState(bool enableAlarm)
{
  if (settings->getValue(DEVICE_LIST).toStringList().count() > 0)
  {
    QStringList errorList;
    QStringList alarmToggleList;

    foreach (QString ipAddress, settings->getValue(DEVICE_LIST).toStringList())
    {
      QLOG_DEBUG() << QString("Requesting device %1 to %2 alarm").arg(ipAddress).arg(enableAlarm ? "ENABLE" : "DISABLE");

      if (enableAlarm)
      {
        if (sendMessage(COMMAND_ENABLE_ALARM, ipAddress, TCP_PORT))
        {
          emit toggleAlarmSuccess(ipAddress);

          alarmToggleList.append(ipAddress);
        }
        else
        {
          emit toggleAlarmFailed(ipAddress);

          errorList.append(ipAddress);
        }
      }
      else
      {
        if (sendMessage(COMMAND_DISABLE_ALARM, ipAddress, TCP_PORT))
        {
          emit toggleAlarmSuccess(ipAddress);

          alarmToggleList.append(ipAddress);
        }
        else
        {
          emit toggleAlarmFailed(ipAddress);

          errorList.append(ipAddress);
        }
      }
    }

    if (errorList.count() > 0)
    {
      QLOG_DEBUG() << QString("Toggled alarm in %1 out of %2 device(s)")
                      .arg(alarmToggleList.count())
                      .arg(settings->getValue(DEVICE_LIST).toStringList().count());
    }
    else
    {
      QLOG_DEBUG() << QString("Toggled alarm in all %1 device(s)").arg(settings->getValue(DEVICE_LIST).toStringList().count());
    }
  }
  else
    QLOG_ERROR() << "No devices are defined";
}

void CasServer::startViewTimeCollection(QStringList ipAddressList)
{
  QLOG_DEBUG() << QString("Starting view time collection thread");
  qfwCollectViewTimes.setFuture(QtConcurrent::run(this, &CasServer::viewTimeCollection, ipAddressList));
}

void CasServer::viewTimeCollection(QStringList ipAddressList)
{
  if (ipAddressList.count() > 0)
  {
    int year = QDate::currentDate().year();
    int weekNum = QDate::currentDate().weekNumber();

    QStringList collectedList;
    QStringList failedList;

    foreach (QString ipAddress, ipAddressList)
    {
      QLOG_DEBUG() << QString("Collecting view times from: %1...").arg(ipAddress);

      // Each booth creates/updates its own viewtimes_<deviceNum>.sqlite database on the server
      if (sendMessage(COMMAND_VIEW_TIME_COLLECTION, ipAddress, TCP_PORT))
      {
        collectedList.append(ipAddress);
        emit collectedViewTimesSuccess(ipAddress);
      }
      else
      {
        failedList.append(ipAddress);
        emit collectedViewTimesFailed(ipAddress);
      }
    }

    if (collectedList.count() > 0)
    {
      // Combine viewtimes<deviceNum>.sqlite results into viewtimes.sqlite database
      // exclude any booths that failed to send view times
      foreach (QString ipAddress, ipAddressList)
      {
        if (!failedList.contains(ipAddress))
        {
          if (dbMgr->mergeViewTimes(year, weekNum, ipAddressToDeviceNum(ipAddress)))
            emit mergeViewTimesSuccess(ipAddress);
          else
            emit mergeViewTimesFailed(ipAddress);
        }
        else
        {
          // Skip merging view times from booths that failed the view time collection since the records in the booth's
          // viewtimes database could be from an older collection
          QLOG_DEBUG() << QString("Not merging view times from %1 since it failed view times collection").arg(ipAddress);
        }
      }

      QLOG_DEBUG() << QString("Creating Top 5 lists...");

      bool ok;
      QStringList top5CategoryList;
      top5CategoryList << "Straight" << "Gay";

      foreach (QString category, top5CategoryList)
      {
        QList<Movie> movieList = dbMgr->getViewTimesByCategory(category, year, weekNum, &ok);

        if (ok)
        {
          if (!dbMgr->deleteTop5Videos(category, year, weekNum))
          {
            QLOG_ERROR() << QString("Could not delete previous %1 videos from Top 5 table").arg(category);
          }

          // Get the last 5 videos in list and insert into table
          if (movieList.count() >= 5)
          {
            for (int i = 0; i < 5; ++i)
            {
              Movie m = movieList.last();

              //cout << "#" << (i + 1) << " - Time: " << m.currentPlayTime << ", Ch #: " << m.ChannelNum() << " - " << m.title << endl;
              //cout.flush();

              QLOG_DEBUG() << QString("#%1 - Time: %2, Ch #: %3 - %4")
                              .arg(i + 1)
                              .arg(m.CurrentPlayTime())
                              .arg(m.ChannelNum())
                              .arg(m.Title());

              if (!dbMgr->insertTop5Video(m.CurrentPlayTime(), m.ChannelNum(), m.Title(), m.Category(), year, weekNum))
              {
                QLOG_ERROR() << QString("Could not insert %1 video in Top 5 table").arg(category);
              }

              movieList.removeLast();
            }
          }
          else
          {
            QLOG_ERROR() << QString("Ran out of %1 videos to add to Top 5!!!").arg(category);
          }
        }
        else
        {
          QLOG_ERROR() << QString("Could not get a list of videos in %1 category").arg(category);
        }

      } // end foreach category
    }
    else
    {
      QLOG_DEBUG() << "View times were not collected from any device so Top 5 cannot be determined";
    }
  }
  else
  {
    QLOG_ERROR() << "Empty address list passed to viewTimeCollection";
  }
}

void CasServer::startClearViewTimes()
{
  QLOG_DEBUG() << QString("Starting clear view times thread");
  qfwClearViewTimes.setFuture(QtConcurrent::run(this, &CasServer::clearViewTimes));
}

void CasServer::clearViewTimes()
{
  // TODO: Clean up this code. This is a quick fix to deal with the view time collection crashing clerkstations. Clerkstations
  // should not have view times collected.
  QStringList onlyBoothAddresses;
  QVariantList boothList = dbMgr->getOnlyBoothInfo(getDeviceAddresses());

  foreach (QVariant v, boothList)
  {
    QVariantMap booth = v.toMap();
    onlyBoothAddresses.append(settings->getValue(ARCADE_SUBNET).toString() + booth["device_num"].toString());
  }

  if (onlyBoothAddresses.count() > 0)
  {
    int year = QDate::currentDate().year();
    int weekNum = QDate::currentDate().weekNumber();

    QStringList errorList;
    QStringList clearedList;

    quint8 returnCode = 0;

    foreach (QString ipAddress, onlyBoothAddresses)
    {
      QLOG_DEBUG() << QString("Clearing view times from: %1...").arg(ipAddress);

      if (sendMessage(COMMAND_CLEAR_VIEW_TIMES, ipAddress, TCP_PORT, &returnCode))
      {
        if (returnCode == ERROR_CODE_IN_SESSION)
          emit clearViewTimesInSession(ipAddress);
        else
          emit clearViewTimesSuccess(ipAddress);

        clearedList.append(ipAddress);
      }
      else
      {
        errorList.append(ipAddress);
        emit clearViewTimesFailed(ipAddress);
      }
    }

    if (errorList.count() > 0)
    {
      QLOG_DEBUG() << QString("Cleared view times from %1 out of %2 device(s)")
                      .arg(clearedList.count())
                      .arg(onlyBoothAddresses.count());
    }
    else
    {
      QLOG_DEBUG() << QString("Cleared view times from all %1 device(s)").arg(onlyBoothAddresses.count());
    }

    // Wait for booths to finish accessing viewtimes.sqlite database, without this pause, there are times when neither us or
    // the booths can use the viewtimes.sqlite file since we access it simultaneously
    sleep(5);

    // Combine viewtimes<deviceNum>.sqlite results into viewtimes.sqlite database
    foreach (QString address, onlyBoothAddresses)
    {
      dbMgr->mergeClearedViewTimes(year, weekNum, ipAddressToDeviceNum(address));
    }

    // Now that the view times are collected and cleared a movie change can take place
    //settings->setValue(ALLOW_MOVIE_CHANGE, true);

    // Save settings
    //dbMgr->setValue("all_settings", settings->getSettings());
  }
  else
    QLOG_ERROR() << "No devices are defined";
}

void CasServer::startMovieChangePrepare(MovieChangeInfo movieChangeInfo, QStringList boothAddresses, bool force, bool deleteOnly)
{
  QLOG_DEBUG() << QString("Starting movie change prepare thread");

  qfwMovieChangePrepare.setFuture(QtConcurrent::run(this, &CasServer::movieChangePrepare, movieChangeInfo, boothAddresses, force, deleteOnly));
}

void CasServer::movieChangePrepare(MovieChangeInfo movieChangeInfo, QStringList boothAddresses, bool force, bool deleteOnly)
{
  // TODO: Verify the data in mediaInfo.xml matches the files (jpgs and ts files)

  //QStringList boothAddresses = settings->getValue(DEVICE_LIST).toStringList();
  QStringList excludedAddresses;

  if (boothAddresses.count() > 0)
  {
    QLOG_DEBUG() << "Verifying all booths are online...";

    QStringList onlineDeviceList;
    QStringList offlineDeviceList;

    // Contact each IP address in the list
    foreach (QString ipAddress, boothAddresses)
    {
      QLOG_DEBUG() << QString("Contacting %1...").arg(ipAddress);
      bool contactedBooth = false;
      int numRetries = 0;

      // TODO: Move to settings
      while (!contactedBooth && numRetries++ < 2)
      {
        if (sendMessage(COMMAND_PING, ipAddress, TCP_PORT))
        {
          contactedBooth = true;
          continue;
        }
        else
        {
          QLOG_ERROR() << QString("Booth at %1 did not respond").arg(ipAddress);
        }

        QLOG_DEBUG() << QString("Waiting to try contacting %1 again...").arg(ipAddress);

        // TODO: Move to settings
        sleep(30);
      }

      if (contactedBooth)
        onlineDeviceList.append(ipAddress);
      else
        offlineDeviceList.append(ipAddress);

    } // endforeach ip address

    // if none of the booths responded then exit now
    if (onlineDeviceList.count() == 0)
    {
      QLOG_ERROR() << "None of the booths are online, cannot start movie change";
      emit movieChangePrepareError("None of the booths are online. If restarting the booths does not fix the problem then contact tech support.");
      return;
    }
    // else if all booths responded or the user is forcing a movie change
    else if (onlineDeviceList.count() == boothAddresses.count() || force)
    {
      bool ok = false;

      // Get count of the number of movies in the dvd_copy_jobs_history table which is how many
      // movies have been sent to the booths
      int numMovies = dbMgr->getNumMoviesSent(&ok);

      if (ok)
      {
        // if there are movies in the booths then check for differences otherwise skip this step
        if (numMovies > 0)
        {
          // See if there are any booths that are behind on movie changes (out of sync), these
          // will be excluded from the current movie change until they are caught up
          QStringList boothsOutOfSync = getBoothsOutOfSync(&ok);

          if (!ok)
          {
            QLOG_ERROR() << "Could not determine movie differences among booths";
            emit movieChangePrepareError("Failed to validate the current movies in the booths. Contact tech support.");
            return;
          }
          else
          {
            if (boothsOutOfSync.count() > 0)
            {
              if (boothsOutOfSync.count() == boothAddresses.count())
              {
                QLOG_ERROR() << QString("All booths are behind on movie changes");
                emit movieChangePrepareError("All booths are behind on movie changes, cannot start movie change. Contact tech support.");
                return;
              }
              else
              {
                if (!force)
                {
                  QLOG_DEBUG() << QString("Found booths that are behind on movie changes and user is not forcing the movie change: %1").arg(boothsOutOfSync.join(", "));

                  // Show user list of booths that don't have the latest movie changes and explain these will be excluded from the current movie change
                  emit movieChangePreparePossibleError(QString("The following booths are behind on movie changes: %1 but are automatically catching up on past movie changes. Do you still want to perform a movie change?").arg(boothsOutOfSync.join(", ")));
                  return;
                }
                else
                {
                  QLOG_DEBUG() << QString("Excluding the following booths from movie change since they are behind on movie changes: %1").arg(boothsOutOfSync.join(", "));

                  QStringList deleteDevices;

                  // Remove booths from the list that are out of sync, these won't be contacted to perform a movie change
                  // Build list of device addresses (convert from IP address) to delete from temp view times
                  foreach (QString ipAddress, boothsOutOfSync)
                  {
                    boothAddresses.removeOne(ipAddress);
                    excludedAddresses.append(ipAddress);

                    deleteDevices.append(this->ipAddressToDeviceNum(ipAddress));
                  }
                }
              }

            } // endif any booths out of sync
            else
            {
              // no differences
              QLOG_DEBUG() << "All booths have the same movies";
            }
          }

        } // endif there are movies in the booths

        // If any booths are not online then exclude these as well
        foreach (QString ipAddress, offlineDeviceList)
        {
          boothAddresses.removeOne(ipAddress);
          excludedAddresses.append(ipAddress);
        }

        // If there are no more booths left in the list then abort the operation
        if (boothAddresses.count() == 0)
        {
          QLOG_DEBUG() << "After removing booths that were behind on movie changes and/or offline, there are no booths left to perform movie change";
          emit movieChangePrepareError("The movie change cannot start because of a combination of booths being offline and behind on movie changes. Contact tech support.");
          return;
        }

      } // endif query successful
      else
      {
        QLOG_ERROR() << "Could not get count of movies in the booths";
        emit movieChangePrepareError("Could not get a count of movies in the booths. Contact tech support.");
        return;
      }

      // Continue if not just deleting movies
      if (!deleteOnly)
      {
        // Scheduled date is current time - time adjustment (default 5 minutes) so uftpd in the booths start before the server
        // TODO: Since we don't schedule future movie changes, this is just confusing. We really don't need a schedule date, just
        // need to tell booths to start uftpd
        QDateTime scheduledDate = QDateTime::currentDateTime().addSecs(-settings->getValue(MOVIE_CHANGE_SCHEDULE_TIME_ADJUST).toInt());

        // Get the current highest channel number in booths based on videos table
        int highestChannelNum = dbMgr->getHighestChannelNum(&ok);

        if (!ok)
        {
          QLOG_ERROR() << "Could not determine highest channel number";
          emit movieChangePrepareError(QString("Could not determine highest channel number. Contact tech support."));
          return;
        }
        else
          QLOG_DEBUG() << QString("Highest channel number: %1").arg(highestChannelNum);

        // get least amount of free disk space among the booths
        qreal lowestFreeDiskSpace = dbMgr->getLowestFreeDiskSpace(ipAddressesToDeviceNums(boothAddresses), &ok);

        // Do not continue if error or zero returned for lowest disk space
        if (!ok || lowestFreeDiskSpace == 0)
        {
          QLOG_ERROR() << "Could not determine lowest free disk space";
          emit movieChangePrepareError(QString("Could not determine the available disk space in the booths. Contact tech support."));
          return;
        }
        else
          QLOG_DEBUG() << QString("Lowest free disk space: %1 GB").arg(lowestFreeDiskSpace);

        // FIXME: Is this really what we should be doing? There is the possibility this doesn't match the dvdcopytasks of the selected week/year
        // load list of downloaded movies from the download directory
        QList<Movie> movieDownloads = getMovieDownloads(movieChangeInfo.VideoPath());

        // Cannot continue if no movies found
        if (movieDownloads.count() == 0)
        {
          QLOG_ERROR() << QString("No movies found. Movie change cannot continue.");
          emit movieChangePrepareError(QString("No movies found. Contact tech support."));
          return;
        }

        // Amount of disk space in GB that the movie change set needs
        qreal reqDiskSpace = 0;

        // Make sure all movies are valid and add up the required disk space
        int numInvalidMovies = 0;
        foreach (Movie m, movieDownloads)
        {
          if (!m.isValid())
          {
            QLOG_ERROR() << QString("Invalid: %1 - %2").arg(m.Dirname()).arg(m.toString());
            ++numInvalidMovies;
          }
          else
          {
            // Convert bytes to gigabytes
            reqDiskSpace += ((qreal)m.FileLength() / (1024 * 1024 * 1024));
          }
        }

        if (numInvalidMovies > 0)
        {
          QLOG_ERROR() << QString("%1 out of %2 movies are invalid. Movie change cannot continue.").arg(numInvalidMovies).arg(movieDownloads.count());
          emit movieChangePrepareError(QString("%1 out of %2 movies are invalid. Contact tech support.").arg(numInvalidMovies).arg(movieDownloads.count()));
          return;
        }

        // Verify there are no duplicate UPCs among the downloads
        QStringList duplicateUPCs;
        foreach (Movie m, movieDownloads)
        {
          if (movieDownloads.indexOf(m) != movieDownloads.lastIndexOf(m))
          {
            QLOG_ERROR() << QString("UPC: %1 found more than once.").arg(m.UPC());
            duplicateUPCs.append(m.UPC());
          }
        }

        if (duplicateUPCs.count() > 0)
        {
          QLOG_ERROR() << QString("Duplicate UPCs found among the movie change set: %1. Movie change cannot continue.").arg(duplicateUPCs.join(", "));
          emit movieChangePrepareError(QString("Duplicate UPCs found among the movie change set: %1. Contact tech support.").arg(duplicateUPCs.join(", ")));
          return;
        }

        QList<Movie> duplicateMovies;

        // Verify the downloaded UPCs don't exist in the current videos table
        QLOG_DEBUG() << "Movie downloads found:";
        foreach (Movie m, movieDownloads)
        {
          QLOG_DEBUG() << QString("%1, Title: %2, Category: %3, Genre: %4")
                          .arg(m.Dirname())
                          .arg(m.Title())
                          .arg(m.Category())
                          .arg(m.Subcategory());

          if (dbMgr->upcExists(m.UPC(), &ok))
          {
            QLOG_ERROR() << QString("UPC: %1 already exists in the booths")
                            .arg(m.UPC());

            duplicateMovies.append(m);
          }
          else
            QLOG_DEBUG() << QString("Verified UPC: %1 does not exist in the booths")
                            .arg(m.UPC());
        }

        if (duplicateMovies.count() > 0)
        {
          QLOG_DEBUG() << QString("%1 out of %2 movies are duplicate UPC(s) that already exist in the arcade.").arg(duplicateMovies.count()).arg(movieDownloads.count());

          if (duplicateMovies.count() < movieDownloads.count())
          {
            if (!force)
            {
              emit movieChangePreparePossibleError(QString("%1 out of %2 movies are duplicate UPC(s) that already exist in the arcade. Any duplicates will be excluded from the movie change. Do you still want to continue?").arg(duplicateMovies.count()).arg(movieDownloads.count()));
              return;
            }
            else
            {
              // Remove duplicates from list
              foreach (Movie m, duplicateMovies)
              {
                movieDownloads.removeOne(m);
              }
            }
          }
          else
          {
            QLOG_ERROR() << QString("All UPCs in the movie change set already exist in the arcade.");
            emit movieChangePrepareError(QString("All UPCs in the movie change set already exist in the arcade. Contact tech support."));
            return;
          }
        }
        else
        {
          QLOG_DEBUG() << "No duplicate UPCs found.";
        }

        if (dbMgr->deleteMovieChanges(movieChangeInfo.Year(), movieChangeInfo.WeekNum()))
          QLOG_DEBUG() << QString("Cleared previous movie changes from table for week #: %1, year: %2").arg(movieChangeInfo.WeekNum()).arg(movieChangeInfo.Year());
        else
        {
          QLOG_ERROR() << QString("Could not clear previous movie changes from table for week #: %1, year: %2. Movie change cannot continue.").arg(movieChangeInfo.WeekNum()).arg(movieChangeInfo.Year());
          emit movieChangePrepareError(QString("Could not clear previous movie changes from database. Contact tech support."));
          return;
        }

        // Subtract the required disk space from the free disk space to determine how
        // much free disk space will be available after the videos are copied to the booths
        lowestFreeDiskSpace -= reqDiskSpace;

        QLOG_DEBUG() << QString("The movie change set requires %1 GB of disk space, after the movie change booths will have %2 GB of free space remaining")
                        .arg(reqDiskSpace)
                        .arg(lowestFreeDiskSpace);

        int newHighestChannelNum = highestChannelNum + movieDownloads.count();

        // if free disk space < min free disk space
        if (lowestFreeDiskSpace < settings->getValue(MIN_FREE_DISKSPACE).toReal())
        {
          QLOG_DEBUG() << "Need to make room for movie downloads";

          // Build dictionary of categories that make up the movie downloads with
          // the value being the number of occurrences
          // Example: We have 6 Straight, 3 Gay and 1 She-Male
          // Result: downloadCategoryCount["Straight"] => 6
          //         downloadCategoryCount["Gay"] => 3
          //         downloadCategoryCount["She-Male"] => 1
          QMap<QString, int> downloadCategoryCount;
          foreach (Movie m, movieDownloads)
          {
            if (downloadCategoryCount.contains(m.Category()))
              downloadCategoryCount[m.Category()]++;
            else
              downloadCategoryCount[m.Category()] = 1;
          }

          // Reverse the key/value pairs in the dictionary
          // so the number of occurrences is the key and the value is the category name
          // QMap class allows inserting the same key multiple times with insertMulti()
          // Example: We have 6 Straight, 3 Gay and 1 She-Male
          // Result: movieChangeCategoryOrder[6] => "Straight"
          //         movieChangeCategoryOrder[3] => "Gay"
          //         movieChangeCategoryOrder[1] => "She-Male"
          // Example: We have 4 Straight, 4 Gay and 2 She-Male
          // Result: movieChangeCategoryOrder[4] => {"Gay", "Straight"}
          //         movieChangeCategoryOrder[1] => "She-Male"
          QMap<int, QString> movieChangeCategoryOrder;
          QMapIterator<QString, int> i(downloadCategoryCount);
          while (i.hasNext())
          {
            i.next();
            QString category = i.key();
            int count = i.value();

            movieChangeCategoryOrder.insertMulti(count, category);
          }

          // Build list of categories in the order they should be changed in the database.
          // The categories with the highest number of movie downloads are changed first.
          // If 2 or more categories have the same number of movies then we add them to
          // the list in a round robin fashion.
          // keys() are returned in ascending order so the categories
          // with the smallest distribution are first. We want categories
          // with the largest first so all we'll insert each item at the
          // start of the list so it is a FILO
          // Example: We have 6 Straight, 3 Gay and 1 She-Male
          // Result: categoryChangeOrder: "Straight", "Straight", "Straight", "Straight", "Straight", "Straight",
          //                              "Gay", "Gay", "Gay", "She-Male"
          // Example: We have 4 Straight, 4 Gay and 2 She-Male
          // Result: categoryChangeOrder: "Gay", "Straight", "Gay", "Straight", "Gay", "Straight", "Gay", "Straight",
          //                              "She-Male", "She-Male"
          QList<QString> categoryChangeOrder;
          foreach (int key, movieChangeCategoryOrder.uniqueKeys())
          {
            QList<QString> categoryList = movieChangeCategoryOrder.values(key);

            //QLOG_DEBUG() << QString("Key: %1, No. Values: %2").arg(key).arg(categoryList.count());

            for (int i = 0; i < key; ++i)
            {
              foreach (QString categoryName, categoryList)
                categoryChangeOrder.insert(0, categoryName);
            }
          }

          QLOG_DEBUG() << "Category change order: ";
          foreach (QString cat, categoryChangeOrder)
          {
            QLOG_DEBUG() << cat;
          }

          QStringList availableCategories = dbMgr->getVideoCategories(&ok);
          if (ok)
          {
            // Build dictionary of current movies in each category. Movies are sorted by popularity lowest to highest
            // and date in, oldest to newest, we exclude movies that are less than 2 weeks old
            QMap<QString, QList<Movie> > viewTimesByCategory;
            foreach (QString category, availableCategories)
            {
              QList<Movie> movieList = dbMgr->getLeastViewedMovies(category, &ok);
              if (ok)
              {
                // Only add list to dictionary if it's not empty
                if (!movieList.isEmpty())
                {
                  viewTimesByCategory[category] = movieList;
                  QLOG_DEBUG() << QString("Category: %1, Movie Count: %2").arg(category).arg(viewTimesByCategory.value(category).count());
                }
              }
              else
              {
                QLOG_ERROR() << QString("Error while getting view times for category: %1. Cannot continue movie change.").arg(category);
                emit movieChangePrepareError(QString("Error while getting view times for category: %1. Contact tech support.").arg(category));
                return;
              }
            }

            // Build list of videos to delete from each booth until
            // enough disk space is freed or we run out of downloads
            // or we run out of videos in every category
            while (lowestFreeDiskSpace < settings->getValue(MIN_FREE_DISKSPACE).toReal() &&
                   !viewTimesByCategory.isEmpty() &&
                   !movieDownloads.isEmpty())
            {
              QString movieChangeCategory = categoryChangeOrder.first();
              categoryChangeOrder.removeFirst();

              // actualMovieChangeCategory may differ from movieChangeCategory after
              // we look for it in the viewTimesByCategory list
              QString actualMovieChangeCategory = movieChangeCategory;

              QList<Movie> movieList;
              if (!viewTimesByCategory.contains(movieChangeCategory))
              {
                QString largestCategoryName = findLargestVideoCategory(viewTimesByCategory);

                QLOG_DEBUG() << QString("View times does not include the category: %1 had to go with the largest category: %2").arg(movieChangeCategory).arg(largestCategoryName);

                actualMovieChangeCategory = largestCategoryName;

              } // endif view times does not contain category

              // Get the list of videos sorted by popularity and dateIn for selected category
              movieList = viewTimesByCategory[actualMovieChangeCategory];
              QLOG_DEBUG() << QString("%1 list count: %2").arg(actualMovieChangeCategory).arg(movieList.count());

              // The list is sorted from least popular to most popular
              // so take from the front of the list
              Movie m = movieList.first();
              movieList.removeFirst();

              // Keep track of how much free disk space we'll have after deleting
              // this movie from the disk
              lowestFreeDiskSpace += (qreal)m.FileLength() / (1024 * 1024 * 1024);

              // Decrement the newHighestChannelNum to reflect how many channels will be available after deleting movie
              --newHighestChannelNum;

              QLOG_DEBUG() << QString("Removing next least played video from category %1, channel #: %2, available disk space now: %3 GB, total channels: %4").arg(actualMovieChangeCategory).arg(m.ChannelNum()).arg(lowestFreeDiskSpace).arg(newHighestChannelNum);

              // Get the next download directory that matches this movie's category
              // If one is not found matching the category then use any download directory that is available

              int dmIndex = findMovieDownloadInCategory(movieDownloads, movieChangeCategory);

              Movie movieDownload;
              if (dmIndex >= 0)
              {
                movieDownload = movieDownloads.at(dmIndex);
                movieDownloads.removeAt(dmIndex);
              }
              else
              {
                QLOG_DEBUG() << QString("Can't find anymore movie downloads matching category: %1, resorting to whatever is available").arg(movieChangeCategory);

                if (!movieDownloads.isEmpty())
                {
                  movieDownload = movieDownloads.first();
                  movieDownloads.removeFirst();
                }
              }

              if (movieDownload.isValid())
              {
                QLOG_DEBUG() << QString("Assigning download movie: %1, category: %2 to channel #: %3")
                                .arg(movieDownload.Dirname())
                                .arg(movieDownload.Category())
                                .arg(m.ChannelNum());

                // Determine what the current movie is at this channel # so we can show the user what's being replaced
                // It is possible that this channel # doesn't exist in the lineup yet
                bool ok;
                Movie previousMovie = dbMgr->getCurrentMovieAtChannel(m.ChannelNum(), &ok);

                if (!ok)
                {
                  // Try cleaning up the movie changes table since we're exiting early
                  dbMgr->deleteMovieChanges(movieChangeInfo.Year(), movieChangeInfo.WeekNum());

                  emit movieChangePrepareError("Database error while preparing for movie change. Contact tech support.");
                  return;
                }

                // insert record into movie change table: download video directory name, movie channel #, delete video
                if (!dbMgr->insertMovieChange(movieChangeInfo.Year(), movieChangeInfo.WeekNum(), movieDownload.Dirname(), m.ChannelNum(), true, scheduledDate, previousMovie))
                {
                  QLOG_ERROR() << "Could not insert into movie change table - year:" << movieChangeInfo.Year() << ", week_num:" << movieChangeInfo.WeekNum() << ", dirname:" << movieDownload.Dirname() << ", ch #:" << m.ChannelNum() << ", scheduled_date:" << scheduledDate << ", previous movie:" << previousMovie.UPC();

                  // Try cleaning up the movie changes table since we're exiting early
                  dbMgr->deleteMovieChanges(movieChangeInfo.Year(), movieChangeInfo.WeekNum());

                  emit movieChangePrepareError("Could not write to database while preparing for movie change. Contact tech support.");
                  return;
                }
              }
              else
              {
                QLOG_ERROR() << "No more movies to insert in movie change table.  Movie change cannot continue.";
                emit movieChangePrepareError("Unexpected error while preparing movie change. Contact tech support.");
                return;
              }

              // If the movieList is now empty then remove
              // the dictionary entry
              if (movieList.isEmpty())
                viewTimesByCategory.remove(actualMovieChangeCategory);
              else
                // Otherwise just overwrite the list with the updated version
                viewTimesByCategory[actualMovieChangeCategory] = movieList;

            } // endwhile

            // if we're still below the minimum free disk space then we need to delete more videos
            if (lowestFreeDiskSpace < settings->getValue(MIN_FREE_DISKSPACE).toReal())
            {
              QLOG_DEBUG() << QString("We still did not free enough disk space: %1 GB. The minimum is: %2 GB")
                              .arg(lowestFreeDiskSpace)
                              .arg(settings->getValue(MIN_FREE_DISKSPACE).toReal());

              QString largestCategoryName = findLargestVideoCategory(viewTimesByCategory);

              // Get the list of videos sorted by popularity and dateIn for selected category
              QList<Movie> movieList = viewTimesByCategory[largestCategoryName];

              QLOG_DEBUG() << QString("Now deleting least popular, oldest videos from largest category: %1 until there's enough free space...").arg(largestCategoryName);

              // delete the least popular videos from the largest category until there is enough free space
              // and we don't run out of videos from the largest category
              while (lowestFreeDiskSpace < settings->getValue(MIN_FREE_DISKSPACE).toReal() && !movieList.isEmpty())
              {
                // The list is sorted from least popular to most popular, oldest to newest
                // so take from the front of the list
                Movie m = movieList.first();
                movieList.removeFirst();

                // Keep track of how much free disk space we'll have after deleting
                // this movie from the disk
                lowestFreeDiskSpace += (qreal)m.FileLength() / (1024 * 1024 * 1024);

                QLOG_DEBUG() << QString("Removing next least played video from category %1, channel #: %2, available disk space now: %3 GB").arg(largestCategoryName).arg(m.ChannelNum()).arg(lowestFreeDiskSpace);

                // Determine what the current movie is at this channel # so we can show the user what's being replaced
                // It is possible that this channel # doesn't exist in the lineup yet
                bool ok;
                Movie previousMovie = dbMgr->getCurrentMovieAtChannel(m.ChannelNum(), &ok);

                if (!ok)
                {
                  // Try cleaning up the movie changes table since we're exiting early
                  dbMgr->deleteMovieChanges(movieChangeInfo.Year(), movieChangeInfo.WeekNum());

                  emit movieChangePrepareError("Database error while preparing for movie change. Contact tech support.");
                  return;
                }

                if (!dbMgr->insertMovieChange(movieChangeInfo.Year(), movieChangeInfo.WeekNum(), QString(), m.ChannelNum(), true, scheduledDate, previousMovie))
                {
                  QLOG_ERROR() << "Could not insert into movie change table - year:" << movieChangeInfo.Year() << ", week_num:" << movieChangeInfo.WeekNum() << ", dirname: NULL, ch #:" << m.ChannelNum() << ", scheduled_date:" << scheduledDate << ", previous movie:" << previousMovie.UPC();

                  // Try cleaning up the movie changes table since we're exiting early
                  dbMgr->deleteMovieChanges(movieChangeInfo.Year(), movieChangeInfo.WeekNum());

                  emit movieChangePrepareError("Could not write to database while preparing for movie change. Contact tech support.");
                  return;
                }

              } // end while not enough free space

            } // end if not enough free space

          } // end if successfully got the categories from videos
          else
          {
            QLOG_ERROR() << "Error while getting list of current videos categories. Movie change cannot continue.";
            emit movieChangePrepareError("Error while getting list of current videos categories. Contact tech support.");
            return;
          }

        } // endif free space too low

        // Insert any remaining movie downloads in the movie change table
        // If we encountered a device with low disk space then the
        // movie downloads list will be smaller or even empty depending on
        // how many movies will have to be deleted to make room for the new ones.
        foreach (Movie m, movieDownloads)
        {
          ++highestChannelNum;

          QLOG_DEBUG() << QString("Assigning download movie: %1, category: %2 to channel #: %3")
                          .arg(m.Dirname())
                          .arg(m.Category())
                          .arg(highestChannelNum);

          // Determine what the current movie is at this channel # so we can show the user what's being replaced
          // It is possible that this channel # doesn't exist in the lineup yet
          bool ok;
          Movie previousMovie = dbMgr->getCurrentMovieAtChannel(highestChannelNum, &ok);

          if (!ok)
          {
            // Try cleaning up the movie changes table since we're exiting early
            dbMgr->deleteMovieChanges(movieChangeInfo.Year(), movieChangeInfo.WeekNum());

            emit movieChangePrepareError("Database error while preparing for movie change. Contact tech support.");
            return;
          }

          // insert record into movie change table: download video directory name, highest channel num, no delete
          if (!dbMgr->insertMovieChange(movieChangeInfo.Year(), movieChangeInfo.WeekNum(), m.Dirname(), highestChannelNum, false, scheduledDate, previousMovie))
          {
            QLOG_ERROR() << "Could not insert into movie change table - year:" << movieChangeInfo.Year() << ", week_num:" << movieChangeInfo.WeekNum() << ", dirname:" << m.Dirname() << ", ch #:" << highestChannelNum << ", scheduled_date:" << scheduledDate << ", previous movie:" << previousMovie.UPC();

            // Try cleaning up the movie changes table since we're exiting early
            dbMgr->deleteMovieChanges(movieChangeInfo.Year(), movieChangeInfo.WeekNum());

            emit movieChangePrepareError("Could not write to database. Contact tech support.");
            return;
          }
        }

        // If we still have not freed enough disk space to satisfy the minimum requirement then exit now
        if (lowestFreeDiskSpace < settings->getValue(MIN_FREE_DISKSPACE).toReal())
        {
          QLOG_ERROR() << QString("Could not free enough disk space to satisfy minimum requirement. Ended up with: %1 GB of free space").arg(lowestFreeDiskSpace);

          // Try cleaning up the movie changes table since we're exiting early
          dbMgr->deleteMovieChanges(movieChangeInfo.Year(), movieChangeInfo.WeekNum());

          emit movieChangePrepareError("Could not free enough disk space on the booths for the new movies. Contact tech support.");
          return;
        }

      } // endif !deleteOnly

      // Let caller know about booths that were excluded
      foreach (QString ipAddress, excludedAddresses)
      {
        emit movieChangeExcluded(ipAddress);
      }

    } // endif all booths are online or forcing movie change
    else
    {
      QLOG_DEBUG() << "Not all booths responded and not forcing movie change";
      emit movieChangePreparePossibleError(QString("The following booths are not online: %1. Are you sure you still want to perform a movie change?").arg(offlineDeviceList.join(", ")));
      return;
    }
  } // endif devices defined
  else
  {
    QLOG_ERROR() << "No devices are defined";
    emit movieChangePrepareError("The software does not seem to be configured properly. Contact tech support.");
  }
}

void CasServer::startMovieChangeClients(QStringList ipAddressList)
{
  QLOG_DEBUG() << QString("Starting movie change clients thread");

  qfwMovieChangeClients.setFuture(QtConcurrent::run(this, &CasServer::movieChangeClients, ipAddressList));
}

void CasServer::movieChangeClients(QStringList ipAddressList)
{
  QStringList errorList;
  QStringList changedList;

  foreach (QString ipAddress, ipAddressList)
  {
    QLOG_DEBUG() << QString("Starting movie change: %1...").arg(ipAddress);

    if (sendMessage(COMMAND_MOVIE_CHANGE, ipAddress, TCP_PORT))
    {
      changedList.append(ipAddress);
      emit movieChangeStartSuccess(ipAddress);
    }
    else
    {
      errorList.append(ipAddress);
      emit movieChangeStartFailed(ipAddress);
    }
  }

  if (errorList.count() > 0)
  {
    QLOG_DEBUG() << QString("Movie change started in %1 out of %2 device(s)")
                    .arg(changedList.count())
                    .arg(ipAddressList.count());
  }
  else
  {
    QLOG_DEBUG() << QString("Movie change started in all %1 device(s)").arg(ipAddressList.count());
  }
}

void CasServer::startMovieChangeRetry(QStringList ipAddressRetryList)
{
  QLOG_DEBUG() << QString("Starting movie change retry thread");

  qfwMovieChangeRetry.setFuture(QtConcurrent::run(this, &CasServer::movieChangeRetry, ipAddressRetryList));
}

void CasServer::movieChangeRetry(QStringList ipAddressRetryList)
{
  QStringList successList;
  QStringList errorList;

  foreach (QString ipAddress, ipAddressRetryList)
  {
    QLOG_DEBUG() << QString("Starting uftpd: %1...").arg(ipAddress);

    if (sendMessage(COMMAND_MOVIE_CHANGE_RETRY, ipAddress, TCP_PORT))
    {
      successList.append(ipAddress);
      emit movieChangeRetrySuccess(ipAddress);
    }
    else
    {
      errorList.append(ipAddress);
      emit movieChangeRetryFailed(ipAddress);
    }
  }

  if (errorList.count() > 0)
  {
    QLOG_DEBUG() << QString("uftpd started on %1 out of %2 device(s)")
                    .arg(successList.count())
                    .arg(ipAddressRetryList.count());
  }
  else
  {
    QLOG_DEBUG() << QString("uftpd started on all %1 device(s)").arg(ipAddressRetryList.count());
  }
}

void CasServer::startMovieChangeCancel(QStringList ipAddressList, int year, int weekNum, bool cleanup)
{
  QLOG_DEBUG() << QString("Starting movie change cancel thread");

  qfwMovieChangeCancel.setFuture(QtConcurrent::run(this, &CasServer::movieChangeCancel, ipAddressList, year, weekNum, cleanup));
}

void CasServer::movieChangeCancel(QStringList ipAddressList, int year, int weekNum, bool cleanup)
{
  QStringList successList;
  QStringList errorList;

  quint8 command = (cleanup ? COMMAND_MOVIE_CHANGE_CANCEL : COMMAND_MOVIE_CHANGE_CANCEL_NO_DELETE);

  foreach (QString ipAddress, ipAddressList)
  {
    QLOG_DEBUG() << QString("Canceling movie change: %1 %2...").arg(ipAddress).arg((cleanup ? "and deleting files already downloaded" : "and not deleting files"));

    if (sendMessage(command, ipAddress, TCP_PORT))
    {
      successList.append(ipAddress);
      emit movieChangeCancelSuccess(ipAddress);
    }
    else
    {
      errorList.append(ipAddress);
      emit movieChangeCancelFailed(ipAddress);
    }
  }

  if (errorList.count() > 0)
  {
    QLOG_DEBUG() << QString("Movie change canceled on %1 out of %2 device(s)")
                    .arg(successList.count())
                    .arg(ipAddressList.count());
  }
  else
  {
    QLOG_DEBUG() << QString("Movie change canceled on all %1 device(s)").arg(ipAddressList.count());
  }

  if (cleanup)
  {
    // Delete movie change records from moviechanges database
    if (!dbMgr->deleteMovieChanges(year, weekNum))
    {
      Global::sendAlert(QString("Could not delete movie change records from server database for year: %1, week #: %2").arg(year).arg(weekNum));
    }

    QStringList deviceAddressList;
    foreach (QString address, ipAddressList)
      deviceAddressList.append(ipAddressToDeviceNum(address));

    // Delete movie change records from moviechanges_<boothnum> databases
    if (!dbMgr->deleteBoothMovieChanges(deviceAddressList, year, weekNum))
    {
      Global::sendAlert(QString("Could not delete movie change records from all booth databases for year: %1, week #: %2").arg(year).arg(weekNum));
    }
  }
}

// TODO: (Low Priority) Movies are returned in no particular order which can be an issue when the customer copied the videos in a particular order (say
// 10 videos in week #, first 7 are gay then the last 3 are transsexual) then when performing a movie change and assigning to a channel
// range, there's no guarantee the first 7 channels will be gay and the last 3 transsexual
QList<Movie> CasServer::getMovieDownloads(QString moviePath)
{
  QList<Movie> movieList;

  QDir weekDir(moviePath);

  // Get list of movie directories in specified directory
  QStringList dirList = weekDir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);

  // Foreach directory found, verify it contains a mediaInfo.xml and add to movie list
  foreach (QString dirName, dirList)
  {
    QDir dlDir(weekDir.absoluteFilePath(dirName));

    if (dlDir.exists("mediaInfo.xml"))
    {
      movieList.append(Movie(dlDir.absoluteFilePath("mediaInfo.xml"), 0));
    }

  } // endfor

  QLOG_DEBUG() << QString("Found %1 video(s) in %2").arg(movieList.count()).arg(weekDir.absolutePath());

  return movieList;
}

int CasServer::findMovieDownloadInCategory(QList<Movie> movieList, QString category)
{
  for (int i = 0; i < movieList.count(); ++i)
  {
    if (movieList.at(i).Category() == category)
      return i;
  }

  return -1;
}

QString CasServer::ipAddressToDeviceNum(QString ipAddress)
{
  QStringList octets = ipAddress.split(".");

  if (octets.count() == 4)
    return octets.at(3);
  else
    return "";
}

QStringList CasServer::ipAddressesToDeviceNums(QStringList ipAddressList)
{
  // Convert IP addresses to device addresses
  QStringList deviceAddressList;
  foreach (QString address, ipAddressList)
    deviceAddressList.append(ipAddressToDeviceNum(address));

  return deviceAddressList;
}

QString CasServer::findLargestVideoCategory(QMap<QString, QList<Movie> > viewTimesByCategory)
{
  // Find the largest movie list to remove a movie from
  // since we couldn't find the category we need
  QMapIterator<QString, QList<Movie> > j(viewTimesByCategory);
  QString largestCategoryName;
  int largestCategoryCount = 0;
  while (j.hasNext())
  {
    j.next();

    if (j.value().count() > largestCategoryCount)
    {
      largestCategoryName = j.key();
      largestCategoryCount = j.value().count();
    }
  }

  return largestCategoryName;
}

void CasServer::sendCollection(QDate collectionDate, QVariantList deviceMeters)
{
  QLOG_DEBUG() << QString("Sending collection to web server for emailing...");

  QVariantList collection;
  qreal totalCredits = 0;
  qreal totalCash = 0;
  qreal totalCcCharges = 0;
  qreal totalPreviews = 0;
  qreal totalProgrammed = 0;

  foreach (QVariant dm, deviceMeters)
  {
    QVariantMap meters = dm.toMap();

    totalCredits += meters["current_credits"].toInt();
    totalCash += meters["current_cash"].toInt();
    totalCcCharges += meters["current_cc_charges"].toInt();
    totalPreviews += meters["current_preview_credits"].toInt();
    totalProgrammed += meters["current_prog_credits"].toInt();

    // Replace current meters with formatted versions
    meters["current_credits"] = QString("$%L1 (%2)").arg(meters["current_credits"].toReal()  / 4, 0, 'f', 2).arg(meters["current_credits"].toInt());    
    meters["current_preview_credits"] = QString("$%L1 (%2)").arg(meters["current_preview_credits"].toReal()  / 4, 0, 'f', 2).arg(meters["current_preview_credits"].toInt());    
    meters["current_cash"] = QString("$%L1 (%2)").arg(meters["current_cash"].toReal()  / 4, 0, 'f', 2).arg(meters["current_cash"].toInt());    
    meters["current_cc_charges"] = QString("$%L1 (%2)").arg(meters["current_cc_charges"].toReal()  / 4, 0, 'f', 2).arg(meters["current_cc_charges"].toInt());
    meters["current_prog_credits"] = QString("$%L1 (%2)").arg(meters["current_prog_credits"].toReal()  / 4, 0, 'f', 2).arg(meters["current_prog_credits"].toInt());    

    // Change to full IP address
    meters["device_num"] = settings->getValue(ARCADE_SUBNET).toString() + meters["device_num"].toString();

    collection.append(meters);
  }

  QVariantMap collectionHeader;
  collectionHeader["collection_date"] = collectionDate.toString("yyyy-MM-dd");
  collectionHeader["recipients"] = settings->getValue(COLLECTION_REPORT_RECIPIENTS).toString();
  collectionHeader["store_name"] = settings->getValue(STORE_NAME).toString();
  collectionHeader["collection"] = collection;
  collectionHeader["total_credits"] = QString("$%L1").arg(totalCredits / 4, 0, 'f', 2);
  collectionHeader["total_cash"] = QString("$%L1").arg(totalCash / 4, 0, 'f', 2);
  collectionHeader["total_cc_charges"] = QString("$%L1").arg(totalCcCharges / 4, 0, 'f', 2);
  collectionHeader["total_previews"] = QString("$%L1").arg(totalPreviews / 4, 0, 'f', 2);
  collectionHeader["total_programmed"] = QString("$%L1").arg(totalProgrammed / 4, 0, 'f', 2);

  QVariantMap collectionMessage;
  collectionMessage["action"] = "sendCollectionReport";
  collectionMessage["passphrase"] = settings->getValue(CUSTOMER_PASSWORD, "", true).toString();
  collectionMessage["data"] = collectionHeader;

  QJson::Serializer serializer;
  QByteArray jsonData = serializer.serialize(collectionMessage);

  QUrl serverUrl(WEB_SERVICE_URL);
  QNetworkRequest request(serverUrl);

  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  netReplySendCollection = netMgr->post(request, jsonData);
  connect(netReplySendCollection, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(sendingCollectionError(QNetworkReply::NetworkError)));
  connect(netReplySendCollection, SIGNAL(finished()), this, SLOT(receivedCollectionsResponse()));
  connect(netReplySendCollection, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(resetTimeout(qint64,qint64)));
  connect(netReplySendCollection, SIGNAL(uploadProgress(qint64,qint64)), this, SLOT(resetTimeout(qint64,qint64)));

  // Start timer which will abort the connection when the time expires
  // Connecting the downloadProgress signal to the resetTimeout slot
  // allows resetting the timer everytime the progress changes
  connect(connectionTimer, SIGNAL(timeout()), this, SLOT(abortConnection()));
  connectionTimer->start();
}

void CasServer::sendDailyMeters(QDateTime metersCaptured, QVariantList deviceMeters)
{
  QLOG_DEBUG() << QString("Sending daily meters to web server for emailing...");

  QVariantList dailyMeters;
  qreal totalCredits = 0;
  qreal totalCash = 0;
  qreal totalCcCharges = 0;
  qreal totalPreviews = 0;
  qreal totalProgrammed = 0;

  foreach (QVariant dm, deviceMeters)
  {
    QVariantMap meters = dm.toMap();

    totalCredits += meters["current_credits"].toInt();
    totalCash += meters["current_cash"].toInt();
    totalCcCharges += meters["current_cc_charges"].toInt();
    totalPreviews += meters["current_preview_credits"].toInt();
    totalProgrammed += meters["current_prog_credits"].toInt();

    // Replace current meters with formatted versions
    meters["current_credits"] = QString("$%L1 (%2)").arg(meters["current_credits"].toReal()  / 4, 0, 'f', 2).arg(meters["current_credits"].toInt());
    meters["current_preview_credits"] = QString("$%L1 (%2)").arg(meters["current_preview_credits"].toReal()  / 4, 0, 'f', 2).arg(meters["current_preview_credits"].toInt());
    meters["current_cash"] = QString("$%L1 (%2)").arg(meters["current_cash"].toReal()  / 4, 0, 'f', 2).arg(meters["current_cash"].toInt());
    meters["current_cc_charges"] = QString("$%L1 (%2)").arg(meters["current_cc_charges"].toReal()  / 4, 0, 'f', 2).arg(meters["current_cc_charges"].toInt());
    meters["current_prog_credits"] = QString("$%L1 (%2)").arg(meters["current_prog_credits"].toReal()  / 4, 0, 'f', 2).arg(meters["current_prog_credits"].toInt());

    // Change to full IP address
    meters["device_num"] = settings->getValue(ARCADE_SUBNET).toString() + meters["device_num"].toString();

    dailyMeters.append(meters);
  }

  QVariantMap dailyMetersHeader;
  dailyMetersHeader["captured"] = metersCaptured.toString("yyyy-MM-dd hh:mm:ss");
  dailyMetersHeader["recipients"] = settings->getValue(DAILY_METERS_REPORT_RECIPIENTS).toString();
  dailyMetersHeader["store_name"] = settings->getValue(STORE_NAME).toString();
  dailyMetersHeader["meters"] = dailyMeters;
  dailyMetersHeader["total_credits"] = QString("$%L1").arg(totalCredits / 4, 0, 'f', 2);
  dailyMetersHeader["total_cash"] = QString("$%L1").arg(totalCash / 4, 0, 'f', 2);
  dailyMetersHeader["total_cc_charges"] = QString("$%L1").arg(totalCcCharges / 4, 0, 'f', 2);
  dailyMetersHeader["total_previews"] = QString("$%L1").arg(totalPreviews / 4, 0, 'f', 2);
  dailyMetersHeader["total_programmed"] = QString("$%L1").arg(totalProgrammed / 4, 0, 'f', 2);

  QVariantMap dailyMetersMessage;
  dailyMetersMessage["action"] = "sendDailyMetersReport";
  dailyMetersMessage["passphrase"] = settings->getValue(CUSTOMER_PASSWORD, "", true).toString();
  dailyMetersMessage["data"] = dailyMetersHeader;

  QJson::Serializer serializer;
  QByteArray jsonData = serializer.serialize(dailyMetersMessage);

  QUrl serverUrl(WEB_SERVICE_URL);
  QNetworkRequest request(serverUrl);

  //QLOG_DEBUG() << QString("Sending daily meters data: %1").arg(jsonData.constData());

  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  netReplySendDailyMeters = netMgr->post(request, jsonData);
  connect(netReplySendDailyMeters, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(sendingDailyMetersError(QNetworkReply::NetworkError)));
  connect(netReplySendDailyMeters, SIGNAL(finished()), this, SLOT(receivedDailyMetersResponse()));
  connect(netReplySendDailyMeters, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(resetTimeout(qint64,qint64)));
  connect(netReplySendDailyMeters, SIGNAL(uploadProgress(qint64,qint64)), this, SLOT(resetTimeout(qint64,qint64)));

  // Start timer which will abort the connection when the time expires
  // Connecting the downloadProgress signal to the resetTimeout slot
  // allows resetting the timer everytime the progress changes
  connect(connectionTimer, SIGNAL(timeout()), this, SLOT(abortConnection()));
  connectionTimer->start();
}

void CasServer::sendCollectionSnapshot(QDateTime startTime, QDateTime endTime, QVariantList deviceMeters)
{
  QLOG_DEBUG() << QString("Sending collection snapshot (%1 - %2) to web server for emailing...").arg(startTime.toString("yyyy-MM-dd hh:mm:ss")).arg(endTime.toString("yyyy-MM-dd hh:mm:ss"));

  QVariantList meterList;
  qreal totalCredits = 0;
  qreal totalCash = 0;
  qreal totalCcCharges = 0;
  qreal totalPreviews = 0;
  qreal totalProgrammed = 0;

  foreach (QVariant dm, deviceMeters)
  {
    QVariantMap meters = dm.toMap();
    meters.remove("start_time");

    totalCredits += meters["current_credits"].toInt();
    totalCash += meters["current_cash"].toInt();
    totalCcCharges += meters["current_cc_charges"].toInt();
    totalPreviews += meters["current_preview_credits"].toInt();
    totalProgrammed += meters["current_prog_credits"].toInt();

    // Replace current meters with formatted versions
    meters["current_credits"] = QString("$%L1 (%2)").arg(meters["current_credits"].toReal()  / 4, 0, 'f', 2).arg(meters["current_credits"].toInt());
    meters["current_preview_credits"] = QString("$%L1 (%2)").arg(meters["current_preview_credits"].toReal()  / 4, 0, 'f', 2).arg(meters["current_preview_credits"].toInt());
    meters["current_cash"] = QString("$%L1 (%2)").arg(meters["current_cash"].toReal()  / 4, 0, 'f', 2).arg(meters["current_cash"].toInt());
    meters["current_cc_charges"] = QString("$%L1 (%2)").arg(meters["current_cc_charges"].toReal()  / 4, 0, 'f', 2).arg(meters["current_cc_charges"].toInt());
    meters["current_prog_credits"] = QString("$%L1 (%2)").arg(meters["current_prog_credits"].toReal()  / 4, 0, 'f', 2).arg(meters["current_prog_credits"].toInt());

    // Change to full IP address
    meters["device_num"] = settings->getValue(ARCADE_SUBNET).toString() + meters["device_num"].toString();

    meterList.append(meters);
  }

  QVariantMap collectionSnapshotHeader;
  collectionSnapshotHeader["start_time"] = startTime.toString("yyyy-MM-dd hh:mm:ss");
  collectionSnapshotHeader["end_time"] = endTime.toString("yyyy-MM-dd hh:mm:ss");
  collectionSnapshotHeader["recipients"] = settings->getValue(COLLECTION_SNAPSHOT_REPORT_RECIPIENTS).toString();
  collectionSnapshotHeader["store_name"] = settings->getValue(STORE_NAME).toString();
  collectionSnapshotHeader["meters"] = meterList;
  collectionSnapshotHeader["total_credits"] = QString("$%L1").arg(totalCredits / 4, 0, 'f', 2);
  collectionSnapshotHeader["total_cash"] = QString("$%L1").arg(totalCash / 4, 0, 'f', 2);
  collectionSnapshotHeader["total_cc_charges"] = QString("$%L1").arg(totalCcCharges / 4, 0, 'f', 2);
  collectionSnapshotHeader["total_previews"] = QString("$%L1").arg(totalPreviews / 4, 0, 'f', 2);
  collectionSnapshotHeader["total_programmed"] = QString("$%L1").arg(totalProgrammed / 4, 0, 'f', 2);

  QVariantMap collectionSnapshotMessage;
  collectionSnapshotMessage["action"] = "sendCollectionSnapshotReport";
  collectionSnapshotMessage["passphrase"] = settings->getValue(CUSTOMER_PASSWORD, "", true).toString();
  collectionSnapshotMessage["data"] = collectionSnapshotHeader;

  QJson::Serializer serializer;
  QByteArray jsonData = serializer.serialize(collectionSnapshotMessage);

  QUrl serverUrl(WEB_SERVICE_URL);
  QNetworkRequest request(serverUrl);

  QLOG_DEBUG() << QString("Sending collection snapshot data: %1").arg(jsonData.constData());

  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  netReplySendCollectionSnapshot = netMgr->post(request, jsonData);
  connect(netReplySendCollectionSnapshot, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(sendingCollectionSnapshotError(QNetworkReply::NetworkError)));
  connect(netReplySendCollectionSnapshot, SIGNAL(finished()), this, SLOT(receivedCollectionSnapshotResponse()));
  connect(netReplySendCollectionSnapshot, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(resetTimeout(qint64,qint64)));
  connect(netReplySendCollectionSnapshot, SIGNAL(uploadProgress(qint64,qint64)), this, SLOT(resetTimeout(qint64,qint64)));

  // Start timer which will abort the connection when the time expires
  // Connecting the downloadProgress signal to the resetTimeout slot
  // allows resetting the timer everytime the progress changes
  connect(connectionTimer, SIGNAL(timeout()), this, SLOT(abortConnection()));
  connectionTimer->start();
}

void CasServer::startUpdateSettings(QStringList ipAddressList)
{
  QLOG_DEBUG() << QString("Starting update booth settings thread");
  qfwUpdateSettings.setFuture(QtConcurrent::run(this, &CasServer::updateSettings, ipAddressList));
}

void CasServer::startRestartDevices()
{
  QLOG_DEBUG() << QString("Starting restart devices thread");
  qfwRestartDevices.setFuture(QtConcurrent::run(this, &CasServer::restartDevices));
}

void CasServer::restartDevices()
{
  if (settings->getValue(DEVICE_LIST).toStringList().count() > 0)
  {
    quint8 returnCode = 0;
    QStringList errorList;
    QStringList restartList;

    foreach (QString ipAddress, settings->getValue(DEVICE_LIST).toStringList())
    {
      QLOG_DEBUG() << QString("Requesting device to restart at: %1...").arg(ipAddress);

      if (sendMessage(COMMAND_RESTART_DEVICE, ipAddress, TCP_PORT, &returnCode))
      {
        if (returnCode == ERROR_CODE_IN_SESSION)
          emit restartDeviceInSession(ipAddress);
        else
          emit restartDeviceSuccess(ipAddress);

        restartList.append(ipAddress);
      }
      else
      {
        errorList.append(ipAddress);
        emit restartDeviceFailed(ipAddress);
      }
    }

    if (errorList.count() > 0)
    {
      QLOG_DEBUG() << QString("Restarted %1 out of %2 device(s)")
                      .arg(restartList.count())
                      .arg(settings->getValue(DEVICE_LIST).toStringList().count());
    }
    else
    {
      QLOG_DEBUG() << QString("Restarted all %1 device(s)").arg(settings->getValue(DEVICE_LIST).toStringList().count());
    }
  }
  else
    QLOG_ERROR() << "No devices are defined";
}

void CasServer::updateSettings(QStringList ipAddressList)
{
  QLOG_DEBUG() << QString("Requesting device settings to be updated at: %1...").arg(ipAddressList.join(", "));

  foreach (QString ipAddress, ipAddressList)
  {
    quint8 returnCode = 0;

    if (sendMessage(COMMAND_UPDATE_SETTINGS, ipAddress, TCP_PORT, &returnCode))
    {
      if (returnCode == ERROR_CODE_IN_SESSION)
        emit updateSettingsInSession(ipAddress);
      else
        emit updateSettingsSuccess(ipAddress);
    }
    else
      emit updateSettingsFailed(ipAddress);
  }
}

void CasServer::startGetBoothDatabases()
{
  QLOG_DEBUG() << QString("Starting get booth databases thread");
  qfwGetBoothDatabases.setFuture(QtConcurrent::run(this, &CasServer::getBoothDatabases));
}


void CasServer::getBoothDatabases()
{
  if (settings->getValue(DEVICE_LIST).toStringList().count() > 0)
  {
    QStringList errorList;
    QStringList dbCopyList;

    foreach (QString ipAddress, settings->getValue(DEVICE_LIST).toStringList())
    {
      QLOG_DEBUG() << QString("Requesting copy of database from: %1...").arg(ipAddress);

      if (sendMessage(COMMAND_SEND_DATABASE, ipAddress, TCP_PORT))
      {
        dbCopyList.append(ipAddress);
        emit getBoothDatabaseSuccess(ipAddress);
      }
      else
      {
        errorList.append(ipAddress);
        emit getBoothDatabaseFailed(ipAddress);
      }
    }

    if (errorList.count() > 0)
    {
      QLOG_DEBUG() << QString("Received database copies from %1 out of %2 device(s)")
                      .arg(dbCopyList.count())
                      .arg(settings->getValue(DEVICE_LIST).toStringList().count());
    }
    else
    {
      QLOG_DEBUG() << QString("Received database copies from all %1 device(s)").arg(settings->getValue(DEVICE_LIST).toStringList().count());
    }
  }
  else
    QLOG_ERROR() << "No devices are defined";
}

QStringList CasServer::getDeviceAddresses()
{
  QStringList addressList;

  foreach (QString ipAddress, settings->getValue(DEVICE_LIST).toStringList())
  {
    QString deviceNum = ipAddressToDeviceNum(ipAddress);

    if (!deviceNum.isEmpty())
      addressList.append(deviceNum);
  }

  return addressList;
}

bool CasServer::isBusy()
{
  return qfwScanDevices.isRunning() || qfwCheckDevices.isRunning() || qfwGetBoothDatabases.isRunning() ||
         qfwCollectViewTimes.isRunning() || qfwClearViewTimes.isRunning() || qfwMovieChangePrepare.isRunning() ||
         qfwMovieChangeRetry.isRunning() || qfwMovieChangeCancel.isRunning() || qfwUpdateSettings.isRunning() ||
         qfwRestartDevices.isRunning() || qfwCollection.isRunning() || qfwToggleAlarmState.isRunning() ||
         qfwCollectDailyViewTimes.isRunning() || qfwClearClerkstations.isRunning();
}

bool CasServer::sendMessage(quint8 command, QString ipAddress, quint16 port, quint8 *returnCode)
{
  bool success = false;
  const int connectionTimeout = 120000;
  int waitConnectTimeout = 10000;

  // This timeout is shorter when scanning the network
  if (command == COMMAND_PING)
    waitConnectTimeout = 1500;

  QTcpSocket socket;
  socket.connectToHost(QHostAddress(ipAddress), port);

  if (socket.waitForConnected(waitConnectTimeout))
  {
    // Build request block
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_7);

    // Block layout: block size, passphrase, command
    out << quint16(0) << settings->getValue(NETWORK_PASSPHRASE, "", true).toString() << command;
    out.device()->seek(0);
    out << quint16(block.size() - sizeof(quint16));
    socket.write(block);
    if (socket.waitForBytesWritten())
    {
      while (socket.bytesAvailable() < (int)sizeof(quint16))
      {
        if (!socket.waitForReadyRead(connectionTimeout))
        {
          //emit error(socket.errorString());
          return success;
        }
      }

      quint16 blockSize;
      QDataStream in(&socket);
      in.setVersion(QDataStream::Qt_4_7);
      in >> blockSize;

      while (socket.bytesAvailable() < blockSize)
      {
        if (!socket.waitForReadyRead(connectionTimeout))
        {
          //emit error(socket.errorString());
          return success;
        }
      }

      // Response block layout: block size, passphrase, error code
      QString returnedPassphrase;
      quint8 errorCode;
      in >> returnedPassphrase >> errorCode;

      switch (errorCode)
      {
        case ERROR_CODE_SUCCESS:
          success = true;
          break;

        case ERROR_CODE_WRONG_PASSPHRASE:
          QLOG_ERROR() << ipAddress << "responded our passphrase is incorrect";
          //cout << ipAddress << " responded our passphrase is incorrect\n";
          //cout.flush();
          break;

        case ERROR_CODE_DB_ERROR:
          QLOG_ERROR() << ipAddress << "encountered a database error";
          //cout << ipAddress << " encountered a database error\n";
          //cout.flush();
          break;

        case ERROR_CODE_IN_SESSION:
          success = true;
          QLOG_DEBUG() << ipAddress << "cannot take place because it is being used or a movie change is in progress. It will be performed once it finishes.";
          //cout << ipAddress << " cannot take place because it is being used.\nIt will be performed once the session is over.\n";
          //cout.flush();
          break;

        case ERROR_CODE_INCOMPLETE_DL:
          QLOG_ERROR() << ipAddress << "does not have all movie downloads";
          //cout << ipAddress << " does not have all movie downloads\n";
          //cout.flush();
          break;

        case ERROR_CODE_INCOMPLETE_MC:
          QLOG_ERROR() << ipAddress << "could not change all movies";
          //cout << ipAddress << " could not change all movies\n";
          //cout.flush();
          break;

        case ERROR_CODE_IDLE_STATE:
          QLOG_DEBUG() << ipAddress << "is idle";
          //cout << ipAddress << " is idle\n";
          //cout.flush();
          success = true;
          break;

        case ERROR_CODE_IN_SESSION_STATE:
          QLOG_DEBUG() << ipAddress << "is in session";
          //cout << ipAddress << " is in session\n";
          //cout.flush();
          success = true;
          break;

        case ERROR_CODE_SETTINGS_COPY_ERROR:
          QLOG_ERROR() << ipAddress << "could not copy settings";
          //cout << ipAddress << " could not copy settings\n";
          //cout.flush();
          break;

        case ERROR_CODE_DB_COPY_ERROR:
          QLOG_DEBUG() << ipAddress << "could not copy database to network share";
          //cout << ipAddress << " could not copy database to network share\n";
          //cout.flush();
          break;

        case ERROR_CODE_UNKNOWN_COMMAND:
          QLOG_ERROR() << ipAddress << "did not recognize the command we sent";
          //cout << ipAddress << " did not recognize the command we sent\n";
          //cout.flush();
          break;

        default:
          QLOG_ERROR() << ipAddress << "responded with unknown error code";
          //cout << ipAddress << " responded with unknown error code\n";
          //cout.flush();
          break;
      }

      // If the caller passed a valid pointer then set the error code
      if (returnCode)
        *returnCode = errorCode;
    }
  }

  return success;
}

void CasServer::startClearClerkstations()
{
  QLOG_DEBUG() << QString("Starting clear clerk stations thread");
  qfwClearClerkstations.setFuture(QtConcurrent::run(this, &CasServer::clearClerkstations));
}

void CasServer::clearClerkstations()
{
  QStringList clerkstationIpAddresses;

  if (settings->getValue(DEVICE_LIST).toStringList().count() > 0)
  {
    QVariantList boothList = dbMgr->getBoothInfo(this->getDeviceAddresses());
    foreach (QVariant v, boothList)
    {
        QVariantMap booth = v.toMap();
        if (booth["device_type_id"].toInt() == Settings::ClerkStation)
        {
          clerkstationIpAddresses.append(QString("%1").arg(settings->getValue(ARCADE_SUBNET).toString().append(booth["device_num"].toString())));
        }
    }

    if (clerkstationIpAddresses.count() > 0)
    {
      quint8 returnCode = 0;
      QStringList errorList;
      QStringList clearList;

      foreach (QString ipAddress, clerkstationIpAddresses)
      {
        QLOG_DEBUG() << QString("Requesting clerkstation to clear all transactions at: %1...").arg(ipAddress);

        if (sendMessage(COMMAND_CLEAR_CLERKSTATION, ipAddress, TCP_PORT, &returnCode))
        {
          if (returnCode == ERROR_CODE_SUCCESS)
          {
            clearList.append(ipAddress);
            emit clearClerkstationsSuccess(ipAddress);
          }
          else
          {
            errorList.append(ipAddress);
            emit clearClerkstationsFailed(ipAddress);
          }
        }
        else
        {
          errorList.append(ipAddress);
          emit clearClerkstationsFailed(ipAddress);
        }
      }

      if (errorList.count() > 0)
      {
        QLOG_DEBUG() << QString("Cleared %1 out of %2 clerk station(s)")
                        .arg(clearList.count())
                        .arg(clerkstationIpAddresses.count());
      }
      else
      {
        QLOG_DEBUG() << QString("Cleared all %1 clerk station(s)").arg(clerkstationIpAddresses.count());
      }
    }
    else
    {
      QLOG_ERROR() << QString("No clerk stations found");
    }
  }
  else
    QLOG_ERROR() << "No devices are defined";
}

void CasServer::finishedSendingViewTimes(bool success)
{
  /*
   * The following code could be used to retry sending several times
   *
  if (success || ++numRetries > MAX_RETRIES)
  {
    if (!success && numRetries > MAX_RETRIES)
    {
      QLOG_ERROR() << QString("Giving up on checking for new tasks, reached maximum retries");

      // Send error alert
      if (!alerter)
        alerter = new Alerter(DEFAULT_ALERT_SERVICE_URL);

      alerter->sendAlert(SOFTWARE_NAME, SOFTWARE_VERSION, QString("Giving up on checking for new tasks, reached maximum retries"));
    }
    else
    {
      //
    }
  }
  else
  {
    timer->setInterval(1 + (qrand() % 30000));
    QLOG_DEBUG() << QString("Retrying to contact server for new tasks in %1 ms. Retry: %2").arg(timer->interval()).arg(numRetries);
    timer->start();
  }*/
}

void CasServer::receivedViewTimesResponse()
{
  connectionTimer->stop();

  bool success = false;

  // The finished signal is always emitted, even if there was a network error
  if (netReplySendViews->error() == QNetworkReply::NoError)
  {
    QByteArray json = netReplySendViews->readAll();

    // Expected response is in JSON:
    // If success then the response will be:
    // { "success" : "description" }
    // If the server has a problem with what the drone sends, the response will be:
    // { "error" : "description" }

    QString response(json.constData());

    bool ok = false;
    QJson::Parser parser;
    QVariant var = parser.parse(json, &ok);

    if (ok)
    {
      QLOG_DEBUG() << QString("Received valid JSON response: %1").arg(response);

      if (var.toMap().contains("error"))
      {
        QLOG_ERROR() << QString("Server returned an error: %1").arg(var.toMap()["error"].toString());
      }
      else if (var.toMap().contains("success"))
      {
        // success response
        QLOG_DEBUG() << QString("View times were sent. Server response: %1").arg(var.toMap()["success"].toString());
      }
      else
      {
        QLOG_ERROR() << QString("Unexpected JSON response");
      }

      success = true;
    }
    else
    {
      QLOG_ERROR() << QString("Did not receive valid JSON response: %1").arg(response);
    }
  }

  netReplySendViews->deleteLater();
  netReplySendViews = 0;

  finishedSendingViewTimes(success);
}

void CasServer::sendingViewTimesError(QNetworkReply::NetworkError error)
{
  Q_UNUSED(error);

  QLOG_ERROR() << QString("Network error while sending view times: %1").arg(netReplySendViews->errorString());
}

void CasServer::receivedCollectionsResponse()
{
  connectionTimer->stop();

  bool success = false;
  QString result = "";

  // The finished signal is always emitted, even if there was a network error
  if (netReplySendCollection->error() == QNetworkReply::NoError)
  {
    QByteArray json = netReplySendCollection->readAll();

    // Expected response is in JSON:
    // If success then the response will be:
    // { "success" : "description" }
    // If the server has a problem with what the drone sends, the response will be:
    // { "error" : "description" }

    QString response(json.constData());

    bool ok = false;
    QJson::Parser parser;
    QVariant var = parser.parse(json, &ok);

    if (ok)
    {
      QLOG_DEBUG() << QString("Received valid JSON response: %1").arg(response);

      if (var.toMap().contains("error"))
      {
        QLOG_ERROR() << QString("Server returned an error: %1").arg(var.toMap()["error"].toString());
        result = "Server returned error while trying to send collection report: " + var.toMap()["error"].toString();
      }
      else if (var.toMap().contains("success"))
      {
        // success response
        QLOG_DEBUG() << QString("Collection report was sent. Server response: %1").arg(var.toMap()["success"].toString());
        result = "The collection report was sent.";
        success = true;
      }
      else
      {
        QLOG_ERROR() << QString("Unexpected JSON response");
        result = "Unexpected response from server. Could not send collection report.";
      }      
    }
    else
    {
      QLOG_ERROR() << QString("Did not receive valid JSON response: %1").arg(response);
      result = "Invalid response from server. Could not send collection report.";
    }
  }
  else
  {
    result = "Network error. Could not send collection report.";
  }

  netReplySendCollection->deleteLater();
  netReplySendCollection = 0;

  if (success)
  {
    emit sendCollectionSuccess(result);
  }
  else
  {
    emit sendCollectionFailed(result);
  }

  finishedSendingCollectionReport(success);
}

void CasServer::finishedSendingCollectionReport(bool success)
{
  emit sendCollectionFinished();

  /*
   * The following code could be used to retry sending several times
   *
  if (success || ++numRetries > MAX_RETRIES)
  {
    if (!success && numRetries > MAX_RETRIES)
    {
      QLOG_ERROR() << QString("Giving up on checking for new tasks, reached maximum retries");

      // Send error alert
      if (!alerter)
        alerter = new Alerter(DEFAULT_ALERT_SERVICE_URL);

      alerter->sendAlert(SOFTWARE_NAME, SOFTWARE_VERSION, QString("Giving up on checking for new tasks, reached maximum retries"));
    }
    else
    {
      //
    }
  }
  else
  {
    timer->setInterval(1 + (qrand() % 30000));
    QLOG_DEBUG() << QString("Retrying to contact server for new tasks in %1 ms. Retry: %2").arg(timer->interval()).arg(numRetries);
    timer->start();
  }*/
}

void CasServer::sendingDailyMetersError(QNetworkReply::NetworkError error)
{
  Q_UNUSED(error);

  QLOG_ERROR() << QString("Network error while sending daily meters report: %1").arg(netReplySendDailyMeters->errorString());
}

void CasServer::receivedDailyMetersResponse()
{
  connectionTimer->stop();

  bool success = false;
  QString result = "";

  // The finished signal is always emitted, even if there was a network error
  if (netReplySendDailyMeters->error() == QNetworkReply::NoError)
  {
    QByteArray json = netReplySendDailyMeters->readAll();

    // Expected response is in JSON:
    // If success then the response will be:
    // { "success" : "description" }
    // If the server has a problem with what the drone sends, the response will be:
    // { "error" : "description" }

    QString response(json.constData());

    bool ok = false;
    QJson::Parser parser;
    QVariant var = parser.parse(json, &ok);

    if (ok)
    {
      QLOG_DEBUG() << QString("Received valid JSON response: %1").arg(response);

      if (var.toMap().contains("error"))
      {
        QLOG_ERROR() << QString("Server returned an error: %1").arg(var.toMap()["error"].toString());
        result = "Server returned error while trying to send daily meters report: " + var.toMap()["error"].toString();
      }
      else if (var.toMap().contains("success"))
      {
        // success response
        QLOG_DEBUG() << QString("Daily meters report was sent. Server response: %1").arg(var.toMap()["success"].toString());
        result = "The daily meters report was sent.";
        success = true;
      }
      else
      {
        QLOG_ERROR() << QString("Unexpected JSON response");
        result = "Unexpected response from server. Could not send daily meters report.";
      }
    }
    else
    {
      QLOG_ERROR() << QString("Did not receive valid JSON response: %1").arg(response);
      result = "Invalid response from server. Could not send daily meters report.";
    }
  }
  else
  {
    result = "Network error. Could not send daily meters report.";
  }

  netReplySendDailyMeters->deleteLater();
  netReplySendDailyMeters = 0;

  if (success)
  {
    emit sendDailyMetersSuccess(result);
  }
  else
  {
    emit sendDailyMetersFailed(result);
  }

  finishedSendingDailyMetersReport(success);
}

void CasServer::finishedSendingDailyMetersReport(bool success)
{
  emit sendDailyMetersFinished();

  /*
   * The following code could be used to retry sending several times
   *
  if (success || ++numRetries > MAX_RETRIES)
  {
    if (!success && numRetries > MAX_RETRIES)
    {
      QLOG_ERROR() << QString("Giving up on checking for new tasks, reached maximum retries");

      // Send error alert
      if (!alerter)
        alerter = new Alerter(DEFAULT_ALERT_SERVICE_URL);

      alerter->sendAlert(SOFTWARE_NAME, SOFTWARE_VERSION, QString("Giving up on checking for new tasks, reached maximum retries"));
    }
    else
    {
      //
    }
  }
  else
  {
    timer->setInterval(1 + (qrand() % 30000));
    QLOG_DEBUG() << QString("Retrying to contact server for new tasks in %1 ms. Retry: %2").arg(timer->interval()).arg(numRetries);
    timer->start();
  }*/
}

void CasServer::sendingCollectionSnapshotError(QNetworkReply::NetworkError error)
{
  Q_UNUSED(error);

  QLOG_ERROR() << QString("Network error while sending collection snapshot report: %1").arg(netReplySendCollectionSnapshot->errorString());
}

void CasServer::receivedCollectionSnapshotResponse()
{
  connectionTimer->stop();

  bool success = false;
  QString result = "";

  // The finished signal is always emitted, even if there was a network error
  if (netReplySendCollectionSnapshot->error() == QNetworkReply::NoError)
  {
    QByteArray json = netReplySendCollectionSnapshot->readAll();

    // Expected response is in JSON:
    // If success then the response will be:
    // { "success" : "description" }
    // If the server has a problem with what the drone sends, the response will be:
    // { "error" : "description" }

    QString response(json.constData());

    bool ok = false;
    QJson::Parser parser;
    QVariant var = parser.parse(json, &ok);

    if (ok)
    {
      QLOG_DEBUG() << QString("Received valid JSON response: %1").arg(response);

      if (var.toMap().contains("error"))
      {
        QLOG_ERROR() << QString("Server returned an error: %1").arg(var.toMap()["error"].toString());
        result = "Server returned error while trying to send collection snapshot report: " + var.toMap()["error"].toString();
      }
      else if (var.toMap().contains("success"))
      {
        // success response
        QLOG_DEBUG() << QString("Collection snapshot report was sent. Server response: %1").arg(var.toMap()["success"].toString());
        result = "The collection snapshot report was sent.";
        success = true;
      }
      else
      {
        QLOG_ERROR() << QString("Unexpected JSON response");
        result = "Unexpected response from server. Could not send collection snapshot report.";
      }
    }
    else
    {
      QLOG_ERROR() << QString("Did not receive valid JSON response: %1").arg(response);
      result = "Invalid response from server. Could not send collection snapshot report.";
    }
  }
  else
  {
    result = "Network error. Could not send collection snapshot report.";
  }

  netReplySendCollectionSnapshot->deleteLater();
  netReplySendCollectionSnapshot = 0;

  if (success)
  {
    emit sendCollectionSnapshotSuccess(result);
  }
  else
  {
    emit sendCollectionSnapshotFailed(result);
  }

  finishedSendingCollectionSnapshotReport(success);
}

void CasServer::finishedSendingCollectionSnapshotReport(bool success)
{
  emit sendCollectionSnapshotFinished();
}

QStringList CasServer::getBoothsOutOfSync(bool *ok)
{
  *ok = true;
  QStringList boothsOutOfSync;

  QStringList boothAddresses;
  foreach (QVariant v, dbMgr->getOnlyBoothInfo(this->getDeviceAddresses()))
  {
    QVariantMap booth = v.toMap();
    boothAddresses.append(booth["device_num"].toString());
  }

  // Compare the videos that are expected to be in the booths from cas-mgr.sqlite to each casplayer_*.sqlite videos table
  QMap<QString, QList<Movie> > boothsMissingMovies = dbMgr->getBoothsWithMissingMovies(boothAddresses, ok);

  if (*ok)
  {
    if (boothsMissingMovies.size())
    {
      QMapIterator<QString, QList<Movie> > i(boothsMissingMovies);
      while (i.hasNext())
      {
        i.next();
        boothsOutOfSync.append(settings->getValue(ARCADE_SUBNET).toString().append(i.key()));

        QStringList missingMovies;
        foreach (Movie m, i.value())
        {
          missingMovies.append(QString("UPC: %1 Channel #: %2").arg(m.UPC()).arg(m.ChannelNum()));
        }

        QLOG_ERROR() << QString("Booth %1 is missing the following movies: %2").arg(i.key()).arg(missingMovies.join(", "));
      }
    }
    else
    {
      QLOG_DEBUG() << "No booths are missing movies";
    }
  }
  else
  {
    *ok = false;
  }

  return boothsOutOfSync;
}

void CasServer::sendingCollectionError(QNetworkReply::NetworkError error)
{
  Q_UNUSED(error);

  QLOG_ERROR() << QString("Network error while sending collection report: %1").arg(netReplySendCollection->errorString());
}

void CasServer::abortConnection()
{
  QLOG_DEBUG() << "Aborting network connection due to timeout";

  if (netReplySendViews)
    netReplySendViews->abort();
  else if (netReplySendCollection)
    netReplySendCollection->abort();
  else if (netReplyUserAuth)
      netReplyUserAuth->abort();
  else
    QLOG_ERROR() << "No network connection appears to be valid, not aborting";
}

void CasServer::resetTimeout(qint64, qint64)
{
  // Restart connection timer
  connectionTimer->start();
}

void CasServer::userAuthorization(QString pass)
{
    QLOG_DEBUG() << QString("Sending request for user authorization...");

    QVariantMap data;

    data["action"] = "userAuthorization";
    data["passphrase"] = settings->getValue(CUSTOMER_PASSWORD, "", true).toString();
    data["password"] = pass;

    QJson::Serializer serializer;
    QByteArray jsonData = serializer.serialize(data);

    QUrl serverUrl(WEB_SERVICE_URL);
    QNetworkRequest request(serverUrl);


    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    netReplyUserAuth = netMgr->post(request, jsonData);
    connect(netReplyUserAuth, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(userAuthorizationError(QNetworkReply::NetworkError)));
    connect(netReplyUserAuth, SIGNAL(finished()), this, SLOT(receivedUserAuthorizationResponse()));
    connect(netReplyUserAuth, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(resetTimeout(qint64,qint64)));
    connect(netReplyUserAuth, SIGNAL(uploadProgress(qint64,qint64)), this, SLOT(resetTimeout(qint64,qint64)));

    // Start timer which will abort the connection when the time expires
    // Connecting the downloadProgress signal to the resetTimeout slot
    // allows resetting the timer everytime the progress changes
    connect(connectionTimer, SIGNAL(timeout()), this, SLOT(abortConnection()));
    connectionTimer->start();
}

void CasServer::userAuthorizationError(QNetworkReply::NetworkError error)
{
    Q_UNUSED(error);

    QLOG_ERROR() << QString("Network error while User Authorization: %1").arg(netReplyUserAuth->errorString());
}

void CasServer::receivedUserAuthorizationResponse()
{
    connectionTimer->stop();

    bool success = false;

    QString res;

    // The finished signal is always emitted, even if there was a network error
    if (netReplyUserAuth->error() == QNetworkReply::NoError)
    {
      QByteArray json = netReplyUserAuth->readAll();

      // Expected response is in JSON:
      // If success then the response will be:
      // { "success" : "description" }
      // If the server has a problem with what the drone sends, the response will be:
      // { "error" : "description" }

      QString response(json.constData());

      bool ok = false;
      QJson::Parser parser;
      QVariant var = parser.parse(json, &ok);

      if (ok)
      {
        QLOG_DEBUG() << QString("Received valid JSON response: %1").arg(response);

        if (var.toMap().contains("error"))
        {
          QLOG_ERROR() << QString("Server returned an error: %1").arg(var.toMap()["error"].toString());
          res = "Incorrect Password";
        }
        else if (var.toMap().contains("success"))
        {
          // success response
          QLOG_DEBUG() << QString("User Authorized . Server response: %1").arg(var.toMap()["success"].toString());
          success = true;
          res = "Login Successfull";
        }
        else
        {
          QLOG_ERROR() << QString("Unexpected JSON response");
        }
      }
      else
      {
        QLOG_ERROR() << QString("Did not receive valid JSON response: %1").arg(response);
      }
    }
    else
    {
        res = netReplyUserAuth->errorString();
    }

    netReplyUserAuth->deleteLater();
    netReplyUserAuth = 0;

    finishedUserAuthorization(success,res);
}

void CasServer::finishedUserAuthorization(bool success,QString response)
{
     emit userAuthorizationReceived(success,response);
}
