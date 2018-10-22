#ifndef MOVIELIBRARYWIDGET_H
#define MOVIELIBRARYWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTableView>
#include <QStandardItemModel>
#include <QTimer>
#include <QProcess>
#include "databasemgr.h"
#include "dvdcopytask.h"
#include "settings.h"
#include "videolookupservice.h"
#include "mediainfowidget.h"
#include "casserver.h"
#include "dvdcopier.h"
#include "discautoloader.h"
#include "discautoloadercontrolpanel.h"
#include "filesystemwatcher.h"
#include "filecopier.h"
#include "videoinfowidget.h"

class MovieLibraryWidget : public QWidget
{
  Q_OBJECT
public:
  explicit MovieLibraryWidget(DatabaseMgr *dbMgr, CasServer *casServer, Settings *settings, QWidget *parent = 0);
  ~MovieLibraryWidget();
  bool isBusy();

  enum Drive_State
  {
    Unknown,
    Import,
    Export
  };

protected:
  bool eventFilter(QObject *obj, QEvent *event);
  void showEvent(QShowEvent *);

signals:
  bool abortTranscoding();
  void initializeCopyProgress(int totalFiles);
  void updateCopyProgress(int numFiles, const QString &overallStatus);
  void cleanupCopyProgress();
  void waitingForExternalDrive();
  void finishedWaitingForExternalDrive();
  void queueDeleteVideo(MovieChangeInfo &movieChangeInfo);
  
public slots:
  void updateEncodeStatus(QString upc, DvdCopyTask::Status status, quint64 fileSize = 0);
  void populateVideoLibrary();
  
private slots:
  void addNewJob();
  void startCopyProcess();
  void retryCopyProcess();
  void deleteJob();
  void dvdCopierStarted();
  void checkUPCs();
  void checkUPCsResult(QList<Movie> videoList);
  void checkVideoUpdateResult(bool success, const QString &message);
  void editDvdCopyTask(const QModelIndex &index);
  void editMovieChangeSetItem(const QModelIndex &index);
  void updateMetadata();
  void metadataUpdateCanceled();
  //void importVideosFromDisk();
  void deleteMovieChangeEntry();
  void showAutoloaderCP();
  void dvdCopierFinished(bool success, QString errorMessage = QString());
  void loadDiscFinished(bool success, QString errorMessage = QString());
  void unloadDiscFinished(bool success, QString errorMessage = QString());
  void checkDeleteUPCs();
  void deleteVideoLookupResults(bool success, const QString &message, const QStringList &upcList);
  void exportButtonClicked();
  void importButtonClicked();
  void externalDriveMounted(QString path);
  void externalDriveTimeout();
  void determinedNumFilesToCopy(int totalFiles);
  void copyProgress(int numFiles);
  void finishedCopying();
  void copyNextMovieChangeSet();
  void refreshMovieChangeSetsView();
  void downloadMetadataClicked();
  void cancelJobClicked();
  void viewVideoLibraryEntry(const QModelIndex &index);
  void deleteVideoLibraryEntry();

private:
  void insertMovieChangeSetRow(DvdCopyTask task);
  void insertDvdCopyJobRow(DvdCopyTask task);
  void insertVideoRow(Movie video);
  int findMovieChangeSetRow(QString upc);
  int findDvdCopyJobRow(QString upc);
  int findVideoRow(QString upc);
  void clearMovieChangeSets();
  void clearMovieCopyJobs();
  void clearMovieLibrary();
  bool videoFilesComplete(QString path);
  void finishCopyJob();
  void prepareDestination(QString destPath);
  bool freeUpDiskSpace(qreal requiredDiskSpace, qreal maxCapacity, qreal freeDiskSpace);
  QStringList getUPCsNeedingMetadata();

  QStandardItemModel *libraryModel;
  QStandardItemModel *movieChangeSetsModel;
  QStandardItemModel *movieCopyJobModel;
  QTableView *currentLibraryTable;
  QTableView *movieChangeSetsTable;
  QTableView *movieCopyJobTable;
  QLabel *lblLibrary;
  QLabel *lblTotalChannels;
  QLabel *lblMovieChangeSets;
  QLabel *lblCurrentJob;
  QPushButton *btnNewJob;
  QPushButton *btnRetryJob;
  QPushButton *btnDeleteJob;
  QPushButton *btnShowAutoloaderCP;
  QPushButton *btnDownloadMetadata;
  QPushButton *btnCancelJob;
  QPushButton *btnImport;
  QPushButton *btnExport;
  QPushButton *btnDeleteVideo;
  QHBoxLayout *buttonLayout;
  QHBoxLayout *videoLibraryLayout;
  QVBoxLayout *verticalLayout;
  QHBoxLayout *importExportButtonLayout;
  DatabaseMgr *dbMgr;
  Settings *settings;
  CasServer *casServer;
  QTimer *upcLookupTimer;
  QTimer *deleteUpcTimer;
  //QTimer *processJobTimer;
  int currentDvdIndex;
  bool firstLoad;
  bool busy;
  VideoLookupService *lookupService;
  MediaInfoWidget *mediaInfoWin;
  VideoInfoWidget *videoInfoWin;
  DiscAutoLoader *discAutoloader;
  DvdCopier *dvdCopier;
  FileSystemWatcher *fsWatcher;
  QList<MovieChangeInfo> selectedImportExportMovieChangeSets;
  QList<MovieChangeInfo> pendingImportExportMovieChangeSets;
  MovieChangeInfo currentImportExportMovieChangeSet;
  QString externalDriveMountPath;
  FileCopier *fileCopier;
  int totalFilesToExport;
  Drive_State driveState;
  bool userRequestedMetadata;
  bool userCanceledJob;
};

#endif // MOVIELIBRARYWIDGET_H
