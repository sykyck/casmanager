#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QXmlStreamReader>
#include <QDate>

#include "movie.h"
#include "qslog/QsLog.h"

Movie::Movie(QObject *parent) : QObject(parent)
{
  clear();
}

Movie::Movie(const Movie &other, QObject *parent) : QObject(parent)
{  
  dirname = other.Dirname();
  channelNum = other.ChannelNum();
  title = other.Title();
  file = other.VideoFile();
  upc = other.UPC();
  frontCover = other.FrontCover();
  backCover = other.BackCover();
  subcategory = other.Subcategory();
  category = other.Category();
  producer = other.Producer();
  dateIn = other.DateIn();
  dateOut = other.DateOut();
  playPosition = other.PlayPosition();
  fileLength = other.FileLength();
  currentPlayTime = other.CurrentPlayTime();
  totalPlayTime = other.TotalPlayTime();
}

Movie &Movie::operator =(const Movie &other)
{
  dirname = other.Dirname();
  channelNum = other.ChannelNum();
  title = other.Title();
  file = other.VideoFile();
  upc = other.UPC();
  frontCover = other.FrontCover();
  backCover = other.BackCover();
  subcategory = other.Subcategory();
  category = other.Category();
  producer = other.Producer();
  dateIn = other.DateIn();
  dateOut = other.DateOut();
  playPosition = other.PlayPosition();
  fileLength = other.FileLength();
  currentPlayTime = other.CurrentPlayTime();
  totalPlayTime = other.TotalPlayTime();

  return *this;
}

QString Movie::Title() const
{
  return title;
}

void Movie::setTitle(const QString &value)
{
  title = value;
}

QString Movie::UPC() const
{
  return upc;
}

void Movie::setUPC(const QString &value)
{
  upc = value;
}

QString Movie::Producer() const
{
  return producer;
}

void Movie::setProducer(const QString &value)
{
  producer = value;
}

QString Movie::Category() const
{
  return category;
}

void Movie::setCategory(const QString &value)
{
  category = value;
}

Movie::Movie(QString filepath, qint16 channelNum, QObject *parent) : QObject(parent)
{
  clear();

  this->channelNum = channelNum;

  QFile xmlFile(filepath);

  if (!xmlFile.exists() || (xmlFile.error() != QFile::NoError))
  {
    QLOG_ERROR() << "Unable to open file:" << filepath;
  }
  else
  {
    dirname = QFileInfo(filepath).dir().dirName();
    //QLOG_DEBUG() << QString("%1 is in directory: %2").arg(filepath).arg(dirname), true);

    xmlFile.open(QIODevice::ReadOnly);
    QXmlStreamReader reader(&xmlFile);
    while (!reader.atEnd())
    {
      reader.readNext();

      if (reader.isStartElement())
      {
        if (reader.name() == "mediaInfo")
        {
          while (!reader.atEnd())
          {
            reader.readNext();

            if (reader.isStartElement() && reader.name() == "mediaTitle")
            {
              title = reader.readElementText().trimmed();
            }
            else if (reader.isStartElement() && reader.name() == "mediaFile")
            {
              //file = QFileInfo(xmlFile).absolutePath() + QDir::separator() + reader.readElementText();
              file = reader.readElementText();
            }
            else if (reader.isStartElement() && reader.name() == "mediaUPC")
            {
              upc = reader.readElementText();
            }
            else if (reader.isStartElement() && reader.name() == "mediaArtFront")
            {
              frontCover = reader.readElementText();
            }
            else if (reader.isStartElement() && reader.name() == "mediaArtBack")
            {
              backCover = reader.readElementText();
            }
            else if (reader.isStartElement() && reader.name() == "mediaGenre")
            {
              subcategory = reader.readElementText().trimmed();
            }
            else if (reader.isStartElement() && reader.name() == "mediaCategory")
            {
              category = reader.readElementText().trimmed();
            }
            else if (reader.isStartElement() && reader.name() == "mediaProducer")
            {
              producer = reader.readElementText().trimmed();
            }
            else if (reader.isStartElement() && reader.name() == "mediaDateIn")
            {

              dateIn = QDateTime::currentDateTime();

              /*dateIn = QDateTime::fromString(reader.readElementText(), "MM/dd/yy");

              // Qt defaults to year 1900 so it assumes a two-digit year is
              // in the 1900's rather than 2000's
              if (dateIn.date().year() < 2000)
                dateIn.setDate(dateIn.addYears(100).date());*/
            }
            /*else if (reader.isStartElement() && reader.name() == "mediaDateOut")
            {
              dateOut = QDateTime::fromString(reader.readElementText(), "MM/dd/yy");

              // Qt defaults to year 1900 so it assumes a two-digit year is
              // in the 1900's rather than 2000's
              if (dateOut.date().year() < 2000)
                dateOut.setDate(dateOut.addYears(100).date());
            }*/
            else if (reader.isStartElement() && reader.name() == "mediaPlayPosition")
            {
              //playPosition = reader.readElementText().toLong();
            }
            else if (reader.isStartElement() && reader.name() == "playTime")
            {

              currentPlayTime = reader.readElementText().toLong();
            }
            else if (reader.isStartElement() && reader.name() == "mediaFileLength")
            {
              fileLength = reader.readElementText().toULongLong();

              if (fileLength == 0)
              {
                QFileInfo xmlFileInfo(filepath);

                QFileInfo tsFile(xmlFileInfo.absolutePath() + QDir::separator() + file);

                fileLength = tsFile.size();

                //QLOG_DEBUG() << QString("Getting size from file: %1, File size: %2").arg(xmlFileInfo.absolutePath() + QDir::separator() + file).arg(fileLength);
              }
              /*else
              {
                QLOG_DEBUG() << QString("Getting size from XML, File size: %1").arg(fileLength), true);
              }*/
            }
          }
        }
      }
    }

    // If category is Transsexual (or She-Male just in case) then we need to convert it to She-Male
    // and possibly change its subcategory. She-Male is now treated much
    // like Gay; anything that isn't subcategorized is put into Other
    if (category == "Transsexual" || category == "She-Male")
    {
      category = "She-Male";

      // Currently there isn't a subcategory called Trannsexual but just in case
      if (subcategory == "She-Male" || subcategory == "Transsexual")
        subcategory = "Other";
    }
    else if (category == "Bisexual")
    {
      category = "Bi-Sexual";
      subcategory = "Bi-Sexual";
    }
    else if (category == "Straight" && subcategory == "Bi-Sexual")
      category = "Bi-Sexual";
    else if (category == "Gay" && subcategory == "All Girl")
      category = "Straight";
    else if (subcategory == "Straght Guy")
      subcategory = "Straight Guy";
    else if (category == "Gay" && subcategory == "Gay")
      subcategory = "Other";
    else if (subcategory == "Large Dick")
      subcategory = "Big Dicks";

    if (reader.hasError())
    {
      QLOG_ERROR() << QString("Error parsing %1 on line %2 column %3: \n%4")
                  .arg(filepath)
                  .arg(reader.lineNumber())
                  .arg(reader.columnNumber())
                  .arg(reader.errorString());
    }
  }
}

void Movie::clear()
{
  dirname.clear();
  channelNum = 0;
  title.clear();
  file.clear();
  upc.clear();
  frontCover.clear();
  backCover.clear();
  subcategory.clear();
  category.clear();
  producer.clear();
  dateIn = QDateTime();
  dateOut = QDateTime();
  playPosition = -1;
  fileLength = 0;
  currentPlayTime = 0;
  totalPlayTime = 0;
}

bool Movie::isValid()
{
  // Validate the properties
  return (title.length() > 0 && category.length() > 0 &&
          subcategory.length() > 0 && producer.length() > 0 &&
          !title.contains("enter title", Qt::CaseInsensitive) &&
          !category.contains("none", Qt::CaseInsensitive) &&
          !subcategory.contains("select a genre", Qt::CaseInsensitive) &&
          !producer.contains("select a producer", Qt::CaseInsensitive));
}

bool Movie::writeXml(const QString &filename)
{
  QFile file(filename);
  if (!file.open(QFile::WriteOnly | QFile::Text))
  {
    QLOG_ERROR() << QString("Cannot write XML file: %1: %2").arg(filename).arg(file.errorString());
    return false;
  }

  /*
   *<mediaInfo>
   *<version>1.0</version>
   *<mediaTitle>Two Cocks For Me Please</mediaTitle>
   *<mediaUPC>898747007389</mediaUPC>
   *<mediaFile>898747007389.ts</mediaFile>
   *<mediaResolution>
   *  <horizontal>720</horizontal>
   *  <vertical>480</vertical>
   *</mediaResolution>
   *<mediaArtFront>898747007389.jpg</mediaArtFront>
   *<mediaArtBack>898747007389b.jpg</mediaArtBack>
   *<mediaGenre>Orgy/Group</mediaGenre>
   *<mediaCategory>Straight</mediaCategory>
   *<mediaProducer>Acid Rain</mediaProducer>
   *<mediaDateIn>00/00/00</mediaDateIn>
   *<mediaDateOut>00/00/00</mediaDateOut>
   *<mediaWeeklyRanking> <week1>0</week1> <week2>0</week2> <week3>0</week3> <week4>0</week4> <week5>0</week5> <week6>0</week6> <week7>0</week7> <week8>0</week8> <week9>0</week9> <week10>0</week10> <week11>0</week11> <week12>0</week12> </mediaWeeklyRanking>
   *<mediaPlayPosition>0</mediaPlayPosition>
   *<playTime>0</playTime>
   *<mediaFileLength>1052900580</mediaFileLength>
   *</mediaInfo>
   */
  QXmlStreamWriter xmlWriter(&file);
  xmlWriter.setAutoFormatting(true);
  xmlWriter.writeStartDocument();

  xmlWriter.writeStartElement("mediaInfo");

  /*xmlWriter.writeStartElement("version");
  xmlWriter.writeCharacters("1.0");
  xmlWriter.writeEndElement(); // end version

  xmlWriter.writeStartElement("mediaTitle");
  xmlWriter.writeCharacters(title);
  xmlWriter.writeEndElement(); // end mediaTitle*/

  xmlWriter.writeTextElement("version", "1.0");
  xmlWriter.writeTextElement("mediaTitle", title);
  xmlWriter.writeTextElement("mediaUPC", upc);
  xmlWriter.writeTextElement("mediaFile", upc + ".ts");

  xmlWriter.writeStartElement("mediaResolution");
  xmlWriter.writeTextElement("horizontal", "720");
  xmlWriter.writeTextElement("vertical", "480");
  xmlWriter.writeEndElement(); // end mediaResolution

  xmlWriter.writeTextElement("mediaArtFront", frontCover);
  xmlWriter.writeTextElement("mediaArtBack", backCover);
  xmlWriter.writeTextElement("mediaGenre", subcategory);
  xmlWriter.writeTextElement("mediaCategory", category);
  xmlWriter.writeTextElement("mediaProducer", producer);
  xmlWriter.writeTextElement("mediaDateIn", "00/00/00");
  xmlWriter.writeTextElement("mediaDateOut", "00/00/00");

  // <week1>0</week1> ... <week12>0</week12>
  xmlWriter.writeStartElement("mediaWeeklyRanking");
  for (int i = 1; i <= 12; ++i)
    xmlWriter.writeTextElement(QString("week%1").arg(i), "0");
  xmlWriter.writeEndElement(); // end mediaWeeklyRanking

  xmlWriter.writeTextElement("mediaPlayPosition", "0");
  xmlWriter.writeTextElement("playTime", "0");
  xmlWriter.writeTextElement("mediaFileLength", QString("%1").arg(fileLength));

  xmlWriter.writeEndElement(); // end mediaInfo

  xmlWriter.writeEndDocument();

  file.close();

  if (file.error())
  {
    QLOG_ERROR() << QString("Cannot close XML file: %1: %2").arg(filename).arg(file.errorString());
    return false;
  }
  else
    return true;
}

QString Movie::toString() const
{
  return QString("Title: %1\nUPC: %2\nCategory: %3\nGenre: %4\nProducer: %5\nFront cover: %6\nBack cover: %7")
      .arg(title)
      .arg(upc)
      .arg(category)
      .arg(subcategory)
      .arg(producer)
      .arg(frontCover)
      .arg(backCover);
}

void Movie::setFileLength(const quint64 &value)
{
  //QLOG_DEBUG() << QString("Setting fileLength to: %1").arg(fileLength), true);
  fileLength = value;
}

QString Movie::BackCover() const
{
  return backCover;
}

void Movie::setBackCover(const QString &value)
{
  backCover = value;
}

QString Movie::FrontCover() const
{
  return frontCover;
}

void Movie::setFrontCover(const QString &value)
{
  frontCover = value;
}

QString Movie::Subcategory() const
{
  return subcategory;
}

void Movie::setSubcategory(const QString &value)
{
  subcategory = value;
}

quint64 Movie::FileLength() const
{
  return fileLength;
}

qint32 Movie::TotalPlayTime() const
{
  return totalPlayTime;
}

void Movie::setTotalPlayTime(const qint32 &value)
{
  totalPlayTime = value;
}

qint32 Movie::CurrentPlayTime() const
{
  return currentPlayTime;
}

void Movie::setCurrentPlayTime(const qint32 &value)
{
  currentPlayTime = value;
}

qint32 Movie::PlayPosition() const
{
  return playPosition;
}

void Movie::setPlayPosition(const qint32 &value)
{
  playPosition = value;
}

QString Movie::Dirname() const
{
  return dirname;
}

void Movie::setDirname(const QString &value)
{
  dirname = value;
}

QDateTime Movie::DateOut() const
{
  return dateOut;
}

void Movie::setDateOut(const QDateTime &value)
{
  dateOut = value;
}

QDateTime Movie::DateIn() const
{
  return dateIn;
}

void Movie::setDateIn(const QDateTime &value)
{
  dateIn = value;
}

QString Movie::VideoFile() const
{
  return file;
}

void Movie::setVideoFile(const QString &value)
{
  file = value;
}

qint16 Movie::ChannelNum() const
{
  return channelNum;
}

void Movie::setChannelNum(const qint16 &value)
{
  channelNum = value;
}
