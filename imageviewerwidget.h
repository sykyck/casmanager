#ifndef IMAGEVIEWERWIDGET_H
#define IMAGEVIEWERWIDGET_H

#include <QDialog>
#include <QLabel>
#include <QVBoxLayout>

class ImageViewerWidget : public QDialog
{
  Q_OBJECT
public:
  explicit ImageViewerWidget(QString imageFile, QWidget *parent = 0);
  ~ImageViewerWidget();
  void setImage(QString imageFile);
  
protected:
  void showEvent(QShowEvent *);

signals:
  
public slots:
  
private:
  QLabel *lblImage;
  QVBoxLayout *verticalLayout;
};

#endif // IMAGEVIEWERWIDGET_H
