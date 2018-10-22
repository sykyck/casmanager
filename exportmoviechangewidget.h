#ifndef EXPORTMOVIECHANGEWIDGET_H
#define EXPORTMOVIECHANGEWIDGET_H

#include <QDialog>
#include <QLabel>
#include <QComboBox>
#include <QListView>
#include <QListWidget>
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

class ExportMovieChangeWidget : public QDialog
{
  Q_OBJECT
public:
  explicit ExportMovieChangeWidget(QList<MovieChangeInfo> movieSetList, QList<MovieChangeInfo> exportedMovieSetList, bool importMode = false, QWidget *parent = 0);
  ~ExportMovieChangeWidget();
  QList<MovieChangeInfo> getSelectedMovieSets();

protected:
  void showEvent(QShowEvent *);
  void accept();

signals:

public slots:

private slots:
  void showExportedVideosClicked();
  void movieChangeSetSelectionChanged(int index);
  void checkboxClicked(QListWidgetItem *item);
  //void setNumVideos(int numVideos);
  void populateMovieChangeCombobox(bool includeExported);
  void showVideoTooltip(QModelIndex index);

private:
  QLabel *lblInstructions;
  QListWidget *lstMovieChangeSets;
  QStandardItemModel *moviesModel;
  QTableView *moviesTable;
  QCheckBox *chkShowExportedVideos;
  //QLineEdit *txtNumVideos;
  QPushButton *btnOK;
  QPushButton *btnCancel;
  QDialogButtonBox *buttonLayout;
  QFormLayout *formLayout;
  QVBoxLayout *verticalLayout;
  bool importMode;
  QList<MovieChangeInfo> movieSetList;
  QList<MovieChangeInfo> exportedMovieSetList;
};

#endif // EXPORTMOVIECHANGEWIDGET_H
