#include "moviechangesetindex.h"
#include "qjson/serializer.h"
#include "qjson/parser.h"
#include "qslog/QsLog.h"
#include <QFile>
#include <QTextStream>

bool exportedLessThan(const QVariant &v1, const QVariant &v2)
{
  if (v1.canConvert<ExportedMovieChangeSet>() && v2.canConvert<ExportedMovieChangeSet>())
  {
    ExportedMovieChangeSet e1 = v1.value<ExportedMovieChangeSet>();
    ExportedMovieChangeSet e2 = v2.value<ExportedMovieChangeSet>();

    return e1.exported < e2.exported;
  }
  else
    return false;
}

MovieChangeSetIndex::MovieChangeSetIndex(QObject *parent) : QObject(parent)
{

}

QList<ExportedMovieChangeSet> MovieChangeSetIndex::getMovieChangeSetList()
{
  //QLOG_DEBUG() << "Before sort";

  // Return a copy of the list sorted by exported date, oldest to newest
  QVariantList temp;
  foreach (ExportedMovieChangeSet e, movieChangeList)
  {
    //QLOG_DEBUG() << e.exported;

    QVariant v;
    v.setValue(e);
    temp.append(v);
  }

  qSort(temp.begin(), temp.end(), exportedLessThan);

  //QLOG_DEBUG() << "After sort";

  movieChangeList.clear();
  foreach (QVariant v, temp)
  {
    //QLOG_DEBUG() << v.value<ExportedMovieChangeSet>().exported;

    movieChangeList.append(v.value<ExportedMovieChangeSet>());
  }

  return movieChangeList;
}

void MovieChangeSetIndex::addMovieChangeSet(ExportedMovieChangeSet mcSet)
{
  // Add to end of list
  movieChangeList.append(mcSet);
}

void MovieChangeSetIndex::removeMovieChangeSet(ExportedMovieChangeSet mcSet)
{
  // Find matching exported date and delete
  for (int i = 0; i < movieChangeList.count(); ++i)
  {
    ExportedMovieChangeSet e = movieChangeList.at(i);

    // If the week, year and exported fields match then remove from list
    if (e.weekNum == mcSet.weekNum && e.year == mcSet.year && e.exported == mcSet.exported)
    {
      QLOG_DEBUG() << "Found matching ExportedMovieChangeSet object to remove from list";

      movieChangeList.removeAt(i);
      break;
    }
  }
}

bool MovieChangeSetIndex::loadMovieChangeSetList(QString filepath)
{
  movieChangeList.clear();

  QFile indexFile(filepath);

  if (!indexFile.exists() || !indexFile.open(QFile::ReadOnly | QFile::Text))
  {
    return false;
  }
  else
  {
    QTextStream in(&indexFile);
    QString contents = in.readAll();
    bool ok = false;
    QJson::Parser parser;
    QVariant var = parser.parse(contents.toAscii(), &ok);

    if (ok)
    {
      foreach (QVariant v, var.toList())
      {
        ExportedMovieChangeSet e;

        QVariantMap map = v.toMap();

        if (map.contains("exported") && map.contains("path") && map.contains("upcs") && map.contains("week_num") && map.contains("year"))
        {
          e.exported = map["exported"].toDateTime();
          e.path = map["path"].toString();
          e.upcList = map["upcs"].toStringList();
          e.weekNum = map["week_num"].toInt();
          e.year = map["year"].toInt();

          movieChangeList.append(e);
        }
      }
    }
  }

  return true;
}

bool MovieChangeSetIndex::saveMovieChangeSetList(QString filepath)
{
  QVariantList vList;

  foreach (ExportedMovieChangeSet e, movieChangeList)
  {
    QVariantMap map;
    map["exported"] = e.exported;
    map["path"] = e.path;
    map["upcs"] = e.upcList;
    map["week_num"] = e.weekNum;
    map["year"] = e.year;

    vList.append(map);
  }

  QJson::Serializer serializer;
  QByteArray jsonData = serializer.serialize(vList);

  QFile indexFile(filepath);

  if (indexFile.open(QIODevice::Text | QIODevice::WriteOnly))
  {
    //QLOG_DEBUG() << "Writing address file: " + addressFilename;
    QTextStream out(&indexFile);

    out << jsonData << endl;

    indexFile.close();

    return true;
  }
  else
  {
    //QLOG_ERROR() << "Could not write address file: " + addressFilename;
    return false;
  }
}

bool MovieChangeSetIndex::FileExists(QString filepath)
{
  QFile indexFile(filepath);

  return indexFile.exists();
}
