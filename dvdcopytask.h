#ifndef DVDCOPYTASK_H
#define DVDCOPYTASK_H

#include <QObject>
#include <QString>
#include <QMetaType>
#include <QMetaEnum>
#include <QDateTime>

class DvdCopyTask : public QObject
{
  Q_OBJECT
public:  
  explicit DvdCopyTask(QObject *parent = 0);
  DvdCopyTask(const DvdCopyTask &other, QObject *parent = 0);
  DvdCopyTask &operator =(const DvdCopyTask &other);

  enum Status
  {
    Pending,
    Working,
    Finished,
    Failed,
    Video_Too_Large
  };

  Q_ENUMS(Status)

  int Year() const;
  void setYear(int value);

  int WeekNum() const;
  void setWeekNum(int value);

  QString UPC() const;
  void setUPC(const QString &value);

  QString Title() const;
  void setTitle(const QString &value);

  QString Producer() const;
  void setProducer(const QString &value);

  QString Category() const;
  void setCategory(const QString &value);

  QString Subcategory() const;
  void setSubcategory(const QString &value);

  QString FrontCover() const;
  void setFrontCover(const QString &value);

  QString BackCover() const;
  void setBackCover(const QString &value);

  quint64 FileLength() const;
  void setFileLength(const quint64 &value);

  DvdCopyTask::Status TaskStatus() const;
  void setTaskStatus(const Status &value);

  QString TaskStatusToString() const;

  static QString ConvertStatusToString(DvdCopyTask::Status status);

  // True if upc, title, producer, category, subcategory, frontCover, backCover are not empty
  bool Complete() const;

  // Copy tasks are numbered in the order added to the queue
  // This number is used for sorting tasks if needing to reload from the database
  int SeqNum() const;
  void setSeqNum(int value);

  // Transcoding status
  DvdCopyTask::Status TranscodeStatus() const;
  void setTranscodeStatus(DvdCopyTask::Status value);
  QString TranscodeStatusToString() const;

private:
  int year;
  int weekNum;
  QString upc;
  QString title;
  QString producer;
  QString category;
  QString subcategory;
  QString frontCover;
  QString backCover;
  quint64 fileLength;
  Status copyStatus;
  int seqNum;
  Status transcodeStatus;
};

// Makes the custom data type known to QMetaType so it can be stored in a QVariant
// The class requires a public default constructor, public copy constructor and public destructor
Q_DECLARE_METATYPE(DvdCopyTask)

#endif // DVDCOPYTASK_H
