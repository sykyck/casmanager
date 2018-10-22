#include "dvdcopytask.h"

DvdCopyTask::DvdCopyTask(QObject *parent) : QObject(parent)
{
  year = 0;
  weekNum = 0;
  upc.clear();
  title.clear();
  producer.clear();
  category.clear();
  subcategory.clear();
  frontCover.clear();
  backCover.clear();
  fileLength = 0;
  copyStatus = DvdCopyTask::Pending;
  seqNum = 0;
  transcodeStatus = DvdCopyTask::Pending;
}

DvdCopyTask::DvdCopyTask(const DvdCopyTask &other, QObject *parent) : QObject(parent)
{
  year = other.Year();
  weekNum = other.WeekNum();
  upc = other.UPC();
  title = other.Title();
  producer = other.Producer();
  category = other.Category();
  subcategory = other.Subcategory();
  frontCover = other.FrontCover();
  backCover = other.BackCover();
  fileLength = other.FileLength();
  copyStatus = other.TaskStatus();
  seqNum = other.SeqNum();
  transcodeStatus = other.TranscodeStatus();
}

DvdCopyTask &DvdCopyTask::operator =(const DvdCopyTask &other)
{
  year = other.Year();
  weekNum = other.WeekNum();
  upc = other.UPC();
  title = other.Title();
  producer = other.Producer();
  category = other.Category();
  subcategory = other.Subcategory();
  frontCover = other.FrontCover();
  backCover = other.BackCover();
  fileLength = other.FileLength();
  copyStatus = other.TaskStatus();
  seqNum = other.SeqNum();
  transcodeStatus = other.TranscodeStatus();

  return *this;
}

DvdCopyTask::Status DvdCopyTask::TaskStatus() const
{
  return copyStatus;
}

void DvdCopyTask::setTaskStatus(const Status &value)
{
  copyStatus = value;
}

QString DvdCopyTask::TaskStatusToString() const
{
  const QMetaObject &mo = DvdCopyTask::staticMetaObject;
  int index = mo.indexOfEnumerator("Status");
  QMetaEnum metaEnum = mo.enumerator(index);

  return QString(metaEnum.key(copyStatus));
}

QString DvdCopyTask::ConvertStatusToString(DvdCopyTask::Status status)
{
  const QMetaObject &mo = DvdCopyTask::staticMetaObject;
  int index = mo.indexOfEnumerator("Status");
  QMetaEnum metaEnum = mo.enumerator(index);

  return QString(metaEnum.key(status));
}

quint64 DvdCopyTask::FileLength() const
{
  return fileLength;
}

void DvdCopyTask::setFileLength(const quint64 &value)
{
  fileLength = value;
}

QString DvdCopyTask::BackCover() const
{
  return backCover;
}

void DvdCopyTask::setBackCover(const QString &value)
{
  backCover = value;
}

QString DvdCopyTask::FrontCover() const
{
  return frontCover;
}

void DvdCopyTask::setFrontCover(const QString &value)
{
  frontCover = value;
}

QString DvdCopyTask::Subcategory() const
{
  return subcategory;
}

void DvdCopyTask::setSubcategory(const QString &value)
{
  subcategory = value;
}

QString DvdCopyTask::Category() const
{
  return category;
}

void DvdCopyTask::setCategory(const QString &value)
{
  category = value;
}

QString DvdCopyTask::Producer() const
{
  return producer;
}

void DvdCopyTask::setProducer(const QString &value)
{
  producer = value;
}

QString DvdCopyTask::Title() const
{
  return title;
}

void DvdCopyTask::setTitle(const QString &value)
{
  title = value;
}

QString DvdCopyTask::UPC() const
{
  return upc;
}

void DvdCopyTask::setUPC(const QString &value)
{
  upc = value;
}

int DvdCopyTask::WeekNum() const
{
  return weekNum;
}

void DvdCopyTask::setWeekNum(int value)
{
  weekNum = value;
}

int DvdCopyTask::Year() const
{
  return year;
}

void DvdCopyTask::setYear(int value)
{
  year = value;
}

bool DvdCopyTask::Complete() const
{
  return (!upc.isEmpty() && !title.isEmpty() && !producer.isEmpty() &&
          !category.isEmpty() && !subcategory.isEmpty() &&
          !frontCover.isEmpty() && !backCover.isEmpty());
}

int DvdCopyTask::SeqNum() const
{
  return seqNum;
}

void DvdCopyTask::setSeqNum(int value)
{
  seqNum = value;
}

DvdCopyTask::Status DvdCopyTask::TranscodeStatus() const
{
  return transcodeStatus;
}

void DvdCopyTask::setTranscodeStatus(Status value)
{
  transcodeStatus = value;
}

QString DvdCopyTask::TranscodeStatusToString() const
{
  const QMetaObject &mo = DvdCopyTask::staticMetaObject;
  int index = mo.indexOfEnumerator("Status");
  QMetaEnum metaEnum = mo.enumerator(index);

  return QString(metaEnum.key(transcodeStatus));
}
