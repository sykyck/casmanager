#include <QVariant>
#include <QDateTime>
#include <QStringList>
#include <QSqlDriver>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include "databasemgr.h"
#include "qslog/QsLog.h"
#include "settings.h"
#include "qjson-backport/qjsondocument.h"
#include "qjson-backport/qjsonobject.h"
#include "qjson-backport/qjsonvalue.h"
#include "qjson-backport/qjsonarray.h"
#include "global.h"
#include "top-couchdb/couchdbserver.h"
#include "top-couchdb/couchdbquery.h"
#include <float.h>

// TODO: Move to settings
const QString PASSWORD_SALT = "09c64eb8667b604a106e52f668bfc2226225ad47";

DatabaseMgr::DatabaseMgr(QString dataPath, QObject *parent) : QObject(parent)
{
  this->dataPath = dataPath;

  couchDB = new CouchDB();

  // TODO: Move params to settings
  couchDB->setServerConfiguration("cas-server", 6984, "cas_admin", "7B445jh8axVFL2tAMoQtLBlg");
  couchDB->server()->setSecureConnection(true);
  couchDB->server()->baseURL(true);

  connect(couchDB, SIGNAL(installationChecked(CouchDBResponse)), this, SLOT(installationChecked(CouchDBResponse)));
  connect(couchDB, SIGNAL(databaseCreated(CouchDBResponse)), this, SLOT(databaseCreated(CouchDBResponse)));
  connect(couchDB, SIGNAL(databasesListed(CouchDBResponse)), this, SLOT(databasesListed(CouchDBResponse)));
  connect(couchDB, SIGNAL(documentCreated(CouchDBResponse)), this, SLOT(documentCreated(CouchDBResponse)));
  connect(couchDB, SIGNAL(documentUpdated(CouchDBResponse)), this, SLOT(documentUpdated(CouchDBResponse)));
  connect(couchDB, SIGNAL(documentDeleted(CouchDBResponse)), this, SLOT(documentDeleted(CouchDBResponse)));
  connect(couchDB, SIGNAL(databaseViewRetrieved(CouchDBResponse)), this, SLOT(databaseViewRetrieved(CouchDBResponse)));
  connect(couchDB, SIGNAL(documentRetrieved(CouchDBResponse)), this, SLOT(documentRetrieved(CouchDBResponse)));
  connect(couchDB, SIGNAL(bulkDocumentsDeleted(CouchDBResponse)), this, SLOT(bulkDocumentsDeleted(CouchDBResponse)));
}

void DatabaseMgr::setDataPath(QString dataPath)
{
  this->dataPath = dataPath;
}

bool DatabaseMgr::movieChangeDone(int year, int weekNum)
{
  bool done = false;

  QSqlQuery query(db);

  query.prepare("SELECT year, week_num FROM dvd_copy_jobs_history WHERE year = :year AND week_num = :week_num");
  query.bindValue(":year", QVariant(year));
  query.bindValue(":week_num", QVariant(weekNum));

  if (query.exec())
  {
    if (query.first())
    {
      done = true;
    }
  }

  return done;
}

bool DatabaseMgr::openDB(QString dbFile)
{
  // Find QSLite driver
  db = QSqlDatabase::addDatabase("QSQLITE", "main");

  db.setDatabaseName(dbFile);

  if (db.open())
    return true;
  else
    return false;
}

bool DatabaseMgr::verifyCasMgrDb()
{
  bool ok = true;

  QSqlQuery query(db);

  query.exec("SELECT name FROM sqlite_master WHERE name = 'dvd_copy_jobs'");

  if (!query.first())
  {
    QLOG_DEBUG() << "Creating dvd_copy_jobs table";

    if (!query.exec(DVD_COPY_JOBS_TABLE))
      ok = false;

    QLOG_DEBUG() << "Creating dvd_copy_jobs index";
    if (!query.exec(DVD_COPY_JOBS_INDEX))
      ok = false;
  }

  query.exec("SELECT name FROM sqlite_master WHERE name = 'dvd_copy_jobs_history'");

  if (!query.first())
  {
    QLOG_DEBUG() << "Creating dvd_copy_jobs_history table";

    if (!query.exec(DVD_COPY_JOBS_HISTORY_TABLE))
      ok = false;

    QLOG_DEBUG() << "Creating dvd_copy_jobs_history index";

    if (!query.exec(DVD_COPY_JOBS_HISTORY_INDEX))
      ok = false;
  }

  query.exec("SELECT name FROM sqlite_master WHERE name = 'settings'");

  if (!query.first())
  {
    QLOG_DEBUG() << "Creating settings table";

    if (!query.exec(SETTINGS_TABLE))
      ok = false;
  }

  query.exec("SELECT name FROM sqlite_master WHERE name = 'video_attributes'");

  if (!query.first())
  {
    QLOG_DEBUG() << "Creating video_attributes table";

    if (!query.exec(VIDEO_ATTRIBS_TABLE))
      ok = false;

    QLOG_DEBUG() << "Creating video_attributes index";

    if (!query.exec(VIDEO_ATTRIBS_INDEX))
      ok = false;
  }

  query.exec("SELECT name FROM sqlite_master WHERE name = 'videos'");

  if (!query.first())
  {
    QLOG_DEBUG() << "Creating videos table";

    if (!query.exec(VIDEOS_TABLE))
      ok = false;

    QLOG_DEBUG() << "Creating videos index";

    if (!query.exec(VIDEOS_INDEX))
      ok = false;
  }

  query.exec("SELECT name FROM sqlite_master WHERE name = 'daily_meters'");

  if (!query.first())
  {
    QLOG_DEBUG() << "Creating daily_meters table";

    if (!query.exec(DAILY_METERS_TABLE))
      ok = false;

    QLOG_DEBUG() << "Creating daily_meters index";
    if (!query.exec(DAILY_METERS_INDEX))
      ok = false;
  }

  query.exec("SELECT name FROM sqlite_master WHERE name = 'delete_upcs'");

  if (!query.first())
  {
    QLOG_DEBUG() << "Creating delete_upcs table";

    if (!query.exec(DELETE_UPCS_TABLE))
      ok = false;
  }

  query.exec("SELECT name FROM sqlite_master WHERE name = 'help_center'");

  if (!query.first())
  {
    QLOG_DEBUG() << "Creating help_center table";

    if (!query.exec(HELP_CENTER_TABLE))
      ok = false;
  }

  query.exec("SELECT name FROM sqlite_master WHERE name = 'least_viewed_videos'");

  if (!query.first())
  {
    QLOG_DEBUG() << "Creating least_viewed_videos table";

    if (!query.exec(LEAST_VIEWED_VIDEOS_TABLE))
      ok = false;

    QLOG_DEBUG() << "Creating least_viewed_videos index";

    if (!query.exec(LEAST_VIEWED_VIDEOS_INDEX))
      ok = false;
  }

  return ok;
}

bool DatabaseMgr::verifySharedDBs()
{
  bool ok = true;

  if (dataPath.isEmpty())
    ok = false;
  else
  {
    QSqlQuery query(db);

    // If the viewtimes database does not exist, it will be created now
    // By us creating the file, we will have r/w access. If a booth creates the file
    // then we wouldn't have write access
    if (query.exec(QString("ATTACH DATABASE '%1/viewtimes.sqlite' AS v").arg(dataPath)))
    {
      query.exec("SELECT name FROM v.sqlite_master WHERE name = 'view_times'");

      if (!query.first())
      {
        QLOG_DEBUG() << "Creating view_times table";

        if (!query.exec(VIEW_TIMES_TABLE))
          ok = false;

        QLOG_DEBUG() << "Creating view_times index";

        if (!query.exec(VIEW_TIMES_INDEX))
          ok = false;
      }

      query.exec("SELECT name FROM v.sqlite_master WHERE name = 'cleared_view_times'");

      if (!query.first())
      {
        QLOG_DEBUG() << "Creating cleared_view_times table";

        if (!query.exec(CLEARED_VIEW_TIMES_TABLE))
          ok = false;

        QLOG_DEBUG() << "Creating cleared_view_times index";

        if (!query.exec(CLEARED_VIEW_TIMES_INDEX))
          ok = false;
      }

      query.exec("SELECT name FROM v.sqlite_master WHERE name = 'top_5_videos'");

      if (!query.first())
      {
        QLOG_DEBUG() << "Creating top_5_videos table";

        if (!query.exec(TOP_5_VIDEOS_TABLE))
          ok = false;

        QLOG_DEBUG() << "Creating top_5_videos index";

        if (!query.exec(TOP_5_VIDEOS_INDEX))
          ok = false;
      }

      if (!query.exec("DETACH v"))
        ok = false;
    }

    if (query.exec(QString("ATTACH DATABASE '%1/moviechanges.sqlite' AS m").arg(dataPath)))
    {
      query.exec("SELECT name FROM m.sqlite_master WHERE name = 'movie_changes'");

      if (!query.first())
      {
        QLOG_DEBUG() << "Creating movie_changes table";

        if (!query.exec(MOVIE_CHANGES_TABLE))
          ok = false;

        QLOG_DEBUG() << "Creating movie_changes index";

        if (!query.exec(MOVIE_CHANGES_INDEX))
          ok = false;
      }

      query.exec("SELECT name FROM m.sqlite_master WHERE name = 'movie_changes_history'");

      if (!query.first())
      {
        QLOG_DEBUG() << "Creating movie_changes_history table";

        if (!query.exec(MOVIE_CHANGES_HISTORY_TABLE))
          ok = false;

        QLOG_DEBUG() << "Creating movie_changes_history index";

        if (!query.exec(MOVIE_CHANGES_HISTORY_INDEX))
          ok = false;
      }

      query.exec("SELECT name FROM m.sqlite_master WHERE name = 'movie_change_queue'");

      if (!query.first())
      {
        QLOG_DEBUG() << "Creating movie_change_queue table";

        if (!query.exec(MOVIE_CHANGE_QUEUE_TABLE))
          ok = false;

        QLOG_DEBUG() << "Creating movie_change_queue index";

        if (!query.exec(MOVIE_CHANGE_QUEUE_INDEX))
          ok = false;
      }

      if (!query.exec("DETACH m"))
        ok = false;
    }
  }

  return ok;
}

void DatabaseMgr::verifyCouchDB()
{
  // TODO: Check CouchDB connection and existence of cas database
  // Create cas database if it doesn't exist
  couchDB->checkInstallation();
}

void DatabaseMgr::closeDB()
{
  db.close();
}

QString DatabaseMgr::getValue(QString keyName, QVariant defaultValue, bool *ok)
{
  QSqlQuery query(db);
  QString value;

  *ok = false;

  query.prepare("SELECT data FROM settings WHERE key_name = :key_name");
  query.bindValue(":key_name", QVariant(keyName));

  if (query.exec())
  {
    if (query.isActive() && query.first())
    {
      value = query.value(0).toString();
      *ok = true;
    }
    else
    {
      //QLOG_DEBUG() << QString("'%1' not found, now adding").arg(keyName);

      // Looks like the key doesn't exist, if defaultValue is set then
      // try adding the record
      if (!defaultValue.isNull())
      {
        query.prepare("INSERT INTO settings (key_name, data) VALUES (:key_name, :data)");
        query.bindValue(":key_name", keyName);
        query.bindValue(":data", defaultValue);

        if (query.exec())
        {
          value = defaultValue.toString();
          *ok = true;
        }
      }
    }
  }

  return value;
}

bool DatabaseMgr::setValue(QString keyName, QString value)
{
  QSqlQuery query(db);

  query.prepare("UPDATE settings SET data = :data WHERE key_name = :key_name");
  query.bindValue(":data", QVariant(value));
  query.bindValue(":key_name", QVariant(keyName));

  if (query.exec())
  {
    //QLOG_DEBUG() << QString("Updated '%1' in settings table").arg(keyName);
    return true;
  }
  else
  {
    QLOG_ERROR() << QString("Could not update '%1' in settings table").arg(keyName);
    return false;
  }
}

bool DatabaseMgr::insertDvdCopyTask(DvdCopyTask task)
{
  bool ok = true;

  QSqlQuery query(db);

  query.prepare("INSERT INTO dvd_copy_jobs (seq_num, year, week_num, upc, category) VALUES (:seq_num, :year, :week_num, :upc, :category)");
  query.bindValue(":seq_num", QVariant(task.SeqNum()));
  query.bindValue(":year", QVariant(task.Year()));
  query.bindValue(":week_num", QVariant(task.WeekNum()));
  query.bindValue(":upc", QVariant(task.UPC()));
  query.bindValue(":category", QVariant(task.Category()));

  if (query.exec())
  {
    QLOG_DEBUG() << "Inserted dvd_copy_jobs record";
  }
  else
  {
    QLOG_ERROR() << "Could not insert record into dvd_copy_jobs table. Error:" << query.lastError().text();
    ok = false;
  }

  return ok;
}

DvdCopyTask DatabaseMgr::getDvdCopyTask(QString upc)
{
  DvdCopyTask task;

  QSqlQuery query(db);

  query.prepare("SELECT year, week_num, upc, title, producer, category, genre, front_cover_file, back_cover_file, file_length, status, complete, seq_num, transcode_status FROM dvd_copy_jobs WHERE upc = :upc");
  query.bindValue(":upc", QVariant(upc));

  if (query.exec())
  {
    if (query.next())
    {
      task.setYear(query.value(0).toInt());
      task.setWeekNum(query.value(1).toInt());
      task.setUPC(query.value(2).toString());
      task.setTitle(query.value(3).toString());
      task.setProducer(query.value(4).toString());
      task.setCategory(query.value(5).toString());
      task.setSubcategory(query.value(6).toString());
      task.setFrontCover(query.value(7).toString());
      task.setBackCover(query.value(8).toString());
      task.setFileLength(query.value(9).toULongLong());
      task.setTaskStatus(DvdCopyTask::Status(query.value(10).toInt()));
      task.setSeqNum(query.value(12).toInt());
      task.setTranscodeStatus(DvdCopyTask::Status(query.value(13).toInt()));
    }
    else
    {
      QLOG_DEBUG() << "Could not find UPC:" << upc << "in dvd_copy_jobs";
    }
  }
  else
  {
    QLOG_ERROR() << "Could not query dvd_copy_jobs. Error:" << query.lastError().text();
  }

  return task;
}

bool DatabaseMgr::importDvdCopyTask(DvdCopyTask task)
{
  bool ok = true;

  QSqlQuery query(db);

  query.prepare("INSERT INTO dvd_copy_jobs (year, week_num, upc, title, producer, category, genre, front_cover_file, back_cover_file, file_length, status, complete, transcode_status) VALUES (:year, :week_num, :upc, :title, :producer, :category, :genre, :front_cover_file, :back_cover_file, :file_length, :status, :complete, :transcode_status)");
  query.bindValue(":year", QVariant(task.Year()));
  query.bindValue(":week_num", QVariant(task.WeekNum()));
  query.bindValue(":upc", QVariant(task.UPC()));
  query.bindValue(":title", QVariant(task.Title()));
  query.bindValue(":producer", QVariant(task.Producer()));
  query.bindValue(":category", QVariant(task.Category()));
  query.bindValue(":genre", QVariant(task.Subcategory()));
  query.bindValue(":front_cover_file", QVariant(task.FrontCover()));
  query.bindValue(":back_cover_file", QVariant(task.BackCover()));
  query.bindValue(":file_length", QVariant(task.FileLength()));
  query.bindValue(":status", QVariant(task.TaskStatus()));
  query.bindValue(":complete", QVariant(task.Complete() ? 1 : 0));
  query.bindValue(":transcode_status", QVariant(task.TranscodeStatus()));

  if (query.exec())
  {
    QLOG_DEBUG() << "Imported dvd_copy_jobs record";
  }
  else
  {
    QLOG_ERROR() << "Could not insert record into dvd_copy_jobs table. Error:" << query.lastError().text();
    ok = false;
  }

  return ok;
}

bool DatabaseMgr::movieSetExists(int year, int weekNum)
{
  bool foundMovieSet = false;

  QSqlQuery query(db);

  query.prepare("SELECT u.year, u.week_num FROM (SELECT year, week_num FROM dvd_copy_jobs UNION SELECT year, week_num FROM dvd_copy_jobs_history) u WHERE u.year = :year AND u.week_num = :week_num");
  query.bindValue(":year", QVariant(year));
  query.bindValue(":week_num", QVariant(weekNum));

  if (query.exec())
  {
    if (query.first())
    {
      foundMovieSet = true;
    }
  }

  return foundMovieSet;
}

MovieChangeInfo DatabaseMgr::getLatestMovieSet()
{
  MovieChangeInfo latestMovieChange;

  QSqlQuery query(db);

  query.prepare("SELECT u.year, u.week_num, u.numVideos FROM (SELECT year, week_num, COUNT(*) numVideos FROM dvd_copy_jobs GROUP BY year, week_num UNION SELECT year, week_num, COUNT(*) numVideos FROM dvd_copy_jobs_history GROUP BY year, week_num) u ORDER BY u.year DESC, u.week_num DESC");

  if (query.exec())
  {
    if (query.first())
    {
      latestMovieChange.setYear(query.value(0).toInt());
      latestMovieChange.setWeekNum(query.value(1).toInt());
      latestMovieChange.setNumVideos(query.value(2).toInt());
    }
  }

  return latestMovieChange;
}

QVariantList DatabaseMgr::performDailyCollection(QStringList addresses)
{
  QSqlQuery query(db);
  QDateTime collectionTime = QDateTime::currentDateTime();
  //int numMissingDailyMeters = 0; // Number of booths missing a previous daily meter record
  //int numBoothsCollected = 0; // Number of booths that had a collection performed between last daily meters and now
  //int numStaleDailyMeters = 0; // Number of booths that don't have a daily meter record for yesterday (different than not having any daily meter record)

  //QVariantList previousMeters;
  //QVariantList currentMeters;

  QVariantList dailyMetersList;
  QVariantMap previousMeters;
  QVariantMap currentMeters;

  QString casplayerDbAlias = Global::randomString(8);

  foreach (QString deviceAddress, addresses)
  {
    previousMeters.clear();
    currentMeters.clear();

    QFile dbFile(QString("%1/casplayer_%2.sqlite").arg(dataPath).arg(deviceAddress));

    if (dbFile.exists() && dbFile.size() > 0)
    {
      // Get the most recent daily meters for current booth address
      query.prepare("SELECT device_num, captured, last_collection, current_credits, total_credits, current_preview_credits, total_preview_credits, current_cash, total_cash, current_cc_charges, total_cc_charges, current_prog_credits, total_prog_credits FROM daily_meters WHERE device_num = :device_num ORDER BY captured DESC LIMIT 1");
      query.bindValue(":device_num", deviceAddress);

      if (query.exec())
      {
        if (query.first())
        {
          previousMeters["device_num"] = query.value(0).toInt();
          previousMeters["captured"] = QDateTime::fromString(query.value(1).toString(), "yyyy-MM-dd hh:mm:ss");;
          previousMeters["last_collection"] = QDateTime::fromString(query.value(2).toString(), "yyyy-MM-dd hh:mm:ss");;
          previousMeters["current_credits"] = query.value(3).toInt();
          previousMeters["total_credits"] = query.value(4).toInt();
          previousMeters["current_preview_credits"] = query.value(5).toInt();
          previousMeters["total_preview_credits"] = query.value(6).toInt();
          previousMeters["current_cash"] = query.value(7).toInt();
          previousMeters["total_cash"] = query.value(8).toInt();
          previousMeters["current_cc_charges"] = query.value(9).toInt();
          previousMeters["total_cc_charges"] = query.value(10).toInt();
          previousMeters["current_prog_credits"] = query.value(11).toInt();
          previousMeters["total_prog_credits"] = query.value(12).toInt();
        }
        /*else
        {
          numMissingDailyMeters++;
        }*/
      }
      else
      {
        // Error executing query on daily_meters table
        QLOG_ERROR() << QString("Could not query the daily_meters table. Error: %1").arg(query.lastError().text());
      }

      QLOG_DEBUG() << QString("Attaching to database on: %1/casplayer_%2.sqlite...").arg(dataPath).arg(deviceAddress);

      if (query.exec(QString("ATTACH DATABASE '%1/casplayer_%2.sqlite' AS %3").arg(dataPath).arg(deviceAddress).arg(casplayerDbAlias)))
      {
        // Get latest meters from booth
        query.prepare(QString("SELECT device_num, last_collection, current_credits, total_credits, current_preview_credits, total_preview_credits, current_cash, total_cash, current_cc_charges, total_cc_charges, current_prog_credits, total_prog_credits, (SELECT data FROM %1.settings WHERE key_name = 'device_alias') device_alias FROM %2.devices WHERE device_num = :device_num").arg(casplayerDbAlias).arg(casplayerDbAlias));
        query.bindValue(":device_num", deviceAddress);

        if (query.exec())
        {
          if (query.first())
          {
            currentMeters["device_num"] = query.value(0).toInt();
            currentMeters["captured"] = collectionTime;
            currentMeters["last_collection"] = QDateTime::fromString(query.value(1).toString(), "yyyy-MM-dd hh:mm:ss");;
            currentMeters["current_credits"] = query.value(2).toInt();
            currentMeters["total_credits"] = query.value(3).toInt();
            currentMeters["current_preview_credits"] = query.value(4).toInt();
            currentMeters["total_preview_credits"] = query.value(5).toInt();
            currentMeters["current_cash"] = query.value(6).toInt();
            currentMeters["total_cash"] = query.value(7).toInt();
            currentMeters["current_cc_charges"] = query.value(8).toInt();
            currentMeters["total_cc_charges"] = query.value(9).toInt();
            currentMeters["current_prog_credits"] = query.value(10).toInt();
            currentMeters["total_prog_credits"] = query.value(11).toInt();
            currentMeters["device_alias"] = query.value(12).toString();

            // Insert latest meters into daily_meters table
            query.prepare(QString("INSERT INTO daily_meters SELECT device_num, :device_alias AS device_alias, :captured AS captured, last_collection, current_credits, total_credits, current_preview_credits, total_preview_credits, current_cash, total_cash, current_cc_charges, total_cc_charges, current_prog_credits, total_prog_credits FROM %1.devices WHERE device_num = :device_num").arg(casplayerDbAlias));
            query.bindValue(":device_alias", currentMeters["device_alias"]);
            query.bindValue(":captured", collectionTime.toString("yyyy-MM-dd hh:mm:ss"));
            query.bindValue(":device_num", deviceAddress);

            if (!query.exec())
            {
              QLOG_ERROR() << QString("Could not insert device record into daily_meters table. Error: %1").arg(query.lastError().text());
            }

            // if most recent daily collection found for current booth
            if (previousMeters.count() > 0)
            {         
              // find difference between last daily meters and current
              currentMeters["current_credits"] = currentMeters["total_credits"].toInt() - previousMeters["total_credits"].toInt();
              currentMeters["current_preview_credits"] = currentMeters["total_preview_credits"].toInt() - previousMeters["total_preview_credits"].toInt();
              currentMeters["current_cash"] = currentMeters["total_cash"].toInt() - previousMeters["total_cash"].toInt();
              currentMeters["current_cc_charges"] = currentMeters["total_cc_charges"].toInt() - previousMeters["total_cc_charges"].toInt();
              currentMeters["current_prog_credits"] = currentMeters["total_prog_credits"].toInt() - previousMeters["total_prog_credits"].toInt();

              // If the difference in seconds between now and the last time daily meters were captured
              // is > 36 hours (36*60*60) then add note
              if (currentMeters["captured"].toDateTime().toTime_t() -
                  previousMeters["captured"].toDateTime().toTime_t() > 36 * 60 * 60)
              {
                // The last daily meters captured from this booth is older than 36 hours
                //numStaleDailyMeters++;

                currentMeters["note"] = QString("The last time daily meters were sent from this booth was more than 36 hours ago: %1.").arg(previousMeters["captured"].toDateTime().toString("yyyy-MM-dd hh:mm:ss"));
              }


              dailyMetersList.append(currentMeters);
            }
            else
            {
              currentMeters["note"] = "These are the first daily meters sent from this booth. There are no prior meters to compare to.";

              // This is the first daily meters captured for this booth so use the current daily meters as-is
              dailyMetersList.append(currentMeters);
            }
          }
          else
          {
            QLOG_ERROR() << QString("No record found in devices table of %1/casplayer_%2.sqlite").arg(dataPath).arg(deviceAddress);
          }
        }
        else
        {
          QLOG_ERROR() << QString("Could not query devices table. Error: %1").arg(query.lastError().text());
        }
      }
      else
        QLOG_ERROR() << QString("Could not attach to database. Error: %1").arg(query.lastError().text());

      // Detach the other database
      if (query.exec(QString("DETACH %1").arg(casplayerDbAlias)))
      {
        QLOG_DEBUG() << QString("Detached database: %1/casplayer_%2.sqlite...").arg(dataPath).arg(deviceAddress);
      }
      else
      {
        QLOG_ERROR() << QString("Could not detach database: %1/casplayer_%2.sqlite. Error: %3").arg(dataPath).arg(deviceAddress).arg(query.lastError().text());
      }
    }
    else
    {
      QLOG_DEBUG() << QString("Database does not exist: %1/casplayer_%2.sqlite").arg(dataPath).arg(deviceAddress);
    }
  }

  return dailyMetersList;
}

// Intended to be executed after a daily meter collection. Gets the difference between the meters just collected
// and those from daysInCollection ago. The same logic and data structure is used as the daily meter collections
QVariantList DatabaseMgr::getCollectionSnapshot(QStringList addresses, quint8 daysInCollection, QDate startDate)
{
  if (!startDate.isValid()) {
      startDate = QDate::currentDate();
  }

  QSqlQuery query(db);
  QDate startTime = startDate.addDays(-daysInCollection);
  QDate endDate = startDate;
  QVariantList metersList;
  QVariantMap previousMeters;
  QVariantMap currentMeters;

  foreach (QString deviceAddress, addresses)
  {
    previousMeters.clear();
    currentMeters.clear();

    // Get the daily meters for current booth address daysInCollection ago
    query.prepare("SELECT device_num, captured, last_collection, total_credits, total_preview_credits, total_cash, total_cc_charges, total_prog_credits FROM daily_meters WHERE device_num = :device_num AND date(captured) = :captured LIMIT 1");
    query.bindValue(":device_num", deviceAddress);
    query.bindValue(":captured", startTime.toString("yyyy-MM-dd"));

    if (query.exec())
    {
      if (query.first())
      {
        previousMeters["device_num"] = query.value(0).toInt();
        previousMeters["captured"] = QDateTime::fromString(query.value(1).toString(), "yyyy-MM-dd hh:mm:ss");;
        previousMeters["last_collection"] = QDateTime::fromString(query.value(2).toString(), "yyyy-MM-dd hh:mm:ss");;
        previousMeters["total_credits"] = query.value(3).toInt();
        previousMeters["total_preview_credits"] = query.value(4).toInt();
        previousMeters["total_cash"] = query.value(5).toInt();
        previousMeters["total_cc_charges"] = query.value(6).toInt();
        previousMeters["total_prog_credits"] = query.value(7).toInt();
      }
    }
    else
    {
      // Error executing query on daily_meters table
      QLOG_ERROR() << QString("Could not query the daily_meters table. Error: %1").arg(query.lastError().text());
    }

    QLOG_DEBUG() << "Device Address: " << deviceAddress;

    // Get the daily meters for current booth address and specified endDate
    query.prepare("SELECT device_num, captured, last_collection, total_credits, total_preview_credits, total_cash, total_cc_charges, total_prog_credits, device_alias, current_credits, current_preview_credits, current_cash, current_cc_charges, current_prog_credits FROM daily_meters WHERE device_num = :device_num AND date(captured) = :captured LIMIT 1");
    query.bindValue(":device_num", deviceAddress);
    query.bindValue(":captured", endDate.toString("yyyy-MM-dd"));

    if (query.exec())
    {
      if (query.first())
      {
        currentMeters["device_num"] = query.value(0).toInt();
        currentMeters["start_time"] = previousMeters["captured"].toDateTime();
        currentMeters["captured"] = QDateTime::fromString(query.value(1).toString(), "yyyy-MM-dd hh:mm:ss");;
        currentMeters["last_collection"] = QDateTime::fromString(query.value(2).toString(), "yyyy-MM-dd hh:mm:ss");;
        currentMeters["total_credits"] = query.value(3).toInt();
        currentMeters["total_preview_credits"] = query.value(4).toInt();
        currentMeters["total_cash"] = query.value(5).toInt();
        currentMeters["total_cc_charges"] = query.value(6).toInt();
        currentMeters["total_prog_credits"] = query.value(7).toInt();
        currentMeters["device_alias"] = query.value(8).toString();

        // if previous daily collection found for current booth
        if (previousMeters.count() > 0)
        {
          // find difference between last meters and current
          currentMeters["current_credits"] = currentMeters["total_credits"].toInt() - previousMeters["total_credits"].toInt();
          currentMeters["current_preview_credits"] = currentMeters["total_preview_credits"].toInt() - previousMeters["total_preview_credits"].toInt();
          currentMeters["current_cash"] = currentMeters["total_cash"].toInt() - previousMeters["total_cash"].toInt();
          currentMeters["current_cc_charges"] = currentMeters["total_cc_charges"].toInt() - previousMeters["total_cc_charges"].toInt();
          currentMeters["current_prog_credits"] = currentMeters["total_prog_credits"].toInt() - previousMeters["total_prog_credits"].toInt();

          // Make sure we're not looking at the same record for this booth
          if (currentMeters["captured"].toDateTime() == previousMeters["captured"].toDateTime())
          {
            QLOG_DEBUG() << QString("These meters are from %1 day(s) ago and there is nothing more recent from booth %2").arg(daysInCollection).arg(deviceAddress);
            currentMeters["note"] = QString("These meters are from %1 day(s) ago and there is nothing more recent!").arg(daysInCollection);
          }

          metersList.append(currentMeters);
        }
        else
        {
          currentMeters["current_credits"] = query.value(9).toInt();
          currentMeters["current_preview_credits"] = query.value(10).toInt();
          currentMeters["current_cash"] = query.value(11).toInt();
          currentMeters["current_cc_charges"] = query.value(12).toInt();
          currentMeters["current_prog_credits"] = query.value(13).toInt();

          currentMeters["note"] = QString("No meters were captured %1 day(s) ago from this booth, this is probably a new booth.").arg(daysInCollection);

          // This is the first collection for this booth so use the current daily meters as-is
          metersList.append(currentMeters);
        }
      }
      else
      {
        QLOG_ERROR() << QString("No record found for booth %1 in daily_meters").arg(deviceAddress);
      }
    }
  }

  return metersList;
}

bool DatabaseMgr::updateDvdCopyTask(DvdCopyTask task)
{
  bool ok = true;

  QSqlQuery query(db);

  // year, week_num, upc, title, producer, category, genre, front_cover_file, back_cover_file, file_length, status, complete
  query.prepare("UPDATE dvd_copy_jobs SET title = :title, producer = :producer, category = :category, genre = :genre, front_cover_file = :front_cover_file, back_cover_file = :back_cover_file, file_length = :file_length, status = :status, complete = :complete, transcode_status = :transcode_status WHERE year = :year AND week_num = :week_num AND upc = :upc");
  query.bindValue(":title", QVariant(task.Title()));
  query.bindValue(":producer", QVariant(task.Producer()));
  query.bindValue(":category", QVariant(task.Category()));
  query.bindValue(":genre", QVariant(task.Subcategory()));
  query.bindValue(":front_cover_file", QVariant(task.FrontCover()));
  query.bindValue(":back_cover_file", QVariant(task.BackCover()));
  query.bindValue(":file_length", QVariant(task.FileLength()));
  query.bindValue(":status", QVariant(task.TaskStatus()));
  query.bindValue(":complete", QVariant(task.Complete() ? 1 : 0));
  query.bindValue(":year", QVariant(task.Year()));
  query.bindValue(":week_num", QVariant(task.WeekNum()));
  query.bindValue(":upc", QVariant(task.UPC()));
  query.bindValue(":transcode_status", QVariant(task.TranscodeStatus()));

  if (query.exec())
  {
    QLOG_DEBUG() << "Updated dvd_copy_jobs record";
  }
  else
  {
    QLOG_ERROR() << "Could not update record in dvd_copy_jobs table. Error:" << query.lastError().text();
    ok = false;
  }

  return ok;
}

bool DatabaseMgr::deleteDvdCopyTask(DvdCopyTask task)
{
  bool ok = true;

  QSqlQuery query(db);

  // year, week_num, upc, title, producer, category, genre, front_cover_file, back_cover_file, file_length, status, complete
  query.prepare("DELETE FROM dvd_copy_jobs WHERE year = :year AND week_num = :week_num AND upc = :upc");
  query.bindValue(":year", QVariant(task.Year()));
  query.bindValue(":week_num", QVariant(task.WeekNum()));
  query.bindValue(":upc", QVariant(task.UPC()));

  if (query.exec())
  {
    QLOG_DEBUG() << "Deleted dvd_copy_jobs record";
  }
  else
  {
    QLOG_ERROR() << "Could not delete record in dvd_copy_jobs table. Error:" << query.lastError().text();
    ok = false;
  }

  return ok;
}

QList<DvdCopyTask> DatabaseMgr::getDvdCopyTasks(DvdCopyTask::Status status, bool notEqual)
{
  QList<DvdCopyTask> dvdCopyTaskList;

  QSqlQuery query(db);


  if (notEqual)
    query.prepare("SELECT year, week_num, upc, title, producer, category, genre, front_cover_file, back_cover_file, file_length, status, complete, seq_num, transcode_status FROM dvd_copy_jobs WHERE status <> :status ORDER BY year, week_num, seq_num");
  else
    query.prepare("SELECT year, week_num, upc, title, producer, category, genre, front_cover_file, back_cover_file, file_length, status, complete, seq_num, transcode_status FROM dvd_copy_jobs WHERE status = :status ORDER BY year, week_num, seq_num");

  query.bindValue(":status", QVariant(status));

  if (query.exec())
  {
    while (query.next())
    {
      DvdCopyTask task;
      task.setYear(query.value(0).toInt());
      task.setWeekNum(query.value(1).toInt());
      task.setUPC(query.value(2).toString());
      task.setTitle(query.value(3).toString());
      task.setProducer(query.value(4).toString());
      task.setCategory(query.value(5).toString());
      task.setSubcategory(query.value(6).toString());
      task.setFrontCover(query.value(7).toString());
      task.setBackCover(query.value(8).toString());
      task.setFileLength(query.value(9).toULongLong());
      task.setTaskStatus(DvdCopyTask::Status(query.value(10).toInt()));
      task.setSeqNum(query.value(12).toInt());
      task.setTranscodeStatus(DvdCopyTask::Status(query.value(13).toInt()));

      dvdCopyTaskList.append(task);
    }
  }
  else
  {
    QLOG_ERROR() << "Could not query dvd_copy_jobs. Error:" << query.lastError().text();
  }

  return dvdCopyTaskList;
}

QList<DvdCopyTask> DatabaseMgr::getDvdCopyTasks(int year, int weekNum, bool includeHistory)
{
  QList<DvdCopyTask> dvdCopyTaskList;

  QSqlQuery query(db);

  QString sql = "SELECT year, week_num, upc, title, producer, category, genre, front_cover_file, back_cover_file, file_length, status, complete, seq_num, transcode_status FROM dvd_copy_jobs WHERE year = :year AND week_num = :week_num ORDER BY seq_num";
  if (includeHistory)
    sql = "SELECT year, week_num, upc, title, producer, category, genre, front_cover_file, back_cover_file, file_length, status, complete, seq_num, transcode_status FROM (SELECT * FROM dvd_copy_jobs UNION SELECT * FROM dvd_copy_jobs_history) WHERE year = :year AND week_num = :week_num ORDER BY seq_num";

  query.prepare(sql);
  query.bindValue(":year", QVariant(year));
  query.bindValue(":week_num", QVariant(weekNum));

  if (query.exec())
  {
    while (query.next())
    {
      DvdCopyTask task;
      task.setYear(query.value(0).toInt());
      task.setWeekNum(query.value(1).toInt());
      task.setUPC(query.value(2).toString());
      task.setTitle(query.value(3).toString());
      task.setProducer(query.value(4).toString());
      task.setCategory(query.value(5).toString());
      task.setSubcategory(query.value(6).toString());
      task.setFrontCover(query.value(7).toString());
      task.setBackCover(query.value(8).toString());
      task.setFileLength(query.value(9).toULongLong());
      task.setTaskStatus(DvdCopyTask::Status(query.value(10).toInt()));
      task.setSeqNum(query.value(12).toInt());
      task.setTaskStatus(DvdCopyTask::Status(query.value(13).toInt()));

      dvdCopyTaskList.append(task);
    }
  }
  else
  {
    QLOG_ERROR() << "Could not query dvd_copy_jobs. Error:" << query.lastError().text();
  }

  return dvdCopyTaskList;
}

bool DatabaseMgr::insertDeleteUPC(QString upc)
{
  // Insert UPC into table, if UPC already exists it will no be inserted since the upc field is also the primary key
  bool ok = true;

  QSqlQuery query(db);

  query.prepare("INSERT INTO delete_upcs (upc) VALUES (:upc)");
  query.bindValue(":upc", upc);

  if (query.exec())
  {
    QLOG_DEBUG() << "Inserted delete_upcs record";
  }
  else
  {
    // Only consider the execution an error if it does not have to do with a duplicate UPC
    if (!query.lastError().text().contains("unique", Qt::CaseInsensitive))
    {
      QLOG_ERROR() << "Could not insert record into delete_upcs table. Error:" << query.lastError().text();
      ok = false;
    }
  }

  return ok;
}

// Delete one or more records from delete_upcs table
bool DatabaseMgr::deleteUPCs(QStringList upcList)
{
  QStringList upcs;
  bool ok = true;

  // Enclose each UPC in single quotes
  foreach (QString upc, upcList)
  {
    upcs.append(QString("'%1'").arg(upc));
  }

  QSqlQuery query(db);

  if (query.exec(QString("DELETE FROM delete_upcs WHERE upc IN (%1)").arg(upcs.join(","))))
  {
    QLOG_DEBUG() << QString("Deleted UPCs: %1 from delete_upcs table").arg(upcs.join(","));
  }
  else
  {
    QLOG_ERROR() << QString("Could not delete UPCs: %1 from delete_upcs table. Error: %2").arg(upcs.join(",")).arg(query.lastError().text());
    ok = false;
  }

  return ok;
}

QStringList DatabaseMgr::getDeleteUPCs()
{
  QStringList upcList;

  QSqlQuery query(db);

  if (query.exec("SELECT upc FROM delete_upcs"))
  {
    while (query.next())
    {
      upcList.append(query.value(0).toString());
    }
  }
  else
  {
    QLOG_ERROR() << "Could not query delete_upcs. Error:" << query.lastError().text();
  }

  return upcList;
}

QList<DvdCopyTask> DatabaseMgr::getVideosToTranscode()
{
  QList<DvdCopyTask> dvdCopyTaskList;

  QSqlQuery query(db);


  query.prepare("SELECT year, week_num, upc, title, producer, category, genre, front_cover_file, back_cover_file, file_length, status, complete, seq_num, transcode_status FROM dvd_copy_jobs WHERE status = :status AND transcode_status = :transcode_status ORDER BY year, week_num, seq_num");
  query.bindValue(":status", QVariant(DvdCopyTask::Finished));
  query.bindValue(":transcode_status", QVariant(DvdCopyTask::Pending));

  if (query.exec())
  {
    while (query.next())
    {
      DvdCopyTask task;
      task.setYear(query.value(0).toInt());
      task.setWeekNum(query.value(1).toInt());
      task.setUPC(query.value(2).toString());
      task.setTitle(query.value(3).toString());
      task.setProducer(query.value(4).toString());
      task.setCategory(query.value(5).toString());
      task.setSubcategory(query.value(6).toString());
      task.setFrontCover(query.value(7).toString());
      task.setBackCover(query.value(8).toString());
      task.setFileLength(query.value(9).toULongLong());
      task.setTaskStatus(DvdCopyTask::Status(query.value(10).toInt()));
      task.setSeqNum(query.value(12).toInt());
      task.setTranscodeStatus(DvdCopyTask::Status(query.value(13).toInt()));

      dvdCopyTaskList.append(task);
    }
  }
  else
  {
    QLOG_ERROR() << "Could not query dvd_copy_jobs. Error:" << query.lastError().text();
  }

  return dvdCopyTaskList;
}

QList<DvdCopyTask> DatabaseMgr::getVideosBeingTranscoded()
{
  QList<DvdCopyTask> dvdCopyTaskList;

  QSqlQuery query(db);


  query.prepare("SELECT year, week_num, upc, title, producer, category, genre, front_cover_file, back_cover_file, file_length, status, complete, seq_num, transcode_status FROM dvd_copy_jobs WHERE status = :status AND transcode_status = :transcode_status ORDER BY year, week_num, seq_num");
  query.bindValue(":status", QVariant(DvdCopyTask::Finished));
  query.bindValue(":transcode_status", QVariant(DvdCopyTask::Working));

  if (query.exec())
  {
    while (query.next())
    {
      DvdCopyTask task;
      task.setYear(query.value(0).toInt());
      task.setWeekNum(query.value(1).toInt());
      task.setUPC(query.value(2).toString());
      task.setTitle(query.value(3).toString());
      task.setProducer(query.value(4).toString());
      task.setCategory(query.value(5).toString());
      task.setSubcategory(query.value(6).toString());
      task.setFrontCover(query.value(7).toString());
      task.setBackCover(query.value(8).toString());
      task.setFileLength(query.value(9).toULongLong());
      task.setTaskStatus(DvdCopyTask::Status(query.value(10).toInt()));
      task.setSeqNum(query.value(12).toInt());
      task.setTranscodeStatus(DvdCopyTask::Status(query.value(13).toInt()));

      dvdCopyTaskList.append(task);
    }
  }
  else
  {
    QLOG_ERROR() << "Could not query dvd_copy_jobs. Error:" << query.lastError().text();
  }

  return dvdCopyTaskList;
}

QList<MovieChangeInfo> DatabaseMgr::getAvailableMovieChangeSets(bool includeIncompleteSets, QList<MovieChangeInfo> excludeList)
{
  QList<MovieChangeInfo> movieChangeInfoList;

  QSqlQuery query(db);

  // Get list of unique year and week numbers and count of videos linked to it
  if (query.exec("SELECT COUNT(*) numVideos, year, week_num FROM dvd_copy_jobs GROUP BY year, week_num ORDER BY year, week_num"))
  {
    while (query.next())
    {
      MovieChangeInfo movieChange;

      movieChange.setYear(query.value(1).toInt());
      movieChange.setWeekNum(query.value(2).toInt());
      movieChange.setNumVideos(query.value(0).toInt());

      bool includeMovieChange = true;

      // If this record is in the list of exclusions, then skip it
      foreach (MovieChangeInfo m, excludeList)
      {
        if (m.Year() == movieChange.Year() &&
            m.WeekNum() == movieChange.WeekNum())
        {
          includeMovieChange = false;
          break;
        }
      }

      if (includeMovieChange)
      {
        // If flag not set then only include complete movie change sets
        if (!includeIncompleteSets)
        {
          QSqlQuery query2(db);

          // Get count of videos that have complete metadata, successfully copied and transcoded for year and week num
          query2.prepare("SELECT COUNT(*) num FROM dvd_copy_jobs WHERE complete = 1 AND status = 2 AND transcode_status = 2 AND year = :year AND week_num = :week_num");
          query2.bindValue(":year", movieChange.Year());
          query2.bindValue(":week_num", movieChange.WeekNum());

          if (query2.exec())
          {
            if (query2.isActive() && query2.first())
            {
              if (movieChange.NumVideos() == query2.value(0).toInt())
              {
                //QLOG_DEBUG() << QString("Match. Year %1, Week # %2, NumVideo %3, Video Count %4").arg(year).arg(weekNum).arg(numVideos).arg(query2.value(0).toInt());

                movieChange.setDvdCopyTaskList(getDvdCopyTasks(movieChange.Year(), movieChange.WeekNum()));

                movieChangeInfoList.append(movieChange);
              }
              //else
              //  QLOG_DEBUG() << QString("No match. Year %1, Week # %2, NumVideo %3, Video Count %4").arg(year).arg(weekNum).arg(numVideos).arg(query2.value(0).toInt());
            }
            else
            {
              QLOG_ERROR() << "Could not query dvd_copy_jobs. Error:" << query2.lastError().text();
            }

          }
          else
          {
            QLOG_ERROR() << "Could not query dvd_copy_jobs. Error:" << query2.lastError().text();
          }
        }
        else
        {
          // Add movie change set to list, regardless if it's complete
          movieChange.setDvdCopyTaskList(this->getDvdCopyTasks(movieChange.Year(), movieChange.WeekNum()));

          movieChangeInfoList.append(movieChange);
        }

      } // endif not in exclude list
      else
      {
        QLOG_DEBUG() << QString("Year: %1, Week #: %2 is in the exclude list").arg(movieChange.Year()).arg(movieChange.WeekNum());
      }
    }
  }
  else
  {
    QLOG_ERROR() << "Could not query dvd_copy_jobs. Error:" << query.lastError().text();
  }

  return movieChangeInfoList;
}

QList<MovieChangeInfo> DatabaseMgr::getExportedMovieChangeSets()
{
  QList<MovieChangeInfo> movieChangeInfoList;

  QSqlQuery query(db);

  // Get list of unique year and week numbers and count of videos linked to it
  if (query.exec("SELECT COUNT(*) numVideos, year, week_num FROM dvd_copy_jobs_history GROUP BY year, week_num ORDER BY year, week_num"))
  {
    while (query.next())
    {
      MovieChangeInfo movieChange;

      movieChange.setYear(query.value(1).toInt());
      movieChange.setWeekNum(query.value(2).toInt());
      movieChange.setNumVideos(query.value(0).toInt());

      QSqlQuery query2(db);

      // Get count of videos that have complete metadata, successfully copied and transcoded for year and week num
      query2.prepare("SELECT COUNT(*) num FROM dvd_copy_jobs_history WHERE complete = 1 AND status = 2 AND transcode_status = 2 AND year = :year AND week_num = :week_num");
      query2.bindValue(":year", movieChange.Year());
      query2.bindValue(":week_num", movieChange.WeekNum());

      if (query2.exec())
      {
        if (query2.isActive() && query2.first())
        {
          if (movieChange.NumVideos() == query2.value(0).toInt())
          {
            movieChange.setDvdCopyTaskList(this->getDvdCopyTasks(movieChange.Year(), movieChange.WeekNum(), true));

            movieChangeInfoList.append(movieChange);
          }
        }
        else
        {
          QLOG_ERROR() << "Could not query dvd_copy_jobs_history. Error:" << query2.lastError().text();
        }

      }
      else
      {
        QLOG_ERROR() << "Could not query dvd_copy_jobs_history. Error:" << query2.lastError().text();
      }
    }
  }
  else
  {
    QLOG_ERROR() << "Could not query dvd_copy_jobs_history. Error:" << query.lastError().text();
  }

  return movieChangeInfoList;
}

QString DatabaseMgr::getDocumentation()
{
  QString documentation;

  QSqlQuery query(db);

  query.prepare("SELECT entry FROM help_center WHERE entry_type = 3");

  if (query.exec())
  {
    if (query.next())
    {
      documentation = query.value(0).toString();
    }
  }
  else
  {
    QLOG_ERROR() << "Could not query help_center for documentation. Error:" << query.lastError().text();
  }

  return documentation;
}

QString DatabaseMgr::getChangelog()
{
  QString changelog;

  QSqlQuery query(db);

  // TODO: Change this to expect one or more changelog type records and sort in descending order
  // this is going to complicate things because then we'll need each entry to NOT be a full HTML document
  query.prepare("SELECT entry FROM help_center WHERE entry_type = 2");

  if (query.exec())
  {
    if (query.next())
    {
      changelog = query.value(0).toString();
    }
  }
  else
  {
    QLOG_ERROR() << "Could not query help_center for changelog. Error:" << query.lastError().text();
  }

  return changelog;
}

QStringList DatabaseMgr::getTipsOfTheDay()
{
  QStringList tipList;

  QSqlQuery query(db);

  query.prepare("SELECT entry FROM help_center WHERE entry_type = 1 ORDER BY help_id");

  if (query.exec())
  {
    while (query.next())
    {
      tipList.append(query.value(0).toString());
    }
  }
  else
  {
    QLOG_ERROR() << "Could not query help_center for tips of the day. Error:" << query.lastError().text();
  }

  return tipList;
}

int DatabaseMgr::getNumMoviesSent(bool *ok)
{
  int numMovies = 0;
  *ok = true;

  QSqlQuery query(db);

  if (query.exec("SELECT COUNT(*) FROM dvd_copy_jobs_history"))
  {
    if (query.first())
    {
      numMovies = query.value(0).toInt();
    }
    else
    {
      QLOG_ERROR() << "No result when getting count of dvd_copy_jobs_history records";
      *ok = false;
    }
  }
  else
  {
    QLOG_ERROR() << "Could not query dvd_copy_jobs_history. Error:" << query.lastError().text();
    *ok = false;
  }

  return numMovies;
}

Movie DatabaseMgr::getCurrentMovieAtChannel(int channelNum, bool *ok)
{
  Movie movie;
  *ok = true;

  QSqlQuery query(db);

  query.prepare(QString("SELECT upc, title, producer, category, genre FROM videos WHERE channel_num = :channel_num"));
  query.bindValue(":channel_num", channelNum);

  if (query.exec())
  {
    if (query.first())
    {
      movie.setUPC(query.value(0).toString());
      movie.setTitle(query.value(1).toString());
      movie.setProducer(query.value(2).toString());
      movie.setCategory(query.value(3).toString());
      movie.setSubcategory(query.value(4).toString());
      movie.setChannelNum(channelNum);
    }
    else
    {
      QLOG_DEBUG() << QString("No record found in videos for channel number : %1").arg(channelNum);
    }
  }
  else
  {
    QLOG_ERROR() << QString("Could not query videos. Error: %1").arg(query.lastError().text());
    *ok = false;
  }

  return movie;
}

QDateTime DatabaseMgr::getLatestMovieChangeDate(QString address)
{
  QDateTime movieChangeDate;

  QSqlQuery query(db);

  QString moviechangesDbAlias = Global::randomString(8);

  QString movieChangeDbName = "moviechanges.sqlite";

  if (!address.isEmpty())
    movieChangeDbName = QString("moviechanges_%1.sqlite").arg(address);

  QLOG_DEBUG() << QString("Attaching to database on: %1/%2...").arg(dataPath).arg(movieChangeDbName);

  if (query.exec(QString("ATTACH DATABASE '%1/%2' AS %3").arg(dataPath).arg(movieChangeDbName).arg(moviechangesDbAlias)))
  {
    if (query.exec(QString("SELECT download_date FROM %1.movie_changes_history GROUP BY download_date ORDER BY download_date DESC LIMIT 1").arg(moviechangesDbAlias)))
    {
      if (query.first())
      {
        movieChangeDate = QDateTime::fromString(query.value(0).toString(), "yyyy-MM-dd hh:mm:ss");
      }
    }
    else
    {
      QLOG_ERROR() << QString("Could not query %1.movie_changes_history. Error: %2").arg(moviechangesDbAlias).arg(query.lastError().text());
    }

    // Detach the other database
    if (query.exec(QString("DETACH %1").arg(moviechangesDbAlias)))
    {
      QLOG_DEBUG() << QString("Detached database: %1/%2...").arg(dataPath).arg(movieChangeDbName);
    }
    else
    {
      QLOG_ERROR() << QString("Could not detach database: %1/%2. Error: %3").arg(dataPath).arg(movieChangeDbName).arg(query.lastError().text());
    }

  } // endif attached to database
  else
  {
    QLOG_ERROR() << QString("Could not attach to database: %1/%2. Error: %3. Database alias: %4").arg(dataPath).arg(movieChangeDbName).arg(query.lastError().text()).arg(moviechangesDbAlias);
  }

  return movieChangeDate;
}

void DatabaseMgr::getCards()
{
  // Get all documents of type card
  couchDB->retrieveView("cas", "_design/app", "allCards");
}

void DatabaseMgr::getCard(QString cardNum)
{
  couchDB->retrieveDocument("cas", cardNum, "getCard");
}

void DatabaseMgr::addCard(ArcadeCard &card)
{
  QJsonDocument d(card.toQJsonObject());
  couchDB->createDocument("cas", d.toJson(true), "addCard");
}

void DatabaseMgr::updateCard(ArcadeCard &card)
{
  couchDB->updateDocument("cas", card.getCardNum(), QJsonDocument(card.toQJsonObject()).toJson(true), "updateCard");
}

void DatabaseMgr::deleteCard(ArcadeCard &card)
{
  couchDB->deleteDocument("cas", card.getCardNum(), card.getRevID(), "deleteCard");
}

void DatabaseMgr::bulkDeleteCards(QList<ArcadeCard> &cards)
{
  QJsonArray jsonArray;
  foreach (ArcadeCard card, cards) {
    QJsonObject d;
    // These are the minimum fields needed to delete a document, note a _deleted property is added to indicate
    // to CouchDB that the document should be deleted
    d["_id"] = card.getCardNum();
    d["_rev"] = card.getRevID();
    d["_deleted"] = true;

    jsonArray.append(d);
  }

  QJsonObject docs;
  docs["docs"] = jsonArray;

  QJsonDocument d(docs);
  QByteArray payload = d.toJson(true);
  couchDB->bulkDocumentsDelete("cas", payload, "bulkDeleteCards");
}

void DatabaseMgr::getUsers()
{
  // Get all documents of type user
  couchDB->retrieveView("cas", "_design/app", "allUsers");
}

void DatabaseMgr::addUser(UserAccount &userAccount)
{
  QString password = userAccount.getPassword();

  QCryptographicHash crypto(QCryptographicHash::Sha1);
  password.append(PASSWORD_SALT);
  crypto.addData(password.toLatin1());
  QByteArray hash = crypto.result();
  QString hashedPassword = QString(hash.toHex());

  userAccount.setPassword(hashedPassword);
  userAccount.setActivated(QDateTime::currentDateTime());
  QJsonDocument d(userAccount.toQJsonObject());
  couchDB->createDocument("cas", d.toJson(true), "addUser");
}

void DatabaseMgr::updateUser(UserAccount &userAccount)
{
  // Updating a user account involves changing the password, nothing else can be modified

  QString password = userAccount.getPassword();

  QCryptographicHash crypto(QCryptographicHash::Sha1);
  password.append(PASSWORD_SALT);
  crypto.addData(password.toLatin1());
  QByteArray hash = crypto.result();
  QString hashedPassword = QString(hash.toHex());

  userAccount.setPassword(hashedPassword);

  couchDB->updateDocument("cas", userAccount.getUsername(), QJsonDocument(userAccount.toQJsonObject()).toJson(true), "updateUser");
}

void DatabaseMgr::deleteUser(UserAccount &userAccount)
{
  couchDB->deleteDocument("cas", userAccount.getUsername(), userAccount.getRevID(), "deleteUser");
}

void DatabaseMgr::getExpiredCards()
{
  // Get all documents of type cards that are >= 30 days old
  couchDB->retrieveView("cas", "_design/app", "getExpiredCards");
}

void DatabaseMgr::addBoothStatus(QJsonDocument &doc)
{
  couchDB->createDocument("cas", doc.toJson(true), "addBoothStatus");
}

void DatabaseMgr::deleteBoothStatus(QString id, QString revID)
{
  couchDB->deleteDocument("cas", id, revID, "deleteBoothStatus");
}

void DatabaseMgr::getBoothStatuses()
{
  // Get all documents of type booth_status
  couchDB->retrieveView("cas", "_design/app", "getBoothStatus");
}

QStringList DatabaseMgr::getMovieChangeDates()
{
  QStringList movieChangeDates;

  QSqlQuery query(db);

  QString moviechangesDbAlias = Global::randomString(8);

  QLOG_DEBUG() << QString("Attaching to database on: %1/moviechanges.sqlite...").arg(dataPath);

  if (query.exec(QString("ATTACH DATABASE '%1/moviechanges.sqlite' AS %2").arg(dataPath).arg(moviechangesDbAlias)))
  {
    if (query.exec(QString("SELECT download_date FROM %1.movie_changes_history GROUP BY download_date ORDER BY download_date DESC").arg(moviechangesDbAlias)))
    {
      while (query.next())
      {
        movieChangeDates.append(query.value(0).toString());
      }
    }
    else
    {
      QLOG_ERROR() << QString("Could not query %1.movie_changes. Error: %2").arg(moviechangesDbAlias).arg(query.lastError().text());
    }

    // Detach the other database
    if (query.exec(QString("DETACH %1").arg(moviechangesDbAlias)))
    {
      QLOG_DEBUG() << QString("Detached database: %1/moviechanges.sqlite").arg(dataPath);
    }
    else
    {
      QLOG_ERROR() << QString("Could not detach database: %1/moviechanges.sqlite. Error: %2").arg(dataPath).arg(query.lastError().text());
    }

  } // endif attached to database
  else
  {
    QLOG_ERROR() << QString("Could not attach to database: %1/moviechanges.sqlite. Error: %2. Database alias: %3").arg(dataPath).arg(query.lastError().text()).arg(moviechangesDbAlias);
  }

  return movieChangeDates;
}

QVariantList DatabaseMgr::getMovieChangeDetails(QString movieChangeDate)
{
  QVariantList movieChangeDetails;

  QSqlQuery query(db);

  QString moviechangesDbAlias = Global::randomString(8);

  QLOG_DEBUG() << QString("Attaching to database on: %1/moviechanges.sqlite...").arg(dataPath);

  if (query.exec(QString("ATTACH DATABASE '%1/moviechanges.sqlite' AS %2").arg(dataPath).arg(moviechangesDbAlias)))
  {
    // Get the movie change details from the movie_changes_history table and link to the the dvd_copy_jobs_history for metadata
    query.prepare(QString("SELECT upc, title, producer, category, genre, channel_num, IFNULL(previous_movie, '(Not Available)') previousMovie FROM %1.movie_changes_history m INNER JOIN dvd_copy_jobs_history h ON m.dirname = h.upc WHERE m.download_date = :download_date").arg(moviechangesDbAlias));
    //query.prepare(QString("SELECT upc, title, producer, category, genre, channel_num FROM %1.movie_changes_history m INNER JOIN dvd_copy_jobs_history h ON m.dirname = h.upc WHERE m.download_date = :download_date").arg(moviechangesDbAlias));
    query.bindValue(":download_date", movieChangeDate);

    if (query.exec())
    {
      while (query.next())
      {
        QVariantMap record;

        record["upc"] = query.value(0).toString();
        record["title"] = query.value(1).toString();
        record["producer"] = query.value(2).toString();
        record["category"] = query.value(3).toString();
        record["subcategory"] = query.value(4).toString();
        record["channel_num"] = query.value(5).toInt();
        record["previous_movie"] = query.value(6).toString();

        movieChangeDetails.append(record);
      }

      // Assume this is a delete only movie change so don't inner join dvd_copy_jobs_history
      if (movieChangeDetails.count() == 0)
      {
        query.prepare(QString("SELECT 'N/A' as upc, 'N/A' as title, 'N/A' as producer, 'N/A' as category, 'N/A' as genre, channel_num, IFNULL(previous_movie, '(Not Available)') previousMovie FROM %1.movie_changes_history WHERE download_date = :download_date").arg(moviechangesDbAlias));
        query.bindValue(":download_date", movieChangeDate);

        if (query.exec())
        {
          while (query.next())
          {
            QVariantMap record;

            record["upc"] = query.value(0).toString();
            record["title"] = query.value(1).toString();
            record["producer"] = query.value(2).toString();
            record["category"] = query.value(3).toString();
            record["subcategory"] = query.value(4).toString();
            record["channel_num"] = query.value(5).toInt();
            record["previous_movie"] = query.value(6).toString();

            movieChangeDetails.append(record);
          }
        }
        else
        {
          QLOG_ERROR() << QString("Could not query %1.movie_changes_history. Error: %2").arg(moviechangesDbAlias).arg(query.lastError().text());
        }
      }
    }
    else
    {
      QLOG_ERROR() << QString("Could not query %1.movie_changes_history. Error: %2").arg(moviechangesDbAlias).arg(query.lastError().text());
    }

    // Detach the other database
    if (query.exec(QString("DETACH %1").arg(moviechangesDbAlias)))
    {
      QLOG_DEBUG() << QString("Detached database: %1/moviechanges.sqlite").arg(dataPath);
    }
    else
    {
      QLOG_ERROR() << QString("Could not detach database: %1/moviechanges.sqlite. Error: %2").arg(dataPath).arg(query.lastError().text());
    }

  } // endif attached to database
  else
  {
    QLOG_ERROR() << QString("Could not attach to database: %1/moviechanges.sqlite. Error: %2. Database alias: %3").arg(dataPath).arg(query.lastError().text()).arg(moviechangesDbAlias);
  }

  return movieChangeDetails;
}

DvdCopyTask DatabaseMgr::getArchivedDvdCopyTask(QString upc)
{
  DvdCopyTask task;

  QSqlQuery query(db);

  query.prepare("SELECT year, week_num, upc, title, producer, category, genre, front_cover_file, back_cover_file, file_length, status, complete, seq_num, transcode_status FROM dvd_copy_jobs_history WHERE upc = :upc");
  query.bindValue(":upc", QVariant(upc));

  if (query.exec())
  {
    if (query.next())
    {
      task.setYear(query.value(0).toInt());
      task.setWeekNum(query.value(1).toInt());
      task.setUPC(query.value(2).toString());
      task.setTitle(query.value(3).toString());
      task.setProducer(query.value(4).toString());
      task.setCategory(query.value(5).toString());
      task.setSubcategory(query.value(6).toString());
      task.setFrontCover(query.value(7).toString());
      task.setBackCover(query.value(8).toString());
      task.setFileLength(query.value(9).toULongLong());
      task.setTaskStatus(DvdCopyTask::Status(query.value(10).toInt()));
      task.setSeqNum(query.value(12).toInt());
      task.setTranscodeStatus(DvdCopyTask::Status(query.value(13).toInt()));
    }
    else
    {
      QLOG_DEBUG() << "Could not find UPC:" << upc << "in dvd_copy_jobs_history";
    }
  }
  else
  {
    QLOG_ERROR() << "Could not query dvd_copy_jobs_history. Error:" << query.lastError().text();
  }

  return task;
}

// maxChannels is from settings and is needed because this information may have changed since when these movie change sets were saved
// Records are returned in the order they were added to the queue, the first item is the first one that was added to the queue
QList<MovieChangeInfo> DatabaseMgr::getMovieChangeQueue(int maxChannels)
{
  QList<MovieChangeInfo> movieChangeQueue;

  QSqlQuery query(db);

  QString movieChangesDbAlias = Global::randomString(8);

  QLOG_DEBUG() << QString("Attaching to database on: %1/moviechanges.sqlite...").arg(dataPath);

  if (query.exec(QString("ATTACH DATABASE '%1/moviechanges.sqlite' AS %2").arg(dataPath).arg(movieChangesDbAlias)))
  {
    // Status: pending, copying, failed, retrying, finished (shouldn't be in table)


    if (query.exec(QString("SELECT year, week_num, override_viewtimes, first_channel, last_channel, status FROM %1.movie_change_queue ORDER BY queue_id").arg(movieChangesDbAlias)))
    {
      while (query.next())
      {
        MovieChangeInfo m;

        m.setYear(query.value(0).toInt());
        m.setWeekNum(query.value(1).toInt());
        m.setOverrideViewtimes(query.value(2).toInt() == 1);
        m.setFirstChannel(query.value(3).toInt());
        m.setLastChannel(query.value(4).toInt());
        m.setStatus(MovieChangeInfo::MovieChangeStatus(query.value(5).toInt()));
        m.setMaxChannels(maxChannels);

        // Get the movies in the movie change set, include the history table because, depending on the status, the
        // movies may have already been archived
        m.setDvdCopyTaskList(this->getDvdCopyTasks(m.Year(), m.WeekNum(), true));
        m.setNumVideos(m.DvdCopyTaskList().count());

        movieChangeQueue.append(m);
      }
    }
    else
    {
      QLOG_ERROR() << QString("Could not query %1.movie_change_queue. Error: %2").arg(movieChangesDbAlias).arg(query.lastError().text());
    }

    // Detach the other database
    if (query.exec(QString("DETACH %1").arg(movieChangesDbAlias)))
    {
      QLOG_DEBUG() << QString("Detached database: %1/moviechanges.sqlite").arg(dataPath);
    }
    else
    {
      QLOG_ERROR() << QString("Could not detach database: %1/moviechanges.sqlite. Error: %2").arg(dataPath).arg(query.lastError().text());
    }

  } // endif attached to database
  else
  {
    QLOG_ERROR() << QString("Could not attach to database: %1/moviechanges.sqlite. Error: %2. Database alias: %3").arg(dataPath).arg(query.lastError().text()).arg(movieChangesDbAlias);
  }

  return movieChangeQueue;
}

bool DatabaseMgr::insertMovieChangeQueue(MovieChangeInfo m)
{
  bool success = false;

  QSqlQuery query(db);

  QString movieChangesDbAlias = Global::randomString(8);

  QLOG_DEBUG() << QString("Attaching to database on: %1/moviechanges.sqlite...").arg(dataPath);

  if (query.exec(QString("ATTACH DATABASE '%1/moviechanges.sqlite' AS %2").arg(dataPath).arg(movieChangesDbAlias)))
  {
    query.prepare(QString("INSERT INTO %1.movie_change_queue (year, week_num, override_viewtimes, first_channel, last_channel, status) VALUES (:year, :week_num, :override_viewtimes, :first_channel, :last_channel, :status)").arg(movieChangesDbAlias));
    query.bindValue(":year", m.Year());
    query.bindValue(":week_num", m.WeekNum());
    query.bindValue(":override_viewtimes", (m.OverrideViewtimes() ? 1 : 0));
    query.bindValue(":first_channel", m.FirstChannel());
    query.bindValue(":last_channel", m.LastChannel());
    query.bindValue(":status", m.Status());

    if (query.exec())
    {
      success = true;
    }
    else
    {
      QLOG_ERROR() << QString("Could not insert record into %1.movie_change_queue. Error: %2").arg(movieChangesDbAlias).arg(query.lastError().text());
    }

    // Detach the other database
    if (query.exec(QString("DETACH %1").arg(movieChangesDbAlias)))
    {
      QLOG_DEBUG() << QString("Detached database: %1/moviechanges.sqlite").arg(dataPath);
    }
    else
    {
      QLOG_ERROR() << QString("Could not detach database: %1/moviechanges.sqlite. Error: %2").arg(dataPath).arg(query.lastError().text());
    }

  } // endif attached to database
  else
  {
    QLOG_ERROR() << QString("Could not attach to database: %1/moviechanges.sqlite. Error: %2. Database alias: %3").arg(dataPath).arg(query.lastError().text()).arg(movieChangesDbAlias);
  }

  return success;
}

bool DatabaseMgr::updateStatusMovieChangeQueue(MovieChangeInfo m)
{
  bool success = false;

  QSqlQuery query(db);

  QString movieChangesDbAlias = Global::randomString(8);

  QLOG_DEBUG() << QString("Attaching to database on: %1/moviechanges.sqlite...").arg(dataPath);

  if (query.exec(QString("ATTACH DATABASE '%1/moviechanges.sqlite' AS %2").arg(dataPath).arg(movieChangesDbAlias)))
  {
    query.prepare(QString("UPDATE %1.movie_change_queue SET status = :status WHERE year = :year AND week_num = :week_num").arg(movieChangesDbAlias));
    query.bindValue(":status", m.Status());
    query.bindValue(":year", m.Year());
    query.bindValue(":week_num", m.WeekNum());

    if (query.exec())
    {
      success = true;
    }
    else
    {
      QLOG_ERROR() << QString("Could not update record in %1.movie_change_queue. Error: %2").arg(movieChangesDbAlias).arg(query.lastError().text());
    }

    // Detach the other database
    if (query.exec(QString("DETACH %1").arg(movieChangesDbAlias)))
    {
      QLOG_DEBUG() << QString("Detached database: %1/moviechanges.sqlite").arg(dataPath);
    }
    else
    {
      QLOG_ERROR() << QString("Could not detach database: %1/moviechanges.sqlite. Error: %2").arg(dataPath).arg(query.lastError().text());
    }

  } // endif attached to database
  else
  {
    QLOG_ERROR() << QString("Could not attach to database: %1/moviechanges.sqlite. Error: %2. Database alias: %3").arg(dataPath).arg(query.lastError().text()).arg(movieChangesDbAlias);
  }

  return success;
}

bool DatabaseMgr::deleteMovieChangeQueue(MovieChangeInfo m)
{
  bool success = false;

  QSqlQuery query(db);

  QString movieChangesDbAlias = Global::randomString(8);

  QLOG_DEBUG() << QString("Attaching to database on: %1/moviechanges.sqlite...").arg(dataPath);

  if (query.exec(QString("ATTACH DATABASE '%1/moviechanges.sqlite' AS %2").arg(dataPath).arg(movieChangesDbAlias)))
  {
    query.prepare(QString("DELETE FROM %1.movie_change_queue WHERE year = :year AND week_num = :week_num").arg(movieChangesDbAlias));
    query.bindValue(":year", m.Year());
    query.bindValue(":week_num", m.WeekNum());

    if (query.exec())
    {
      success = true;
    }
    else
    {
      QLOG_ERROR() << QString("Could not delete record from %1.movie_change_queue. Error: %2").arg(movieChangesDbAlias).arg(query.lastError().text());
    }

    // Detach the other database
    if (query.exec(QString("DETACH %1").arg(movieChangesDbAlias)))
    {
      QLOG_DEBUG() << QString("Detached database: %1/moviechanges.sqlite").arg(dataPath);
    }
    else
    {
      QLOG_ERROR() << QString("Could not detach database: %1/moviechanges.sqlite. Error: %2").arg(dataPath).arg(query.lastError().text());
    }

  } // endif attached to database
  else
  {
    QLOG_ERROR() << QString("Could not attach to database: %1/moviechanges.sqlite. Error: %2. Database alias: %3").arg(dataPath).arg(query.lastError().text()).arg(movieChangesDbAlias);
  }

  return success;
}

bool DatabaseMgr::clearMovieChangeQueue()
{
  bool success = false;

  QSqlQuery query(db);

  QString movieChangesDbAlias = Global::randomString(8);

  QLOG_DEBUG() << QString("Attaching to database on: %1/moviechanges.sqlite...").arg(dataPath);

  if (query.exec(QString("ATTACH DATABASE '%1/moviechanges.sqlite' AS %2").arg(dataPath).arg(movieChangesDbAlias)))
  {
    if (query.exec(QString("DELETE FROM %1.movie_change_queue").arg(movieChangesDbAlias)))
    {
      success = true;
    }
    else
    {
      QLOG_ERROR() << QString("Could not delete all records from %1.movie_change_queue. Error: %2").arg(movieChangesDbAlias).arg(query.lastError().text());
    }

    // Detach the other database
    if (query.exec(QString("DETACH %1").arg(movieChangesDbAlias)))
    {
      QLOG_DEBUG() << QString("Detached database: %1/moviechanges.sqlite").arg(dataPath);
    }
    else
    {
      QLOG_ERROR() << QString("Could not detach database: %1/moviechanges.sqlite. Error: %2").arg(dataPath).arg(query.lastError().text());
    }

  } // endif attached to database
  else
  {
    QLOG_ERROR() << QString("Could not attach to database: %1/moviechanges.sqlite. Error: %2. Database alias: %3").arg(dataPath).arg(query.lastError().text()).arg(movieChangesDbAlias);
  }

  return success;
}

// attribType: 1 = Producer, 2 = category, 3 = subcategory
QStringList DatabaseMgr::getVideoAttributes(int attribType)
{
  QStringList valueList;

  QSqlQuery query(db);

  query.prepare("SELECT value FROM video_attributes WHERE attribute_type = :type ORDER BY value");
  query.bindValue(":type", QVariant(attribType));

  if (query.exec())
  {
    while (query.next())
    {
      valueList.append(query.value(0).toString());
    }
  }
  else
  {
    QLOG_ERROR() << "Could not query video_attributes. Error:" << query.lastError().text();
  }

  return valueList;
}

int DatabaseMgr::getHighestChannelNum(bool *ok)
{
  int channelNum = 0;
  *ok = true;

  QSqlQuery query(db);

  if (query.exec(QString("SELECT MAX(channel_num) FROM videos")))
  {
    if (query.first())
    {
      if (!query.value(0).isNull())
        channelNum = query.value(0).toInt();
      else
        QLOG_DEBUG() << "No records in videos table";
    }
    else
    {
      QLOG_ERROR() << "No record found for highest channel number";
      *ok = false;
    }
  }
  else
  {
    QLOG_ERROR() << QString("Could not query videos table. Error: %1").arg(query.lastError().text());
    *ok = false;
  }

  return channelNum;
}

qreal DatabaseMgr::getLowestFreeDiskSpace(QStringList deviceAddressList, bool *ok)
{
  qreal currentLowest = FLT_MAX;
  *ok = true;

  foreach (QVariant v, this->getBoothInfo(deviceAddressList))
  {
    QVariantMap booth = v.toMap();

    currentLowest = qMin(booth["free_space_gb"].toDouble(), currentLowest);
  }

  if (currentLowest == FLT_MAX)
  {
    QLOG_ERROR() << QString("Could not determine the lowest free disk space: %1").arg(currentLowest);
    *ok = false;
    currentLowest = 0;
  }

  return currentLowest;
}

QList<Movie> DatabaseMgr::getViewTimesByCategory(QString category, int year, int weekNum, bool *ok)
{
  QList<Movie> movieList;
  *ok = true;

  QSqlQuery query(db);

  QString viewtimesDbAlias = Global::randomString(8);

  QLOG_DEBUG() << QString("Attaching to database on: %1/viewtimes.sqlite...").arg(dataPath);

  if (query.exec(QString("ATTACH DATABASE '%1/viewtimes.sqlite' AS %2").arg(dataPath).arg(viewtimesDbAlias)))
  {
    // Sort view times by play time, date in so the least popular and oldest videos are at the top
    query.prepare(QString("SELECT channel_num, SUM(current_play_time) playTime, MAX(file_length) fileSize, title, category, MIN(date_in) dateIn FROM %1.view_times WHERE year = :year AND week_num = :week_num AND category = :category GROUP BY channel_num, title, category ORDER BY playTime, dateIn").arg(viewtimesDbAlias));
    query.bindValue(":year", QVariant(year));
    query.bindValue(":week_num", QVariant(weekNum));
    query.bindValue(":category", QVariant(category));

    if (query.exec())
    {
      while (query.next())
      {
        Movie m;

        m.setChannelNum(query.value(0).toInt());
        m.setCurrentPlayTime(query.value(1).toInt());
        m.setFileLength(query.value(2).toLongLong());
        m.setTitle(query.value(3).toString());
        m.setCategory(query.value(4).toString());

        movieList.append(m);
      }
    }
    else
    {
      QLOG_ERROR() << QString("Could not query %1.view_times. Error: %2").arg(viewtimesDbAlias).arg(query.lastError().text());
      *ok = false;
    }

    // Detach the other database
    if (query.exec(QString("DETACH %1").arg(viewtimesDbAlias)))
    {
      QLOG_DEBUG() << QString("Detached database: %1/viewtimes.sqlite").arg(dataPath);
    }
    else
    {
      QLOG_ERROR() << QString("Could not detach database: %1/viewtimes.sqlite. Error: %2").arg(dataPath).arg(query.lastError().text());
      *ok = false;
    }

  } // endif attached to database
  else
  {
    QLOG_ERROR() << QString("Could not attach to database: %1/viewtimes.sqlite. Error: %2. Database alias: %3").arg(dataPath).arg(query.lastError().text()).arg(viewtimesDbAlias);
    *ok = false;
  }

  return movieList;
}
QStringList DatabaseMgr::getVideoCategories(bool *ok)
{
  QStringList categoryList;
  *ok = true;

  QSqlQuery query(db);

  if (query.exec(QString("SELECT category FROM videos GROUP BY category ORDER BY category")))
  {
    while (query.next())
    {
      categoryList.append(query.value(0).toString());
    }
  }
  else
  {
    QLOG_ERROR() << QString("Could not query videos table. Error: %1").arg(query.lastError().text());
    *ok = false;
  }

  return categoryList;
}

bool DatabaseMgr::mergeViewTimes(int year, int weekNum, QString deviceAddress)
{
  bool ok = true;

  QSqlQuery query(db);

  QString viewtimesSrcDbAlias = Global::randomString(8);
  QString viewtimesDestDbAlias = Global::randomString(8);

  QLOG_DEBUG() << QString("Attaching to database on: %1/viewtimes_%2.sqlite...").arg(dataPath).arg(deviceAddress);

  // Connect to viewtimes_<deviceAddress>.sqlite (source)
  if (query.exec(QString("ATTACH DATABASE '%1/viewtimes_%2.sqlite' AS %3").arg(dataPath).arg(deviceAddress).arg(viewtimesSrcDbAlias)))
  {
    QLOG_DEBUG() << QString("Attaching to database on: %1/viewtimes.sqlite...").arg(dataPath);

    // Connect to viewtimes.sqlite (destination)
    if (query.exec(QString("ATTACH DATABASE '%1/viewtimes.sqlite' AS %2").arg(dataPath).arg(viewtimesDestDbAlias)))
    {
      query.prepare(QString("DELETE FROM %1.view_times WHERE year = :year AND week_num = :week_num AND device_num = :device_num").arg(viewtimesDestDbAlias));
      query.bindValue(":year", QVariant(year));
      query.bindValue(":week_num", QVariant(weekNum));
      query.bindValue(":device_num", QVariant(deviceAddress));

      if (!query.exec())
      {
        QLOG_ERROR() << QString("Could not delete existing records from %1.view_times. Error: %2").arg(viewtimesDestDbAlias).arg(query.lastError().text());
        ok = false;
      }

      if (ok)
      {
        QLOG_DEBUG() << "Deleted any existing view times for current week # from this address";
        query.prepare(QString("INSERT INTO %1.view_times SELECT * FROM %2.view_times WHERE year = :year AND week_num = :week_num AND device_num = :device_num").arg(viewtimesDestDbAlias).arg(viewtimesSrcDbAlias));

        query.bindValue(":year", QVariant(year));
        query.bindValue(":week_num", QVariant(weekNum));
        query.bindValue(":device_num", QVariant(deviceAddress));

        if (query.exec())
        {
          QLOG_DEBUG() << "Finished inserting video records";
        } // endif insert into view_times succeeded
        else
        {
          QLOG_ERROR() << QString("Could not insert records into %1.view_times. Error: %2").arg(viewtimesDestDbAlias).arg(query.lastError().text());
          ok = false;
        }
      }

      // Detach the other databases
      if (query.exec(QString("DETACH %1").arg(viewtimesSrcDbAlias)))
      {
        QLOG_DEBUG() << QString("Detached database: %1/viewtimes_%2.sqlite...").arg(dataPath).arg(deviceAddress);
      }
      else
      {
        QLOG_ERROR() << QString("Could not detach database: %1/viewtimes_%2.sqlite. Error: %3").arg(dataPath).arg(deviceAddress).arg(query.lastError().text());
        ok = false;
      }

      if (query.exec(QString("DETACH %1").arg(viewtimesDestDbAlias)))
      {
        QLOG_DEBUG() << QString("Detached database: %1/viewtimes.sqlite").arg(dataPath);
      }
      else
      {
        QLOG_ERROR() << QString("Could not detach database: %1/viewtimes.sqlite. Error: %2").arg(dataPath).arg(query.lastError().text());
        ok = false;
      }

    } // endif attached to viewtimes.sqlite
    else
    {
      QLOG_ERROR() << QString("Could not attach to database: %1/viewtimes.sqlite. Error: %2. Database alias: %3").arg(dataPath).arg(query.lastError().text()).arg(viewtimesDestDbAlias);
      ok = false;

      // Detach database here since we couldn't attach to destination
      if (query.exec(QString("DETACH %1").arg(viewtimesSrcDbAlias)))
      {
        QLOG_DEBUG() << QString("Detached database: %1/viewtimes_%2.sqlite...").arg(dataPath).arg(deviceAddress);
      }
      else
      {
        QLOG_ERROR() << QString("Could not detach database: %1/viewtimes_%2.sqlite. Error: %3").arg(dataPath).arg(deviceAddress).arg(query.lastError().text());
        ok = false;
      }
    }

  } // endif attached to viewtimes_<deviceNum>.sqlite
  else
  {
    QLOG_ERROR() << QString("Could not attach to database: %1/viewtimes_%2.sqlite. Error: %3. Database alias: %4").arg(dataPath).arg(deviceAddress).arg(query.lastError().text()).arg(viewtimesSrcDbAlias);
    ok = false;
  }

  return ok;
}

bool DatabaseMgr::mergeClearedViewTimes(int year, int weekNum, QString deviceAddress)
{
  bool ok = true;

  QSqlQuery query(db);

  QString viewtimesSrcDbAlias = Global::randomString(8);
  QString viewtimesDestDbAlias = Global::randomString(8);

  QLOG_DEBUG() << QString("Attaching to database on: %1/viewtimes_%2.sqlite...").arg(dataPath).arg(deviceAddress);

  // Connect to viewtimes_<deviceAddress>.sqlite
  if (query.exec(QString("ATTACH DATABASE '%1/viewtimes_%2.sqlite' AS %3").arg(dataPath).arg(deviceAddress).arg(viewtimesSrcDbAlias)))
  {
    QLOG_DEBUG() << QString("Attaching to database on: %1/viewtimes.sqlite...").arg(dataPath);

    // Connect to viewtimes.sqlite
    if (query.exec(QString("ATTACH DATABASE '%1/viewtimes.sqlite' AS %2").arg(dataPath).arg(viewtimesDestDbAlias)))
    {
      query.prepare(QString("DELETE FROM %1.cleared_view_times WHERE year = :year AND week_num = :week_num AND device_num = :device_num").arg(viewtimesDestDbAlias));
      query.bindValue(":year", QVariant(year));
      query.bindValue(":week_num", QVariant(weekNum));
      query.bindValue(":device_num", QVariant(deviceAddress));

      if (query.exec())
      {
        QLOG_DEBUG() << "Deleted any existing cleared view time records for current week # from this address";

        query.prepare(QString("INSERT INTO %1.cleared_view_times SELECT * FROM %2.cleared_view_times WHERE year = :year AND week_num = :week_num AND device_num = :device_num").arg(viewtimesDestDbAlias).arg(viewtimesSrcDbAlias));
        query.bindValue(":year", QVariant(year));
        query.bindValue(":week_num", QVariant(weekNum));
        query.bindValue(":device_num", QVariant(deviceAddress));

        if (query.exec())
        {
          QLOG_DEBUG() << "Finished inserting cleared view time record";
        }
        else
        {
          QLOG_ERROR() << QString("Could not insert record into %1.cleared_view_times. Error: %2").arg(viewtimesDestDbAlias).arg(query.lastError().text());
          ok = false;
        }
      }
      else
      {
        QLOG_ERROR() << QString("Could not delete existing record from %1.cleared_view_times. Error: %2").arg(viewtimesDestDbAlias).arg(query.lastError().text());
        ok = false;
      }

      // Detach the other databases
      if (query.exec(QString("DETACH %1").arg(viewtimesSrcDbAlias)))
      {
        QLOG_DEBUG() << QString("Detached database: %1/viewtimes_%2.sqlite...").arg(dataPath).arg(deviceAddress);
      }
      else
      {
        QLOG_ERROR() << QString("Could not detach database: %1/viewtimes_%2.sqlite. Error: %3").arg(dataPath).arg(deviceAddress).arg(query.lastError().text());
        ok = false;
      }

      if (query.exec(QString("DETACH %1").arg(viewtimesDestDbAlias)))
      {
        QLOG_DEBUG() << QString("Detached database: %1/viewtimes.sqlite").arg(dataPath);
      }
      else
      {
        QLOG_ERROR() << QString("Could not detach database: %1/viewtimes.sqlite. Error: %2").arg(dataPath).arg(query.lastError().text());
        ok = false;
      }

    } // endif attached to viewtimes.sqlite
    else
    {
      QLOG_ERROR() << QString("Could not attach to database: %1/viewtimes.sqlite. Error: %2. Database alias: %3").arg(dataPath).arg(query.lastError().text()).arg(viewtimesDestDbAlias);
      ok = false;

      // Detach database here since we couldn't attach to destination
      if (query.exec(QString("DETACH %1").arg(viewtimesSrcDbAlias)))
      {
        QLOG_DEBUG() << QString("Detached database: %1/viewtimes_%2.sqlite...").arg(dataPath).arg(deviceAddress);
      }
      else
      {
        QLOG_ERROR() << QString("Could not detach database: %1/viewtimes_%2.sqlite. Error: %3").arg(dataPath).arg(deviceAddress).arg(query.lastError().text());
        ok = false;
      }
    }

  } // endif attached to viewtimes_<deviceNum>.sqlite
  else
  {
    QLOG_ERROR() << QString("Could not attach to database: %1/viewtimes_%2.sqlite. Error: %3. Database alias: %4").arg(dataPath).arg(deviceAddress).arg(query.lastError().text()).arg(viewtimesSrcDbAlias);
    ok = false;
  }

  // query.clear();

  return ok;
}

// The year and weekNum are no longer always based on the current date, now the user could have many movie changes queued and may select them in any order
bool DatabaseMgr::insertMovieChange(int year, int weekNum, QString dirname, int channelNum, bool deleteVideo, QDateTime scheduledDate, Movie previousMovie)
{
  bool ok = true;

  QSqlQuery query(db);

  QString moviechangesDbAlias = Global::randomString(8);

  QLOG_DEBUG() << QString("Attaching to database on: %1/moviechanges.sqlite...").arg(dataPath);

  if (query.exec(QString("ATTACH DATABASE '%1/moviechanges.sqlite' AS %2").arg(dataPath).arg(moviechangesDbAlias)))
  {
    // If there wasn't a previous movie assigned to the channel # then indicate this with "N/A"
    QString previousMovieDesc = "N/A";
    if (previousMovie.isValid())
    {
      previousMovieDesc = QString("%1 / %2 / %3 / %4")
                          .arg(previousMovie.UPC())
                          .arg(previousMovie.Title())
                          .arg(previousMovie.Producer())
                          .arg(previousMovie.Category());
    }

    query.prepare(QString("INSERT INTO %1.movie_changes (year, week_num, dirname, channel_num, delete_movie, download_date, previous_movie) VALUES (:year, :week_num, :dirname, :channel_num, :delete_movie, :download_date, :previous_movie)").arg(moviechangesDbAlias));
    query.bindValue(":year", QVariant(year));
    query.bindValue(":week_num", QVariant(weekNum));
    // A NULL dirname means there isn't a video to replace the channel, channels will be shifted down during the actual movie change to fill the gap
    query.bindValue(":dirname", (dirname.isEmpty() ? QVariant() : QVariant(dirname)));
    query.bindValue(":channel_num", QVariant(channelNum));
    query.bindValue(":delete_movie", QVariant((deleteVideo ? 1 : 0)));
    query.bindValue(":download_date", scheduledDate.toString("yyyy-MM-dd hh:mm:ss"));
    query.bindValue(":previous_movie", previousMovieDesc);

    if (query.exec())
    {
      QLOG_DEBUG() << "Inserted movie change record";
    }
    else
    {
      QLOG_ERROR() << QString("Could not insert record in %1.movie_changes. Error: %2").arg(moviechangesDbAlias).arg(query.lastError().text());
      ok = false;
    }

    // Detach the other database
    if (query.exec(QString("DETACH %1").arg(moviechangesDbAlias)))
    {
      QLOG_DEBUG() << QString("Detached database: %1/moviechanges.sqlite").arg(dataPath);
    }
    else
    {
      QLOG_ERROR() << QString("Could not detach database: %1/moviechanges.sqlite. Error: %2").arg(dataPath).arg(query.lastError().text());
      ok = false;
    }

  } // endif attached to database
  else
  {
    QLOG_ERROR() << QString("Could not attach to database: %1/moviechanges.sqlite. Error: %2. Database alias: %3").arg(dataPath).arg(query.lastError().text()).arg(moviechangesDbAlias);
    ok = false;
  }

  return ok;
}

bool DatabaseMgr::deleteMovieChanges(int year, int weekNum)
{
  bool ok = true;

  QSqlQuery query(db);

  QString moviechangesDbAlias = Global::randomString(8);

  QLOG_DEBUG() << QString("Attaching to database on: %1/moviechanges.sqlite...").arg(dataPath);

  if (query.exec(QString("ATTACH DATABASE '%1/moviechanges.sqlite' AS %2").arg(dataPath).arg(moviechangesDbAlias)))
  {
    query.prepare(QString("DELETE FROM %1.movie_changes WHERE week_num = :week_num AND year = :year").arg(moviechangesDbAlias));
    query.bindValue(":week_num", QVariant(weekNum));
    query.bindValue(":year", QVariant(year));

    if (!query.exec())
    {
      QLOG_ERROR() << QString("Could not delete records from %1.movie_changes. Error: %2").arg(moviechangesDbAlias).arg(query.lastError().text());
      ok = false;
    }

    // Detach the other database
    if (query.exec(QString("DETACH %1").arg(moviechangesDbAlias)))
    {
      QLOG_DEBUG() << QString("Detached database: %1/moviechanges.sqlite").arg(dataPath);
    }
    else
    {
      QLOG_ERROR() << QString("Could not detach database: %1/moviechanges.sqlite. Error: %2").arg(dataPath).arg(query.lastError().text());
      ok = false;
    }

  } // endif attached to database
  else
  {
    QLOG_ERROR() << QString("Could not attach to database: %1/moviechanges.sqlite. Error: %2. Database alias: %3").arg(dataPath).arg(query.lastError().text()).arg(moviechangesDbAlias);
    ok = false;
  }

  return ok;
}

bool DatabaseMgr::deleteBoothMovieChanges(QStringList addresses, int year, int weekNum)
{
  bool ok = true;

  QSqlQuery query(db);

  QString moviechangesDbAlias = Global::randomString(8);

  foreach (QString deviceAddress, addresses)
  {
    QLOG_DEBUG() << QString("Attaching to database on: %1/moviechanges_%2.sqlite...").arg(dataPath).arg(deviceAddress);

    if (query.exec(QString("ATTACH DATABASE '%1/moviechanges_%2.sqlite' AS %3").arg(dataPath).arg(deviceAddress).arg(moviechangesDbAlias)))
    {
      query.prepare(QString("DELETE FROM %1.movie_changes WHERE week_num = :week_num AND year = :year").arg(moviechangesDbAlias));
      query.bindValue(":week_num", QVariant(weekNum));
      query.bindValue(":year", QVariant(year));

      if (query.exec())
      {
        QLOG_DEBUG() << "Deleted records from movie_changes";
      }
      else
      {
        QLOG_ERROR() << QString("Could not delete records from %1.movie_changes. Error: %2").arg(moviechangesDbAlias).arg(query.lastError().text());
        ok = false;
      }

      // Detach the other database
      if (query.exec(QString("DETACH %1").arg(moviechangesDbAlias)))
      {
        QLOG_DEBUG() << QString("Detached database: %1/moviechanges_%2.sqlite").arg(dataPath).arg(deviceAddress);
      }
      else
      {
        QLOG_ERROR() << QString("Could not detach database: %1/moviechanges_%2.sqlite. Error: %3").arg(dataPath).arg(deviceAddress).arg(query.lastError().text());
        ok = false;
      }

    } // endif attached to database
    else
    {
      QLOG_ERROR() << QString("Could not attach to database: %1/moviechanges_%2.sqlite. Error: %3. Database alias: %4").arg(dataPath).arg(deviceAddress).arg(query.lastError().text()).arg(moviechangesDbAlias);
      ok = false;
    }
  }

  return ok;
}

bool DatabaseMgr::deleteTop5Videos(QString category, int year, int weekNum)
{
  bool ok = true;

  QSqlQuery query(db);

  QString viewtimesDbAlias = Global::randomString(8);

  QLOG_DEBUG() << QString("Attaching to database on: %1/viewtimes.sqlite...").arg(dataPath);

  if (query.exec(QString("ATTACH DATABASE '%1/viewtimes.sqlite' AS %2").arg(dataPath).arg(viewtimesDbAlias)))
  {
    query.prepare(QString("DELETE FROM %1.top_5_videos WHERE year = :year AND week_num = :week_num AND category = :category").arg(viewtimesDbAlias));
    query.bindValue(":year", QVariant(year));
    query.bindValue(":week_num", QVariant(weekNum));
    query.bindValue(":category", QVariant(category));

    if (!query.exec())
    {
      QLOG_ERROR() << QString("Could not delete records from %1.top_5_videos. Error: %2").arg(viewtimesDbAlias).arg(query.lastError().text());
      ok = false;
    }

    // Detach the other database
    if (query.exec(QString("DETACH %1").arg(viewtimesDbAlias)))
    {
      QLOG_DEBUG() << QString("Detached database: %1/viewtimes.sqlite").arg(dataPath);
    }
    else
    {
      QLOG_ERROR() << QString("Could not detach database: %1/viewtimes.sqlite. Error: %2").arg(dataPath).arg(query.lastError().text());
      ok = false;
    }

  } // endif attached to database
  else
  {
    QLOG_ERROR() << QString("Could not attach to database: %1/viewtimes.sqlite. Error: %2. Database alias: %3").arg(dataPath).arg(query.lastError().text()).arg(viewtimesDbAlias);
    ok = false;
  }

  return ok;
}

bool DatabaseMgr::insertTop5Video(int playTime, int channelNum, QString title, QString category, int year, int weekNum)
{
  bool ok = true;

  QSqlQuery query(db);

  QString viewtimesDbAlias = Global::randomString(8);

  QLOG_DEBUG() << QString("Attaching to database on: %1/viewtimes.sqlite...").arg(dataPath);

  if (query.exec(QString("ATTACH DATABASE '%1/viewtimes.sqlite' AS %2").arg(dataPath).arg(viewtimesDbAlias)))
  {
    query.prepare(QString("INSERT INTO %1.top_5_videos (year, week_num, play_time, channel_num, title, category) VALUES (:year, :week_num, :play_time, :channel_num, :title, :category)").arg(viewtimesDbAlias));
    query.bindValue(":year", QVariant(year));
    query.bindValue(":week_num", QVariant(weekNum));
    query.bindValue(":play_time", QVariant(playTime));
    query.bindValue(":channel_num", QVariant(channelNum));
    query.bindValue(":title", QVariant(title));
    query.bindValue(":category", QVariant(category));

    if (query.exec())
    {
      QLOG_DEBUG() << "Inserted top 5 record";
    }
    else
    {
      QLOG_ERROR() << QString("Could not insert record in %1.top_5_videos. Error: %2").arg(viewtimesDbAlias).arg(query.lastError().text());
      ok = false;
    }

    // Detach the other database
    if (query.exec(QString("DETACH %1").arg(viewtimesDbAlias)))
    {
      QLOG_DEBUG() << QString("Detached database: %1/viewtimes.sqlite").arg(dataPath);
    }
    else
    {
      QLOG_ERROR() << QString("Could not detach database: %1/viewtimes.sqlite. Error: %2").arg(dataPath).arg(query.lastError().text());
      ok = false;
    }

  } // endif attached to database
  else
  {
    QLOG_ERROR() << QString("Could not attach to database: %1/viewtimes.sqlite. Error: %2. Database alias: %3").arg(dataPath).arg(query.lastError().text()).arg(viewtimesDbAlias);
    ok = false;
  }

  return ok;
}

bool DatabaseMgr::upcExists(QString upc, bool *ok)
{
  bool foundUPC = false;
  *ok = true;

  QSqlQuery query(db);

  query.prepare(QString("SELECT COUNT(*) FROM videos WHERE upc = :upc"));
  query.bindValue(":upc", upc);

  if (query.exec())
  {
    if (query.first())
    {
      if (query.value(0).toInt() > 0)
        foundUPC = true;
    }
    else
    {
      QLOG_ERROR() << "No result when getting count for UPC";
      *ok = false;
    }
  }
  else
  {
    QLOG_ERROR() << QString("Could not query videos table. Error: %1").arg(query.lastError().text());
    *ok = false;
  }

  return foundUPC;
}

bool DatabaseMgr::archiveMovieChanges(int year, int weekNum)
{
  bool ok = true;

  QSqlQuery query(db);

  QString moviechangesDbAlias = Global::randomString(8);

  QLOG_DEBUG() << QString("Attaching to database on: %1/moviechanges.sqlite...").arg(dataPath);

  if (query.exec(QString("ATTACH DATABASE '%1/moviechanges.sqlite' AS %2").arg(dataPath).arg(moviechangesDbAlias)))
  {
    query.prepare(QString("INSERT INTO %1.movie_changes_history SELECT year, week_num, dirname, channel_num, delete_movie, download_date, download_started, download_finished, movies_changed, previous_movie FROM %2.movie_changes WHERE week_num = :week_num AND year = :year").arg(moviechangesDbAlias).arg(moviechangesDbAlias));
    query.bindValue(":week_num", QVariant(weekNum));
    query.bindValue(":year", QVariant(year));

    if (query.exec())
    {
      QLOG_DEBUG() << QString("Inserted records from %1.movie_changes to %2.movie_changes_history").arg(moviechangesDbAlias).arg(moviechangesDbAlias);

      query.prepare(QString("DELETE FROM %1.movie_changes WHERE week_num = :week_num AND year = :year").arg(moviechangesDbAlias));
      query.bindValue(":week_num", QVariant(weekNum));
      query.bindValue(":year", QVariant(year));

      if (query.exec())
      {
        QLOG_DEBUG() << "Deleted archived records from movie_changes";
      }
      else
      {
        QLOG_ERROR() << QString("Could not delete records from %1.movie_changes. Error: %2").arg(moviechangesDbAlias).arg(query.lastError().text());
        ok = false;
      }
    }
    else
    {
      QLOG_ERROR() << QString("Could not insert records from %1.movie_changes to %2.movie_changes_history. Error: %3").arg(moviechangesDbAlias).arg(moviechangesDbAlias).arg(query.lastError().text());
      ok = false;
    }

    // Detach the other database
    if (query.exec(QString("DETACH %1").arg(moviechangesDbAlias)))
    {
      QLOG_DEBUG() << QString("Detached database: %1/moviechanges.sqlite").arg(dataPath);
    }
    else
    {
      QLOG_ERROR() << QString("Could not detach database: %1/moviechanges.sqlite. Error: %2").arg(dataPath).arg(query.lastError().text());
      ok = false;
    }

  } // endif attached to database
  else
  {
    QLOG_ERROR() << QString("Could not attach to database: %1/moviechanges.sqlite. Error: %2. Database alias: %3").arg(dataPath).arg(query.lastError().text()).arg(moviechangesDbAlias);
    ok = false;
  }

  return ok;
}

bool DatabaseMgr::archiveBoothMovieChanges(QStringList addresses, int year, int weekNum)
{
  bool ok = true;

  QSqlQuery query(db);

  QString moviechangesDbAlias = Global::randomString(8);

  foreach (QString deviceAddress, addresses)
  {
    QLOG_DEBUG() << QString("Attaching to database on: %1/moviechanges_%2.sqlite...").arg(dataPath).arg(deviceAddress);

    if (query.exec(QString("ATTACH DATABASE '%1/moviechanges_%2.sqlite' AS %3").arg(dataPath).arg(deviceAddress).arg(moviechangesDbAlias)))
    {
      query.prepare(QString("INSERT INTO %1.movie_changes_history SELECT year, week_num, dirname, channel_num, delete_movie, download_date, download_started, download_finished, movies_changed, previous_movie FROM %2.movie_changes WHERE week_num = :week_num AND year = :year").arg(moviechangesDbAlias).arg(moviechangesDbAlias));
      query.bindValue(":week_num", QVariant(weekNum));
      query.bindValue(":year", QVariant(year));

      if (query.exec())
      {
        QLOG_DEBUG() << QString("Inserted records from %1.movie_changes to %2.movie_changes_history").arg(moviechangesDbAlias).arg(moviechangesDbAlias);

        query.prepare(QString("DELETE FROM %1.movie_changes WHERE week_num = :week_num AND year = :year").arg(moviechangesDbAlias));
        query.bindValue(":week_num", QVariant(weekNum));
        query.bindValue(":year", QVariant(year));

        if (query.exec())
        {
          QLOG_DEBUG() << "Deleted archived records from movie_changes";
        }
        else
        {
          QLOG_ERROR() << QString("Could not delete records from %1.movie_changes. Error: %2").arg(moviechangesDbAlias).arg(query.lastError().text());
          ok = false;
        }
      }
      else
      {
        QLOG_ERROR() << QString("Could not insert records from %1.movie_changes to %2.movie_changes_history. Error: %3").arg(moviechangesDbAlias).arg(moviechangesDbAlias).arg(query.lastError().text());
        ok = false;
      }

      // Detach the other database
      if (query.exec(QString("DETACH %1").arg(moviechangesDbAlias)))
      {
        QLOG_DEBUG() << QString("Detached database: %1/moviechanges_%2.sqlite").arg(dataPath).arg(deviceAddress);
      }
      else
      {
        QLOG_ERROR() << QString("Could not detach database: %1/moviechanges_%2.sqlite. Error: %3").arg(dataPath).arg(deviceAddress).arg(query.lastError().text());
        ok = false;
      }

    } // endif attached to database
    else
    {
      QLOG_ERROR() << QString("Could not attach to database: %1/moviechanges_%2.sqlite. Error: %3. Database alias: %4").arg(dataPath).arg(deviceAddress).arg(query.lastError().text()).arg(moviechangesDbAlias);
      ok = false;
    }
  }

  return ok;
}

bool DatabaseMgr::archiveDvdCopyTasks(int year, int weekNum)
{
  bool ok = true;

  QSqlQuery query(db);

  query.prepare("INSERT INTO dvd_copy_jobs_history SELECT * FROM dvd_copy_jobs WHERE week_num = :week_num AND year = :year");
  query.bindValue(":week_num", QVariant(weekNum));
  query.bindValue(":year", QVariant(year));

  if (query.exec())
  {
    QLOG_DEBUG() << "Inserted records from dvd_copy_jobs to dvd_copy_jobs_history";

    query.prepare("DELETE FROM dvd_copy_jobs WHERE week_num = :week_num AND year = :year");
    query.bindValue(":week_num", QVariant(weekNum));
    query.bindValue(":year", QVariant(year));

    if (query.exec())
    {
      QLOG_DEBUG() << "Deleted archived records from dvd_copy_jobs";
    }
    else
    {
      QLOG_ERROR() << "Could not delete records from dvd_copy_jobs. Error:" << query.lastError().text();
      ok = false;
    }
  }
  else
  {
    QLOG_ERROR() << "Could not insert records from dvd_copy_jobs to dvd_copy_jobs_history. Error:" << query.lastError().text();
    ok = false;
  }

  return ok;
}

bool DatabaseMgr::restoreDvdCopyTasks(int year, int weekNum)
{
  bool ok = true;

  QSqlQuery query(db);

  query.prepare("INSERT INTO dvd_copy_jobs SELECT * FROM dvd_copy_jobs_history WHERE week_num = :week_num AND year = :year");
  query.bindValue(":week_num", QVariant(weekNum));
  query.bindValue(":year", QVariant(year));

  if (query.exec())
  {
    QLOG_DEBUG() << "Inserted records from dvd_copy_jobs_history to dvd_copy_jobs";

    query.prepare("DELETE FROM dvd_copy_jobs_history WHERE week_num = :week_num AND year = :year");
    query.bindValue(":week_num", QVariant(weekNum));
    query.bindValue(":year", QVariant(year));

    if (query.exec())
    {
      QLOG_DEBUG() << "Deleted restored records from dvd_copy_jobs_history";
    }
    else
    {
      QLOG_ERROR() << "Could not delete records from dvd_copy_jobs_history. Error:" << query.lastError().text();
      ok = false;
    }
  }
  else
  {
    QLOG_ERROR() << "Could not insert records from dvd_copy_jobs_history to dvd_copy_jobs. Error:" << query.lastError().text();
    ok = false;
  }

  return ok;
}

QVariantList DatabaseMgr::getOnlyBoothInfo(QStringList addresses)
{
  QVariantList allBooths = this->getBoothInfo(addresses);
  QVariantList onlyBooths;

  foreach (QVariant v, allBooths)
  {
    QVariantMap booth = v.toMap();

    // Add to list if not ClerkStation
    if (booth["device_type_id"].toInt() != Settings::ClerkStation)
    {
      onlyBooths.append(v);
    }
  }

  return onlyBooths;
}

QVariantList DatabaseMgr::getBoothInfo(QStringList addresses)
{
  QVariantList boothList;

  QSqlQuery query(db);

  // Used for converting enum value to string representation
  const QMetaObject &mo = Settings::staticMetaObject;
  int index = mo.indexOfEnumerator("Device_Type");
  QMetaEnum metaEnum = mo.enumerator(index);

  QString casplayerDbAlias = Global::randomString(8);

  foreach (QString deviceAddress, addresses)
  {
    QFile dbFile(QString("%1/casplayer_%2.sqlite").arg(dataPath).arg(deviceAddress));

    QVariantMap device;

    if (dbFile.exists() && dbFile.size() > 0)
    {
      QLOG_DEBUG() << QString("Attaching to database on: %1/casplayer_%2.sqlite...").arg(dataPath).arg(deviceAddress);

      if (query.exec(QString("ATTACH DATABASE '%1/casplayer_%2.sqlite' AS %3").arg(dataPath).arg(deviceAddress).arg(casplayerDbAlias)))
      {
        if (query.exec(QString("SELECT key_name, data FROM %1.settings WHERE key_name IN ('device_address', 'device_type', 'device_alias', 'bill_acceptor_in_service', 'enable_meters', 'download_started', 'download_finished', 'movies_changed', 'enable_bill_acceptor', 'free_space_gb')").arg(casplayerDbAlias)))
        {
          QDateTime downloadStarted, downloadFinished, moviesChanged;

          while (query.next())
          {
            if (query.value(0).toString() == "device_address")
            {
              device["device_num"] = query.value(1).toString();
            }
            else if (query.value(0).toString() == "device_type")
            {
              device["device_type"] = metaEnum.valueToKey(query.value(1).toInt());
              device["device_type_id"] = query.value(1).toInt();
            }
            else if (query.value(0).toString() == "device_alias")
            {
              device["device_alias"] = query.value(1).toString();
            }
            else if (query.value(0).toString() == "bill_acceptor_in_service")
            {
              device["bill_acceptor_in_service"] = query.value(1).toInt() == 1;
            }
            else if (query.value(0).toString() == "enable_meters")
            {
              device["enable_meters"] = query.value(1).toInt() == 1;
            }
            else if (query.value(0).toString() == "download_started")
            {
              if (!query.value(1).isNull())
                downloadStarted = QDateTime::fromString(query.value(1).toString(), "yyyy-MM-dd hh:mm:ss");
              //else
              //QLOG_DEBUG() << "download_started is null";
            }
            else if (query.value(0).toString() == "download_finished")
            {
              if (!query.value(1).isNull())
                downloadFinished = QDateTime::fromString(query.value(1).toString(), "yyyy-MM-dd hh:mm:ss");
              //else
              //QLOG_DEBUG() << "download_finished is null";
            }
            else if (query.value(0).toString() == "movies_changed")
            {
              if (!query.value(1).isNull())
                moviesChanged = QDateTime::fromString(query.value(1).toString(), "yyyy-MM-dd hh:mm:ss");
              //else
              //QLOG_DEBUG() << "movies_changed is null";
            }
            else if (query.value(0).toString() == "enable_bill_acceptor")
            {
              device["enable_bill_acceptor"] = query.value(1).toInt() == 1;
            }
            else if (query.value(0).toString() == "free_space_gb")
            {
              device["free_space_gb"] = query.value(1).toDouble();
            }
          }

          // The scheduled key is not really needed but is still used as an indicator that the other 3 keys exist
          device["scheduled"] = true;
          device["download_started"] = (!downloadStarted.isValid() ? QDateTime() : downloadStarted);
          device["download_finished"] = (!downloadFinished.isValid() ? QDateTime() : downloadFinished);
          device["movies_changed"] = (!moviesChanged.isValid() ? QDateTime() : moviesChanged);

          device["found"] = true;
        }

        if (query.exec(QString("SELECT MAX(channel_num) FROM %1.videos").arg(casplayerDbAlias)))
        {
          if (query.first())
          {
            device["num_channels"] = query.value(0).toInt();
          }
        }

        query.prepare(QString("SELECT last_used, current_credits, current_cash, current_cc_charges, current_prog_credits FROM %1.devices WHERE device_num = :device_num").arg(casplayerDbAlias));
        query.bindValue(":device_num", deviceAddress);

        if (query.exec())
        {
          if (query.first())
          {
            device["last_used"] = QDateTime::fromString(query.value(0).toString(), "yyyy-MM-dd hh:mm:ss");
            device["current_credits"] = query.value(1).toInt();
            device["current_cash"] = query.value(2).toInt();
            device["current_cc_charges"] = query.value(3).toInt();
            device["current_prog_credits"] = query.value(4).toInt();
          }
        }

        // Detach the other database
        if (query.exec(QString("DETACH %1").arg(casplayerDbAlias)))
        {
          QLOG_DEBUG() << QString("Detached database: %1/casplayer_%2.sqlite...").arg(dataPath).arg(deviceAddress);
        }
        else
        {
          QLOG_ERROR() << QString("Could not detach database: %1/casplayer_%2.sqlite. Error: %3").arg(dataPath).arg(deviceAddress).arg(query.lastError().text());
        }

      } // endif attached to database
      else
      {
        QLOG_ERROR() << QString("Could not attach to database: %1/casplayer_%2.sqlite. Error: %3").arg(dataPath).arg(deviceAddress).arg(query.lastError().text());

        device["device_num"] = deviceAddress;
        device["found"] = false;
      }
    }
    else
    {
      QLOG_DEBUG() << QString("Database does not exist: %1/casplayer_%2.sqlite").arg(dataPath).arg(deviceAddress);

      device["device_num"] = deviceAddress;
      device["found"] = false;
    }

    boothList.append(device);
  }

  return boothList;
}

QVariantList DatabaseMgr::getclerkSessionsByDate(QDate selectedDate, QStringList addresses)
{
    QVariantList sessionList;

    QSqlQuery query(db);

    QString casplayerDbAlias = Global::randomString(8);

    foreach (QString deviceAddress, addresses)
    {
      QFile dbFile(QString("%1/casplayer_%2.sqlite").arg(dataPath).arg(deviceAddress));

      QVariantMap session;

      if (dbFile.exists() && dbFile.size() > 0)
      {
        QLOG_DEBUG() << QString("Attaching to database on: %1/casplayer_%2.sqlite...").arg(dataPath).arg(deviceAddress);

        if (query.exec(QString("ATTACH DATABASE '%1/casplayer_%2.sqlite' AS %3").arg(dataPath).arg(deviceAddress).arg(casplayerDbAlias)))
        {
          query.prepare(QString("SELECT trans_start_time, trans_end_time, username, amount, device_num, arcade_amount, preview_amount FROM %1.clerksessions WHERE date(trans_start_time) = :filterDate OR date(trans_end_time) = :filterDate ORDER BY username ASC").arg(casplayerDbAlias));
          query.bindValue(":filterDate", selectedDate.toString("yyyy-MM-dd"));
          if (query.exec())
          {
            while (query.next())
            {
              session["user"] = query.value(2).toString();
              session["startTime"] = QDateTime::fromString(query.value(0).toString(), "yyyy-MM-dd hh:mm:ss");
              session["endTime"] = QDateTime::fromString(query.value(1).toString(), "yyyy-MM-dd hh:mm:ss");
              session["amount"] = query.value(3).toDouble();
              session["deviceNum"] = query.value(4).toString();
              session["arcade_amount"] = query.value(5).toDouble();
              session["preview_amount"] = query.value(6).toDouble();

              sessionList.append(session);
            }
          }
          else
            QLOG_ERROR() << QString("Could not query clerksessions table in database %1/casplayer_%2.sqlite. Error: %3").arg(dataPath).arg(deviceAddress).arg(query.lastError().text());
        }
        else
          QLOG_ERROR() << QString("Could not attach to database %1/casplayer_%2.sqlite. Error: %3. Database alias: %4").arg(dataPath).arg(deviceAddress).arg(query.lastError().text()).arg(casplayerDbAlias);

        // Detach the other database
        if (query.exec(QString("DETACH %1").arg(casplayerDbAlias)))
        {
          QLOG_DEBUG() << QString("Detached database: %1/casplayer_%2.sqlite...").arg(dataPath).arg(deviceAddress);
        }
        else
        {
          QLOG_ERROR() << QString("Could not detach database: %1/casplayer_%2.sqlite. Error: %3").arg(dataPath).arg(deviceAddress).arg(query.lastError().text());
        }
      }
      else
      {
        QLOG_DEBUG() << QString("Database does not exist: %1/casplayer_%2.sqlite").arg(dataPath).arg(deviceAddress);
      }
    }

    return sessionList;
}

QVariantList DatabaseMgr::getLatestBoothCollection(QStringList addresses)
{
  QVariantList boothList;

  QSqlQuery query(db);

  QString casplayerDbAlias = Global::randomString(8);

  foreach (QString deviceAddress, addresses)
  {
    QFile dbFile(QString("%1/casplayer_%2.sqlite").arg(dataPath).arg(deviceAddress));

    QVariantMap device;

    if (dbFile.exists() && dbFile.size() > 0)
    {
      QLOG_DEBUG() << QString("Attaching to database on: %1/casplayer_%2.sqlite...").arg(dataPath).arg(deviceAddress);

      if (query.exec(QString("ATTACH DATABASE '%1/casplayer_%2.sqlite' AS %3").arg(dataPath).arg(deviceAddress).arg(casplayerDbAlias)))
      {
        if (query.exec(QString("SELECT collected, device_num, current_credits, total_credits, current_preview_credits, total_preview_credits, current_cash, total_cash, current_cc_charges, total_cc_charges, current_prog_credits, total_prog_credits FROM %1.collections ORDER BY collected DESC LIMIT 1").arg(casplayerDbAlias)))
        {
          if (query.first())
          {
            device["collected"] = QDateTime::fromString(query.value(0).toString(), "yyyy-MM-dd hh:mm:ss");
            device["device_num"] = query.value(1).toInt();
            device["current_credits"] = query.value(2).toInt();
            device["total_credits"] = query.value(3).toInt();
            device["current_preview_credits"] = query.value(4).toInt();
            device["total_preview_credits"] = query.value(5).toInt();
            device["current_cash"] = query.value(6).toInt();
            device["total_cash"] = query.value(7).toInt();
            device["current_cc_charges"] = query.value(8).toInt();
            device["total_cc_charges"] = query.value(9).toInt();
            device["current_prog_credits"] = query.value(10).toInt();
            device["total_prog_credits"] = query.value(11).toInt();
          }
          else
            QLOG_ERROR() << "No records found in collections table";
        }
        else
          QLOG_ERROR() << QString("Could not query collections table in database %1/casplayer_%2.sqlite. Error: %3").arg(dataPath).arg(deviceAddress).arg(query.lastError().text());
      }
      else
        QLOG_ERROR() << QString("Could not attach to database %1/casplayer_%2.sqlite. Error: %3. Database alias: %4").arg(dataPath).arg(deviceAddress).arg(query.lastError().text()).arg(casplayerDbAlias);

      // Detach the other database
      if (query.exec(QString("DETACH %1").arg(casplayerDbAlias)))
      {
        QLOG_DEBUG() << QString("Detached database: %1/casplayer_%2.sqlite...").arg(dataPath).arg(deviceAddress);
      }
      else
      {
        QLOG_ERROR() << QString("Could not detach database: %1/casplayer_%2.sqlite. Error: %3").arg(dataPath).arg(deviceAddress).arg(query.lastError().text());
      }
    }
    else
    {
      QLOG_DEBUG() << QString("Database does not exist: %1/casplayer_%2.sqlite").arg(dataPath).arg(deviceAddress);
    }

    boothList.append(device);
  }

  return boothList;
}

QList<QDate> DatabaseMgr::getViewTimeCollectionDates()
{
  QList<QDate> dateList;

  QSqlQuery query(db);

  QString viewtimesDbAlias = Global::randomString(8);

  QLOG_DEBUG() << QString("Attaching to database on: %1/viewtimes.sqlite...").arg(dataPath);

  if (query.exec(QString("ATTACH DATABASE '%1/viewtimes.sqlite' AS %2").arg(dataPath).arg(viewtimesDbAlias)))
  {
    if (query.exec(QString("SELECT collection_date FROM %1.view_times GROUP BY collection_date ORDER BY collection_date DESC").arg(viewtimesDbAlias)))
    {
      while (query.next())
      {
        dateList.append(QDate::fromString(query.value(0).toString(), "yyyy-MM-dd"));
      }
    }
    else
    {
      QLOG_ERROR() << QString("Could not query %1.view_times. Error: %2").arg(viewtimesDbAlias).arg(query.lastError().text());
    }

    // Detach the other database
    if (query.exec(QString("DETACH %1").arg(viewtimesDbAlias)))
    {
      QLOG_DEBUG() << QString("Detached database: %1/viewtimes.sqlite").arg(dataPath);
    }
    else
    {
      QLOG_ERROR() << QString("Could not detach database: %1/viewtimes.sqlite. Error: %2").arg(dataPath).arg(query.lastError().text());
    }

  } // endif attached to database
  else
  {
    QLOG_ERROR() << QString("Could not attach to database: %1/viewtimes.sqlite. Error: %2. Database alias: %3").arg(dataPath).arg(query.lastError().text()).arg(viewtimesDbAlias);
  }

  return dateList;
}

QVariantList DatabaseMgr::getViewTimeCollection(QDate collectionDate, int deviceNum)
{
  QVariantList collection;

  QSqlQuery query(db);

  QString viewtimesDbAlias = Global::randomString(8);

  QLOG_DEBUG() << QString("Attaching to database on: %1/viewtimes.sqlite...").arg(dataPath);

  if (query.exec(QString("ATTACH DATABASE '%1/viewtimes.sqlite' AS %2").arg(dataPath).arg(viewtimesDbAlias)))
  {
    if (deviceNum > 0)
    {
      // View times from one booth
      query.prepare(QString("SELECT upc, title, producer, category, genre, channel_num, current_play_time, ((strftime('%s', collection_date) - strftime('%s', date_in)) / 604800) as num_weeks FROM %1.view_times WHERE collection_date = :collection_date AND device_num = :device_num ORDER BY current_play_time").arg(viewtimesDbAlias));
      query.bindValue(":collection_date", collectionDate.toString("yyyy-MM-dd"));
      query.bindValue(":device_num", deviceNum);
    }
    else
    {
      // Combined view times
      query.prepare(QString("SELECT upc, title, producer, category, genre, channel_num, SUM(current_play_time) play_time, ((strftime('%s', collection_date) - strftime('%s', date_in)) / 604800) as num_weeks FROM %1.view_times WHERE collection_date = :collection_date GROUP BY upc, title, producer, channel_num, category, genre ORDER BY SUM(current_play_time)").arg(viewtimesDbAlias));
      query.bindValue(":collection_date", collectionDate.toString("yyyy-MM-dd"));
    }

    if (query.exec())
    {
      while (query.next())
      {
        QVariantMap record;

        record["upc"] = query.value(0).toString();
        record["title"] = query.value(1).toString();
        record["producer"] = query.value(2).toString();
        record["category"] = query.value(3).toString();
        record["subcategory"] = query.value(4).toString();
        record["channel_num"] = query.value(5).toInt();
        record["play_time"] = query.value(6).toInt();
        record["num_weeks"] = query.value(7).toInt();

        collection.append(record);
      }
    }
    else
    {
      QLOG_ERROR() << QString("Could not query %1.view_times. Error: %2").arg(viewtimesDbAlias).arg(query.lastError().text());
    }

    // Detach the other database
    if (query.exec(QString("DETACH %1").arg(viewtimesDbAlias)))
    {
      QLOG_DEBUG() << QString("Detached database: %1/viewtimes.sqlite").arg(dataPath);
    }
    else
    {
      QLOG_ERROR() << QString("Could not detach database: %1/viewtimes.sqlite. Error: %2").arg(dataPath).arg(query.lastError().text());
    }

  } // endif attached to database
  else
  {
    QLOG_ERROR() << QString("Could not attach to database: %1/viewtimes.sqlite. Error: %2. Database alias: %3").arg(dataPath).arg(query.lastError().text()).arg(viewtimesDbAlias);
  }

  return collection;
}

QList<QDate> DatabaseMgr::getCollectionDates(QStringList addresses)
{
  QList<QDate> dateList;

  QSqlQuery query(db);

  QString casplayerDbAlias = Global::randomString(8);

  foreach (QString deviceNum, addresses)
  {
    QLOG_DEBUG() << QString("Attaching to database on: %1/casplayer_%2.sqlite...").arg(dataPath).arg(deviceNum);

    if (query.exec(QString("ATTACH DATABASE '%1/casplayer_%2.sqlite' AS %3").arg(dataPath).arg(deviceNum).arg(casplayerDbAlias)))
    {
      if (query.exec(QString("SELECT date(collected) FROM %1.collections GROUP BY date(collected) ORDER BY collected DESC").arg(casplayerDbAlias)))
      {
        while (query.next())
        {
          QDate collectionDate = QDate::fromString(query.value(0).toString(), "yyyy-MM-dd");

          // Only add date to list if it doesn't exist
          if (!dateList.contains(collectionDate))
            dateList.append(collectionDate);
        }
      }
      else
      {
        QLOG_ERROR() << QString("Could not query %1.collections. Error: %2").arg(casplayerDbAlias).arg(query.lastError().text());
      }

      // Detach the other database
      if (query.exec(QString("DETACH %1").arg(casplayerDbAlias)))
      {
        QLOG_DEBUG() << QString("Detached database: %1/casplayer_%2.sqlite...").arg(dataPath).arg(deviceNum);
      }
      else
      {
        QLOG_ERROR() << QString("Could not detach database: %1/casplayer_%2.sqlite. Error: %3").arg(dataPath).arg(deviceNum).arg(query.lastError().text());
      }

    } // endif attached to database
    else
    {
      QLOG_ERROR() << QString("Could not attach to database: %1/casplayer_%2.sqlite. Error: %3. Database alias: %4").arg(dataPath).arg(deviceNum).arg(query.lastError().text()).arg(casplayerDbAlias);
    }
  }

  // Sort dates in descending order
  qStableSort(dateList.begin(), dateList.end(), qGreater<QDate>());

  return dateList;
}

QVariantList DatabaseMgr::getCollection(QDate collectionDate, QStringList deviceList)
{
  QVariantList collection;

  QSqlQuery query(db);

  QString casplayerDbAlias = Global::randomString(8);

  foreach (QString deviceNum, deviceList)
  {
    QLOG_DEBUG() << QString("Attaching to database on: %1/casplayer_%2.sqlite...").arg(dataPath).arg(deviceNum);

    if (query.exec(QString("ATTACH DATABASE '%1/casplayer_%2.sqlite' AS %3").arg(dataPath).arg(deviceNum).arg(casplayerDbAlias)))
    {
      query.prepare(QString("SELECT device_num, SUM(current_credits) current_credits, MAX(total_credits) total_credits, SUM(current_preview_credits) current_preview_credits, MAX(total_preview_credits) total_preview_credits, SUM(current_cash) current_cash, MAX(total_cash) total_cash, SUM(current_cc_charges) current_cc_charges, MAX(total_cc_charges) total_cc_charges, SUM(current_prog_credits) current_prog_credits, MAX(total_prog_credits) total_prog_credits, (SELECT data FROM %1.settings WHERE key_name = 'device_alias') device_alias FROM %2.collections WHERE date(collected) = :collected GROUP BY device_num").arg(casplayerDbAlias).arg(casplayerDbAlias));
      query.bindValue(":collected", collectionDate.toString("yyyy-MM-dd"));

      if (query.exec())
      {
        if (query.first())
        {
          QVariantMap record;

          record["device_num"] = query.value(0).toInt();
          record["current_credits"] = query.value(1).toInt();
          record["total_credits"] = query.value(2).toInt();
          record["current_preview_credits"] = query.value(3).toInt();
          record["total_preview_credits"] = query.value(4).toInt();
          record["current_cash"] = query.value(5).toInt();
          record["total_cash"] = query.value(6).toInt();
          record["current_cc_charges"] = query.value(7).toInt();
          record["total_cc_charges"] = query.value(8).toInt();
          record["current_prog_credits"] = query.value(9).toInt();
          record["total_prog_credits"] = query.value(10).toInt();
          record["device_alias"] = query.value(11).toString();

          collection.append(record);
        }
      }
      else
      {
        QLOG_ERROR() << QString("Could not query %1.collections. Error: %2").arg(casplayerDbAlias).arg(query.lastError().text());
      }

      // Detach the other database
      if (query.exec(QString("DETACH %1").arg(casplayerDbAlias)))
      {
        QLOG_DEBUG() << QString("Detached database: %1/casplayer_%2.sqlite...").arg(dataPath).arg(deviceNum);
      }
      else
      {
        QLOG_ERROR() << QString("Could not detach database: %1/casplayer_%2.sqlite. Error: %3").arg(dataPath).arg(deviceNum).arg(query.lastError().text());
      }

    } // endif attached to database
    else
    {
      QLOG_ERROR() << QString("Could not attach to database: %1/casplayer_%2.sqlite. Error: %3. Database alias: %4").arg(dataPath).arg(deviceNum).arg(query.lastError().text()).arg(casplayerDbAlias);
    }
  }

  return collection;
}

QList<QDate> DatabaseMgr::getMeterDates()
{
  QList<QDate> dateList;
  QSqlQuery query(db);

  if (query.exec(QString("SELECT date(captured) FROM daily_meters GROUP BY date(captured) ORDER BY captured DESC")))
  {
    while (query.next())
    {
      QDate meterDate = QDate::fromString(query.value(0).toString(), "yyyy-MM-dd");

      // Only add date to list if it doesn't exist
      if (!dateList.contains(meterDate))
        dateList.append(meterDate);
    }
  }
  else
  {
      QLOG_ERROR() << QString("Could not query %1.daily_meters. Error: %2").arg("main").arg(query.lastError().text());
  }

  // Sort dates in descending order
  qStableSort(dateList.begin(), dateList.end(), qGreater<QDate>());

  return dateList;
}

// Returns list of QVariantMaps with keys: scheduled_date, year, week_num, upc, title, category, channel_num, previous_movie
QVariantList DatabaseMgr::getPendingMovieChange(bool deleteOnly)
{
  QVariantList pendingMovieChanges;

  QSqlQuery query(db);

  QString moviechangesDbAlias = Global::randomString(8);

  QLOG_DEBUG() << QString("Attaching to database on: %1/moviechanges.sqlite...").arg(dataPath);

  if (query.exec(QString("ATTACH DATABASE '%1/moviechanges.sqlite' AS %2").arg(dataPath).arg(moviechangesDbAlias)))
  {
    QString sql = QString("SELECT download_date, mm.year, mm.week_num, d.upc, d.title, d.category, mm.channel_num, mm.previous_movie FROM %1.movie_changes mm INNER JOIN dvd_copy_jobs_history d ON mm.year = d.year AND mm.week_num = d.week_num AND mm.dirname = d.upc").arg(moviechangesDbAlias);
    if (deleteOnly)
      sql = QString("SELECT download_date, year, week_num, 'N/A' as upc, 'N/A' as title, 'N/A' as category, channel_num, previous_movie FROM %1.movie_changes").arg(moviechangesDbAlias);

    if (query.exec(sql))
    {
      while (query.next())
      {
        QVariantMap video;

        video["scheduled_date"] = QDateTime::fromString(query.value(0).toString(), "yyyy-MM-dd hh:mm:ss");
        video["year"] = query.value(1).toInt();
        video["week_num"] = query.value(2).toInt();
        video["upc"] = query.value(3).toString();
        video["title"] = query.value(4).toString();
        video["category"] = query.value(5).toString();
        video["channel_num"] = query.value(6).toInt();
        video["previous_movie"] = query.value(7).toString();

        pendingMovieChanges.append(video);
      }
    }
    else
    {
      QLOG_ERROR() << QString("Could not query %1.movie_changes. Error: %2").arg(moviechangesDbAlias).arg(query.lastError().text());
    }

    // Detach the other database
    if (query.exec(QString("DETACH %1").arg(moviechangesDbAlias)))
    {
      QLOG_DEBUG() << QString("Detached database: %1/moviechanges.sqlite").arg(dataPath);
    }
    else
    {
      QLOG_ERROR() << QString("Could not detach database: %1/moviechanges.sqlite. Error: %2").arg(dataPath).arg(query.lastError().text());
    }

  } // endif attached to database
  else
  {
    QLOG_ERROR() << QString("Could not attach to database: %1/moviechanges.sqlite. Error: %2. Database alias: %3").arg(dataPath).arg(query.lastError().text()).arg(moviechangesDbAlias);
  }

  return pendingMovieChanges;
}

bool DatabaseMgr::upcExists(QString upc)
{
  bool foundUPC = false;

  QSqlQuery query(db);

  query.prepare("SELECT u.upc FROM (SELECT upc FROM dvd_copy_jobs UNION SELECT upc FROM dvd_copy_jobs_history UNION SELECT upc FROM videos) u WHERE u.upc = :upc");
  query.bindValue(":upc", QVariant(upc));

  if (query.exec())
  {
    if (query.first())
    {
      foundUPC = true;
    }
  }

  return foundUPC;
}

QList<Movie> DatabaseMgr::getVideos()
{
  QList<Movie> videoList;

  QSqlQuery query(db);

  if (query.exec(QString("SELECT upc, title, producer, category, genre, date_in, channel_num, file_length FROM videos ORDER BY channel_num")))
  {
    while (query.next())
    {
      Movie m;
      m.setUPC(query.value(0).toString());
      m.setTitle(query.value(1).toString());
      m.setProducer(query.value(2).toString());
      m.setCategory(query.value(3).toString());
      m.setSubcategory(query.value(4).toString());
      m.setDateIn(QDateTime::fromString(query.value(5).toString(), "yyyy-MM-dd"));
      m.setChannelNum(query.value(6).toInt());
      m.setFileLength(query.value(7).toLongLong());

      videoList.append(m);
    }
  }
  else
  {
    QLOG_ERROR() << QString("Could not query videos table. Error: %1").arg(query.lastError().text());
  }

  return videoList;
}


void DatabaseMgr::databaseCreated(const CouchDBResponse &response)
{
  if (response.status() == COUCHDB_SUCCESS)
  {
    emit verifyCouchDbFinished(QString(), true);

    // TODO: Create views
  }
  else
    emit verifyCouchDbFinished("Could not create database in CAS database server.", false);
}

void DatabaseMgr::databasesListed(const CouchDBResponse &response)
{
  if (response.status() == COUCHDB_SUCCESS)
  {
    emit verifyCouchDbFinished(QString(), true);
    return; // FIXME: REMOVE THE LINE ABOVE AND THIS LINE
    //qDebug() << "CouchDB database list:" << response.document();
    // TODO: Move database name to settings
    if (response.document().array().contains(QJsonValue("cas")))
      emit verifyCouchDbFinished(QString(), true);
    else
    {
      QLOG_DEBUG() << "Database not found on CouchDB server, now creating...";

      // Could not find cas database so create it now
      // TODO: Move database name to settings
      couchDB->createDatabase("cas");
    }
  }
  else
    emit verifyCouchDbFinished("Could not get list of databases from CAS database server.", false);
}

void DatabaseMgr::documentUpdated(const CouchDBResponse &response)
{
  if (response.query()->property("callerID").isNull())
  {
    QLOG_DEBUG() << Q_FUNC_INFO << "received response without expected ID";
    return;
  }

  if (response.status() == COUCHDB_SUCCESS)
  {
    if (response.query()->property("callerID").toString() == "updateCard")
    {
      ArcadeCard card(response.documentObj());

      //QLOG_DEBUG() << "slot cardDocumentUpdated rev:" << card.getRevID();

      emit updateCardFinished(card, true);
      //emit updateCardFinished(response.documentObj()["id"].toString(), response.documentObj()["rev"].toString(), true);
    }
    else if (response.query()->property("callerID").toString() == "updateUser")
    {
      UserAccount userAccount(response.documentObj());

      emit updateUserFinished(userAccount, true);
    }
  }
  else
  {
    //QLOG_ERROR() << "slot cardDocumentUpdated";

    if (response.query()->property("callerID").toString() == "updateCard")
    {
      ArcadeCard card(response.documentObj());
      emit updateCardFinished(card, false);
    }
    else if (response.query()->property("callerID").toString() == "updateUser")
    {
      UserAccount userAccount(response.documentObj());

      emit updateUserFinished(userAccount, false);
    }
  }
}

void DatabaseMgr::documentCreated(const CouchDBResponse &response)
{
  if (response.query()->property("callerID").isNull())
  {
    QLOG_DEBUG() << Q_FUNC_INFO << "received response without expected ID";
    return;
  }

  if (response.query()->property("callerID").toString() == "addCard")
  {
    ArcadeCard card;
    emit addCardFinished(card, response.status() == COUCHDB_SUCCESS);
  }
  else if (response.query()->property("callerID").toString() == "addUser")
  {
    if (response.status() == COUCHDB_SUCCESS)
    {
      emit addUserFinished(response.documentObj()["_id"].toString(), response.status() == COUCHDB_SUCCESS);
    }
    else
    {
      emit addUserFinished(QString(), response.status() == COUCHDB_SUCCESS);
    }
  }
  else if (response.query()->property("callerID").toString() == "addBoothStatus")
  {
    if (response.status() == COUCHDB_SUCCESS)
    {
      emit addBoothStatusFinished(response.documentObj()["_id"].toString(), response.status() == COUCHDB_SUCCESS);
    }
    else
    {
      emit addBoothStatusFinished(QString(), response.status() == COUCHDB_SUCCESS);
    }
  }
}

void DatabaseMgr::documentDeleted(const CouchDBResponse &response)
{
  if (response.query()->property("callerID").isNull())
  {
    QLOG_DEBUG() << Q_FUNC_INFO << "received response without expected ID";
    return;
  }

  if (response.status() == COUCHDB_SUCCESS)
  {
    QLOG_DEBUG() << "Returned response from delete:" << response.document();

    if (response.query()->property("callerID").toString() == "deleteCard")
    {
      emit deleteCardFinished(response.documentObj()["id"].toString(), true);
    }
    else if (response.query()->property("callerID").toString() == "deleteUser")
    {
      emit deleteUserFinished(response.documentObj()["id"].toString(), true);
    }
    else if (response.query()->property("callerID").toString() == "deleteBoothStatus")
    {
      emit deleteBoothStatusFinished(response.documentObj()["id"].toString(), true);
    }
  }
  else
  {
    if (response.query()->property("callerID").toString() == "deleteUser")
    {
      emit deleteCardFinished(response.documentObj()["_id"].toString(), false);
    }
    else if (response.query()->property("callerID").toString() == "deleteUser")
    {
      emit deleteUserFinished(response.documentObj()["id"].toString(), false);
    }
    else if (response.query()->property("callerID").toString() == "deleteBoothStatus")
    {
      emit deleteBoothStatusFinished(response.documentObj()["id"].toString(), false);
    }
  }
}

void DatabaseMgr::databaseViewRetrieved(const CouchDBResponse &response)
{
  if (response.query()->property("callerID").isNull())
  {
    QLOG_DEBUG() << Q_FUNC_INFO << "received response without expected ID";
    return;
  }

  if (response.query()->property("callerID").toString() == "allCards")
  {
    //QLOG_DEBUG() << response.documentObj();
    QList<ArcadeCard> cardList;

    if (response.documentObj().contains("total_rows"))
    {
      QLOG_DEBUG() << "total_rows:" << response.documentObj()["total_rows"].toDouble();
    }

    if (response.documentObj().contains("rows"))
    {
      //QLOG_DEBUG() << "rows:";

      foreach (QJsonValue row, response.documentObj()["rows"].toArray())
      {
        QJsonObject obj = row.toObject();
        QString cardNum = obj["id"].toString();
        QJsonObject cardObject = obj["value"].toObject();
        cardObject["_id"] = cardNum;

        //ArcadeCard card(cardObject);
        cardList.append(ArcadeCard(cardObject));
      }

      emit getCardsFinished(cardList, true);
    }
    else
    {
      emit getCardsFinished(cardList, false);
    }
  }
  else if (response.query()->property("callerID").toString() == "allUsers")
  {
    QList<UserAccount> users;

    if (response.documentObj().contains("total_rows"))
    {
      QLOG_DEBUG() << "total_rows:" << response.documentObj()["total_rows"].toDouble();
    }

    if (response.documentObj().contains("rows"))
    {
      //QLOG_DEBUG() << "rows:";

      foreach (QJsonValue row, response.documentObj()["rows"].toArray())
      {
        UserAccount userAccount;
        QJsonObject obj = row.toObject();
        userAccount.setUsername(obj["id"].toString());
        userAccount.setRevID(obj["value"].toString());

        users.append(userAccount);
      }

      emit getUsersFinished(users, true);
    }
    else
    {
      emit getUsersFinished(users, false);
    }
  }
  else if (response.query()->property("callerID").toString() == "getExpiredCards")
  {
    QList<ArcadeCard> cardList;

    if (response.documentObj().contains("total_rows"))
    {
      QLOG_DEBUG() << "total_rows:" << response.documentObj()["total_rows"].toDouble();
    }

    if (response.documentObj().contains("rows"))
    {
      foreach (QJsonValue row, response.documentObj()["rows"].toArray())
      {
        QJsonObject obj = row.toObject();
        QString cardNum = obj["id"].toString();
        QJsonObject cardObject = obj["value"].toObject();
        cardObject["_id"] = cardNum;

        cardList.append(ArcadeCard(cardObject));
      }

      emit getExpiredCardsFinished(cardList, true);
    }
    else
    {
      emit getExpiredCardsFinished(cardList, false);
    }
  }
  else if (response.query()->property("callerID").toString() == "getBoothStatus")
  {
    QList<QJsonObject> docs;

    if (response.documentObj().contains("total_rows"))
    {
      QLOG_DEBUG() << "total_rows:" << response.documentObj()["total_rows"].toDouble();
    }

    if (response.documentObj().contains("rows"))
    {
      //QLOG_DEBUG() << "rows:";

      foreach (QJsonValue row, response.documentObj()["rows"].toArray())
      {
        docs.append(row.toObject()["value"].toObject());
      }

      emit getBoothStatusesFinished(docs, true);
    }
    else
    {
      emit getBoothStatusesFinished(docs, false);
    }
  }
}

void DatabaseMgr::documentRetrieved(const CouchDBResponse &response)
{
  if (response.query()->property("callerID").isNull())
  {
    QLOG_DEBUG() << Q_FUNC_INFO << "received response without expected ID";
    return;
  }

  if (response.status() == COUCHDB_SUCCESS)
  {
    if (response.query()->property("callerID").toString() == "getCard")
    {
      ArcadeCard card(response.documentObj());
      emit getCardFinished(card, true);
    }
    else if (response.query()->property("callerID").toString() == "getUser")
    {
      UserAccount userAccount(response.documentObj());
      emit getUserFinished(userAccount, true);
    }
  }
  else
  {
    if (response.query()->property("callerID").toString() == "getCard")
    {
      ArcadeCard emptyCard;
      emit getCardFinished(emptyCard, false);
    }
    else if (response.query()->property("callerID").toString() == "getUser")
    {
      UserAccount userAccount;
      emit getUserFinished(userAccount, false);
    }
  }
}

void DatabaseMgr::bulkDocumentsDeleted(const CouchDBResponse &response)
{
  emit bulkDeleteCardsFinished(response.document().array().size(), response.status() == COUCHDB_SUCCESS);
}

void DatabaseMgr::installationChecked(const CouchDBResponse &response)
{
  if (response.status() == COUCHDB_SUCCESS)
  {
    couchDB->listDatabases();
  }
  else
    emit verifyCouchDbFinished("Could not connect to CAS database server.", false);
}

bool DatabaseMgr::insertLeastViewedMovies(QList<Movie> movies)
{
  int numInserts = 0;

  QSqlQuery query(db);

  foreach (Movie m, movies)
  {
    query.prepare("INSERT INTO least_viewed_videos (upc, title, producer, category, genre, channel_num, playtime) VALUES (:upc, :title, :producer, :category, :genre, :channel_num, :playtime)");
    query.bindValue(":upc", m.UPC());
    query.bindValue(":title", m.Title());
    query.bindValue(":producer", m.Producer());
    query.bindValue(":category", m.Category());
    query.bindValue(":genre", m.Subcategory());
    query.bindValue(":channel_num", m.ChannelNum());
    query.bindValue(":playtime", m.TotalPlayTime());

    if (query.exec())
    {
      ++numInserts;
    }
    else
    {
      QLOG_ERROR() << QString("Could not insert record into least_viewed_videos. Error: %1").arg(query.lastError().text());
    }
  }

  return numInserts == movies.count();
}

bool DatabaseMgr::deleteLeastViewedMovies()
{
  bool success = false;

  QSqlQuery query(db);

  if (query.exec(QString("DELETE FROM least_viewed_videos")))
  {
    success = true;
  }
  else
  {
    QLOG_ERROR() << QString("Could not delete all records from least_viewed_videos. Error: %1").arg(query.lastError().text());
  }

  return success;
}

QList<Movie> DatabaseMgr::getLeastViewedMovies(QString category, bool *ok)
{
  QList<Movie> movieList;

  QSqlQuery query(db);

  *ok = true;

  query.prepare("SELECT a.upc, a.title, a.producer, a.category, a.genre, a.channel_num, a.playtime, v.file_length FROM least_viewed_videos as a JOIN videos as v ON a.upc = v.upc WHERE a.category = :category ORDER BY a.playtime");
  query.bindValue(":category", category);

  if (query.exec())
  {
    while (query.next())
    {
      Movie m;
      m.setUPC(query.value(0).toString());
      m.setTitle(query.value(1).toString());
      m.setProducer(query.value(2).toString());
      m.setCategory(query.value(3).toString());
      m.setSubcategory(query.value(4).toString());
      m.setChannelNum(query.value(5).toInt());
      m.setTotalPlayTime(query.value(6).toInt());
      m.setFileLength(query.value(7).toLongLong());

      movieList.append(m);
    }
  }
  else
  {
    QLOG_ERROR() << QString("Could not query least_viewed_videos table. Error: %1").arg(query.lastError().text());
    *ok = false;
  }

  return movieList;
}

QMap<QString, QList<Movie> > DatabaseMgr::getBoothsWithMissingMovies(QStringList addresses, bool *ok)
{
  QMap<QString, QList<Movie> > boothsMissingMovies;

  QSqlQuery query(db);

  QString casplayerDbAlias = Global::randomString(8);
  *ok = true;

  foreach (QString deviceAddress, addresses)
  {
    QFile dbFile(QString("%1/casplayer_%2.sqlite").arg(dataPath).arg(deviceAddress));

    if (dbFile.exists() && dbFile.size() > 0)
    {
      QLOG_DEBUG() << QString("Attaching to database on: %1/casplayer_%2.sqlite...").arg(dataPath).arg(deviceAddress);

      if (query.exec(QString("ATTACH DATABASE '%1/casplayer_%2.sqlite' AS %3").arg(dataPath).arg(deviceAddress).arg(casplayerDbAlias)))
      {
        // Select records by upc, channel_num in videos table that are not in the casplayer_*.sqlite videos table
        //if (query.exec(QString("SELECT upc, channel_num FROM videos WHERE (upc, channel_num) NOT IN (SELECT upc, channel_num FROM %1.videos) ORDER BY channel_num").arg(casplayerDbAlias)))
        if (query.exec(QString("SELECT upc, channel_num FROM videos WHERE (upc) NOT IN (SELECT upc FROM %1.videos) ORDER BY channel_num").arg(casplayerDbAlias)))
        {
          QList<Movie> missingMovies;
          while (query.next())
          {
            Movie m;
            m.setUPC(query.value(0).toString());
            m.setChannelNum(query.value(1).toInt());

            missingMovies.append(m);
          }

          if (missingMovies.length())
          {
            boothsMissingMovies[deviceAddress] = missingMovies;
          }
        }
        else
        {
          QLOG_ERROR() << QString("Could not query videos table in database %1/casplayer_%2.sqlite. Error: %3").arg(dataPath).arg(deviceAddress).arg(query.lastError().text());
          *ok = false;
        }
      }
      else
      {
        QLOG_ERROR() << QString("Could not attach to database %1/casplayer_%2.sqlite. Error: %3. Database alias: %4").arg(dataPath).arg(deviceAddress).arg(query.lastError().text()).arg(casplayerDbAlias);
        *ok = false;
      }

      // Detach the other database
      if (query.exec(QString("DETACH %1").arg(casplayerDbAlias)))
      {
        QLOG_DEBUG() << QString("Detached database: %1/casplayer_%2.sqlite...").arg(dataPath).arg(deviceAddress);
      }
      else
      {
        QLOG_ERROR() << QString("Could not detach database: %1/casplayer_%2.sqlite. Error: %3").arg(dataPath).arg(deviceAddress).arg(query.lastError().text());
        *ok = false;
      }
    }
    else
    {
      QLOG_DEBUG() << QString("Database does not exist: %1/casplayer_%2.sqlite").arg(dataPath).arg(deviceAddress);
    }
  }

  return boothsMissingMovies;
}

bool DatabaseMgr::updateVideosFromMovieChange(int year, int weekNum)
{
  // The videos table contains the videos that should currently be in the booths and is updated each
  // time a movie change finishes. If movies are added to the booths then they get inserted into the videos
  // table. If a channel is replaced in the booths then the appropriate video by channel #
  // is updated in the videos table, if a movie is deleted without being replaced then it gets deleted from
  // the videos table and channel numbers are shifted down

  int numUpdates = 0;

  QVariantList movieChangeSet = getMovieChangeData(year, weekNum);

  if (movieChangeSet.length())
  {
    QSqlQuery query(db);

    foreach (QVariant v, movieChangeSet)
    {
      QVariantMap movieChange = v.toMap();

      if (movieChange["upc"].toString().isEmpty())
      {
        // Delete record
        query.prepare("DELETE FROM videos WHERE channel_num = :channel_num");
        query.bindValue(":channel_num", movieChange["channel_num"].toInt());

        if (query.exec())
        {
          ++numUpdates;
        }
        else
        {
          QLOG_ERROR() << QString("Could not delete records from videos table. Error: %1").arg(query.lastError().text());
        }

        if (!shiftChannels(movieChange["channel_num"].toInt()))
        {
          // Indicate there was an error by decrementing counter
          --numUpdates;
        }
      }
      else
      {
        if (movieChange["delete_movie"].toBool())
        {
          query.prepare("UPDATE videos SET upc = :upc, title = :title, producer = :producer, category = :category, genre = :genre, date_in = :date_in, file_length = :file_length WHERE channel_num = :channel_num");
        }
        else
        {
          query.prepare("INSERT INTO videos (upc, title, producer, category, genre, channel_num, date_in, file_length) VALUES (:upc, :title, :producer, :category, :genre, :channel_num, :date_in, :file_length)");
        }

        query.bindValue(":upc", movieChange["upc"].toString());
        query.bindValue(":title", movieChange["title"].toString());
        query.bindValue(":producer", movieChange["producer"].toString());
        query.bindValue(":category", movieChange["category"].toString());
        query.bindValue(":genre", movieChange["subcategory"].toString());
        query.bindValue(":channel_num", movieChange["channel_num"].toInt());
        query.bindValue(":date_in", movieChange["date_in"].toString());
        query.bindValue(":file_length", movieChange["file_length"].toLongLong());

        if (query.exec())
        {
          ++numUpdates;
        }
        else
        {
          QLOG_ERROR() << QString("Could not update/insert record in videos table. Error: %1").arg(query.lastError().text());
          QLOG_DEBUG() << "movieChange keys:" << movieChange.keys();
          QLOG_DEBUG() << "movieChange values:" << movieChange.values();
        }
      }
    }
  }
  else
  {
    QLOG_ERROR() << QString("No records returned for movie change, year: %1, week #: %2").arg(year).arg(weekNum);
  }

  return numUpdates == movieChangeSet.length();
}

QVariantList DatabaseMgr::getMovieChangeData(int year, int weekNum)
{
  QVariantList movieChangeDetails;

  QSqlQuery query(db);

  QString moviechangesDbAlias = Global::randomString(8);

  QLOG_DEBUG() << QString("Attaching to database on: %1/moviechanges.sqlite...").arg(dataPath);

  if (query.exec(QString("ATTACH DATABASE '%1/moviechanges.sqlite' AS %2").arg(dataPath).arg(moviechangesDbAlias)))
  {
    // Get the movie change details from the movie_changes_history table and link to the the dvd_copy_jobs_history for metadata
    // The results are sorted so records that are just deletes in movie_changes_history table are last with the channel in descending order
    query.prepare(QString("SELECT upc, title, producer, category, genre, channel_num, date(download_date) formattedDateIn, file_length, delete_movie FROM %1.movie_changes_history m LEFT JOIN dvd_copy_jobs_history h ON m.dirname = h.upc WHERE m.year = :year AND m.week_num = :week_num ORDER BY upc DESC, channel_num DESC").arg(moviechangesDbAlias));
    query.bindValue(":year", year);
    query.bindValue(":week_num", weekNum);

    if (query.exec())
    {
      while (query.next())
      {
        QVariantMap record;

        record["upc"] = query.value(0).toString();
        record["title"] = query.value(1).toString();
        record["producer"] = query.value(2).toString();
        record["category"] = query.value(3).toString();
        record["subcategory"] = query.value(4).toString();
        record["channel_num"] = query.value(5).toInt();
        record["date_in"] = query.value(6).toString();
        record["file_length"] = query.value(7).toLongLong();
        record["delete_movie"] = query.value(8).toInt() == 1;

        movieChangeDetails.append(record);
      }
    }
    else
    {
      QLOG_ERROR() << QString("Could not query %1.movie_changes_history. Error: %2").arg(moviechangesDbAlias).arg(query.lastError().text());
    }

    // Detach the other database
    if (query.exec(QString("DETACH %1").arg(moviechangesDbAlias)))
    {
      QLOG_DEBUG() << QString("Detached database: %1/moviechanges.sqlite").arg(dataPath);
    }
    else
    {
      QLOG_ERROR() << QString("Could not detach database: %1/moviechanges.sqlite. Error: %2").arg(dataPath).arg(query.lastError().text());
    }

  } // endif attached to database
  else
  {
    QLOG_ERROR() << QString("Could not attach to database: %1/moviechanges.sqlite. Error: %2. Database alias: %3").arg(dataPath).arg(query.lastError().text()).arg(moviechangesDbAlias);
  }

  return movieChangeDetails;
}

bool DatabaseMgr::shiftChannels(int channelNum)
{
  bool success = false;
  QSqlQuery query(db);

  query.prepare("UPDATE videos SET channel_num = (channel_num - 1) WHERE channel_num > :channel_num");
  query.bindValue(":channel_num", channelNum);

  if (query.exec())
  {
    success = true;
  }
  else
  {
    QLOG_ERROR() << QString("Could not shift channel numbers in videos table. Error: %1").arg(query.lastError().text());
  }

  return success;
}

int DatabaseMgr::getMaxMovieChangeYear(bool *ok)
{
  *ok = false;
  int maxYear = 0;

  QSqlQuery query(db);

  QString moviechangesDbAlias = Global::randomString(8);

  QLOG_DEBUG() << QString("Attaching to database on: %1/moviechanges.sqlite...").arg(dataPath);

  if (query.exec(QString("ATTACH DATABASE '%1/moviechanges.sqlite' AS %2").arg(dataPath).arg(moviechangesDbAlias)))
  {
    if (query.exec(QString("SELECT MAX(year) FROM (SELECT year FROM %1.movie_changes_history UNION SELECT year FROM %2.movie_changes)").arg(moviechangesDbAlias).arg(moviechangesDbAlias)))
    {
      if (query.first())
      {
        maxYear = query.value(0).toInt();
        *ok = true;
      }
    }
    else
    {
      QLOG_ERROR() << QString("Could not query %1.movie_changes_history and %2.movie_changes. Error: %3").arg(moviechangesDbAlias).arg(moviechangesDbAlias).arg(query.lastError().text());
    }

    // Detach the other database
    if (query.exec(QString("DETACH %1").arg(moviechangesDbAlias)))
    {
      QLOG_DEBUG() << QString("Detached database: %1/moviechanges.sqlite").arg(dataPath);
    }
    else
    {
      QLOG_ERROR() << QString("Could not detach database: %1/moviechanges.sqlite. Error: %2").arg(dataPath).arg(query.lastError().text());
    }

  } // endif attached to database
  else
  {
    QLOG_ERROR() << QString("Could not attach to database: %1/moviechanges.sqlite. Error: %2. Database alias: %3").arg(dataPath).arg(query.lastError().text()).arg(moviechangesDbAlias);
  }

  return maxYear;
}
