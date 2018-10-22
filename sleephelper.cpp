#include "sleephelper.h"

void SleepHelper::sleep(unsigned long secs)
{
  QThread::sleep(secs);
}

void SleepHelper::msleep(unsigned long msecs)
{
  QThread::msleep(msecs);
}

void SleepHelper::usleep(unsigned long usecs)
{
  QThread::usleep(usecs);
}
