#include "imageviewerwidget.h"
#include <QImage>

ImageViewerWidget::ImageViewerWidget(QString imageFile, QWidget *parent) : QDialog(parent)
{
  lblImage = new QLabel;

  verticalLayout = new QVBoxLayout;
  verticalLayout->addWidget(lblImage);

  this->setLayout(verticalLayout);

  setImage(imageFile);

  this->setWindowTitle(tr("Image Viewer"));
}

ImageViewerWidget::~ImageViewerWidget()
{
 // verticalLayout->deleteLater();
}

void ImageViewerWidget::setImage(QString imageFile)
{
  QImage image(imageFile);

  if (image.isNull())
  {
    lblImage->setText(tr("Image not found"));
    lblImage->setAlignment(Qt::AlignCenter);
  }
  else
    lblImage->setPixmap(QPixmap::fromImage(image));
}

void ImageViewerWidget::showEvent(QShowEvent *)
{
  if (lblImage->pixmap())
    this->setFixedSize(this->sizeHint());
  else
  {
    QSize size = this->sizeHint();
    size.setHeight(size.height() + 100);
    size.setWidth(size.width() + 100);

    this->setFixedSize(size);
  }
}
