#ifndef MOVIECHANGESETINDEX_H
#define MOVIECHANGESETINDEX_H

#include <QObject>
#include "exportedmoviechangeset.h"

class MovieChangeSetIndex : public QObject
{
  Q_OBJECT
public:
  explicit MovieChangeSetIndex(QObject *parent = 0);
  QList<ExportedMovieChangeSet> getMovieChangeSetList();
  void addMovieChangeSet(ExportedMovieChangeSet mcSet);
  void removeMovieChangeSet(ExportedMovieChangeSet mcSet);
  bool loadMovieChangeSetList(QString filepath);
  bool saveMovieChangeSetList(QString filepath);
  static bool FileExists(QString filepath);
signals:
  
public slots:

private:
  QList<ExportedMovieChangeSet> movieChangeList;

};

#endif // MOVIECHANGESETINDEX_H
