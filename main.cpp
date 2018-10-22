#include "qtsingleapplication/QtSingleApplication"
#include <QFileInfo>
#include <stdio.h>
#include <sys/time.h>
#include <QDir>
#include "qslog/QsLog.h"
#include "qslog/QsLogDest.h"
#include "mainwindow.h"
#include <signal.h>

#include "analytics/piwiktracker.h"

PiwikTracker *gPiwikTracker = NULL;

void signalHandler(int sig)
{
  qDebug("\nQuit the application (request signal = %d).\n", sig);
  QtSingleApplication::quit();
}

void catchUnixSignals(QList<int> quitSignals, QList<int> ignoreSignals = QList<int>())
{
  // all these signals will be ignored.
  foreach (int sig, ignoreSignals)
    signal(sig, SIG_IGN);

  // each of these signals calls the handler (quits the QCoreApplication).
  foreach (int sig, quitSignals)
    signal(sig, signalHandler);
}

// Seeds the random number generator by first trying to read the urandom device
// and if that doesn't succeed, it falls back to using the current time
void initRndNumGen(void)
{
  unsigned int number;
  FILE *filePtr = fopen("/dev/urandom", "r");

  if (filePtr)
  {
    fread(&number, 1, sizeof(number), filePtr);
    fclose(filePtr);

    qsrand(number);
  }
  else
  {
    struct timeval time;
    gettimeofday(&time, NULL);
    number = (unsigned int) time.tv_sec + time.tv_usec;
    qsrand(number);
  }

  //qDebug(qPrintable(QString("Random number seed: %1").arg(number)));
}

int main(int argc, char *argv[])
{
  QtSingleApplication app(argc, argv);

  QList<int> signalList;
  signalList << SIGQUIT << SIGINT << SIGTERM << SIGHUP;
  catchUnixSignals(signalList);

  if (app.isRunning())
  {
    qDebug("Another instance of '%s' is already running, now exiting.", qPrintable(QFileInfo(QtSingleApplication::applicationFilePath()).fileName()));

    return 0;
  }
  else
  {
    using namespace QsLogging;

    // Init the logging mechanism
    Logger &logger = Logger::instance();
    logger.setLoggingLevel(QsLogging::TraceLevel);
    const QString logPath(QDir(app.applicationDirPath()).filePath("cas-mgr.log"));

    // Add file and console destinations
    DestinationPtr fileDestination(DestinationFactory::MakeFileDestination(logPath, EnableLogRotation, MaxSizeBytes(1024 * 1024 * 2), MaxOldLogCount(20)));
    DestinationPtr debugDestination(DestinationFactory::MakeDebugOutputDestination());
    logger.addDestination(debugDestination);
    logger.addDestination(fileDestination);


    // Change style to CleanLooks which is the default for Gnome
    // this style shows buttons and comboboxes highlighted when they have focus
    // The downside to the cleanlooks style is the tooltips are too hard to read (white text on yellow background)
    // so the QToolTip style has to be overridden
    app.setStyle("cleanlooks");

    // Seed the random number generator
    initRndNumGen();

    gPiwikTracker = new PiwikTracker(&app);
    MainWindow win;
    win.show();

    return app.exec();
  }
}
