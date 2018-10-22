#include "mediainfowidget.h"
#include "qslog/QsLog.h"
#include "global.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QProcess>
#include <QDesktopServices>
#include <QUrl>
#include <QSettings>
#include <QDebug>

// FIXME: Double-check mediaInfo.xml file, missing cover images in file at a store

MediaInfoWidget::MediaInfoWidget(DatabaseMgr *dbMgr, Settings *settings, QWidget *parent) :  QDialog(parent)
{  
  // TODO: Use QFormLayout instead!

  // Disable maximize,minimize and resizing of window
  this->setFixedSize(422, 325);

  this->dbMgr = dbMgr;

  this->settings = settings;

  lblUPC = new QLabel(tr("UPC"));
  lblTitle = new QLabel(tr("Title"));
  lblProducer = new QLabel(tr("Producer"));
  lblCategory = new QLabel(tr("Category"));
  lblSubcategory = new QLabel(tr("Subcategory"));
  lblFrontCover = new QLabel(tr("Front Cover"));
  lblBackCover = new QLabel(tr("Back Cover"));
  lblNoBackCover = new QLabel(tr("No Back Cover"));

  txtUPC = new QLineEdit;
  txtUPC->setEnabled(false);
  txtUPC->setMaxLength(15);
  txtTitle = new QLineEdit;
  txtTitle->setMaxLength(254);

  producers = new QComboBox;
  producers->setMaxVisibleItems(15);
  producers->setEditable(true);
  producers->setInsertPolicy(QComboBox::NoInsert);
  producers->insertItem(0, "");
  producers->addItems(dbMgr->getVideoAttributes(1));

  categories = new QComboBox;
  categories->insertItem(0, "");

  // Store employees sometimes select Non-Adult as the category by mistake. There is some special handling for the Non-Adult category in the CASplayer software
  // where it's ignored in certain situations. Need to look into the logic behind that and either remove it as an option in the CAS Manager or change CASplayer to deal with it
  QStringList categoryList = dbMgr->getVideoAttributes(2);
  if (categoryList.contains("Non-Adult"))
    categoryList.removeAll("Non-Adult");

  categories->addItems(categoryList);

  subcategories = new QComboBox;
  subcategories->setMaxVisibleItems(15);
  subcategories->setEditable(true);
  subcategories->setInsertPolicy(QComboBox::NoInsert);
  subcategories->insertItem(0, "");
  subcategories->addItems(dbMgr->getVideoAttributes(3));

  txtFrontCover = new QLineEdit;
  txtFrontCover->setReadOnly(true);

  txtBackCover = new QLineEdit;
  txtBackCover->setReadOnly(true);

  chkNoBackCover = 0;
  if (settings->getValue(SHOW_NO_BACKCOVER_OPTION).toBool())
  {
    chkNoBackCover = new QCheckBox;

    if (settings->getValue(ENABLE_NO_BACK_COVER).toBool())
      chkNoBackCover->setChecked(true);
    else
      chkNoBackCover->setChecked(false);
  }

  btnSearch = new QToolButton;
  btnSearch->setToolButtonStyle(Qt::ToolButtonTextOnly);
  btnSearch->setText(tr("Search"));
  connect(btnSearch, SIGNAL(clicked()), this, SLOT(openWebBrowser()));

  btnSave = new QPushButton(tr("Save"));
  btnCancel = new QPushButton(tr("Cancel"));
  connect(btnSave, SIGNAL(clicked()), this, SLOT(accept()));
  connect(btnCancel, SIGNAL(clicked()), this, SLOT(reject()));

  btnFindFrontCoverFile = new QToolButton;
  btnFindFrontCoverFile->setToolButtonStyle(Qt::ToolButtonTextOnly);
  btnFindFrontCoverFile->setText("...");
  btnFindBackCoverFile = new QToolButton;
  btnFindBackCoverFile->setToolButtonStyle(Qt::ToolButtonTextOnly);
  btnFindBackCoverFile->setText("...");
  btnViewFrontCoverFile = new QToolButton;
  btnViewFrontCoverFile->setToolButtonStyle(Qt::ToolButtonTextOnly);
  btnViewFrontCoverFile->setText(tr("View"));
  btnViewBackCoverFile = new QToolButton;
  btnViewBackCoverFile->setToolButtonStyle(Qt::ToolButtonTextOnly);
  btnViewBackCoverFile->setText(tr("View"));
  connect(btnFindFrontCoverFile, SIGNAL(clicked()), this, SLOT(chooseFrontCover()));
  connect(btnFindBackCoverFile, SIGNAL(clicked()), this, SLOT(chooseBackCover()));
  connect(btnViewFrontCoverFile, SIGNAL(clicked()), this, SLOT(showFrontCover()));
  connect(btnViewBackCoverFile, SIGNAL(clicked()), this, SLOT(showBackCover()));

  buttonLayout = new QHBoxLayout;  
  frontCoverButtonLayout = new QHBoxLayout;
  backCoverButtonLayout = new QHBoxLayout;

  gridLayout = new QGridLayout;
  gridLayout->addWidget(lblUPC, 0, 0);
  gridLayout->addWidget(txtUPC, 0, 1, 1, 2);
  gridLayout->addWidget(btnSearch, 0, 3);

  gridLayout->addWidget(lblTitle, 1, 0);
  gridLayout->addWidget(txtTitle, 1, 1, 1, 3);

  gridLayout->addWidget(lblProducer, 2, 0);
  gridLayout->addWidget(producers, 2, 1, 1, 3);

  gridLayout->addWidget(lblCategory, 3, 0);
  gridLayout->addWidget(categories, 3, 1, 1, 3);

  gridLayout->addWidget(lblSubcategory, 4, 0);
  gridLayout->addWidget(subcategories, 4, 1, 1, 3);

  gridLayout->addWidget(lblFrontCover, 5, 0);
  gridLayout->addWidget(txtFrontCover, 5, 1, 1, 2);
  frontCoverButtonLayout->addWidget(btnFindFrontCoverFile);
  frontCoverButtonLayout->addWidget(btnViewFrontCoverFile);
  gridLayout->addLayout(frontCoverButtonLayout, 5, 3);

  gridLayout->addWidget(lblBackCover, 6, 0);
  gridLayout->addWidget(txtBackCover, 6, 1, 1, 2);
  backCoverButtonLayout->addWidget(btnFindBackCoverFile);
  backCoverButtonLayout->addWidget(btnViewBackCoverFile);
  gridLayout->addLayout(backCoverButtonLayout, 6, 3);

  if (settings->getValue(SHOW_NO_BACKCOVER_OPTION).toBool())
  {
    gridLayout->addWidget(lblNoBackCover, 7, 0);
    gridLayout->addWidget(chkNoBackCover, 7, 1, 1, 3);
  }

  buttonLayout->addWidget(btnSave);
  buttonLayout->addWidget(btnCancel);
  gridLayout->addLayout(buttonLayout, 8, 0, 1, 4, Qt::AlignCenter);

  if (settings->getValue(SHOW_NO_BACKCOVER_OPTION).toBool())
  {
    connect(chkNoBackCover, SIGNAL(clicked()), this, SLOT(noBackCoverChecked()));
  }

  noBackCoverChecked();

  this->setLayout(gridLayout);

  this->setWindowTitle(tr("Video Metadata"));
}

MediaInfoWidget::~MediaInfoWidget()
{
  //gridLayout->deleteLater();
}

void MediaInfoWidget::populateForm(const QString &videoPath, const Movie &movie, const QString &frontCoverFile, const QString &backCoverFile)
{
  //QLOG_DEBUG() << QString("movie = %1, frontCoverFile = %2, backCoverFile = %3").arg(movie.toString()).arg(frontCoverFile).arg(backCoverFile);

  metadataChanged = false;

  this->videoPath = videoPath;
  this->movie = movie;
  this->frontCoverFile = frontCoverFile;
  this->backCoverFile = backCoverFile;

  txtUPC->setText(movie.UPC());
  txtTitle->setText(movie.Title());

  producers->setCurrentIndex(0);
  if (!movie.Producer().isEmpty())
  {
    int index = producers->findText(movie.Producer());
    if (index > -1)
      producers->setCurrentIndex(index);
    else
    {
      // Item not in the list so add it and select it
      producers->insertItem(producers->count(), movie.Producer());
      producers->setCurrentIndex(producers->count() - 1);
    }
  }

  categories->setCurrentIndex(0);
  if (!movie.Category().isEmpty())
  {
    int index = categories->findText(movie.Category());
    if (index > -1)
      categories->setCurrentIndex(index);
  }

  subcategories->setCurrentIndex(0);
  if (!movie.Subcategory().isEmpty())
  {
    int index = subcategories->findText(movie.Subcategory());
    if (index > -1)
      subcategories->setCurrentIndex(index);
    else
    {
      // Item not in the list so add it and select it
      subcategories->insertItem(subcategories->count(), movie.Subcategory());
      subcategories->setCurrentIndex(subcategories->count() - 1);
    }
  }

  if (!frontCoverFile.isEmpty())
    txtFrontCover->setText(frontCoverFile);
  else
    txtFrontCover->clear();

  if (!backCoverFile.isEmpty())
    txtBackCover->setText(backCoverFile);
  else
    txtBackCover->clear();
}

Movie &MediaInfoWidget::getMovie()
{
  return movie;
}

bool MediaInfoWidget::coverFilesChanged()
{
  // if no back cover checked then only check front cover
  // else check front and back cover
  if (chkNoBackCover && chkNoBackCover->isChecked())
  {
    return txtFrontCover->text() != frontCoverFile;
  }
  else
  {
    return txtFrontCover->text() != frontCoverFile || txtBackCover->text() != backCoverFile;
  }
}

bool MediaInfoWidget::movieChanged()
{
  return metadataChanged;
}
/*
void MediaInfoWidget::resizeEvent(QResizeEvent *e)
{
  qDebug() << e->size();
}*/

void MediaInfoWidget::showEvent(QShowEvent *)
{
  txtTitle->setFocus();
  txtTitle->selectAll();
}

void MediaInfoWidget::accept()
{
  QString errors = isValidInput();

  // To try and educate users, warn about using all capital letters in the title and producer
  if (txtTitle->text().length() > 0 && txtTitle->text() == txtTitle->text().toUpper())
  {
    QLOG_DEBUG() << "User submitted a video title with all capital letters, showing warning message";

    if (QMessageBox::question(this, tr("Warning"), tr("Using all capital letters for the title can be hard to read and doesn't look professional. If the movie title is actually all caps on the DVD box then you can ignore this warning. Do you want to fix the title?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
    {
      QLOG_DEBUG() << "User chose to fix the video title";
      return;
    }
    else
      QLOG_DEBUG() << "User chose to leave the title in all caps";
  }

  if (producers->currentText().trimmed().length() > 0 && producers->currentText() == producers->currentText().toUpper())
  {
    QLOG_DEBUG() << "User submitted a producer with all capital letters, showing warning message";

    if (QMessageBox::question(this, tr("Warning"), tr("Using all capital letters for the producer can be hard to read and doesn't look professional. If the producer is actually all caps on the DVD box then you can ignore this warning. Do you want to fix the producer?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
    {
      QLOG_DEBUG() << "User chose to fix the producer";
      return;
    }
    else
      QLOG_DEBUG() << "User chose to leave the producer in all caps";
  }

  metadataChanged = false;
  if (errors.isEmpty())
  {
    if (movie.Title() != txtTitle->text().trimmed())
    {
      movie.setTitle(txtTitle->text().trimmed());
      metadataChanged = true;
    }

    if (movie.Producer() != producers->currentText().trimmed())
    {
      movie.setProducer(producers->currentText().trimmed());
      metadataChanged = true;
    }

    if (movie.Category() != categories->currentText())
    {
      movie.setCategory(categories->currentText());
      metadataChanged = true;
    }

    if (movie.Subcategory() != subcategories->currentText().trimmed())
    {
      movie.setSubcategory(subcategories->currentText().trimmed());
      metadataChanged = true;
    }

    QDir videoFilePath(videoPath);

    // If the front cover file is != to the initial file
    if (txtFrontCover->text() != frontCoverFile)
    {
       //QLOG_DEBUG() << QString("Front cover changed: %1 <> %2").arg(txtFrontCover->text()).arg(frontCoverFile);

       QFileInfo file(txtFrontCover->text());

       // if the JPEG is too large then resize it
       if (file.size() > 102400)
       {
         // resize the file and put the new version in the to the UPC directory with the correct name

         QStringList arguments;
         arguments << txtFrontCover->text() << "-density" << "72x72" << "-quality" << "40" << "-resize" << "500x733" << "-strip" << videoFilePath.absoluteFilePath(QString("%1.jpg").arg(movie.UPC()));

         QLOG_DEBUG() << QString("Resizing front cover image: convert %1").arg(arguments.join(" "));

         // Quality levels (names used in Photoshop) Very High = 80, High = 60, Medium = 30, Low = 10
         // convert <input_file> -density 72x72 -quality 30 -resize 500x733 -strip <output_file>
         int exitCode = QProcess::execute("convert", arguments);

         if (exitCode != 0)
         {
           QLOG_ERROR() << QString("Resize operation exit code: %1").arg(exitCode);
           QMessageBox::warning(this, tr("Resize Error"), tr("Error while trying to resize the front cover file image."));
         }
         else
         {
           QLOG_DEBUG() << QString("Resized front cover image successfully");
           QFile::remove(file.absoluteFilePath());
           movie.setFrontCover(QString("%1.jpg").arg(movie.UPC()));
         }
       }
       else
       {
         // File isn't too big so just copy/rename it in the appropriate directory

         // If the file already exists in the destination then delete it first
         if (QFile::exists(videoFilePath.absoluteFilePath(QString("%1.jpg").arg(movie.UPC()))))
         {
           if (!QFile::remove(videoFilePath.absoluteFilePath(QString("%1.jpg").arg(movie.UPC()))))
           {
             QLOG_ERROR() << QString("Could not delete the existing file: %1 to replace with the new file: %2").arg(videoFilePath.absoluteFilePath(QString("%1.jpg").arg(movie.UPC()))).arg(txtFrontCover->text());
             QMessageBox::warning(this, tr("Error"), tr("Could not replace the front cover image. The existing image could not be deleted."));
           }
         }

         if (QFile::copy(txtFrontCover->text(), videoFilePath.absoluteFilePath(QString("%1.jpg").arg(movie.UPC()))))
         {
           QLOG_DEBUG() << QString("Copied front cover image: %1 to: %2").arg(txtFrontCover->text()).arg(videoFilePath.absoluteFilePath(QString("%1.jpg").arg(movie.UPC())));
           QFile::remove(txtFrontCover->text());
           movie.setFrontCover(QString("%1.jpg").arg(movie.UPC()));
         }
         else
         {
           QLOG_ERROR() << QString("Could not copy file: %1 to: %2").arg(txtFrontCover->text()).arg(videoFilePath.absoluteFilePath(QString("%1.jpg").arg(movie.UPC())));
           QMessageBox::warning(this, tr("Copy Error"), tr("Could not copy the front cover image to the appropriate directory."));
         }
       }
    }
    else
    {
      // Front cover was not changed and it exists so set it in the
      // movie object otherwise it will get wiped out
      if (frontCoverFile.length() > 0)
      {
        movie.setFrontCover(QString("%1.jpg").arg(movie.UPC()));
      }
    }

    // If no back cover checked then create a placeholder image for back cover
    if (chkNoBackCover && chkNoBackCover->isChecked())
    {
      // If the file already exists in the destination then delete it first
      if (QFile::exists(videoFilePath.absoluteFilePath(QString("%1b.jpg").arg(movie.UPC()))))
      {
        if (!QFile::remove(videoFilePath.absoluteFilePath(QString("%1b.jpg").arg(movie.UPC()))))
        {
          QLOG_ERROR() << QString("Could not delete the existing file: %1 to replace with the new place holder image file").arg(videoFilePath.absoluteFilePath(QString("%1b.jpg").arg(movie.UPC())));
          QMessageBox::warning(this, tr("Error"), tr("Could not replace the back cover image. The existing image could not be deleted."));
        }
      }

      // Create placeholder image for back cover
      // convert -size 500x733 -background black -fill white -pointsize 48 -gravity center label:'No Back Cover' 1111111111111b.jpg
      QStringList arguments;
      arguments << "-size" << "500x733" << "-background" << "black" << "-fill" << "white" << "-pointsize" << "48" << "-gravity" << "center" << "label:'No Back Cover'" << videoFilePath.absoluteFilePath(QString("%1b.jpg").arg(movie.UPC()));

      int exitCode = QProcess::execute("convert", arguments);

      if (exitCode != 0)
      {
        QLOG_ERROR() << QString("Convert operation exit code: %1").arg(exitCode);
        QMessageBox::warning(this, tr("Image Error"), tr("Error while trying to create the back cover place holder image."));
      }
      else
      {
        QLOG_DEBUG() << QString("Created back cover place holder image");
        movie.setBackCover(QString("%1b.jpg").arg(movie.UPC()));
      }

    } // endif "no back cover" checked
    else
    {      
      // If the back cover file is != to the initial file
      if (txtBackCover->text() != backCoverFile)
      {
        //QLOG_DEBUG() << QString("Back cover changed: %1 <> %2").arg(txtBackCover->text()).arg(backCoverFile);

         QFileInfo file(txtBackCover->text());

         // if the JPEG is too large then resize it
         if (file.size() > 102400)
         {
           // resize the file and put the new version in the to the UPC directory with the correct name
           QStringList arguments;
           arguments << txtBackCover->text() << "-density" << "72x72" << "-quality" << "40" << "-resize" << "500x733" << "-strip" << videoFilePath.absoluteFilePath(QString("%1b.jpg").arg(movie.UPC()));

           QLOG_DEBUG() << QString("Resizing back cover image: convert %1").arg(arguments.join(" "));

           // Quality levels (names used in Photoshop) Very High = 80, High = 60, Medium = 30, Low = 10
           // convert <input_file> -density 72x72 -quality 30 -resize 500x733 -strip <output_file>
           int exitCode = QProcess::execute("convert", arguments);

           if (exitCode != 0)
           {
             QLOG_ERROR() << QString("Resize operation exit code: %1").arg(exitCode);
             QMessageBox::warning(this, tr("Resize Error"), tr("Error while trying to resize the back cover file image."));
           }
           else
           {
             QLOG_DEBUG() << QString("Resized back cover image successfully");
             QFile::remove(file.absoluteFilePath());
             movie.setBackCover(QString("%1b.jpg").arg(movie.UPC()));
           }
         }
         else
         {
           // File isn't too big so just copy/rename it in the appropriate directory

           // If the file already exists in the destination then delete it first
           if (QFile::exists(videoFilePath.absoluteFilePath(QString("%1b.jpg").arg(movie.UPC()))))
           {
             if (!QFile::remove(videoFilePath.absoluteFilePath(QString("%1b.jpg").arg(movie.UPC()))))
             {
               QLOG_ERROR() << QString("Could not delete the existing file: %1 to replace with the new file: %2").arg(videoFilePath.absoluteFilePath(QString("%1b.jpg").arg(movie.UPC()))).arg(txtBackCover->text());
               QMessageBox::warning(this, tr("Error"), tr("Could not replace the back cover image. The existing image could not be deleted."));
             }
           }

           if (QFile::copy(txtBackCover->text(), videoFilePath.absoluteFilePath(QString("%1b.jpg").arg(movie.UPC()))))
           {
             QLOG_DEBUG() << QString("Copied back cover image: %1 to: %2").arg(txtBackCover->text()).arg(videoFilePath.absoluteFilePath(QString("%1b.jpg").arg(movie.UPC())));
             QFile::remove(txtBackCover->text());
             movie.setBackCover(QString("%1b.jpg").arg(movie.UPC()));
           }
           else
           {
             QLOG_ERROR() << QString("Could not copy file: %1 to: %2").arg(txtBackCover->text()).arg(videoFilePath.absoluteFilePath(QString("%1b.jpg").arg(movie.UPC())));
             QMessageBox::warning(this, tr("Copy Error"), tr("Could not copy the back cover image to the appropriate directory."));
           }
         }
      }
      else
      {
        // Back cover was not changed and it exists so set it in the
        // movie object otherwise it will get wiped out
        if (backCoverFile.length() > 0)
        {
          movie.setBackCover(QString("%1b.jpg").arg(movie.UPC()));
        }
      }
    }

    QDialog::accept();
  }
  else
  {
    QLOG_DEBUG() << QString("User tried to submit incomplete metadata for UPC: %1. Error: %2").arg(txtUPC->text()).arg(errors);
    QMessageBox::warning(this, tr("Invalid"), tr("Correct the following problems:\n%1").arg(errors));
  }
}

void MediaInfoWidget::reject()
{
  bool continueReject = true;

  if (dataChanged())
  {
    if (QMessageBox::question(this, tr("Changes made"), tr("Changes were made to the metadata of this video, are you sure you want to lose these?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
      continueReject = false;
  }

  if (continueReject)
    QDialog::reject();
}

void MediaInfoWidget::showFrontCover()
{
  ImageViewerWidget viewer(txtFrontCover->text());
  viewer.exec();
}

void MediaInfoWidget::showBackCover()
{
  ImageViewerWidget viewer(txtBackCover->text());
  viewer.exec();
}

void MediaInfoWidget::chooseFrontCover()
{
  QSettings settings("IA", SOFTWARE_NAME);
  QString videoCoverPath = settings.value("video_cover_path", QDir::homePath()).toString();

  QString fileName = QFileDialog::getOpenFileName(this, "Select Front Cover Image", videoCoverPath, "JPEG image (*.jpg)");

  if (!fileName.isEmpty())
  {
    txtFrontCover->setText(fileName);

    // Update the video cover path so we return to the same directory next time
    QFileInfo file(fileName);
    settings.setValue("video_cover_path", file.absolutePath());
  }
}

void MediaInfoWidget::chooseBackCover()
{
  QSettings settings("IA", SOFTWARE_NAME);
  QString videoCoverPath = settings.value("video_cover_path", QDir::homePath()).toString();

  QString fileName = QFileDialog::getOpenFileName(this, "Select Back Cover Image", videoCoverPath, "JPEG image (*.jpg)");

  if (!fileName.isEmpty())
  {
    txtBackCover->setText(fileName);

    // Update the video cover path so we return to the same directory next time
    QFileInfo file(fileName);
    settings.setValue("video_cover_path", file.absolutePath());
  }
}

void MediaInfoWidget::openWebBrowser()
{
  QString queryString = QString("https://www.google.com/#q=%1").arg(txtUPC->text());

  QLOG_DEBUG() << QString("User clicked the Search button: %1").arg(queryString);
  QDesktopServices::openUrl(QUrl(queryString));
}

void MediaInfoWidget::noBackCoverChecked()
{
  if (chkNoBackCover && chkNoBackCover->isChecked())
  {
    txtBackCover->setEnabled(false);
    btnViewBackCoverFile->setEnabled(false);
    btnFindBackCoverFile->setEnabled(false);
  }
  else
  {
    txtBackCover->setEnabled(true);
    btnViewBackCoverFile->setEnabled(true);
    btnFindBackCoverFile->setEnabled(true);
  }
}

QString MediaInfoWidget::isValidInput()
{
  QStringList errorList;

  // Now the only field that is mandatory is the Title, this allows the user to specify just the title and our UPC lookup service can do the rest
  txtTitle->setText(txtTitle->text().trimmed());

  if (txtTitle->text().isEmpty())
    errorList.append("- Title cannot be empty.");
  else if (containsExtendedChars(txtTitle->text()))
    errorList.append("- Title cannot contain extended characters. If you cannot see any of these characters, try to clear the Title field and type the title instead of copying/pasting it.");
  else if (txtTitle->text().contains("\""))
    errorList.append("- Title cannot contain the double-qoute (\") character.");
  else if (txtTitle->text().contains("  "))
    errorList.append("- Words in the Title cannot be separated by more than one space.");  

  //if (producers->currentText().trimmed().length() == 0)
  //  errorList.append("- Select a Producer.");
  if (containsExtendedChars(producers->currentText()))
    errorList.append("- Producer cannot contain extended characters. If you cannot see any of these characters, try to clear the Producer field and type it instead of copying/pasting it.");
  else if (producers->currentText().contains("\""))
    errorList.append("- Producer cannot contain the double-qoute (\") character.");
  else if (producers->currentText().contains("  "))
    errorList.append("- Words in the Producer cannot be separated by more than one space.");

  //if (categories->currentIndex() == 0)
    //errorList.append("- Select a Category.");

  //if (subcategories->currentText().trimmed().length() == 0)
   // errorList.append("- Select a Subcategory.");
   if (containsExtendedChars(subcategories->currentText()))
    errorList.append("- Subcategory cannot contain extended characters. If you cannot see any of these characters, try to clear the Subcategory field and type it instead of copying/pasting it.");
  else if (subcategories->currentText().contains("\""))
    errorList.append("- Subcategory cannot contain the double-qoute (\") character.");
  else if (subcategories->currentText().contains("  "))
    errorList.append("- Words in the Subcategory cannot be separated by more than one space.");
  // This is annoying but I'm seeing the customer type "all sex" instead of "All-Sex" which creates a new subcategory
  // so this will ignore their entry and replace with ours in the list
  else if (subcategories->currentText().toLower() == "all sex")
  {
    int index = subcategories->findText("All-Sex");
    if (index > -1)
      subcategories->setCurrentIndex(index);
  }

  if (!txtFrontCover->text().trimmed().isEmpty())
  {
    QFileInfo file(txtFrontCover->text());

    if (!file.exists())
      errorList.append("- Front cover image does not exist. If you do not want to select a front cover image right now then leave the field blank.");
  }

  // Check the back cover if the check box doesn't exist or the check box exists and is checked
  if (!chkNoBackCover || (chkNoBackCover && !chkNoBackCover->isChecked()))
  {
    if (!txtBackCover->text().trimmed().isEmpty())
    {
      QFileInfo file(txtBackCover->text());

      if (!file.exists())
        errorList.append("- Back cover image does not exist. If you do not want to select a back cover image right now then leave the field blank.");
    }
  }

  return errorList.join("\n");
}

bool MediaInfoWidget::containsExtendedChars(const QString &s)
{
   bool found = false;

   foreach (QChar c, s)
   {
     //QLOG_DEBUG() << QString("%1 = %2").arg(c).arg(int(c.toAscii()));

     if (int(c.toAscii()) < 32 || int(c.toAscii()) > 126)
     {
       found = true;
       break;
     }
   }

   return found;
}

bool MediaInfoWidget::dataChanged()
{
  bool changed = false;

  if (movie.Title() != txtTitle->text().trimmed())
  {
    changed = true;
  }

  if (movie.Producer() != producers->currentText().trimmed())
  {
    changed = true;
  }

  if (movie.Category() != categories->currentText())
  {
    changed = true;
  }

  if (movie.Subcategory() != subcategories->currentText().trimmed())
  {
    changed = true;
  }

  return changed || coverFilesChanged();
}
