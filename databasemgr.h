#ifndef DATABASEMGR_H
#define DATABASEMGR_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <QDate>
#include <QVariant>
#include <QCryptographicHash>
#include "dvdcopytask.h"
#include "moviechangeinfo.h"
#include "movie.h"
#include "arcadecard.h"
#include "useraccount.h"
#include "top-couchdb/couchdb.h"

// CAS Manager tables
const QString SETTINGS_TABLE = "CREATE TABLE settings (key_name TEXT PRIMARY KEY NOT NULL UNIQUE, data TEXT)";
const QString DVD_COPY_JOBS_TABLE = "CREATE TABLE dvd_copy_jobs (year INTEGER NOT NULL, week_num INTEGER NOT NULL, upc TEXT NOT NULL, title TEXT, producer TEXT, category TEXT, genre TEXT, front_cover_file TEXT, back_cover_file TEXT, file_length INTEGER NOT NULL DEFAULT 0, status INTEGER NOT NULL DEFAULT 0, complete INTEGER NOT NULL DEFAULT 0, seq_num INTEGER NOT NULL DEFAULT 0, transcode_status INTEGER NOT NULL DEFAULT 0)";
const QString DVD_COPY_JOBS_INDEX = "CREATE INDEX DVD_COPY_JOBS_INDEX ON dvd_copy_jobs (year ASC, week_num ASC, upc ASC, status ASC, seq_num ASC, transcode_status ASC)";
const QString DVD_COPY_JOBS_HISTORY_TABLE = "CREATE TABLE dvd_copy_jobs_history (year INTEGER NOT NULL, week_num INTEGER NOT NULL, upc TEXT NOT NULL, title TEXT, producer TEXT, category TEXT NOT NULL, genre TEXT, front_cover_file TEXT, back_cover_file TEXT, file_length INTEGER NOT NULL DEFAULT 0, status INTEGER NOT NULL DEFAULT 0, complete INTEGER NOT NULL DEFAULT 0, seq_num INTEGER NOT NULL DEFAULT 0, transcode_status INTEGER NOT NULL DEFAULT 0)";
const QString DVD_COPY_JOBS_HISTORY_INDEX = "CREATE INDEX DVD_COPY_JOBS_HISTORY_INDEX ON dvd_copy_jobs_history (year ASC, week_num ASC, upc ASC, status ASC, seq_num ASC, transcode_status ASC)";
const QString VIDEO_ATTRIBS_TABLE = "CREATE TABLE video_attributes (attribute_type INTEGER NOT NULL, value TEXT NOT NULL)";
const QString VIDEO_ATTRIBS_INDEX = "CREATE INDEX VIDEO_ATTRIB_INDEX on video_attributes (attribute_type ASC, value ASC)";
const QString VIDEOS_TABLE = "CREATE TABLE videos (upc TEXT PRIMARY KEY NOT NULL UNIQUE, title TEXT NOT NULL, producer TEXT NOT NULL, category TEXT NOT NULL, genre TEXT NOT NULL, date_in TEXT NOT NULL, channel_num INTEGER NOT NULL, file_length INTEGER NOT NULL)";
const QString VIDEOS_INDEX = "CREATE INDEX VIDEOS_INDEX ON videos (channel_num ASC);";
const QString DAILY_METERS_TABLE = "CREATE TABLE daily_meters (device_num INTEGER NOT NULL, device_alias TEXT, captured TEXT, last_collection TEXT, current_credits INTEGER NOT NULL DEFAULT 0, total_credits INTEGER NOT NULL DEFAULT 0, current_preview_credits INTEGER NOT NULL DEFAULT 0, total_preview_credits INTEGER NOT NULL DEFAULT 0, current_cash INTEGER NOT NULL DEFAULT 0, total_cash INTEGER NOT NULL  DEFAULT 0, current_cc_charges INTEGER NOT NULL DEFAULT 0, total_cc_charges INTEGER NOT NULL DEFAULT 0, current_prog_credits INTEGER NOT NULL DEFAULT 0, total_prog_credits INTEGER NOT NULL DEFAULT 0)";
const QString DAILY_METERS_INDEX = "CREATE INDEX daily_meters_index on daily_meters (device_num ASC, captured ASC)";
const QString DELETE_UPCS_TABLE = "CREATE TABLE delete_upcs (upc TEXT PRIMARY KEY NOT NULL UNIQUE)";
// help_center holds records for the changelog, documentation and tip of the day. entry_type: 1 = tip of the day, 2 = changelog, 3 = documentation
// The changelog and documentation types at the moment are intended to just have one entry each
const QString HELP_CENTER_TABLE = "CREATE TABLE help_center (help_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, entry_type INTEGER NOT NULL, entry TEXT)";
const QString LEAST_VIEWED_VIDEOS_TABLE = "CREATE TABLE least_viewed_videos (upc TEXT PRIMARY KEY NOT NULL UNIQUE, title TEXT NOT NULL, producer TEXT NOT NULL, category TEXT NOT NULL, genre TEXT NOT NULL, channel_num INTEGER NOT NULL, playtime INTEGER NOT NULL)";
const QString LEAST_VIEWED_VIDEOS_INDEX = "CREATE INDEX LEAST_VIEWED_VIDEOS_INDEX ON least_viewed_videos (category ASC, playtime ASC)";

// Shared tables
const QString MOVIE_CHANGES_TABLE = "CREATE TABLE m.movie_changes (year INTEGER NOT NULL, week_num INTEGER NOT NULL, dirname TEXT, channel_num INTEGER NOT NULL, delete_movie INTEGER NOT NULL, download_date TEXT NOT NULL, download_started TEXT, download_finished TEXT, movies_changed TEXT, previous_movie TEXT)";
const QString MOVIE_CHANGES_INDEX = "CREATE INDEX m.MOVIE_CHANGES_INDEX ON movie_changes (week_num ASC, year ASC, delete_movie ASC, download_started ASC, download_finished ASC)";
const QString MOVIE_CHANGES_HISTORY_TABLE = "CREATE TABLE m.movie_changes_history (year INTEGER NOT NULL, week_num INTEGER NOT NULL, dirname TEXT, channel_num INTEGER NOT NULL, delete_movie INTEGER NOT NULL, download_date TEXT NOT NULL, download_started TEXT, download_finished TEXT, movies_changed TEXT, previous_movie TEXT)";
const QString MOVIE_CHANGES_HISTORY_INDEX = "CREATE INDEX m.MOVIE_CHANGES_HISTORY_INDEX ON movie_changes_history (week_num ASC, year ASC, delete_movie ASC, download_started ASC, download_finished ASC)";
const QString MOVIE_CHANGE_QUEUE_TABLE = "CREATE TABLE m.movie_change_queue (queue_id INTEGER PRIMARY KEY NOT NULL, year INTEGER NOT NULL, week_num INTEGER NOT NULL, override_viewtimes INTEGER NOT NULL DEFAULT 0, first_channel INTEGER NOT NULL DEFAULT 0, last_channel INTEGER NOT NULL DEFAULT 0, status INTEGER NOT NULL DEFAULT 0)";
const QString MOVIE_CHANGE_QUEUE_INDEX = "CREATE INDEX m.MOVIE_CHANGE_QUEUE_INDEX ON movie_change_queue (week_num ASC, year ASC)";
const QString TOP_5_VIDEOS_TABLE = "CREATE TABLE v.top_5_videos (year INTEGER NOT NULL, week_num INTEGER NOT NULL, play_time INTEGER NOT NULL, channel_num INTEGER NOT NULL, title TEXT NOT NULL, category TEXT NOT NULL)";
const QString TOP_5_VIDEOS_INDEX = "CREATE INDEX v.TOP_5_INDEX ON top_5_videos (year ASC, week_num ASC, play_time ASC, category ASC)";
const QString VIEW_TIMES_TABLE = "CREATE TABLE v.view_times (year INT, week_num INT, collection_date TEXT, device_num INT, upc TEXT, title TEXT, producer TEXT, category TEXT, genre TEXT, duration INTEGER NOT NULL DEFAULT (0), date_in TEXT, channel_num INT, current_play_time INT, total_play_time INT, media_file TEXT, front_cover_file TEXT, back_cover_file TEXT, file_length INT)";
const QString VIEW_TIMES_INDEX = "CREATE INDEX v.VIDEO_INDEX ON view_times (year ASC, week_num ASC, category ASC, channel_num ASC, current_play_time DESC, collection_date ASC, device_num ASC)";
const QString CLEARED_VIEW_TIMES_TABLE = "CREATE TABLE v.cleared_view_times (year INTEGER NOT NULL, week_num INTEGER NOT NULL, device_num INTEGER NOT NULL, cleared INTEGER NOT NULL)";
const QString CLEARED_VIEW_TIMES_INDEX = "CREATE INDEX v.CLEARED_VIEW_TIMES_INDEX ON cleared_view_times (year ASC, week_num ASC, device_num ASC)";

class DatabaseMgr : public QObject
{
  Q_OBJECT
public:
  explicit DatabaseMgr(QString dataPath = QString(), QObject *parent = 0);

  // Open/create the database
  bool openDB(QString dbFile);

  bool verifyCasMgrDb();

  bool verifySharedDBs();

  void verifyCouchDB();

  // Close database
  void closeDB();

  QString getValue(QString keyName, QVariant defaultValue, bool *ok);
  bool setValue(QString keyName, QString value);

  // Insert DVD copy task into current_jobs table
  bool insertDvdCopyTask(DvdCopyTask task);

  // Get DVD copy task by upc
  DvdCopyTask getDvdCopyTask(QString upc);

  // Update DVD copy task record
  bool updateDvdCopyTask(DvdCopyTask task);

  // Delete DVD copy task record
  bool deleteDvdCopyTask(DvdCopyTask task);

  // Get DVD copy tasks matching status, order by year, week num, seq_num
  QList<DvdCopyTask> getDvdCopyTasks(DvdCopyTask::Status status, bool notEqual = false);

  // Get DVD copy tasks matching year and week num order by seq_num
  QList<DvdCopyTask> getDvdCopyTasks(int year, int weekNum, bool includeHistory = false);
  // Insert UPC to delete from UPC lookup queue on web server into delete_upcs table
  bool insertDeleteUPC(QString upc);

  // Delete one or more UPCs from the delete_upcs table
  bool deleteUPCs(QStringList upcList);

  // Return list of UPCs from delete_upcs table
  QStringList getDeleteUPCs();

  // Get list of videos that have finished copying but need to be transcoded
  QList<DvdCopyTask> getVideosToTranscode();

  // Get list of videos that have finished copying and were in the middle of being transcoded
  // Technically there can be only one video at a time being transcoded (at the moment) so only
  // one record should be returned
  QList<DvdCopyTask> getVideosBeingTranscoded();

  // Get list of movie change sets that have all metadata or optionally include ones that are not complete
  QList<MovieChangeInfo> getAvailableMovieChangeSets(bool includeIncompleteSets = false, QList<MovieChangeInfo> excludeList = QList<MovieChangeInfo>());

  QStringList getVideoAttributes(int attribType);

  // Returns the highest channel number from videos table
  int getHighestChannelNum(bool *ok);

  // Returns the lowest free disk space out of all devices based on casplayer_*.sqlite databases
  qreal getLowestFreeDiskSpace(QStringList deviceAddressList, bool *ok);

  // Group by channel num, sum the play time, order by play time where movie is in specified category from the specified view time collection
  QList<Movie> getViewTimesByCategory(QString category, int year, int weekNum, bool *ok);

  // Gets a list of all categories that make up the latest view time collection
  QStringList getVideoCategories(bool *ok);

  bool mergeViewTimes(int year, int weekNum, QString deviceAddress);

  bool mergeClearedViewTimes(int year, int weekNum, QString deviceAddress);

  // Insert movie change into database
  bool insertMovieChange(int year, int weekNum, QString dirname, int channelNum, bool deleteVideo, QDateTime scheduledDate, Movie previousMovie);

  // Delete existing movie changes for specified year and week #
  bool deleteMovieChanges(int year, int weekNum);

  // Delete movie change set from specified list of booth device addresses (not IP address)
  bool deleteBoothMovieChanges(QStringList addresses, int year, int weekNum);

  // Delete existing top 5 videos for specified category and week #
  bool deleteTop5Videos(QString category, int year, int weekNum);

  // Insert top 5 video into database
  bool insertTop5Video(int playTime, int channelNum, QString title, QString category, int year, int weekNum);

  // Returns true if the UPC already exists in the videos table
  bool upcExists(QString upc, bool *ok);

  // Moves records from movie_changes to movie_changes_history
  bool archiveMovieChanges(int year, int weekNum);

  // Moves records from movie_changes to movie_changes_history in all specified booth addresses (moviechanges_<address>.sqlite)
  bool archiveBoothMovieChanges(QStringList addresses, int year, int weekNum);

  // Moves records from dvd_copy_jobs to dvd_copy_jobs_history
  bool archiveDvdCopyTasks(int year, int weekNum);

  // Moves records from dvd_copy_jobs_history back to dvd_copy_jobs
  bool restoreDvdCopyTasks(int year, int weekNum);

  // Calls getBoothInfo but excludes clerkstations
  QVariantList getOnlyBoothInfo(QStringList addresses);

  // Returns list of QVariantMaps with keys:
  // device_num, device_type, device_type_id, device_alias, found, num_channels, last_used, current_cash, free_space_gb, bill_acceptor_in_service, enable_meters, enable_bill_acceptor
  // if year and weekNum > 0 then it includes: scheduled, download_started, download_finished, movies_changed
  QVariantList getBoothInfo(QStringList addresses);

  // Returns list of QVariantMaps with keys: scheduled_date, year, week_num, upc, title, category
  QVariantList getPendingMovieChange(bool deleteOnly = false);

  // Returns true if UPC is found in dvd_copy_* tables
  bool upcExists(QString upc);

  bool importDvdCopyTask(DvdCopyTask task);

  bool movieSetExists(int year, int weekNum);

  void setDataPath(QString dataPath);

  // Returns true if the year and weekNum found in the the dvd_copy_jobs_history table
  // Records are moved here when a movie change is scheduled with the booths
  bool movieChangeDone(int year, int weekNum);

  // Get list of videos that should be in the arcade based on past movie changes
  QList<Movie> getVideos();

  // Get latest collections record from casplayer_<address>.sqlite database
  QVariantList getLatestBoothCollection(QStringList addresses);

  QList<QDate> getViewTimeCollectionDates();

  // Get combined view times for the specified collection date if deviceNum = 0, otherwise
  // get just the specified device
  // SELECT upc, title, producer, category, genre, channel_num, SUM(current_play_time) totalPlaytime, ((strftime('%s', 'now') - date_in) / 604800) as numWeeks FROM view_times WHERE collection_date = 1408597200 GROUP BY upc, title, producer, channel_num, category, genre ORDER BY SUM(current_play_time)
  QVariantList getViewTimeCollection(QDate collectionDate, int deviceNum = 0);

  // Get list of collection dates in descending order
  QList<QDate> getCollectionDates(QStringList addresses);

  // Connect to each casplayer_X.sqlite database in deviceList and get collection for specified date
  // device_num, current_credits, total_credits, current_preview_credits, total_preview_credits, current_cash, total_cash, current_cc_charges, total_cc_charges, current_prog_credits, total_prog_credits
  QVariantList getCollection(QDate collectionDate, QStringList deviceList);

  // Get list of daily meter dates in descending order
  QList<QDate> getMeterDates();

  // Return the latest movie change set (year and week #)
  MovieChangeInfo getLatestMovieSet();

  // Assumes the casplayer_*.sqlite databases have been recently copied from booths.
  // Gets the device record from each database and inserts into the daily_collection table of cas-mgr.sqlite
  QVariantList performDailyCollection(QStringList addresses);

  QVariantList getCollectionSnapshot(QStringList addresses, quint8 daysInCollection, QDate startDate = QDate());

  // Get clerk sessions history
  QVariantList getclerkSessionsByDate(QDate selectedDate, QStringList addresses);

  // Returns movie change sets from the history table
  QList<MovieChangeInfo> getExportedMovieChangeSets();

  QString getDocumentation();

  QString getChangelog();

  QStringList getTipsOfTheDay();

  // Return a count of the number of movies ever sent to the booths
  int getNumMoviesSent(bool *ok);

  // Get what the current movie is for a given channel # in the videos table
  Movie getCurrentMovieAtChannel(int channelNum, bool *ok);

  // Get list of movie change dates in descending order
  QStringList getMovieChangeDates();

  // Get list of movies in the the selected movie change (upc, title, producer, category, subcategory, channel_num, previous_movie)
  QVariantList getMovieChangeDetails(QString movieChangeDate);

  DvdCopyTask getArchivedDvdCopyTask(QString upc);

  QList<MovieChangeInfo> getMovieChangeQueue(int maxChannels);
  bool insertMovieChangeQueue(MovieChangeInfo m);
  bool updateStatusMovieChangeQueue(MovieChangeInfo m);
  bool deleteMovieChangeQueue(MovieChangeInfo m);
  bool clearMovieChangeQueue();

  // Return the download_date of the most recent movie change from the movie_changes_history table
  // If no address is specified then moviechanges.sqlite is used otherwise moviechanges_<address>.sqlite is used
  QDateTime getLatestMovieChangeDate(QString address = QString());

  // These methods start asynchronous calls to the CouchDB server so
  // the caller needs to connect the equivalent signals to get responses
  void getCards();
  void getCard(QString cardNum);
  void addCard(ArcadeCard &card);
  void updateCard(ArcadeCard &card);
  void deleteCard(ArcadeCard &card);
  void bulkDeleteCards(QList<ArcadeCard> &cards);
  void getUsers();
  void addUser(UserAccount &userAccount);
  void updateUser(UserAccount &userAccount);
  void deleteUser(UserAccount &userAccount);
  void getExpiredCards();
  void addBoothStatus(QJsonDocument &doc);
  void deleteBoothStatus(QString id, QString revID);
  void getBoothStatuses();  

  bool insertLeastViewedMovies(QList<Movie> movies);
  bool deleteLeastViewedMovies();
  QList<Movie> getLeastViewedMovies(QString category, bool *ok);
  QMap<QString, QList<Movie> > getBoothsWithMissingMovies(QStringList addresses, bool *ok);

  bool updateVideosFromMovieChange(int year, int weekNum);
  QVariantList getMovieChangeData(int year, int weekNum);

  bool shiftChannels(int channelNum);

  int getMaxMovieChangeYear(bool *ok);

signals:
  void verifyCouchDbFinished(QString errorMessage, bool ok);
  void getCardsFinished(QList<ArcadeCard> &cards, bool ok);
  void addCardFinished(ArcadeCard &card, bool ok);
  void updateCardFinished(ArcadeCard &card, bool ok);
  void deleteCardFinished(QString cardNum, bool ok);
  void bulkDeleteCardsFinished(int numCards, bool ok);
  void cardChangesDetected();
  void getCardFinished(ArcadeCard &card, bool ok);
  void getUsersFinished(QList<UserAccount> &users, bool ok);
  void addUserFinished(QString username, bool ok);
  void deleteUserFinished(QString username, bool ok);
  void getUserFinished(UserAccount &userAccount, bool ok);
  void updateUserFinished(UserAccount &userAccount, bool ok);
  void getExpiredCardsFinished(QList<ArcadeCard> &cards, bool ok);
  void addBoothStatusFinished(QString id, bool ok);
  void deleteBoothStatusFinished(QString id, bool ok);
  void getBoothStatusesFinished(QList<QJsonObject> &docs, bool ok);
  
private slots:
  void installationChecked(const CouchDBResponse &response);
  void databaseCreated(const CouchDBResponse &response);
  void databasesListed(const CouchDBResponse &response);
  void documentUpdated(const CouchDBResponse &response);
  void documentCreated(const CouchDBResponse &response);
  void documentDeleted(const CouchDBResponse &response);
  void databaseViewRetrieved(const CouchDBResponse &response);
  void documentRetrieved(const CouchDBResponse &response);
  void bulkDocumentsDeleted(const CouchDBResponse &response);

private:
  QSqlDatabase db;
  QString dataPath;
  CouchDB *couchDB;
};

#endif // DATABASEMGR_H
