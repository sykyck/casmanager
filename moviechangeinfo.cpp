#include "moviechangeinfo.h"

MovieChangeInfo::MovieChangeInfo(QObject *parent) : QObject(parent)
{
  year = 0;
  weekNum = 0;
  numVideos = 0;
  overrideViewtimes = false;
  firstChannel = 0;
  lastChannel = 0;
  maxChannels = 0;
  videoPath.clear();
  status = Pending;
}

MovieChangeInfo::MovieChangeInfo(const MovieChangeInfo &other, QObject *parent) : QObject(parent)
{
  year = other.Year();
  weekNum = other.WeekNum();
  numVideos = other.NumVideos();
  overrideViewtimes = other.OverrideViewtimes();
  firstChannel = other.FirstChannel();
  lastChannel = other.LastChannel();
  maxChannels = other.MaxChannels();
  videoPath = other.VideoPath();
  status = other.Status();
  dvdCopyTaskList = other.DvdCopyTaskList();
}

MovieChangeInfo &MovieChangeInfo::operator =(const MovieChangeInfo &other)
{
  year = other.Year();
  weekNum = other.WeekNum();
  numVideos = other.NumVideos();
  overrideViewtimes = other.OverrideViewtimes();
  firstChannel = other.FirstChannel();
  lastChannel = other.LastChannel();
  maxChannels = other.MaxChannels();
  videoPath = other.VideoPath();
  status = other.Status();
  dvdCopyTaskList = other.DvdCopyTaskList();

  return *this;
}

QString MovieChangeInfo::MovieChangeStatusToString(MovieChangeInfo::MovieChangeStatus status)
{
  const QMetaObject &mo = MovieChangeInfo::staticMetaObject;
  int index = mo.indexOfEnumerator("MovieChangeStatus");
  QMetaEnum metaEnum = mo.enumerator(index);

  return QString(metaEnum.key(status));
}

QList<DvdCopyTask> MovieChangeInfo::DvdCopyTaskList() const
{
  return dvdCopyTaskList;
}

void MovieChangeInfo::setDvdCopyTaskList(const QList<DvdCopyTask> &value)
{
  dvdCopyTaskList = value;
}

QString MovieChangeInfo::VideoPath() const
{
  return videoPath;
}

void MovieChangeInfo::setVideoPath(const QString &value)
{
  videoPath = value;
}

MovieChangeInfo::MovieChangeStatus MovieChangeInfo::Status() const
{
  return status;
}

void MovieChangeInfo::setStatus(const MovieChangeStatus &value)
{
  status = value;
}

int MovieChangeInfo::MaxChannels() const
{
  return maxChannels;
}

void MovieChangeInfo::setMaxChannels(int value)
{
  if (value >= 0)
    maxChannels = value;
}

int MovieChangeInfo::LastChannel() const
{
  return lastChannel;
}

void MovieChangeInfo::setLastChannel(int value)
{
  if (value >= 0)
    lastChannel = value;
}

int MovieChangeInfo::FirstChannel() const
{
  return firstChannel;
}

void MovieChangeInfo::setFirstChannel(int value)
{
  if (value >= 0)
    firstChannel = value;
}

bool MovieChangeInfo::OverrideViewtimes() const
{
  return overrideViewtimes;
}

void MovieChangeInfo::setOverrideViewtimes(bool value)
{
  overrideViewtimes = value;
}

int MovieChangeInfo::NumVideos() const
{
  return numVideos;
}

void MovieChangeInfo::setNumVideos(int value)
{
  if (value >= 0)
    numVideos = value;
}

int MovieChangeInfo::WeekNum() const
{
  return weekNum;
}

void MovieChangeInfo::setWeekNum(int value)
{
  if (value > 0)
    weekNum = value;
}

int MovieChangeInfo::Year() const
{
  return year;
}

void MovieChangeInfo::setYear(int value)
{
  if (value > 0)
    year = value;
}
