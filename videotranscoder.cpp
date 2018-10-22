#include "videotranscoder.h"
#include <QStringList>
#include <QFile>
#include "filehelper.h"
#include "global.h"

VideoTranscoder::VideoTranscoder(QString videoTranscodingProg, QObject *parent) : QObject(parent)
{
  this->videoTranscodingProg = videoTranscodingProg;

  transcodeProc = new QProcess;
  connect(transcodeProc, SIGNAL(started()), this, SIGNAL(transcodingStarted()));
  connect(transcodeProc, SIGNAL(finished(int)), this, SLOT(transcodingProcFinished(int)));
  connect(transcodeProc, SIGNAL(error(QProcess::ProcessError)), this, SLOT(transcodingProcError(QProcess::ProcessError)));
}

VideoTranscoder::~VideoTranscoder()
{
  if (transcodeProc->state() != QProcess::NotRunning)
    transcodeProc->kill();

  transcodeProc->deleteLater();
}

void VideoTranscoder::startTranscoding(QString srcVideo, QString destVideo, QString logFile)
{
  this->srcVideo = srcVideo;
  this->destVideo = destVideo;
  this->logFile = logFile;

  QStringList arguments;
  arguments << srcVideo << destVideo << logFile;
  transcodeProc->start(videoTranscodingProg, arguments);
}

bool VideoTranscoder::isTranscoding()
{
  if (transcodeProc->state() != QProcess::NotRunning)
    return true;
  else
    return false;
}

void VideoTranscoder::abortTranscoding()
{
  if (transcodeProc->state() != QProcess::NotRunning)
  {
    // Send "q" to ffmpeg to exit early
    transcodeProc->write("q\n");

    // If the command cannot be written then kill process
    if (!transcodeProc->waitForBytesWritten())
    {
      transcodeProc->terminate();
      Global::killProcess("ffmpeg");
    }
  }
}

void VideoTranscoder::transcodingProcFinished(int exitCode)
{
  bool success = false;
  QString errorMessage = "";

  QString log = FileHelper::readTextFile(logFile);

  if (exitCode == 0)
  {
    if (log.contains("time="))
    {
      if (QFile::exists(destVideo))
      {
        success = true;
      }
      else
      {
        errorMessage = "Could not find transcoded file.";
      }
    }
    else
    {
      errorMessage = "Transcoding error, log file missing token.";
    }
  }
  else
  {
    errorMessage = QString("Transcoding process exit code: %1").arg(exitCode);
  }

  emit transcodingFinished(success, errorMessage);
}

void VideoTranscoder::transcodingProcError(QProcess::ProcessError errorCode)
{
  transcodingFinished(false, QString("Transcoding process error code: %1").arg(errorCode));
}
