#include "storeshift.h"

StoreShift::StoreShift()
{

}

int StoreShift::getShiftNum() const
{
  return shiftNum;
}

void StoreShift::setShiftNum(int value)
{
  shiftNum = value;
}

QTime StoreShift::getStartTime() const
{
  return startTime;
}

void StoreShift::setStartTime(const QTime &value)
{
  startTime = value;
}

QTime StoreShift::getEndTime() const
{
  return endTime;
}

void StoreShift::setEndTime(const QTime &value)
{
  endTime = value;
}

void StoreShift::readJson(const QJsonObject &json)
{
  shiftNum = (int)json["shift_num"].toDouble();
  startTime = QTime::fromString(json["start_time"].toString(), "hh:mm");
  endTime = QTime::fromString(json["end_time"].toString(), "hh:mm");
}

void StoreShift::writeJson(QJsonObject &json) const
{
  json["shift_num"] = shiftNum;
  json["start_time"] = startTime.toString("hh:mm");
  json["end_time"] = endTime.toString("hh:mm");
}
