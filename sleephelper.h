#ifndef SLEEPHELPER_H
#define SLEEPHELPER_H

#include <QThread>

class SleepHelper : public QThread
{
  Q_OBJECT
public:
  static void sleep(unsigned long secs);
  static void msleep(unsigned long msecs);
  static void usleep(unsigned long usecs);
};

#endif // SLEEPHELPER_H
