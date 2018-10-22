#include "videoinfowidget.h"
#include "qslog/QsLog.h"
#include <QMessageBox>
#include <QEvent>
#include <QMouseEvent>
#include <QApplication>
#include "global.h"
#include "imageviewerwidget.h"

const int COVER_WIDTH = 93;
const int COVER_HEIGHT = 150;

VideoInfoWidget::VideoInfoWidget(DatabaseMgr *dbMgr, Settings *settings, QWidget *parent) :  QDialog(parent)
{
  this->setFixedSize(422, 400);

  this->dbMgr = dbMgr;

  this->settings = settings;

  lblUPC = new QLabel(tr("UPC"));
  lblTitle = new QLabel(tr("Title"));
  lblProducer = new QLabel(tr("Producer"));
  lblCategory = new QLabel(tr("Category"));
  lblSubcategory = new QLabel(tr("Subcategory"));
  lblFrontCover = new QLabel;
  lblFrontCover->installEventFilter(this);
  lblFrontCover->setMouseTracking(true);
  lblBackCover = new QLabel;
  lblBackCover->installEventFilter(this);
  lblBackCover->setMouseTracking(true);

  txtUPC = new QLineEdit;
  txtUPC->setEnabled(false);
  txtUPC->setMaxLength(15);
  txtTitle = new QLineEdit;
  txtTitle->setMaxLength(254);
  txtTitle->setEnabled(false);
  txtProducer = new QLineEdit;
  txtProducer->setEnabled(false);
  txtCategory = new QLineEdit;
  txtCategory->setEnabled(false);
  txtSubcategory = new QLineEdit;
  txtSubcategory->setEnabled(false);

  btnClose = new QPushButton(tr("Close"));
  connect(btnClose, SIGNAL(clicked()), this, SLOT(close()));

  gridLayout = new QGridLayout;

  gridLayout->addWidget(lblUPC, 0, 0);
  gridLayout->addWidget(txtUPC, 0, 1, 1, 3);

  gridLayout->addWidget(lblTitle, 1, 0);
  gridLayout->addWidget(txtTitle, 1, 1, 1, 3);

  gridLayout->addWidget(lblProducer, 2, 0);
  gridLayout->addWidget(txtProducer, 2, 1, 1, 3);

  gridLayout->addWidget(lblCategory, 3, 0);
  gridLayout->addWidget(txtCategory, 3, 1, 1, 3);

  gridLayout->addWidget(lblSubcategory, 4, 0);
  gridLayout->addWidget(txtSubcategory, 4, 1, 1, 3);

  gridLayout->addWidget(new QLabel("Front/Back Covers"), 5, 0, 1, 1, Qt::AlignTop);
  gridLayout->addWidget(lblFrontCover, 5, 1, 1, 1, Qt::AlignTop);
  gridLayout->addWidget(lblBackCover, 5, 2, 1, 2, Qt::AlignTop);

  gridLayout->addWidget(btnClose, 8, 3);

  this->setLayout(gridLayout);

  this->setWindowTitle(tr("Video Info"));
}

void VideoInfoWidget::populateForm(const DvdCopyTask &copyTask)
{
  txtUPC->setText(copyTask.UPC());
  txtTitle->setText(copyTask.Title());
  txtProducer->setText(copyTask.Producer());
  txtCategory->setText(copyTask.Category());
  txtSubcategory->setText(copyTask.Subcategory());

  frontCoverFile = QString("%1/%2/%3/%4/%5").arg(settings->getValue(VIDEO_PATH).toString()).arg(copyTask.Year()).arg(copyTask.WeekNum()).arg(copyTask.UPC()).arg(copyTask.FrontCover());
  QPixmap frontImage(frontCoverFile);
  lblFrontCover->setPixmap(frontImage.scaled(COVER_WIDTH, COVER_HEIGHT, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));

  backCoverFile = QString("%1/%2/%3/%4/%5").arg(settings->getValue(VIDEO_PATH).toString()).arg(copyTask.Year()).arg(copyTask.WeekNum()).arg(copyTask.UPC()).arg(copyTask.BackCover());
  QPixmap backImage(backCoverFile);
  lblBackCover->setPixmap(backImage.scaled(COVER_WIDTH, COVER_HEIGHT, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
}

bool VideoInfoWidget::eventFilter(QObject *target, QEvent *event)
{
  if (event->type() == QEvent::MouseButtonPress)
  {
    if (target == lblFrontCover)
    {
      showFrontCover();

      // True means we handled it
      return true;
    }
    else if (target == lblBackCover)
    {
      showBackCover();

      // True means we handled it
      return true;
    }

    // Returning false means to pass the event on to the intended target
    return false;
  }

  // Not a mouse button press event so pass on to base class's implementation of eventFilter
  return QDialog::eventFilter(target, event);
}

void VideoInfoWidget::showFrontCover()
{
  ImageViewerWidget viewer(frontCoverFile);
  viewer.exec();
}

void VideoInfoWidget::showBackCover()
{
  ImageViewerWidget viewer(backCoverFile);
  viewer.exec();
}
