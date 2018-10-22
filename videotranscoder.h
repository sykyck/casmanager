#ifndef VIDEOTRANSCODER_H
#define VIDEOTRANSCODER_H

#include <QObject>
#include <QString>
#include <QProcess>

class VideoTranscoder : public QObject
{
  Q_OBJECT
public:
  explicit VideoTranscoder(QString videoTranscodingProg, QObject *parent = 0);
  ~VideoTranscoder();
  void startTranscoding(QString srcVideo, QString destVideo, QString logFile);
  bool isTranscoding();
  
signals:
  void transcodingStarted();
  void transcodingFinished(bool success, QString errorMessage = QString());
  
public slots:
  void abortTranscoding();

private slots:
  void transcodingProcFinished(int exitCode);
  void transcodingProcError(QProcess::ProcessError errorCode);

private:
  QProcess *transcodeProc;
  QString videoTranscodingProg;
  QString srcVideo;
  QString destVideo;
  QString logFile;
  
};

#endif // VIDEOTRANSCODER_H
