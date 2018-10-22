#ifndef MEDIAINFO_H
#define MEDIAINFO_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QMetaType>
#include <QList>

class Movie : public QObject
{
  Q_OBJECT
public:
  explicit Movie(QObject *parent = 0);
  Movie(const Movie &other, QObject *parent = 0);
  Movie(QString filepath, qint16 channelNum, QObject *parent = 0);
  Movie &operator =(const Movie &other);

  // Returns true if UPCs are equal
  inline bool operator ==(const Movie &other)
  {
    return (upc == other.upc);
  }

  void clear();
  bool isValid();  

  bool writeXml(const QString &filename);

  QString Title() const;
  void setTitle(const QString &value);

  QString UPC() const;
  void setUPC(const QString &value);

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

  QString toString() const;

  QString VideoFile() const;
  void setVideoFile(const QString &value);

  QDateTime DateIn() const;
  void setDateIn(const QDateTime &value);

  QDateTime DateOut() const;
  void setDateOut(const QDateTime &value);

  qint32 PlayPosition() const;
  void setPlayPosition(const qint32 &value);

  qint32 CurrentPlayTime() const;
  void setCurrentPlayTime(const qint32 &value);

  qint32 TotalPlayTime() const;
  void setTotalPlayTime(const qint32 &value);

  qint16 ChannelNum() const;
  void setChannelNum(const qint16 &value);

  QString Dirname() const;
  void setDirname(const QString &value);

private:
  // Name of the directory the video is within
  QString dirname;

  qint16 channelNum;
  QString upc;
  QString title;
  QString file;
  QString producer;
  QString category;
  QString subcategory;
  QString frontCover;
  QString backCover;
  quint64 fileLength;
  QDateTime dateIn;
  QDateTime dateOut;
  qint32 playPosition;
  qint32 currentPlayTime;
  qint32 totalPlayTime;
};

// Makes the custom data type known to QMetaType so it can be stored in a QVariant
// The class requires a public default constructor, public copy constructor and public destructor
Q_DECLARE_METATYPE(Movie)
Q_DECLARE_METATYPE(QList<Movie>)

#endif // MEDIAINFO_H
