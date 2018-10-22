#ifndef MOVIECHANGEINFO_H
#define MOVIECHANGEINFO_H

#include <QObject>
#include <QMetaType>
#include <QMetaEnum>
#include "dvdcopytask.h"

class MovieChangeInfo : public QObject
{
  Q_OBJECT
public:
  explicit MovieChangeInfo(QObject *parent = 0);
  MovieChangeInfo(const MovieChangeInfo &other, QObject *parent = 0);
  MovieChangeInfo &operator =(const MovieChangeInfo &other);

  enum MovieChangeStatus
  {
    Pending,
    Processing,
    Copying,
    Waiting_For_Booths,
    Finished,
    Failed,
    Retrying
  };

  Q_ENUMS(MovieChangeStatus)

  static QString MovieChangeStatusToString(MovieChangeInfo::MovieChangeStatus status);

  int Year() const;
  void setYear(int value);

  int WeekNum() const;
  void setWeekNum(int value);

  int NumVideos() const;
  void setNumVideos(int value);

  bool OverrideViewtimes() const;
  void setOverrideViewtimes(bool value);

  int FirstChannel() const;
  void setFirstChannel(int value);

  int LastChannel() const;
  void setLastChannel(int value);

  int MaxChannels() const;
  void setMaxChannels(int value);

  MovieChangeStatus Status() const;
  void setStatus(const MovieChangeStatus &value);

  QString VideoPath() const;
  void setVideoPath(const QString &value);

  QList<DvdCopyTask> DvdCopyTaskList() const;
  void setDvdCopyTaskList(const QList<DvdCopyTask> &value);

private:
  int year;                 // Year of the movie change set
  int weekNum;              // Week # of movie change set although not really used as this, more of just a sequential number
  int numVideos;            // Number of movies in the movie change set, should really represent the size of dvdCopyTaskList but dvdCopyTaskList is not always populated
  bool overrideViewtimes;   // Flag set when ignoring viewtimes and replacing a channel range
  int firstChannel;         // First channel in channel range to replace. Used when overrideViewtimes is set
  int lastChannel;          // Last channel in channel range to replace. Used when overrideViewtimes is set
  int maxChannels;          // 0 = no maximum channel
  MovieChangeStatus status; // Current status of the movie change set in the movie change process
  QString videoPath;        // Full path to where movie change set is located such as: /var/cas-mgr/share/videos/2015/32
  QList<DvdCopyTask> dvdCopyTaskList; // Movies in the movie change set
};

// Makes the custom data type known to QMetaType so it can be stored in a QVariant
// The class requires a public default constructor, public copy constructor and public destructor
Q_DECLARE_METATYPE(MovieChangeInfo)

#endif // MOVIECHANGEINFO_H
