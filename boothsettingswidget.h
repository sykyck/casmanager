#ifndef BOOTHSETTINGSWIDGET_H
#define BOOTHSETTINGSWIDGET_H

#include <QWidget>
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
#include <QStackedWidget>
#include <QTreeWidget>
#include <QSplitter>
#include <QComboBox>
#include <QListWidget>
#include <QCheckBox>
#include <QTimer>
#include <QProcess>
#include <QVariant>
#include <QTabWidget>
#include <QTableView>
#include <QStandardItemModel>
#include "databasemgr.h"
#include "casserver.h"
#include "boothsettings.h"
#include "settings.h"
#include "paymentgatewaywidget.h"
#include "storeshift.h"
#include "videolookupservice.h"

class BoothSettingsWidget : public QWidget
{
  Q_OBJECT
public:
  explicit BoothSettingsWidget(DatabaseMgr *dbMgr, CasServer *casServer, Settings *settings, QWidget *parent = 0);
  ~BoothSettingsWidget();
  bool isBusy();

protected:
  void showEvent(QShowEvent *);

signals:
  void updatingBooth(bool updating);
  void initializeSettingsUpdateProgress(int totalBooths);
  void updateSettingsUpdateProgress(int numBooths, const QString &overallStatus);
  void cleanupSettingsUpdateProgress();
  
public slots:

private slots:
  void onCurrentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);
  void addTreeNode(QString label, QVariant data, QString parentLabel);
  void populateForm(QString deviceAddress);
  void insertShiftRow(StoreShift &shift);
  //QVariantList getCategoryFilters();
  QVariantList getUiTypes();
  QVariantList getBillInterfaceTypes();
  QVariantList getDeviceTypes();
  void onSaveButtonClicked();
  void onUndoButtonClicked();
  void populateDeviceTree();
  void onDeviceTypeChanged();
  void onEnableBuddyBoothChanged();
  void onUsePhysicalBuddyButtonChanged();
  void onEnableBillAcceptorChanged();
  void onAllowCreditCardChanged();
  void onEnableCardSwipe();
  void onCashOnlyChanged();
  //void onEnableStreamingPreviewChanged();
  void onDemoChangedChanged();
  void onEnableMetersChanged();
  void onEnableFreePlay();
  void onPaymentGatewayButtonClicked();

  void updateSettingsSuccess(QString ipAddress);
  void updateSettingsInSession(QString ipAddress);
  void updateSettingsFailed(QString ipAddress);
  void updateSettingsFinished();

  void updateProgress();

  void updateBoothSettingFieldStates();
  void onEditShiftsClicked();
  void onAddUserClicked();
  void onDeleteUserClicked();
  void onChangePasswordClicked();
  void updateAllBooths();
  void clearShiftsTable();
  void getUsersFinished(QList<UserAccount> &users, bool ok);
  void clearUsersTable();
  void insertUserRow(QString username, QString revID);
  void addUserFinished(QString username, bool ok);
  void deleteUserFinished(QString username, bool ok);
  void updateUserFinished(UserAccount &userAccount, bool ok);

  void populateProducers(QStringList producers);
  void addProducerClicked();
  void removeProducerClicked();

private:
  bool boothSettingsChanged(QString deviceAddress);
  void saveBoothSettings(QString deviceAddress);
  void clearChildNodes();
  bool validateInput(bool globalSettings = false);
  bool containsExtendedChars(const QString &s);

  QString getFirstDevice(QString treeNodeName = QString());

  QLabel *lblGeneralSettings;
  QLabel *lblArcadeSettings;
  QLabel *lblPreviewSettings;
  QLabel *lblTheaterSettings;
  QLabel *lblStoreShifts;
  QLabel *lblUsers;

  QComboBox *cmbDeviceTypes;
  QLineEdit *txtDeviceAddress; // read only
  QLineEdit *txtDeviceAlias;
  //QCheckBox *chkEnableStreamingPreview;
  QComboBox *cmbArcadeUiType;
  QComboBox *cmbBillAcceptorInterface;
  //QComboBox *cmbPulsesPerDollar;
  QSpinBox *spnStoreNum;
  QSpinBox *spnAreaNum;
  QCheckBox *chkUseCVS;
  QCheckBox *chkBuddyBooth;
  QCheckBox *chkUsePhysicalBuddyButton;
  QSpinBox *spnBuddyAddress;
  QCheckBox *chkUseCardSwipe;
  QCheckBox *chkEnableOnscreenKeypad;
  QCheckBox *chkShowKeypadHelp;
  QCheckBox *chkAllowCreditCards;
  QDoubleSpinBox *spnCreditChargeAmount;
  QCheckBox *chkCashOnly;
  QCheckBox *chkCoinSlot;
  QDoubleSpinBox *spnPreviewCost;
  QDoubleSpinBox *spnThreeVideoPreviewCost;
  QDoubleSpinBox *spnFiveVideoPreviewCost;
  QSpinBox *spnSecondsPerCredit;
  //QSpinBox *spnPreviewSessionMinutes;
  QSpinBox *spnVideoBrowsingMinutes;
  QDoubleSpinBox *spnAdditionalPreviewMinutesCost;
  QSpinBox *spnAdditionalPreviewMinutes;
  QCheckBox *chkAllowAdditionalPreviewMinutesSinglePreview;
  QSpinBox *spnMaxTimePreviewCardProgram;
  QSpinBox *spnLowCreditThreshold;
  QSpinBox *spnLowPreviewMinuteThreshold;
  QCheckBox *chkCcDevelopmentMode;
  QCheckBox *chkDemoMode;
  QCheckBox *chkEnableBillAcceptorDoor;
  QCheckBox *chkEnableBillAcceptorDoorAlarm;
  //QComboBox *cmbCategoryFilter;
  QCheckBox *chkEnableNrMeters;
  QCheckBox *chkEnableMeters;
  QCheckBox *chkEnableFreePlay;
  QSpinBox *spnFreePlayInactivity;
  QSpinBox *spnResumeFreePlaySession;
  QComboBox *cmbTheaterCategory;

  enum ShiftsColumns
  {
    ShiftNum,
    StartTime,
    EndTime
  };

  enum UsersColumns
  {
    Username,
    RevID
  };

  QStandardItemModel *usersModel;
  QTableView *usersTable;

  QStandardItemModel *shiftsModel;
  QTableView *shiftsTable;

  QPushButton *btnSave;
  QPushButton *btnUndo;
  QPushButton *btnPaymentGateway;
  QPushButton *btnEditShifts;
  QPushButton *btnAddUser;
  QPushButton *btnDeleteUser;
  QPushButton *btnChangePassword;

  QLineEdit *txtCustomCategory;
  QPushButton *btnAddProducer;
  QPushButton *btnRemoveProducer;
  QListWidget *lstAssignedProducers;
  QComboBox *cmbProducer;

  QTreeWidget *deviceTree;

  QWidget *containerWidget;
  QGridLayout *generalSettingsLayout;
  QGridLayout *arcadeSettingsLayout;
  QVBoxLayout *arcadeSettingsVLayout;
  QGridLayout *previewSettingsLayout;
  QVBoxLayout *previewSettingsVLayout;
  QGridLayout *theaterSettingsLayout;
  QVBoxLayout *theaterSettingsVLayout;
  QDialogButtonBox *buttonLayout;
  QTabWidget *boothSettingsTabs;
  QWidget *arcadeSettingsWidget;
  QWidget *previewSettingsWidget;
  QWidget *theaterSettingsWidget;
  QWidget *customCategoryWidget;
  QGridLayout *customCategoryLayout;

  QVBoxLayout *verticalLayout;
  QHBoxLayout *horzLayout;
  QHBoxLayout *shiftsUsersLayout;
  QVBoxLayout *shiftsLayout;
  QHBoxLayout *shiftButtonsLayout;
  QVBoxLayout *usersLayout;
  QHBoxLayout *userButtonsLayout;

  DatabaseMgr *dbMgr;
  CasServer *casServer;
  Settings *settings;
  VideoLookupService *lookupService;
  bool firstLoad;
  QStringList missedDevices;
  bool reloadDeviceTree;
  bool busy;
  QString ccClientID;
  QString ccMerchantAccountID;
  qint8 authorizeNetMarketType;
  qint8 authorizeNetDeviceType;
  int totalBoothsToUpdate;
  QStringList updateDeviceSettingsSuccessList;
  QStringList updateDeviceSettingsFailList;
  QStringList updateDeviceSettingsInSessionList;

  QList<StoreShift> storeShifts;
};

#endif // BOOTHSETTINGSWIDGET_H
