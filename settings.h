#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QMetaEnum>
#include <QTime>

// List of known device addresses in the arcade
const QString DEVICE_LIST = "device_list";

// First 3 octets of the IP addresses used in the arcade. Such as: 10.0.0.
const QString ARCADE_SUBNET = "arcade_subnet";
const QString DEFAULT_ARCADE_SUBNET = "10.0.0.";

// First usable IP address that can be assigned to a booth
const QString MIN_ADDRESS = "min_address";
const QString DEFAULT_MIN_ADDRESS = "10.0.0.21";

// Last usable IP address that can be assigned to a booth
const QString MAX_ADDRESS = "max_address";
const QString DEFAULT_MAX_ADDRESS = "10.0.0.21";

// Passphrase used for authenticating with booths over the network (encrypted)
const QString NETWORK_PASSPHRASE = "network_passphrase";
const QString DEFAULT_NETWORK_PASSPHRASE = "U2FsdGVkX19olXHogC6mtj5mtCh5yCfuHPwjy3YypSGJP1qz3ws6S7vT6XEfjdut\n"; // encrypted

// Minimum amount of free disk space (in GB) before videos have to be deleted
const QString MIN_FREE_DISKSPACE = "min_free_diskspace";
const qreal DEFAULT_MIN_FREE_DISKSPACE = 33.0;

// Time of day to begin downloading videos from server
const QString MOVIE_CHANGE_TIME = "movie_change_time";
const QString DEFAULT_MOVIE_CHANGE_TIME = "04:00";

// Path to directory containing databases shared with booths
const QString DATA_PATH = "data_path";
const QString DEFAULT_DATA_PATH = "share/data";

// Path to directory containing videos that have been copied with the DVD program
const QString VIDEO_PATH = "video_path";
const QString DEFAULT_VIDEO_PATH = "share/videos";

// Path to directory where video metadata is temporarily downloaded to
const QString VIDEO_METADATA_PATH = "video_metadata_path";
const QString DEFAULT_VIDEO_METADATA_PATH = "share/videos/metadata";

// File path to DVD copying program
const QString DVD_COPY_PROG = "dvd_copy_prog";
const QString DEFAULT_DVD_COPY_PROG = "/var/cas-mgr/copy-dvd.sh";

// Password used to contact web services (encrypted)
const QString CUSTOMER_PASSWORD = "customer_password";
const QString DEFAULT_CUSTOMER_PASSWORD = "U2FsdGVkX1/m9UyoP86/NbbnpK8/xsytDLW9pFvtUDDBtYuxt93SGRj38gs7EHaO\n346CxlHMlfrLucrD3tVQAw==\n"; // encrypted

// Flag set when the disc auto-loader should be used
const QString ENABLE_AUTO_LOADER = "enable_auto_loader";
const bool DEFAULT_ENABLE_AUTO_LOADER = true;

// Flag set when view times should be uploaded to our server
const QString UPLOAD_VIEW_TIMES = "upload_view_times";
const bool DEFAULT_UPLOAD_VIEW_TIMES = false;

// How often to check booth status in minutes
const QString BOOTH_STATUS_INTERVAL = "booth_status_interval";
const int DEFAULT_BOOTH_STATUS_INTERVAL = 15;

// Number of milliseconds between software update check
const QString SOFTWARE_UPDATE_INTERVAL = "software_update_interval";
const int DEFAULT_SOFTWARE_UPDATE_INTERVAL = 60 * 60 * 1000; // 60 min x 60 sec x 1000 msec

// Number of milliseconds between movie change status check
const QString MOVIE_CHANGE_STATUS_INTERVAL = "movie_change_status_interval";
const int DEFAULT_MOVIE_CHANGE_STATUS_INTERVAL = 10 * 60 * 1000; // 10 min x 60 sec x 1000 msec

// Number of milliseconds between UPC lookups
const QString UPC_LOOKUP_INTERVAL = "upc_lookup_interval";
const int DEFAULT_UPC_LOOKUP_INTERVAL = 60 * 60 * 1000; // 60 min x 60 sec x 1000 msec

// Flag set if we should share movie metadata with web server
const QString UPLOAD_MOVIE_METADATA = "upload_movie_metadata";
const bool DEFAULT_UPLOAD_MOVIE_METADATA = true;

// Number of SECONDS between restarts
//const QString RESTART_DEVICE_INTERVAL = "restart_device_interval";
//const int DEFAULT_RESTART_DEVICE_INTERVAL = 24 * 60 * 60; // 24 hours

// Time of day the booths should be restarted
const QString RESTART_DEVICE_TIME = "restart_device_time";
const QTime DEFAULT_RESTART_DEVICE_TIME = QTime::fromString("04:00", "hh:mm");

// Last time the devices were restarted
const QString LAST_RESTART_DEVICE_DATE = "last_restart_device_date";

// Flag set when no back cover checkbox should be checked by default
const QString ENABLE_NO_BACK_COVER = "enable_no_back_cover";
const bool DEFAULT_ENABLE_NO_BACK_COVER = false;

// Enforce a maximum number of channels in the arcade booths, set to zero for no limit
// The minimum free disk space is still taken into consideration during a movie change
const QString MAX_CHANNELS = "max_channels";
const int DEFAULT_MAX_CHANNELS = 0;

// Device path of DVD drive (example: /dev/cdrom1)
const QString DVD_DRIVE_MOUNT = "dvd_drive_mount";
const QString DEFAULT_DVD_DRIVE_MOUNT = "/media/cdrom1";

// Drive letter in WINE that is assigned to the device path of DVD drive
const QString WINE_DVD_DRIVE_LETTER = "wine_dvd_drive_letter";
const QString DEFAULT_WINE_DVD_DRIVE_LETTER = "F";

// Device path of the autoloader's DVD drive (example: /dev/sr1)
const QString AUTOLOADER_DVD_DRIVE_MOUNT = "autoloader_dvd_drive_mount";
const QString DEFAULT_AUTOLOADER_DVD_DRIVE_MOUNT = "/dev/sr1";

// Drive letter in WINE that is assigned to the device path of the autoloader's DVD drive
const QString WINE_AUTOLOADER_DVD_DRIVE_LETTER = "wine_autoloader_dvd_drive_letter";
const QString DEFAULT_WINE_AUTOLOADER_DVD_DRIVE_LETTER = "F";

// File path of the log file created by DVD Decrypter
const QString DVD_COPIER_LOG_FILE = "dvd_copier_log_file";
const QString DEFAULT_DVD_COPIER_LOG_FILE = "/var/cas-mgr/DVDDecrypter.log";

// DVD Decrypter process name
const QString DVD_COPIER_PROC_NAME = "dvd_copier_proc_name";
const QString DEFAULT_DVD_COPIER_PROC_NAME = "DVDDecrypter";

// File path to autoloader client program
const QString AUTOLOADER_PROG_FILE = "autoloader_prog_file";
const QString DEFAULT_AUTOLOADER_PROG_FILE = "/var/cas-mgr/autoloader";

// File path to autoloader program's response file
const QString AUTOLOADER_RESPONSE_FILE = "autoloader_response_file";
const QString DEFAULT_AUTOLOADER_RESPONSE_FILE = "/var/cas-mgr/autoloader-00/response";

// File path to file created each time a DVD is mounted in the autoloader's DVD drive
const QString AUTOLOADER_DVD_DRIVE_STATUS_FILE = "autoloader_dvd_drive_status_file";
const QString DEFAULT_AUTOLOADER_DVD_DRIVE_STATUS_FILE = "/var/cas-mgr/drive-status.log";

const QString WINE_DVD_COPY_DEST_DRIVE_LETTER = "wine_dvd_copy_dest_drive_letter";
const QString DEFAULT_WINE_DVD_COPY_DEST_DRIVE_LETTER = "R";

// Number of minutes to wait for DVD Decrypter to copy disc
const QString DVD_COPIER_TIMEOUT = "dvd_copier_timeout";
const int DEFAULT_DVD_COPIER_TIMEOUT = 30;

// File path to UFTP server program
const QString FILE_SERVER_PROG_FILE = "file_server_prog_file";
const QString DEFAULT_FILE_SERVER_PROG_FILE = "/var/cas-mgr/uftp";

// File path to UFTP log
const QString FILE_SERVER_LOG_FILE = "file_server_log_file";
const QString DEFAULT_FILE_SERVER_LOG_FILE = "/var/cas-mgr/uftp.log";

// Milliseconds to wait for a DVD that was inserted to be mounted
const QString DVD_MOUNT_TIMEOUT = "dvd_mount_timeout";
const int DEFAULT_DVD_MOUNT_TIMEOUT = 90000; // Was 50 seconds but moved to 90 after seeing some DVDs take a long time to mount

// File path to transcoding program/script
const QString TRANSCODER_PROG_FILE = "transcoder_prog_file";
const QString DEFAULT_TRANSCODER_PROG_FILE = "/var/cas-mgr/transcode-video.sh";

// File path to transcoding log
const QString TRANSCODER_LOG_FILE = "transcoder_log_file";
const QString DEFAULT_TRANSCODER_LOG_FILE = "/var/cas-mgr/transcode-video.log";

// Flag set when view times and been collected and view times cleared
const QString ALLOW_MOVIE_CHANGE = "allow_movie_change";
const bool DEFAULT_ALLOW_MOVIE_CHANGE = false;

// Flag set when the Category drop down in the add video widget should be shown
const QString SHOW_EXTRA_ADD_VIDEO_FIELDS = "show_extra_add_video_fields";
const bool DEFAULT_SHOW_EXTRA_ADD_VIDEO_FIELDS = false;

// Flag set when the No Back Cover option in the media info widget should be shown
const QString SHOW_NO_BACKCOVER_OPTION = "show_no_backcover_option";
const bool DEFAULT_SHOW_NO_BACKCOVER_OPTION = false;

// Maximum number of videos in a movie change set
const QString MAX_MOVIE_CHANGE_SET_SIZE = "max_movie_change_set_size";
const int DEFAULT_MAX_MOVIE_CHANGE_SET_SIZE = 13;

// Name of the store the CAS Manager is located at
const QString STORE_NAME = "store_name";

// Flag set if collection report should be sent after each booth collection operation
const QString ENABLE_COLLECTION_REPORT = "enable_collection_report";
const bool DEFAULT_ENABLE_COLLECTION_REPORT = false;

// Comma-delimited list of email addresses to send collection report to
const QString COLLECTION_REPORT_RECIPIENTS = "collection_report_recipients";
const QString DEFAULT_COLLECTION_REPORT_RECIPIENTS = "";

// Time to send daily meters report
const QString DAILY_METERS_TIME = "daily_meters_time";
const QTime DEFAULT_DAILY_METERS_TIME = QTime::fromString("00:00", "hh:mm");

// Number of seconds between daily meter collections
const QString DAILY_METERS_INTERVAL = "daily_meters_interval";
const int DEFAULT_DAILY_METERS_INTERVAL = 24 * 60 * 60; // 24 hours

// Flag set if daily meters should be sent
const QString ENABLE_DAILY_METERS = "enable_daily_meters";
const bool DEFAULT_ENABLE_DAILY_METERS = false;

// Flag set if collection snapshot should be sent
const QString ENABLE_COLLECTION_SNAPSHOT = "enable_collection_snapshot";
const bool DEFAULT_ENABLE_COLLECTION_SNAPSHOT = false;

// Comma-delimited list of email addresses to send daily meters report to
const QString DAILY_METERS_REPORT_RECIPIENTS = "daily_meters_report_recipients";

// Comma-delimited list of email addresses to collection snapshot report to
const QString COLLECTION_SNAPSHOT_REPORT_RECIPIENTS = "collection_snapshot_report_recipients";
const QString DEFAULT_COLLECTION_SNAPSHOT_REPORT_RECIPIENTS = "";

// How many days between each collection snapshot
const QString COLLECTION_SNAPSHOT_INTERVAL = "collection_snapshot_interval";
const int DEFAULT_COLLECTION_SNAPSHOT_INTERVAL = 13;

// Day of week to perform collection snapshot, 1 = Monday to 7 = Sunday
const QString COLLECTION_SNAPSHOT_SCHEDULE_DAY = "collection_snapshot_schedule_day";
const int DEFAULT_COLLECTION_SNAPSHOT_SCHEDULE_DAY = 1;

// First scheduled collection snapshot - all subsequent collections will be on this day of the week
const QString FIRST_COLLECTION_SNAPSHOT_DATE = "first_collection_snapshot_date";

// Last time the collection snapshot was sent
const QString LAST_COLLECTION_SNAPSHOT_DATE = "last_collection_snapshot_date";
const QDateTime DEFAULT_LAST_COLLECTION_SNAPSHOT_DATE = QDateTime::fromString("1/1/2000 0:00");

// Last time the daily meters was sent
const QString LAST_DAILY_METERS_DATE = "last_daily_meters_date";

// Maximum size of a transcoded video file in bytes
const QString MAX_TRANSCODED_VIDEO_FILE_SIZE = "max_transcoded_video_file_size";
const qlonglong DEFAULT_MAX_TRANSCODED_VIDEO_FILE_SIZE = 5905580032; // 5.5 GB

// Transmission speed in bits to send data to the booths with UFTP. 10Kbits has been a good speed, not too slow and not too fast.
// When it's set too high, it causes the booths to lock up
const QString TRANSMISSION_SPEED = "transmission_speed";
const int DEFAULT_TRANSMISSION_SPEED = 10000;

// UFTP can be configured to write to an easily parsable file that provides status information about each client and the files being sent
const QString FILE_SERVER_STATUS_FILE = "file_server_status_file";
const QString DEFAULT_FILE_SERVER_STATUS_FILE = "/var/cas-mgr/uftp-status.log";

// The file contains a list of files to send by UFTP
const QString FILE_SERVER_LIST_FILE = "file_server_list_file";
const QString DEFAULT_FILE_SERVER_LIST_FILE = "/var/cas-mgr/uftp-file-list.conf";

// The number of seconds to subtract from the scheduled movie change date/time so that the booths
// have UFTPD already running by the time UFTP on the server begins contacting clients
const QString MOVIE_CHANGE_SCHEDULE_TIME_ADJUST = "movie_change_schedule_time_adjust";
const int DEFAULT_MOVIE_CHANGE_SCHEDULE_TIME_ADJUST = 300;

// Application mode
const QString APP_MODE = "app_mode";
const int DEFAULT_APP_MODE = 0;

// External hard drive mount path
const QString EXT_HD_MOUNT_PATH = "ext_hd_mount_path";
const QString DEFAULT_EXT_HD_MOUNT_PATH = "/media/cas-videos";

// Time in milliseconds to wait for external hard drive
const QString EXT_HD_MOUNT_TIMEOUT = "ext_hd_mount_timeout";
const int DEFAULT_EXT_HD_MOUNT_TIMEOUT = 30000;

// Internal drive path for checking disk space when importing videos
const QString INTERNAL_DRIVE_PATH = "internal_drive_path";
const QString DEFAULT_INTERNAL_DRIVE_PATH = "/dev/sda1";

// Time in milliseconds to wait between checking for UPCs to send to server to be deleted from lookup queue
const QString DELETE_UPC_INTERVAL = "delete_upc_interval";
const int DEFAULT_DELETE_UPC_INTERVAL = 5 * 60 * 1000; // 5 min x 60 sec x 1000 msec

// Flag set if this is the first time the software has been run
const QString FIRST_RUN = "first_run";
const bool DEFAULT_FIRST_RUN = true;

// Flag set if the Help Center should be shown on startup
const QString SHOW_HELP_CENTER_ON_STARTUP = "show_help_center_on_startup";
const bool DEFAULT_SHOW_HELP_CENTER_ON_STARTUP = true;

// Flag set if share cards
const QString USE_SHARE_CARDS = "use_share_cards";
const bool DEFAULT_USE_SHARE_CARDS = false;

const QString REMOTE_COUCHDB = "remote_couch_db";
const QString DEFAULT_REMOTE_COUCHDB = "";

// Flag set if the Help Center should be shown on startup with the Changelog tab
// selected. This overrides the show_help_center_on_startup flag but the flag is cleared
// after first execution. This is typically done after the CAS Manager is updated
const QString SHOW_CHANGELOG_ON_STARTUP = "show_changelog_on_startup";
const bool DEFAULT_SHOW_CHANGELOG_ON_STARTUP = true;

// Current tip of the day index
const QString TIP_OF_THE_DAY_INDEX = "tip_of_the_day_index";
const int DEFAULT_TIP_OF_THE_DAY_INDEX = 0;

// Day of week to perform view time collection, 1 = Monday to 7 = Sunday
const QString VIEWTIME_COLLECTION_SCHEDULE_DAY = "viewtime_collection_schedule_day";
const int DEFAULT_VIEWTIME_COLLECTION_SCHEDULE_DAY = 1;

// Time of day to perform view time collection: hh:mm
const QString VIEWTIME_COLLECTION_SCHEDULE_TIME = "viewtime_collection_schedule_time";
const QTime DEFAULT_VIEWTIME_COLLECTION_SCHEDULE_TIME = QTime::fromString("05:00", "hh:mm");

// Last date/time the view time collection was performed
const QString LAST_VIEWTIME_COLLECTION = "last_viewtime_collection";
const QDateTime DEFAULT_LAST_VIEWTIME_COLLECTION = QDateTime::fromString("1/1/2000 0:00");

// Last date/time a movie change was done
const QString LAST_MOVIE_CHANGE = "last_movie_change";

// List of IP addresses of booths that are not responding
const QString BOOTHS_DOWN = "booths_down";

const QString MOVIE_CHANGE_DURATION_THRESHOLD = "movie_change_duration_threshold";
const uint DEFAULT_MOVIE_CHANGE_DURATION_THRESHOLD = 60 * 60 * 18; // 18 hours in seconds

// Flag set if information on the movie change queuing should be shown when clicking Add Movie Change button
const QString SHOW_NEW_MOVIE_CHANGE_FEATURE = "show_new_movie_change_feature";
const bool DEFAULT_SHOW_NEW_MOVIE_CHANGE_FEATURE = true;

// Number of times a booth won't respond before considering it down
const QString BOOTH_NO_RESPONSE_THRESHOLD = "booth_no_response_threshold";
const int DEFAULT_BOOTH_NO_RESPONSE_THRESHOLD = 2;

// Minimum number of weeks a movie should be in the arcade before possibly being replaced during a movie change
const QString MIN_NUM_WEEKS_MOVIECHANGE = "min_num_weeks_moviechange";
const int DEFAULT_MIN_NUM_WEEKS_MOVIECHANGE = 2;

// Max movie change retries before giving up on movie change
const QString MAX_MOVIECHANGE_RETRIES = "max_movie_change_retries";
const int DEFAULT_MAX_MOVIECHANGE_RETRIES = 3;

// Time to wait between movie change retries in milliseconds
const QString MOVIECHANGE_RETRY_TIMEOUT = "movie_change_retry_timeout";
const int DEFAULT_MOVIECHANGE_RETRY_TIMEOUT = 1000 * 60 * 5; // 5 minutes in milliseconds

// Time in milliseconds to wait between refreshing the cards data
const QString UPDATE_CARDS_INTERVAL = "update_cards_interval";
const int DEFAULT_UPDATE_CARDS_INTERVAL = 5 * 60 * 1000; // 5 min x 60 sec x 1000 msec

// Time in milliseconds to authorize the user
const QString AUTH_INTERVAL = "auth_interval";
const int DEFAULT_AUTH_INTERVAL = 5 * 60 * 1000; // 5 min x 60 sec x 1000 msec

// Last time check for expired cards was done
const QString LAST_EXPIRED_CARDS_CHECK_DATE = "last_expired_cards_check_date";

// Starting value used for deleting movies from the arcade
const QString DELETE_MOVIE_YEAR = "delete_movie_year";
const int DEFAULT_DELETE_MOVIE_YEAR = 123456;

// List of possible values for Authorize.net x_market_type field
const QString AUTHORIZE_NET_MARKET_TYPES = "authorize_net_market_types";
const QStringList DEFAULT_AUTHORIZE_NET_MARKET_TYPES = (QStringList() << "0 - E-commerce" << "1 - Moto" << "2 - Retail");

// List of possible values for Authorize.net x_device_type field
const QString AUTHORIZE_NET_DEVICE_TYPES = "authorize_net_device_types";
const QStringList DEFAULT_AUTHORIZE_NET_DEVICE_TYPES = (QStringList() << "0 - N/A" << "1 - Unknown" << "2 - Unattended Terminal"
                                                        << "3 - Self Service Terminal" << "4 - Electronic Cash Register"
                                                        << "5 - Personal Computer-Based Terminal" << "6 - AirPay"
                                                        << "7 - Wireless POS" << "8 - Website" << "9 - Dial Terminal"
                                                        << "10 - Virtual Terminal");

class Settings : public QObject
{
  Q_OBJECT
public:
  enum Device_Type
  {
    Arcade = 1,
    Preview,
    Theater,
    ClerkStation
  };

  enum UI_Type
  {
    Simple = 1,
    Categories,
    Full
  };

  enum App_Mode
  {
    All_Functions,    // Normal mode
    DVD_Copying_Only, // Only show Movie Library tab
    No_DVD_Copying    // Normal mode but no functions related to copying DVDs
  };

  Q_ENUMS(Device_Type)
  Q_ENUMS(UI_Type)
  Q_ENUMS(App_Mode)

  explicit Settings(QString settings, QObject *parent = 0);
  QVariant getValue(QString keyName, QVariant defaultValue = QVariant(), bool decrypt = false);
  void setValue(QString keyName, QVariant value, bool encrypt = false);
  void loadSettings(QString settings);
  QString getSettings();

private:
  QVariantMap _settings;

  // Destination path in the Windows VM to copy DVDs to. This is a shared folder between the host and guest OS
};

#endif // SETTINGS_H
