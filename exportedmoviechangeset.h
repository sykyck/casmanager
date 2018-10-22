#ifndef EXPORTEDMOVIECHANGESET_H
#define EXPORTEDMOVIECHANGESET_H

#include <QDateTime>
#include <QString>
#include <QStringList>
#include <QMetaType>
#include <QMetaEnum>
#include <QVariant>

struct ExportedMovieChangeSet
{
  QDateTime exported;
  QStringList upcList;
  QString path;
  int year;
  int weekNum;
};

// Makes the custom data type known to QMetaType so it can be stored in a QVariant
// The class requires a public default constructor, public copy constructor and public destructor
Q_DECLARE_METATYPE(ExportedMovieChangeSet)
Q_DECLARE_METATYPE(QList<ExportedMovieChangeSet>)

#endif // EXPORTEDMOVIECHANGESET_H
