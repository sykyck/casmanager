#ifndef APPSETTINGSWIDGET_H
#define APPSETTINGSWIDGET_H

#include <QDialog>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QStackedLayout>
#include <QTreeWidget>
#include <QSplitter>
#include <QComboBox>
#include <QCheckBox>
#include <QTimer>
#include <QProcess>
#include <QVariant>
#include <QTimeEdit>
#include <QDateEdit>
#include "databasemgr.h"
#include "casserver.h"
#include "boothsettings.h"
#include "settings.h"
#include "sharedcardservices.h"

class AppSettingsWidget : public QDialog
{
  Q_OBJECT
public:
  explicit AppSettingsWidget(DatabaseMgr *dbMgr, CasServer *casServer, Settings *settings, SharedCardServices *sharedCardService, bool firstRun = false, QWidget *parent = 0);
  bool restartRequired();
    
protected:
  void showEvent(QShowEvent *);

signals:
  
public slots:
  
private slots:
  void onSaveClicked();
  void onSendCollectionReportClicked();
  void onSendDailyMeterReportClicked();
  void onAppModeChanged(int index);
  void onSendCollectionSnapshotReportClicked();
  void onShareCardsClick();

  void onOneWayReplication();
  void onTwoWayReplication();
  void onReplicationError();

private:
  bool validateInput();

  QFormLayout *formLayout;
  QDialogButtonBox *buttonLayout;
  QVBoxLayout *verticalLayout;

  QLineEdit *txtStoreName;
  QCheckBox *chkSendCollectionReport;
  QLineEdit *txtCollectionReportRecipients;
  QCheckBox *chkSendDailyMeterReport;
  QLineEdit *txtDailyMeterReportRecipients;
  QTimeEdit *dailyMeterSendTime;
  QCheckBox *chkSendCollectionSnapshotReport;
  QSpinBox *spnCollectionSnapshotInterval;
  QLineEdit *txtCollectionSnapshotReportRecipients;

  QLineEdit *txtDataPath;
  QLineEdit *txtVideoPath;
  QLineEdit *txtMetadataPath;
  QLineEdit *txtDvdCopyProg;
  QLineEdit *txtCustomerPassword;
  QLineEdit *txtVideoLookupURL;
  QLineEdit *txtVideoUpdateURL;

  QSpinBox *spnMinimumBoothAddress;
  QSpinBox *spnMaximumBoothAddress;
  QCheckBox *chkUploadViewTimes;
  QLineEdit *txtViewTimesServerURL;
  QLineEdit *txtDvdCopyingDrive;
  QLineEdit *txtDvdCopyingDestPath;
  QCheckBox *chkTestMode;
  QCheckBox *chkShareCards;
  QLineEdit *txtRemoteDatabaseName;
  QComboBox *cmbAppModes;
  QDateEdit *dtFirstCollectionDate;

  QPushButton *btnSave;
  QPushButton *btnCancel;

  DatabaseMgr *dbMgr;
  CasServer *casServer;
  Settings *settings;
  SharedCardServices *sharedCardService;

  bool appModeChanged;
  bool addressRangeChanged;
};

#endif // APPSETTINGSWIDGET_H
