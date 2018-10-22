#ifndef MOVIECHANGEWEEKNUMWIDGET_H
#define MOVIECHANGEWEEKNUMWIDGET_H

#include <QDialog>
#include <QLabel>
#include <QComboBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QCheckBox>
#include <QSpinBox>
#include <QStandardItemModel>
#include <QTableView>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QDateTimeEdit>
#include "dvdcopytask.h"
#include "moviechangeinfo.h"

class MovieChangeWeekNumWidget : public QDialog
{
  Q_OBJECT
public:
  explicit MovieChangeWeekNumWidget(QList<MovieChangeInfo> movieSetList, int highestChannelNum, QWidget *parent = 0);
  ~MovieChangeWeekNumWidget();
  QList<MovieChangeInfo> getSelectedMovieSet();

protected:
  void showEvent(QShowEvent *);
  void accept();

signals:

public slots:
  void setFirstChannel(int channelNum);
  void setLastChannel(int channelNum);
  void setNumVideos(int numVideos);

private slots:
  void overrideViewtimesClicked();
  void firstChannelChanged(int channelNum);
  void movieChangeSetSelectionChanged(int index);
  void allMovieChangeSetsClicked();

private:
  QLabel *lblInstructions;
  QComboBox *cmbMovieSets;
  QStandardItemModel *moviesModel;
  QTableView *moviesTable;
  //QDateTimeEdit *scheduledDate;
  QCheckBox *chkOverrideViewtimes;
  QLineEdit *txtNumVideos;
  QSpinBox *spnFirstChannel;
  QSpinBox *spnLastChannel;
  QCheckBox *chkAllMovieChangeSets;

  QPushButton *btnOK;
  QPushButton *btnCancel;
  QDialogButtonBox *buttonLayout;
  QFormLayout *formLayout;
  QVBoxLayout *verticalLayout;

  int highestChannelNum;
};

#endif // MOVIECHANGEWEEKNUMWIDGET_H
