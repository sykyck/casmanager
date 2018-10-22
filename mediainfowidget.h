#ifndef MEDIAINFOWIDGET_H
#define MEDIAINFOWIDGET_H

#include <QDialog>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QCheckBox>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QStringList>
#include <QKeyEvent>
#include "movie.h"
#include "databasemgr.h"
#include "imageviewerwidget.h"
#include "settings.h"

class MediaInfoWidget : public QDialog
{
  Q_OBJECT
public:
  explicit MediaInfoWidget(DatabaseMgr *dbMgr, Settings *settings, QWidget *parent = 0);
  ~MediaInfoWidget();
  void populateForm(const QString &videoPath, const Movie &movie, const QString &frontCoverFile = QString(), const QString &backCoverFile = QString());
  Movie &getMovie();
  bool coverFilesChanged();
  bool movieChanged();

protected:
  //void resizeEvent(QResizeEvent *e);
  void showEvent(QShowEvent *);
  void accept();
  void reject();

signals:
  
public slots:
  void showFrontCover();
  void showBackCover();
  void chooseFrontCover();
  void chooseBackCover();
  void openWebBrowser();
  void noBackCoverChecked();

private:
  QString isValidInput();
  bool containsExtendedChars(const QString &s);
  bool dataChanged();

  QLabel *lblUPC;
  QLabel *lblTitle;
  QLabel *lblProducer;
  QLabel *lblCategory;
  QLabel *lblSubcategory;
  QLabel *lblFrontCover;
  QLabel *lblBackCover;
  QLabel *lblNoBackCover;

  QLineEdit *txtUPC;
  QLineEdit *txtTitle;
  QComboBox *producers;
  QComboBox *categories;
  QComboBox *subcategories;
  QLineEdit *txtFrontCover;
  QLineEdit *txtBackCover;

  QPushButton *btnSave;
  QPushButton *btnCancel;
  QToolButton *btnFindFrontCoverFile;
  QToolButton *btnFindBackCoverFile;
  QToolButton *btnViewFrontCoverFile;
  QToolButton *btnViewBackCoverFile;
  QToolButton *btnSearch;
  QCheckBox *chkNoBackCover;

  QGridLayout *gridLayout;
  QHBoxLayout *buttonLayout;
  QHBoxLayout *frontCoverButtonLayout;
  QHBoxLayout *backCoverButtonLayout;

  // Used for getting producers, categories and subcategories
  DatabaseMgr *dbMgr;

  Settings *settings;

  // Local copy of the video being updated
  Movie movie;

  // Path to the directory containing the files for the video
  QString videoPath;

  QString frontCoverFile;
  QString backCoverFile;

  bool metadataChanged;
};

#endif // MEDIAINFOWIDGET_H
