#ifndef STORESHIFT_H
#define STORESHIFT_H

#include <QTime>
#include "qjson-backport/qjsonobject.h"

class StoreShift
{
public:
  StoreShift();

  int getShiftNum() const;
  void setShiftNum(int value);

  QTime getStartTime() const;
  void setStartTime(const QTime &value);

  QTime getEndTime() const;
  void setEndTime(const QTime &value);

  void readJson(const QJsonObject &json);
  void writeJson(QJsonObject &json) const;

private:
  int shiftNum;
  QTime startTime;
  QTime endTime;
};

#endif // STORESHIFT_H
