#ifndef ADDVIDEOWIDGET_H
#define ADDVIDEOWIDGET_H

#include <QDialog>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QTableView>
#include <QStandardItemModel>
#include <QCalendarWidget>
#include <QListWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QStringList>
#include <QKeyEvent>
#include "databasemgr.h"
#include "settings.h"

class AddVideoWidget : public QDialog
{
  Q_OBJECT
public:
  explicit AddVideoWidget(int currentMovieChangeSetSize, Settings *settings, DatabaseMgr *dbMgr, QWidget *parent = 0);
  ~AddVideoWidget();
  QStringList getDvdQueue();

protected:
  void accept();
  void reject();

signals:
  
public slots:
  
private slots:
  void addToQueue();
  void removeFromQueue();
  QString isValidInput();

private:
  QLabel *lblUPC;
  QLabel *lblCategory;
  QLabel *lblVideoQueue;
  QLabel *lblVideoQueueSize;

  QLineEdit *txtUPC;
  QPushButton *btnStart;
  QPushButton *btnCancel;
  QPushButton *btnAdd;
  QPushButton *btnRemove;
  QListWidget *videoQueue;
  QComboBox *videoCategories;
  QGridLayout *gridLayout;

  DatabaseMgr *dbMgr;
  Settings *settings;

  int currentMovieChangeSetSize;
};

#endif // ADDVIDEOWIDGET_H
