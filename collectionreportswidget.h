#ifndef COLLECTIONREPORTSWIDGET_H
#define COLLECTIONREPORTSWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTableView>
#include <QStandardItemModel>
#include <QTimer>
#include <QProcess>
#include <QComboBox>
#include <QLineEdit>
#include <QTabWidget>
#include <QDateEdit>
#include "settings.h"
#include "databasemgr.h"
#include "casserver.h"

class CollectionReportsWidget : public QWidget
{
  Q_OBJECT
public:
  explicit CollectionReportsWidget(DatabaseMgr *dbMgr, CasServer *casServer, Settings *settings, QTabWidget *parentTab = 0, QWidget *parent = 0);
  ~CollectionReportsWidget();
  bool isBusy();

protected:
  void showEvent(QShowEvent *);

signals:

public slots:

private slots:
  void collectionDateChanged(int index);
  void snapshotDateChanged(QDate selectedDate);
  void collectionTypeChanged(int index);
  void sendReportClicked();
  void sendCollectionSuccess(QString message);
  void sendCollectionFailed(QString message);
  void sendCollectionFinished();

private:
  void clearTotalFields();
  void setCollectionModel(const QVariantList &model);
  void resetCollectionModel();

  // map: device_num, current_credits, total_credits, current_preview_credits, total_preview_credits, current_cash, total_cash, current_cc_charges, total_cc_charges, current_prog_credits, total_prog_credits
  void insertCollection(QVariantMap collection);

  QStandardItemModel *collectionModel;
  QTableView *collectionTable;
  QLabel *lblCollectionType;
  QComboBox *cmbCollectionType;
  QLabel *lblCollectionDate;
  QComboBox *cmbCollectionDate;
  QDateEdit *edtSnapshotDate;
  QPushButton *btnSendReport;

  QLabel *lblTotalCredits;
  QLineEdit *txtTotalCredits;

  QLabel *lblTotalCash;
  QLineEdit *txtTotalCash;

  QLabel *lblTotalCcCharges;
  QLineEdit *txtTotalCcCharges;

  QLabel *lblTotalPreviews;
  QLineEdit *txtTotalPreviews;

  QLabel *lblTotalProgrammed;
  QLineEdit *txtTotalProgrammed;

  QHBoxLayout *reportCriteriaLayout;
  QHBoxLayout *totalsLayout;
  QVBoxLayout *verticalLayout;
  bool firstLoad;
  bool busy;

  DatabaseMgr *dbMgr;
  CasServer *casServer;
  Settings *settings;
  QTabWidget *parentTab;

  QVariantList currentCollection;
};

#endif // COLLECTIONREPORTSWIDGET_H
