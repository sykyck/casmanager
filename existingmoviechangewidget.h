#ifndef EXISTINGMOVIECHANGEWIDGET_H
#define EXISTINGMOVIECHANGEWIDGET_H

#include <QDialog>
#include <QLabel>
#include <QVBoxLayout>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QRadioButton>
#include <QComboBox>
#include <QStandardItemModel>
#include <QTableView>
#include "moviechangeinfo.h"

class ExistingMovieChangeWidget : public QDialog
{
  Q_OBJECT
public:
  explicit ExistingMovieChangeWidget(QList<MovieChangeInfo> movieChangeSets, int maxMovieChangeSetSize, QWidget *parent = 0);
  MovieChangeInfo getMovieChangeSelection();
  
signals:

protected:
  void showEvent(QShowEvent *);
  void accept();

private slots:
  void movieSetChanged();
  void itemSelectionChanged(int);
  
private:
  QLabel *lblInstructions;
  QLabel *lblMovieChangeSetSize;
  QRadioButton *rdoNewMovieSet;
  QRadioButton *rdoExistingMovieSet;
  QComboBox *cmbMovieSets;
  QStandardItemModel *moviesModel;
  QTableView *moviesTable;
  QPushButton *btnOK;
  QPushButton *btnCancel;
  QDialogButtonBox *buttonLayout;
  QVBoxLayout *verticalLayout;

  int maxMovieChangeSetSize;
  //QList<MovieChangeInfo> movieChangeSets;
};

#endif // EXISTINGMOVIECHANGEWIDGET_H
