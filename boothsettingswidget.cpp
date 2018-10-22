#include "boothsettingswidget.h"
#include <QHeaderView>
#include <QDebug>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <QCoreApplication>
#include "storeshiftswidget.h"
#include "useraccountwidget.h"
#include "qslog/QsLog.h"
#include <math.h>
#include "global.h"

// FIXME: Translate the IP address to the booth address so it's not so confusing assigning an address.
// So booth #1 is 20 + 1 = 10.0.0.21, booth #2 is 20 + 2 = 10.0.0.22, etc. Do the same on the booth side in CASplayer
// Change the buddy address selection to also start at address 1 so again, the user can easily figure out what booth to assign

const int SETTINGS_COLUMN_0 = 0;
const int SETTINGS_COLUMN_1 = 1;
const int SETTINGS_COLUMN_2 = 2;
const int SETTINGS_COLUMN_3 = 3;
const int SETTINGS_COLUMN_4 = 4;
const int SETTINGS_COLUMN_5 = 5;
const int SETTINGS_COLUMN_6 = 6;
const int SETTINGS_COLUMN_7 = 7;

const int DEVICE_ADDRESS_ROW = 1;
const int DEVICE_NAME_ROW = 2;
const int DEVICE_TYPE_ROW = 3;
const int ENABLE_BILL_ACCEPTOR_ROW = 4;
const int ACCEPTOR_INTERFACE_ROW = 5;
const int ENABLE_BILL_ACCEPTOR_ALARM_ROW = 6;
const int ENABLE_CARD_READER_ROW = 7;

const int ENABLE_FREE_PLAY_ROW = 1;
const int ALLOW_CREDIT_CARDS_ROW = 2;
const int PAYMENT_GATEWAY_ROW = 3;
const int ENABLE_CC_TEST_ROW = 4;
const int ENABLE_DEMO_MODE_ROW = 5;
const int ENABLE_KEYPAD_ROW = 6;
const int SHOW_KEYPAD_HELP_ROW = 7;

const int USE_CVS_ROW = 1;
const int CASH_ONLY_ROW = 2;
const int ENABLE_COIN_SLOT_ROW = 3;
const int STORE_NUM_ROW = 4;
const int AREA_NUM_ROW = 5;
const int ENABLE_NR_METERS_ROW = 6;
const int ENABLE_METERS_ROW = 7;

const int UI_TYPE_ROW = 1;
const int SECONDS_PER_CREDIT_ROW = 2;
const int CC_CHARGE_AMOUNT_ROW = 3;
const int REM_CREDITS_WARN_ROW = 4;
const int FREE_PLAY_INACTIVITY_ROW = 5;
const int FREE_PLAY_RESUME_ROW = 6;

const int ENABLE_BUDDY_BOOTH_ROW = 1;
const int BUDDY_ADDRESS_ROW = 2;
const int USE_PHYSICAL_BUDDY_BUTTON_ROW = 3;

const int PREVIEW_PRICE_ROW = 1;
const int THREE_VIDEO_DELUXE_ROW = 2;
const int FIVE_VIDEO_DELUXE_ROW = 3;
const int PREVIEW_BROWSING_INACTIVITY_ROW = 4;
const int EXTEND_PREVIEW_PRICE_ROW = 5;
const int EXTEND_PREVIEW_SESSION_MINUTES_ROW = 6;

const int ALLOW_EXTENDING_PREVIEW_WITHOUT_DELUXE_ROW = 1;
const int MAX_PREVIEW_CARD_PROG_BY_CUSTOMER_ROW = 2;
const int REMAINING_MINUTES_WARNING_ROW = 3;

const int THEATER_CATEGORY_ROW = 1;

const QString ALL_DEVICES_NODE_NAME = "All Devices";
const QString CLERKSTATIONS_NODE_NAME = "Clerk Stations";
const QString ARCADE_BOOTHS_NODE_NAME = "Arcade Booths";
const QString PREVIEW_BOOTHS_NODE_NAME = "Preview Booths";
const QString THEATERS_NODE_NAME = "Theaters";

BoothSettingsWidget::BoothSettingsWidget(DatabaseMgr *dbMgr, CasServer *casServer, Settings *settings, QWidget *parent) : QWidget(parent)
{
  this->dbMgr = dbMgr;
  this->casServer = casServer;
  this->settings = settings;
  firstLoad = true;
  reloadDeviceTree = false;
  busy = false;
  totalBoothsToUpdate = 0;
  ccClientID.clear();
  ccMerchantAccountID.clear();
  authorizeNetMarketType = 0;
  authorizeNetDeviceType = 0;

  connect(this->casServer, SIGNAL(updateSettingsSuccess(QString)), this, SLOT(updateSettingsSuccess(QString)));
  connect(this->casServer, SIGNAL(updateSettingsFailed(QString)), this, SLOT(updateSettingsFailed(QString)));
  connect(this->casServer, SIGNAL(updateSettingsInSession(QString)), this, SLOT(updateSettingsInSession(QString)));
  connect(this->casServer, SIGNAL(updateSettingsFinished()), this, SLOT(updateSettingsFinished()));

  connect(this->dbMgr, SIGNAL(getUsersFinished(QList<UserAccount>&,bool)), this, SLOT(getUsersFinished(QList<UserAccount>&,bool)));
  connect(this->dbMgr, SIGNAL(addUserFinished(QString, bool)), this, SLOT(addUserFinished(QString, bool)));
  connect(this->dbMgr, SIGNAL(deleteUserFinished(QString,bool)), this, SLOT(deleteUserFinished(QString,bool)));
  connect(this->dbMgr, SIGNAL(updateUserFinished(UserAccount&,bool)), this, SLOT(updateUserFinished(UserAccount&,bool)));

  btnEditShifts = new QPushButton(tr("Edit Shifts"));
  connect(btnEditShifts, SIGNAL(clicked()), this, SLOT(onEditShiftsClicked()));

  btnAddUser = new QPushButton(tr("Add User"));
  connect(btnAddUser, SIGNAL(clicked()), this, SLOT(onAddUserClicked()));

  btnDeleteUser = new QPushButton(tr("Delete User"));
  connect(btnDeleteUser, SIGNAL(clicked()), this, SLOT(onDeleteUserClicked()));

  btnChangePassword = new QPushButton(tr("Change Password"));
  connect(btnChangePassword, SIGNAL(clicked()), this, SLOT(onChangePasswordClicked()));

  lblStoreShifts = new QLabel(tr("Store Shifts"));
  lblStoreShifts->setStyleSheet("font-weight:bold;");

  lblUsers = new QLabel(tr("Clerk Station Users"));
  lblUsers->setStyleSheet("font-weight:bold;");

  cmbDeviceTypes = new QComboBox;
  foreach (QVariant item, getDeviceTypes())
  {
    QVariantMap keyValue = item.toMap();

    foreach (QString key, keyValue.keys())
      cmbDeviceTypes->addItem(key, keyValue[key]);
  }

  connect(cmbDeviceTypes, SIGNAL(currentIndexChanged(int)), this, SLOT(onDeviceTypeChanged()));

  txtDeviceAlias = new QLineEdit;
  txtDeviceAlias->setMaxLength(100);

  txtDeviceAddress = new QLineEdit;
  txtDeviceAddress->setEnabled(false);

  //chkEnableStreamingPreview = new QCheckBox;
  //connect(chkEnableStreamingPreview, SIGNAL(clicked()), this, SLOT(onEnableStreamingPreviewChanged()));

  cmbArcadeUiType = new QComboBox;
  foreach (QVariant item, getUiTypes())
  {
    QVariantMap keyValue = item.toMap();

    foreach (QString key, keyValue.keys())
      cmbArcadeUiType->addItem(key, keyValue[key]);
  }

  cmbBillAcceptorInterface = new QComboBox;
  foreach (QVariant item, getBillInterfaceTypes())
  {
    QVariantMap keyValue = item.toMap();

    foreach (QString key, keyValue.keys())
      cmbBillAcceptorInterface->addItem(key, keyValue[key]);
  }


  //cmbPulsesPerDollar = new QComboBox;
  //cmbPulsesPerDollar->addItem("1", 1);
  //cmbPulsesPerDollar->addItem("4", 4);
  //cmbPulsesPerDollar->setEnabled(false);

  spnStoreNum = new QSpinBox;
  spnStoreNum->setMinimum(0);
  spnStoreNum->setMaximum(999);

  spnAreaNum = new QSpinBox;
  spnAreaNum->setMinimum(0);
  spnAreaNum->setMaximum(999);

  chkUseCVS = new QCheckBox;

  chkBuddyBooth = new QCheckBox;
  connect(chkBuddyBooth, SIGNAL(clicked()), this, SLOT(onEnableBuddyBoothChanged()));

  chkUsePhysicalBuddyButton = new QCheckBox;
  connect(chkUsePhysicalBuddyButton, SIGNAL(clicked()), this, SLOT(onUsePhysicalBuddyButtonChanged()));

  spnBuddyAddress = new QSpinBox;
  spnBuddyAddress->setMinimum(21);
  spnBuddyAddress->setMaximum(254);

  chkUseCardSwipe = new QCheckBox;
  connect(chkUseCardSwipe, SIGNAL(clicked()), this, SLOT(onEnableCardSwipe()));

  chkEnableOnscreenKeypad = new QCheckBox;
  chkShowKeypadHelp = new QCheckBox;

  chkAllowCreditCards = new QCheckBox;
  connect(chkAllowCreditCards, SIGNAL(clicked()), this, SLOT(onAllowCreditCardChanged()));

  spnCreditChargeAmount = new QDoubleSpinBox;
  spnCreditChargeAmount->setMinimum(.01);
  spnCreditChargeAmount->setMaximum(999);
  spnCreditChargeAmount->setDecimals(2);
  spnCreditChargeAmount->setSingleStep(1);
  spnCreditChargeAmount->setPrefix("$");

  chkCashOnly = new QCheckBox;
  connect(chkCashOnly, SIGNAL(clicked()), this, SLOT(onCashOnlyChanged()));

  chkCoinSlot = new QCheckBox;

  spnPreviewCost = new QDoubleSpinBox;
  spnPreviewCost->setMinimum(.25);
  spnPreviewCost->setMaximum(999);
  spnPreviewCost->setDecimals(2);
  spnPreviewCost->setSingleStep(1);
  spnPreviewCost->setPrefix("$");

  spnThreeVideoPreviewCost = new QDoubleSpinBox;
  spnThreeVideoPreviewCost->setMinimum(.25);
  spnThreeVideoPreviewCost->setMaximum(999);
  spnThreeVideoPreviewCost->setDecimals(2);
  spnThreeVideoPreviewCost->setSingleStep(1);
  spnThreeVideoPreviewCost->setPrefix("$");

  spnFiveVideoPreviewCost = new QDoubleSpinBox;
  spnFiveVideoPreviewCost->setMinimum(.25);
  spnFiveVideoPreviewCost->setMaximum(999);
  spnFiveVideoPreviewCost->setDecimals(2);
  spnFiveVideoPreviewCost->setSingleStep(1);
  spnFiveVideoPreviewCost->setPrefix("$");

  spnSecondsPerCredit = new QSpinBox;
  spnSecondsPerCredit->setMinimum(1);
  spnSecondsPerCredit->setMaximum(999);

  //spnPreviewSessionMinutes = new QSpinBox;
  //spnPreviewSessionMinutes->setMinimum(1);
  //spnPreviewSessionMinutes->setMaximum(999);

  spnVideoBrowsingMinutes = new QSpinBox;
  spnVideoBrowsingMinutes->setMinimum(1);
  spnVideoBrowsingMinutes->setMaximum(999);

  spnAdditionalPreviewMinutesCost = new QDoubleSpinBox;
  spnAdditionalPreviewMinutesCost->setMinimum(.25);
  spnAdditionalPreviewMinutesCost->setMaximum(999);
  spnAdditionalPreviewMinutesCost->setDecimals(2);
  spnAdditionalPreviewMinutesCost->setSingleStep(1);
  spnAdditionalPreviewMinutesCost->setPrefix("$");

  spnAdditionalPreviewMinutes = new QSpinBox;
  spnAdditionalPreviewMinutes->setMinimum(1);
  spnAdditionalPreviewMinutes->setMaximum(999);

  chkAllowAdditionalPreviewMinutesSinglePreview = new QCheckBox;

  spnMaxTimePreviewCardProgram = new QSpinBox;
  spnMaxTimePreviewCardProgram->setMinimum(1);
  spnMaxTimePreviewCardProgram->setMaximum(999);

  spnLowCreditThreshold = new QSpinBox;
  spnLowCreditThreshold->setMinimum(1);
  spnLowCreditThreshold->setMaximum(999);

  spnLowPreviewMinuteThreshold = new QSpinBox;
  spnLowPreviewMinuteThreshold->setMinimum(1);
  spnLowPreviewMinuteThreshold->setMaximum(999);

  chkCcDevelopmentMode = new QCheckBox;

  btnPaymentGateway = new QPushButton(tr("Payment Gateway..."));
  connect(btnPaymentGateway, SIGNAL(clicked()), this, SLOT(onPaymentGatewayButtonClicked()));

  chkDemoMode = new QCheckBox;

  chkEnableBillAcceptorDoor = new QCheckBox;
  connect(chkEnableBillAcceptorDoor, SIGNAL(clicked()), this, SLOT(onEnableBillAcceptorChanged()));

  chkEnableBillAcceptorDoorAlarm = new QCheckBox;

  chkEnableNrMeters = new QCheckBox;

  chkEnableMeters = new QCheckBox;
  connect(chkEnableMeters, SIGNAL(clicked()), this, SLOT(onEnableMetersChanged()));

  chkEnableFreePlay = new QCheckBox;
  connect(chkEnableFreePlay, SIGNAL(clicked()), this, SLOT(onEnableFreePlay()));

  spnFreePlayInactivity = new QSpinBox;
  spnFreePlayInactivity->setMinimum(1);
  spnFreePlayInactivity->setMaximum(45);

  spnResumeFreePlaySession = new QSpinBox;
  spnResumeFreePlaySession->setMinimum(1);
  spnResumeFreePlaySession->setMaximum(15);

  cmbTheaterCategory = new QComboBox;
  cmbTheaterCategory->insertItem(0, "All Titles");
  cmbTheaterCategory->addItems(dbMgr->getVideoAttributes(2));

  btnSave = new QPushButton(tr("Save"));
  connect(btnSave, SIGNAL(clicked()), this, SLOT(onSaveButtonClicked()));

  btnUndo = new QPushButton(tr("Undo Changes"));
  connect(btnUndo, SIGNAL(clicked()), this, SLOT(onUndoButtonClicked()));


  lblGeneralSettings = new QLabel(tr("General Settings"));
  lblGeneralSettings->setStyleSheet("font-weight:bold;");
  //lblArcadeSettings = new QLabel(tr("Arcade Settings"));
  //lblArcadeSettings->setStyleSheet("font-weight:bold; margin:15px 0 0 0;");
  lblArcadeSettings = new QLabel;
  //lblArcadeSettings->setStyleSheet("font-weight:bold; margin:15px 0 0 0;");
  //lblPreviewSettings = new QLabel(tr("Preview Settings"));
  //lblPreviewSettings->setStyleSheet("font-weight:bold; margin:15px 0 0 0;");
  lblPreviewSettings = new QLabel;
  //lblPreviewSettings->setStyleSheet("font-weight:bold; margin:15px 0 0 0;");
  //lblTheaterSettings = new QLabel(tr("Theater Settings"));
  //lblTheaterSettings->setStyleSheet("font-weight:bold; margin:15px 0 0 0;");
  lblTheaterSettings = new QLabel;
  //lblTheaterSettings->setStyleSheet("font-weight:bold; margin:15px 0 0 0;");

  // Column 1,2
  generalSettingsLayout = new QGridLayout;
  generalSettingsLayout->addWidget(lblGeneralSettings, 0, SETTINGS_COLUMN_0, 1, 8);
  generalSettingsLayout->addWidget(new QLabel(tr("Address")), DEVICE_ADDRESS_ROW, SETTINGS_COLUMN_0);
  generalSettingsLayout->addWidget(txtDeviceAddress, DEVICE_ADDRESS_ROW, SETTINGS_COLUMN_1);
  generalSettingsLayout->addWidget(new QLabel(tr("Name")), DEVICE_NAME_ROW, SETTINGS_COLUMN_0);
  generalSettingsLayout->addWidget(txtDeviceAlias, DEVICE_NAME_ROW, SETTINGS_COLUMN_1);
  generalSettingsLayout->addWidget(new QLabel(tr("Device Type")), DEVICE_TYPE_ROW, SETTINGS_COLUMN_0);
  generalSettingsLayout->addWidget(cmbDeviceTypes, DEVICE_TYPE_ROW, SETTINGS_COLUMN_1);
  generalSettingsLayout->addWidget(new QLabel(tr("Enable Bill Acceptor")), ENABLE_BILL_ACCEPTOR_ROW, SETTINGS_COLUMN_0);
  generalSettingsLayout->addWidget(chkEnableBillAcceptorDoor, ENABLE_BILL_ACCEPTOR_ROW, SETTINGS_COLUMN_1);
  generalSettingsLayout->addWidget(new QLabel(tr("Acceptor Interface")), ACCEPTOR_INTERFACE_ROW, SETTINGS_COLUMN_0);
  generalSettingsLayout->addWidget(cmbBillAcceptorInterface, ACCEPTOR_INTERFACE_ROW, SETTINGS_COLUMN_1);
  generalSettingsLayout->addWidget(new QLabel(tr("Enable Bill Acceptor Alarm")), ENABLE_BILL_ACCEPTOR_ALARM_ROW, SETTINGS_COLUMN_0);
  generalSettingsLayout->addWidget(chkEnableBillAcceptorDoorAlarm, ENABLE_BILL_ACCEPTOR_ALARM_ROW, SETTINGS_COLUMN_1);
  generalSettingsLayout->addWidget(new QLabel(tr("Enable Card Reader")), ENABLE_CARD_READER_ROW, SETTINGS_COLUMN_0);
  generalSettingsLayout->addWidget(chkUseCardSwipe, ENABLE_CARD_READER_ROW, SETTINGS_COLUMN_1);

  // Column 3,4
  generalSettingsLayout->addWidget(new QLabel(tr("Enable Free Play")), ENABLE_FREE_PLAY_ROW, SETTINGS_COLUMN_3);
  generalSettingsLayout->addWidget(chkEnableFreePlay, ENABLE_FREE_PLAY_ROW, SETTINGS_COLUMN_4);
  generalSettingsLayout->addWidget(new QLabel(tr("Allow Credit Cards")), ALLOW_CREDIT_CARDS_ROW, SETTINGS_COLUMN_3);
  generalSettingsLayout->addWidget(chkAllowCreditCards, ALLOW_CREDIT_CARDS_ROW, SETTINGS_COLUMN_4);
  generalSettingsLayout->addWidget(btnPaymentGateway, PAYMENT_GATEWAY_ROW, SETTINGS_COLUMN_3);
  generalSettingsLayout->addWidget(new QLabel(tr("Enable CC Test Mode")), ENABLE_CC_TEST_ROW, SETTINGS_COLUMN_3);
  generalSettingsLayout->addWidget(chkCcDevelopmentMode, ENABLE_CC_TEST_ROW, SETTINGS_COLUMN_4);
  generalSettingsLayout->addWidget(new QLabel(tr("Enable Demo Mode")), ENABLE_DEMO_MODE_ROW, SETTINGS_COLUMN_3);
  generalSettingsLayout->addWidget(chkDemoMode, ENABLE_DEMO_MODE_ROW, SETTINGS_COLUMN_4);
  generalSettingsLayout->addWidget(new QLabel(tr("Enable Keypad")), ENABLE_KEYPAD_ROW, SETTINGS_COLUMN_3);
  generalSettingsLayout->addWidget(chkEnableOnscreenKeypad, ENABLE_KEYPAD_ROW, SETTINGS_COLUMN_4);
  generalSettingsLayout->addWidget(new QLabel(tr("Show Keypad Help")), SHOW_KEYPAD_HELP_ROW, SETTINGS_COLUMN_3);
  generalSettingsLayout->addWidget(chkShowKeypadHelp, SHOW_KEYPAD_HELP_ROW, SETTINGS_COLUMN_4);

  // Column 5,6
  generalSettingsLayout->addWidget(new QLabel(tr("Use CVS")), USE_CVS_ROW, SETTINGS_COLUMN_6);
  generalSettingsLayout->addWidget(chkUseCVS, USE_CVS_ROW, SETTINGS_COLUMN_7);
  generalSettingsLayout->addWidget(new QLabel(tr("Cash Only")), CASH_ONLY_ROW, SETTINGS_COLUMN_6);
  generalSettingsLayout->addWidget(chkCashOnly, CASH_ONLY_ROW, SETTINGS_COLUMN_7);
  generalSettingsLayout->addWidget(new QLabel(tr("Enable Coin Slot")), ENABLE_COIN_SLOT_ROW, SETTINGS_COLUMN_6);
  generalSettingsLayout->addWidget(chkCoinSlot, ENABLE_COIN_SLOT_ROW, SETTINGS_COLUMN_7);
  generalSettingsLayout->addWidget(new QLabel(tr("Store #")), STORE_NUM_ROW, SETTINGS_COLUMN_6);
  generalSettingsLayout->addWidget(spnStoreNum, STORE_NUM_ROW, SETTINGS_COLUMN_7);
  generalSettingsLayout->addWidget(new QLabel(tr("Area #")), AREA_NUM_ROW, SETTINGS_COLUMN_6);
  generalSettingsLayout->addWidget(spnAreaNum, AREA_NUM_ROW, SETTINGS_COLUMN_7);
  generalSettingsLayout->addWidget(new QLabel(tr("Enable NR Meters")), ENABLE_NR_METERS_ROW, SETTINGS_COLUMN_6);
  generalSettingsLayout->addWidget(chkEnableNrMeters, ENABLE_NR_METERS_ROW, SETTINGS_COLUMN_7);
  generalSettingsLayout->addWidget(new QLabel(tr("Enable Meters")), ENABLE_METERS_ROW, SETTINGS_COLUMN_6);
  generalSettingsLayout->addWidget(chkEnableMeters, ENABLE_METERS_ROW, SETTINGS_COLUMN_7);

  // Set the column sizes so that the ones with widgets are given more horizontal space than ones without
  generalSettingsLayout->setColumnStretch(SETTINGS_COLUMN_0, 2);
  generalSettingsLayout->setColumnStretch(SETTINGS_COLUMN_1, 2);
  generalSettingsLayout->setColumnStretch(SETTINGS_COLUMN_2, 1); // spacer column
  generalSettingsLayout->setColumnStretch(SETTINGS_COLUMN_3, 2);
  generalSettingsLayout->setColumnStretch(SETTINGS_COLUMN_4, 2);
  generalSettingsLayout->setColumnStretch(SETTINGS_COLUMN_5, 1); // spacer column
  generalSettingsLayout->setColumnStretch(SETTINGS_COLUMN_6, 2);
  generalSettingsLayout->setColumnStretch(SETTINGS_COLUMN_7, 2);

  arcadeSettingsLayout = new QGridLayout;
  arcadeSettingsLayout->addWidget(lblArcadeSettings, 0, 0, 1, 5);
  arcadeSettingsLayout->addWidget(new QLabel(tr("UI Type")), UI_TYPE_ROW, SETTINGS_COLUMN_0);
  arcadeSettingsLayout->addWidget(cmbArcadeUiType, UI_TYPE_ROW, SETTINGS_COLUMN_1);
  arcadeSettingsLayout->addWidget(new QLabel(tr("Seconds/Credit")), SECONDS_PER_CREDIT_ROW, SETTINGS_COLUMN_0);
  arcadeSettingsLayout->addWidget(spnSecondsPerCredit, SECONDS_PER_CREDIT_ROW, SETTINGS_COLUMN_1);
  arcadeSettingsLayout->addWidget(new QLabel(tr("CC Charge Amount")), CC_CHARGE_AMOUNT_ROW, SETTINGS_COLUMN_0);
  arcadeSettingsLayout->addWidget(spnCreditChargeAmount, CC_CHARGE_AMOUNT_ROW, SETTINGS_COLUMN_1);
  arcadeSettingsLayout->addWidget(new QLabel(tr("Remaining Credits Warning")), REM_CREDITS_WARN_ROW, SETTINGS_COLUMN_0);
  arcadeSettingsLayout->addWidget(spnLowCreditThreshold, REM_CREDITS_WARN_ROW, SETTINGS_COLUMN_1);
  arcadeSettingsLayout->addWidget(new QLabel(tr("Free Play Inactivity")), FREE_PLAY_INACTIVITY_ROW, SETTINGS_COLUMN_0);
  arcadeSettingsLayout->addWidget(spnFreePlayInactivity, FREE_PLAY_INACTIVITY_ROW, SETTINGS_COLUMN_1);
  arcadeSettingsLayout->addWidget(new QLabel(tr("Free Play Resume")), FREE_PLAY_RESUME_ROW, SETTINGS_COLUMN_0);
  arcadeSettingsLayout->addWidget(spnResumeFreePlaySession, FREE_PLAY_RESUME_ROW, SETTINGS_COLUMN_1);

  arcadeSettingsLayout->addWidget(new QLabel(tr("Enable Buddy Booth")), ENABLE_BUDDY_BOOTH_ROW, SETTINGS_COLUMN_3);
  arcadeSettingsLayout->addWidget(chkBuddyBooth, ENABLE_BUDDY_BOOTH_ROW, SETTINGS_COLUMN_4);
  arcadeSettingsLayout->addWidget(new QLabel(tr("Buddy Address")), BUDDY_ADDRESS_ROW, SETTINGS_COLUMN_3);
  arcadeSettingsLayout->addWidget(spnBuddyAddress, BUDDY_ADDRESS_ROW, SETTINGS_COLUMN_4);
  arcadeSettingsLayout->addWidget(new QLabel(tr("Use Physical Buddy Button")), USE_PHYSICAL_BUDDY_BUTTON_ROW, SETTINGS_COLUMN_3);
  arcadeSettingsLayout->addWidget(chkUsePhysicalBuddyButton, USE_PHYSICAL_BUDDY_BUTTON_ROW, SETTINGS_COLUMN_4);

  customCategoryWidget = new QWidget;
  customCategoryWidget->setStyleSheet("border-left: 1px solid #aaa;");

  txtCustomCategory = new QLineEdit;
  txtCustomCategory->setMaxLength(18);

  btnAddProducer = new QPushButton("Add");
  connect(btnAddProducer, SIGNAL(clicked()), this, SLOT(addProducerClicked()));

  btnRemoveProducer = new QPushButton("Remove");
  connect(btnRemoveProducer, SIGNAL(clicked()), this, SLOT(removeProducerClicked()));

  lstAssignedProducers = new QListWidget;
  lstAssignedProducers->setSelectionMode(QAbstractItemView::SingleSelection);

  cmbProducer = new QComboBox;
  cmbProducer->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  cmbProducer->setEditable(true);
  cmbProducer->setInsertPolicy(QComboBox::NoInsert);

  customCategoryLayout = new QGridLayout;
  customCategoryLayout->addWidget(new QLabel("Custom Category"), 0, 0);
  customCategoryLayout->addWidget(txtCustomCategory, 0, 1, 1, 3);
  customCategoryLayout->addWidget(new QLabel("Producers"), 1, 0);
  customCategoryLayout->addWidget(cmbProducer, 1, 1, 1, 3);
  customCategoryLayout->addWidget(btnAddProducer, 2, 0, 1, 2);
  customCategoryLayout->addWidget(btnRemoveProducer, 2, 2, 1, 2);
  customCategoryLayout->addWidget(lstAssignedProducers, 3, 0, 3, 4);

  customCategoryWidget->setLayout(customCategoryLayout);

  arcadeSettingsLayout->addWidget(customCategoryWidget, UI_TYPE_ROW, SETTINGS_COLUMN_5, 6, 4);

  lookupService = new VideoLookupService(WEB_SERVICE_URL, settings->getValue(VIDEO_METADATA_PATH).toString(), settings->getValue(CUSTOMER_PASSWORD, "", true).toString());
  connect(lookupService, SIGNAL(producerListingResults(QStringList)), this, SLOT(populateProducers(QStringList)));

  previewSettingsLayout = new QGridLayout;
  previewSettingsLayout->addWidget(lblPreviewSettings, 0, 0, 1, 5);
  previewSettingsLayout->addWidget(new QLabel(tr("Preview Price")), PREVIEW_PRICE_ROW, SETTINGS_COLUMN_0);
  previewSettingsLayout->addWidget(spnPreviewCost, PREVIEW_PRICE_ROW, SETTINGS_COLUMN_1);
  previewSettingsLayout->addWidget(new QLabel(tr("3 Video Deluxe Package")), THREE_VIDEO_DELUXE_ROW, SETTINGS_COLUMN_0);
  previewSettingsLayout->addWidget(spnThreeVideoPreviewCost, THREE_VIDEO_DELUXE_ROW, SETTINGS_COLUMN_1);
  previewSettingsLayout->addWidget(new QLabel(tr("5 Video Deluxe Package")), FIVE_VIDEO_DELUXE_ROW, SETTINGS_COLUMN_0);
  previewSettingsLayout->addWidget(spnFiveVideoPreviewCost, FIVE_VIDEO_DELUXE_ROW, SETTINGS_COLUMN_1);
  previewSettingsLayout->addWidget(new QLabel(tr("Preview Browsing Inactivity")), PREVIEW_BROWSING_INACTIVITY_ROW, SETTINGS_COLUMN_0);
  previewSettingsLayout->addWidget(spnVideoBrowsingMinutes, PREVIEW_BROWSING_INACTIVITY_ROW, SETTINGS_COLUMN_1);
  previewSettingsLayout->addWidget(new QLabel(tr("Extend Preview Price")), EXTEND_PREVIEW_PRICE_ROW, SETTINGS_COLUMN_0);
  previewSettingsLayout->addWidget(spnAdditionalPreviewMinutesCost, EXTEND_PREVIEW_PRICE_ROW, SETTINGS_COLUMN_1);
  previewSettingsLayout->addWidget(new QLabel(tr("Extend Preview Session Minutes")), EXTEND_PREVIEW_SESSION_MINUTES_ROW, SETTINGS_COLUMN_0);
  previewSettingsLayout->addWidget(spnAdditionalPreviewMinutes, EXTEND_PREVIEW_SESSION_MINUTES_ROW, SETTINGS_COLUMN_1);

  previewSettingsLayout->addWidget(new QLabel(tr("Allow Extending Preview w/o Deluxe")), ALLOW_EXTENDING_PREVIEW_WITHOUT_DELUXE_ROW, SETTINGS_COLUMN_3);
  previewSettingsLayout->addWidget(chkAllowAdditionalPreviewMinutesSinglePreview, ALLOW_EXTENDING_PREVIEW_WITHOUT_DELUXE_ROW, SETTINGS_COLUMN_4);
  previewSettingsLayout->addWidget(new QLabel(tr("Max. Preview Card Prog By Customer")), MAX_PREVIEW_CARD_PROG_BY_CUSTOMER_ROW, SETTINGS_COLUMN_3);
  previewSettingsLayout->addWidget(spnMaxTimePreviewCardProgram, MAX_PREVIEW_CARD_PROG_BY_CUSTOMER_ROW, SETTINGS_COLUMN_4);
  previewSettingsLayout->addWidget(new QLabel(tr("Remaining Minutes Warning")), REMAINING_MINUTES_WARNING_ROW, SETTINGS_COLUMN_3);
  previewSettingsLayout->addWidget(spnLowPreviewMinuteThreshold, REMAINING_MINUTES_WARNING_ROW, SETTINGS_COLUMN_4);

  theaterSettingsLayout = new QGridLayout;
  theaterSettingsLayout->addWidget(lblTheaterSettings, 0, 0, 1, 5);
  theaterSettingsLayout->addWidget(new QLabel(tr("Category")), THEATER_CATEGORY_ROW, SETTINGS_COLUMN_0);
  theaterSettingsLayout->addWidget(cmbTheaterCategory, THEATER_CATEGORY_ROW, SETTINGS_COLUMN_1);

  buttonLayout = new QDialogButtonBox;
  buttonLayout->addButton(btnSave, QDialogButtonBox::AcceptRole);
  buttonLayout->addButton(btnUndo, QDialogButtonBox::DestructiveRole);

  deviceTree = new QTreeWidget;
  deviceTree->setColumnCount(1);
  deviceTree->header()->hide();
  // Horizontal size policy set to Max so it doesn't grow horizontally, it's only wide enough to accomadate its items
  deviceTree->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);

  connect(deviceTree, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(onCurrentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)));
  QTreeWidgetItem *rootAllDevices = new QTreeWidgetItem(deviceTree);
  rootAllDevices->setText(0, ALL_DEVICES_NODE_NAME);
  rootAllDevices->setData(0, Qt::UserRole, ALL_DEVICES_NODE_NAME);

  // Shift #, Start Time, End Time
  shiftsModel = new QStandardItemModel(0, 3);
  shiftsModel->setHorizontalHeaderItem(ShiftNum, new QStandardItem(QString("Shift #")));
  shiftsModel->setHorizontalHeaderItem(StartTime, new QStandardItem(QString("Start Time")));
  shiftsModel->setHorizontalHeaderItem(EndTime, new QStandardItem(QString("End Time")));

  shiftsTable = new QTableView;
  shiftsTable->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
  //  shiftsTable->horizontalHeader()->setStretchLastSection(true);
  shiftsTable->horizontalHeader()->setStyleSheet("font:bold Arial;");
  shiftsTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
  //shiftsTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // for some reason I need to set this because it's always showing it even though there's enough horizontal space
  shiftsTable->setSelectionMode(QAbstractItemView::SingleSelection);
  shiftsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  //shiftsTable->setWordWrap(true);
  shiftsTable->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
  shiftsTable->verticalHeader()->hide();
  shiftsTable->setModel(shiftsModel);
  shiftsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  shiftsTable->setSelectionMode(QAbstractItemView::NoSelection);

  // Username, RevID
  usersModel = new QStandardItemModel(0, 2);
  usersModel->setHorizontalHeaderItem(Username, new QStandardItem(QString("Username")));
  usersModel->setHorizontalHeaderItem(RevID, new QStandardItem(QString("RevID")));

  QSortFilterProxyModel *sortFilter = new QSortFilterProxyModel;
  sortFilter->setSourceModel(usersModel);
  sortFilter->sort(0, Qt::AscendingOrder);

  usersTable = new QTableView;
  usersTable->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
  usersTable->horizontalHeader()->setStretchLastSection(true);
  usersTable->horizontalHeader()->setStyleSheet("font:bold Arial;");
  usersTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
  usersTable->setSelectionMode(QAbstractItemView::SingleSelection);
  usersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  usersTable->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
  usersTable->verticalHeader()->hide();
  usersTable->setModel(sortFilter);
  usersTable->setColumnHidden(RevID, true);
  usersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

  shiftsUsersLayout = new QHBoxLayout;
  shiftsLayout = new QVBoxLayout;
  shiftButtonsLayout = new QHBoxLayout;
  usersLayout = new QVBoxLayout;
  userButtonsLayout = new QHBoxLayout;

  shiftButtonsLayout->addWidget(btnEditShifts, 0, Qt::AlignLeft);
  shiftButtonsLayout->addStretch(1);

//  shiftsLayout->addWidget(lblStoreShifts);
//  shiftsLayout->addWidget(shiftsTable, 3);
//  shiftsLayout->addLayout(shiftButtonsLayout, 1);

  userButtonsLayout->addWidget(btnAddUser, 0, Qt::AlignLeft);
  userButtonsLayout->addWidget(btnDeleteUser, 0, Qt::AlignLeft);
  userButtonsLayout->addWidget(btnChangePassword, 0, Qt::AlignLeft);
  userButtonsLayout->addStretch(1);

  usersLayout->addWidget(lblUsers);
  usersLayout->addWidget(usersTable, 3);
  usersLayout->addLayout(userButtonsLayout, 1);

  shiftsUsersLayout->addLayout(shiftsLayout);
  shiftsUsersLayout->addLayout(usersLayout);

  containerWidget = new QWidget;
  arcadeSettingsWidget = new QWidget;
  previewSettingsWidget = new QWidget;
  theaterSettingsWidget = new QWidget;

  arcadeSettingsVLayout = new QVBoxLayout;
  arcadeSettingsVLayout->addLayout(arcadeSettingsLayout);
  arcadeSettingsVLayout->addStretch(5);

  previewSettingsVLayout = new QVBoxLayout;
  previewSettingsVLayout->addLayout(previewSettingsLayout);
  previewSettingsVLayout->addStretch(5);

  theaterSettingsVLayout = new QVBoxLayout;
  theaterSettingsVLayout->addLayout(theaterSettingsLayout);
  theaterSettingsVLayout->addStretch(5);

  arcadeSettingsWidget->setLayout(arcadeSettingsVLayout);
  arcadeSettingsWidget->setWindowTitle(tr("Arcade Settings"));

  previewSettingsWidget->setLayout(previewSettingsVLayout);
  previewSettingsWidget->setWindowTitle(tr("Preview Settings"));

  theaterSettingsWidget->setLayout(theaterSettingsVLayout);
  theaterSettingsWidget->setWindowTitle(tr("Theater Settings"));

  boothSettingsTabs = new QTabWidget;
  boothSettingsTabs->addTab(arcadeSettingsWidget, arcadeSettingsWidget->windowTitle());

  verticalLayout = new QVBoxLayout;
  verticalLayout->addLayout(generalSettingsLayout);
  verticalLayout->addSpacing(20);
  verticalLayout->addWidget(boothSettingsTabs);
  verticalLayout->addWidget(buttonLayout);
  verticalLayout->addSpacing(20);
  verticalLayout->addLayout(shiftsUsersLayout, 2);

  containerWidget->setLayout(verticalLayout);

  horzLayout = new QHBoxLayout;
  horzLayout->addWidget(deviceTree);
  horzLayout->addWidget(containerWidget);

  this->setLayout(horzLayout);

  // on first load, use the list of known devices and request a copy of the casplayer_<address>.sqlite database from each
  // when finished, alert user if any devices could not be contacted
  // if there are databases for the missing devices then tell user these will be used but they may not be current
  // populate the tree view with the known devices
  // When a node is clicked in the tree view, the form is populated with the appropriate booth's settings
  // Once the user modifies the settings and clicks save, the new settings are written to the database and the booth is told to restart
  // If settings are modified and the user clicks a different node, tell the user the settings haven't been saved and give the option to save now or discard
  // Clicking the Undo Changes button allows reverting to the initial settings loaded when the node was clicked. It also effectively clears the flag that any changes were made to the settings
  // Clicking Save button when nothing has been changed results in a message stating this fact, the booth will not be restarted
  // If the address is changed, verify it doesn't conflict with another booth upon saving
}

BoothSettingsWidget::~BoothSettingsWidget()
{
  //horzLayout->deleteLater();
  lookupService->deleteLater();
}

bool BoothSettingsWidget::isBusy()
{
  return busy;
}

void BoothSettingsWidget::showEvent(QShowEvent *)
{
  QLOG_DEBUG() << QString("Showing Device Settings tab");

  populateDeviceTree();
  dbMgr->getUsers();
  lookupService->startProducerListing();

  if (firstLoad)
  {
    /*disconnect(casServer, SIGNAL(getBoothDatabaseSuccess(QString)), 0, 0);
    disconnect(casServer, SIGNAL(getBoothDatabaseFailed(QString)), 0, 0);
    disconnect(casServer, SIGNAL(getBoothDatabasesFinished()), 0, 0);

    connect(casServer, SIGNAL(getBoothDatabaseSuccess(QString)), this, SLOT(getBoothDatabaseSuccess(QString)));
    connect(casServer, SIGNAL(getBoothDatabaseFailed(QString)), this, SLOT(getBoothDatabaseFailed(QString)));
    connect(casServer, SIGNAL(getBoothDatabasesFinished()), this, SLOT(getBoothDatabasesFinished()));*/



    firstLoad = false;
  }
}

void BoothSettingsWidget::onCurrentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
  QLOG_DEBUG() << QString("Item selection changed in the device tree under Device Settings tab");

  // If there was a previous booth selected, make sure no changes were made
  if (previous)
  {
      QLOG_DEBUG()<<previous->data(0, Qt::UserRole).toInt();
    if (previous->data(0, Qt::UserRole).toInt() > 0)
    {
      // If changes found then ask user what to do
      if (boothSettingsChanged(previous->data(0, Qt::UserRole).toString()))
      {
        QLOG_DEBUG() << QString("User selected a different node in the booth tree before saving settings of booth: %1").arg(previous->data(0, Qt::UserRole).toString());

        if (QMessageBox::question(this, tr("Unsaved Settings"), tr("Settings for device address %1 were changed but not saved. Do you want to save them now?").arg(previous->data(0, Qt::UserRole).toString()), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
        {
          QLOG_DEBUG() << QString("User chose to save settings of booth: %1").arg(previous->data(0, Qt::UserRole).toString());

          if (validateInput())
          {
            updateDeviceSettingsSuccessList.clear();
            updateDeviceSettingsFailList.clear();
            updateDeviceSettingsInSessionList.clear();

            emit updatingBooth(true);

            // Process events so the tabs widget gets disabled
            QCoreApplication::processEvents();

            saveBoothSettings(previous->data(0, Qt::UserRole).toString());

            totalBoothsToUpdate = 1;
            emit initializeSettingsUpdateProgress(totalBoothsToUpdate);
            updateProgress();
            casServer->startUpdateSettings(QStringList() << settings->getValue(ARCADE_SUBNET).toString() + previous->data(0, Qt::UserRole).toString());
          }
          else
          {
            QLOG_DEBUG() << "Lost settings because of invalid input";
            QMessageBox::warning(this, tr("Unsaved Settings"), tr("Settings for device address %1 were lost. Return to it and input your changes again.").arg(previous->data(0, Qt::UserRole).toString()));
          }
        }
        else
        {
          // discard changes
          QLOG_DEBUG() << QString("User chose to discard setting changes for booth: %1").arg(previous->data(0, Qt::UserRole).toString());
        }
      }
    }
  }

  if (current->data(0, Qt::UserRole).toString() == ARCADE_BOOTHS_NODE_NAME ||
      current->data(0, Qt::UserRole).toString() == PREVIEW_BOOTHS_NODE_NAME ||
      current->data(0, Qt::UserRole).toString() == THEATERS_NODE_NAME ||
      current->data(0, Qt::UserRole).toString() == CLERKSTATIONS_NODE_NAME ||
      current->data(0, Qt::UserRole).toString() == ALL_DEVICES_NODE_NAME)
  {
    if (casServer->getDeviceAddresses().count() > 0)
    {
      containerWidget->setEnabled(true);

      if (current->data(0, Qt::UserRole).toString() == ALL_DEVICES_NODE_NAME)
      {
        boothSettingsTabs->clear();
        boothSettingsTabs->insertTab(0, arcadeSettingsWidget, arcadeSettingsWidget->windowTitle());
        boothSettingsTabs->insertTab(1, previewSettingsWidget, previewSettingsWidget->windowTitle());
        boothSettingsTabs->insertTab(2, theaterSettingsWidget, theaterSettingsWidget->windowTitle());

        // Populate the form with the first booth in the list
        QString deviceNum = getFirstDevice();

        if (!deviceNum.isEmpty())
          populateForm(deviceNum);
      }
      else if (current->data(0, Qt::UserRole).toString() == ARCADE_BOOTHS_NODE_NAME)
      {
        //QLOG_DEBUG() << QString(ARCADE_BOOTHS_NODE_NAME + " node selected");

        boothSettingsTabs->clear();
        boothSettingsTabs->insertTab(0, arcadeSettingsWidget, arcadeSettingsWidget->windowTitle());

        QString deviceNum = getFirstDevice(current->data(0, Qt::UserRole).toString());

        if (!deviceNum.isEmpty())
          populateForm(deviceNum);
      }
      else if (current->data(0, Qt::UserRole).toString() == PREVIEW_BOOTHS_NODE_NAME)
      {
        //QLOG_DEBUG() << QString(PREVIEW_BOOTHS_NODE_NAME + " node selected");

        boothSettingsTabs->clear();
        boothSettingsTabs->insertTab(0, previewSettingsWidget, previewSettingsWidget->windowTitle());

        QString deviceNum = getFirstDevice(current->data(0, Qt::UserRole).toString());

        if (!deviceNum.isEmpty())
          populateForm(deviceNum);
      }
      else if (current->data(0, Qt::UserRole).toString() == CLERKSTATIONS_NODE_NAME)
      {
        QLOG_DEBUG() << QString(CLERKSTATIONS_NODE_NAME + " node selected");

        boothSettingsTabs->clear();

        QString deviceNum = getFirstDevice(current->data(0, Qt::UserRole).toString());

        if (!deviceNum.isEmpty())
          populateForm(deviceNum);
      }
      else
      {
        //QLOG_DEBUG() << QString(THEATERS_NODE_NAME + " node selected");

        boothSettingsTabs->clear();
        boothSettingsTabs->insertTab(0, theaterSettingsWidget, theaterSettingsWidget->windowTitle());

        QString deviceNum = getFirstDevice(current->data(0, Qt::UserRole).toString());

        if (!deviceNum.isEmpty())
          populateForm(deviceNum);
      }
    }
    else
    {
      //QLOG_DEBUG() << QString("No devices in tree");

      boothSettingsTabs->clear();
      boothSettingsTabs->insertTab(0, arcadeSettingsWidget, arcadeSettingsWidget->windowTitle());
    }
  }
  else
  {
    //QLOG_DEBUG() << QString("Individual device selected in tree");

    // An individual device was selected in the device tree

    populateForm(current->data(0, Qt::UserRole).toString());
  }

  updateBoothSettingFieldStates();
}

void BoothSettingsWidget::addTreeNode(QString label, QVariant data, QString parentLabel)
{
  // This only supports a 2 level hierarchy:
  // Root
  //   - Level 1
  //       Level 2
  //       Level 2
  //       ...
  //   - Level 1
  //       Level 2
  //       ...
  //   ...

  QTreeWidgetItem *root = deviceTree->topLevelItem(0);

  // 1st level
  if (root->text(0) == parentLabel)
  {
    QTreeWidgetItem *childNode = new QTreeWidgetItem(root);
    childNode->setText(0, label);
    childNode->setData(0, Qt::UserRole, data);
  }
  else
  {
    bool found = false;

    // 2nd level
    for (int i = 0; i < root->childCount(); ++i)
    {
      QTreeWidgetItem *parentNode = root->child(i);

      if (parentNode->text(0) == parentLabel)
      {
        QTreeWidgetItem *childNode = new QTreeWidgetItem(parentNode);
        childNode->setText(0, label);
        childNode->setData(0, Qt::UserRole, data);
        found = true;
        break;
      }
    }

    // 1st not found so add 1st and 2nd level
    if (!found)
    {
      QTreeWidgetItem *parentNode = new QTreeWidgetItem(root);
      parentNode->setText(0, parentLabel);
      parentNode->setData(0, Qt::UserRole, parentLabel);

      QTreeWidgetItem *childNode = new QTreeWidgetItem(parentNode);
      childNode->setText(0, label);
      childNode->setData(0, Qt::UserRole, data);
    }
  }
}

void BoothSettingsWidget::populateForm(QString deviceAddress)
{
  BoothSettings booth;
  QString selectedNodeData = deviceTree->currentItem()->data(0, Qt::UserRole).toString();

  if (booth.load(QString("%1/casplayer_%2.sqlite").arg(settings->getValue(DATA_PATH).toString()).arg(deviceAddress)))
  {
    int index = -1;

    ccClientID = booth.CcClientID();
    ccMerchantAccountID = booth.CcMerchantAccountID();
    authorizeNetMarketType = booth.AuthorizeNetMarketType();
    authorizeNetDeviceType = booth.AuthorizeNetDeviceType();

    if (selectedNodeData != ALL_DEVICES_NODE_NAME)
    {
      index = cmbDeviceTypes->findText(booth.DeviceTypeToString());
      if (index > -1)
      {
        //qDebug("cmbDeviceTypes item");
        cmbDeviceTypes->setCurrentIndex(index);
        onDeviceTypeChanged();
      }
    }
    else
    {
      cmbDeviceTypes->setCurrentIndex(-1);
    }

    index = cmbArcadeUiType->findText(booth.UiTypeToString());
    if (index > -1)
    {
      // qDebug("cmbArcadeUiType item");
      cmbArcadeUiType->setCurrentIndex(index);
    }

    index = cmbBillAcceptorInterface->findText(booth.BillAcceptorInterfaceToString());
    if (index > -1)
    {
      // qDebug("cmbBillAcceptorInterface item");
      cmbBillAcceptorInterface->setCurrentIndex(index);
    }

    /*
    index = cmbPulsesPerDollar->findText(QString("%1").arg(booth.PulsesPerDollar()));
    if (index > -1)
    {
     // qDebug("cmbPulsesPerDollar item");
      cmbPulsesPerDollar->setCurrentIndex(index);
    }
    else
    {
      // Default to 1 pulse per dollar
      cmbPulsesPerDollar->setCurrentIndex(0);
    }
    */

    /*
    index = cmbCategoryFilter->findText(booth.CategoryFilter());
    if (index > -1)
    {
     // qDebug("cmbCategoryFilter item");
      cmbCategoryFilter->setCurrentIndex(index);
    }
    else
    {
     // qDebug("cmbCategoryFilter item NOT FOUND");
      cmbCategoryFilter->setCurrentIndex(0); // None
    }
    */

    if (selectedNodeData.toInt() != 0)
    {
      txtDeviceAddress->setText(QString("%1%2").arg(settings->getValue(ARCADE_SUBNET).toString()).arg(booth.DeviceAddress()));
      txtDeviceAlias->setText(booth.DeviceAlias());
    }
    else
    {
      txtDeviceAddress->clear();
      txtDeviceAlias->clear();
    }

    //chkEnableStreamingPreview->setChecked(booth.StreamingPreview());
    //onEnableStreamingPreviewChanged();

    spnStoreNum->setValue(booth.StoreNum());
    spnAreaNum->setValue(booth.AreaNum());
    chkUseCVS->setChecked(booth.UseCVS());

    chkUsePhysicalBuddyButton->setChecked(booth.EnablePhysicalBuddyButton());
    onUsePhysicalBuddyButtonChanged();

    if (selectedNodeData.toInt() != 0)
    {
      chkBuddyBooth->setChecked(booth.BuddyBooth());
      spnBuddyAddress->setValue(booth.BuddyAddress());
    }
    else
    {
      spnBuddyAddress->setValue(21);
      chkBuddyBooth->setChecked(false);
    }
    onEnableBuddyBoothChanged();

    chkUseCardSwipe->setChecked(booth.EnableCardSwipe());
    onEnableCardSwipe();

    chkShowKeypadHelp->setChecked(booth.ShowKeyPadInfo());

    chkAllowCreditCards->setChecked(booth.AllowCreditCards());
    onAllowCreditCardChanged();

    spnCreditChargeAmount->setValue(booth.CreditChargeAmount());

    chkCashOnly->setChecked(booth.CashOnly());
    onCashOnlyChanged();

    chkEnableOnscreenKeypad->setChecked(booth.EnableOnscreenKeypad());
    chkCoinSlot->setChecked(booth.CoinSlot());
    spnPreviewCost->setValue(booth.PreviewCost());
    spnThreeVideoPreviewCost->setValue(booth.ThreeVideoPreviewCost());
    spnFiveVideoPreviewCost->setValue(booth.FiveVideoPreviewCost());
    spnSecondsPerCredit->setValue(booth.SecondsPerCredit());
    //spnPreviewSessionMinutes->setValue(booth.PreviewSessionMinutes());
    spnVideoBrowsingMinutes->setValue(booth.VideoBrowsingInactivityMinutes());
    spnAdditionalPreviewMinutesCost->setValue(booth.AdditionalPreviewMinutesCost());
    spnAdditionalPreviewMinutes->setValue(booth.AdditionalPreviewMinutes());
    chkAllowAdditionalPreviewMinutesSinglePreview->setChecked(booth.AllowAdditionalPreviewMinutesSinglePreview());
    spnMaxTimePreviewCardProgram->setValue(booth.MaxTimePreviewCardProgram());
    spnLowCreditThreshold->setValue(booth.LowCreditThreshold());
    spnLowPreviewMinuteThreshold->setValue(booth.LowPreviewMinuteThreshold());
    chkCcDevelopmentMode->setChecked(booth.CcDevelopmentMode());
    chkDemoMode->setChecked(booth.DemoMode());
    onDemoChangedChanged();

    if (booth.DeviceType() == BoothSettings::ClerkStation && (booth.EnableBillAcceptor() || booth.EnableBillAcceptorDoorAlarm()))
    {
      // Force bill acceptor off if clerkstation
      chkEnableBillAcceptorDoor->setChecked(false);
      chkEnableBillAcceptorDoorAlarm->setChecked(false);
    }
    else
    {
      chkEnableBillAcceptorDoor->setChecked(booth.EnableBillAcceptor());
      chkEnableBillAcceptorDoorAlarm->setChecked(booth.EnableBillAcceptorDoorAlarm());
    }
    onEnableBillAcceptorChanged();

    chkEnableNrMeters->setChecked(booth.EnableNrMeters());
    chkEnableMeters->setChecked(booth.EnableMeters());
    onEnableMetersChanged();

    chkEnableFreePlay->setChecked(booth.EnableFreePlay());
    onEnableFreePlay();

    spnFreePlayInactivity->setValue(booth.FreePlayInactivityTimeout());

    spnResumeFreePlaySession->setValue(booth.ResumeFreePlaySessionTimeout());

    // Because of needing to be backwards compatible with CAS I, convert Transsexual -> She-Male for the CASplayer and She-Male -> Transsexual for CAS Mgr
    // I can't even remember why at this point...
    QString theaterCategory = booth.TheaterCategory();
    if (theaterCategory == "She-Male")
      theaterCategory = "Transsexual";

    index = cmbTheaterCategory->findText(theaterCategory);
    if (index > -1)
    {
      cmbTheaterCategory->setCurrentIndex(index);
    }

    clearShiftsTable();

    storeShifts = booth.Shifts();

    foreach (StoreShift shift, storeShifts)
    {
      insertShiftRow(shift);
    }

    txtCustomCategory->clear();
    lstAssignedProducers->clear();
    if (!booth.CustomMenuCategory().name.isEmpty())
    {
      txtCustomCategory->setText(booth.CustomMenuCategory().name);
      lstAssignedProducers->insertItems(0, booth.CustomMenuCategory().producers);
    }
  }
  else
  {
    QLOG_ERROR() << QString("Could not open: %1/casplayer_%2.sqlite").arg(settings->getValue(DATA_PATH).toString()).arg(deviceAddress);

    QMessageBox::warning(this, tr("Load Error"), tr("Could not load settings from device: %1.").arg(deviceAddress));
  }
}

void BoothSettingsWidget::getUsersFinished(QList<UserAccount> &users, bool ok)
{
  clearUsersTable();

  if (ok)
  {
    foreach (UserAccount user, users)
    {
      insertUserRow(user.getUsername(), user.getRevID());
    }
  }
  else
  {
    QLOG_ERROR() << "Could not get list of users from database";
  }
}

void BoothSettingsWidget::clearUsersTable()
{
  if (usersModel->rowCount() > 0)
  {
    usersModel->removeRows(0, usersModel->rowCount());
  }
}

void BoothSettingsWidget::insertUserRow(QString username, QString revID)
{
  QStandardItem *usernameField, *revIdField;

  usernameField = new QStandardItem(username);
  usernameField->setData(username);

  revIdField = new QStandardItem(revID);
  revIdField->setData(revID);

  int row = usersModel->rowCount();

  usersModel->setItem(row, Username, usernameField);
  usersModel->setItem(row, RevID, revIdField);
}

void BoothSettingsWidget::addUserFinished(QString username, bool ok)
{
  if (ok)
  {
    QLOG_DEBUG() << "New user created:" << username;
    dbMgr->getUsers();
  }
  else
  {
    QLOG_ERROR() << "Could not create user account";
    QMessageBox::warning(this, tr("User Account"), tr("Could not create user account. Please try again."));
  }
}

void BoothSettingsWidget::deleteUserFinished(QString username, bool ok)
{
  if (ok)
  {
    QLOG_DEBUG() << "Deleted user:" << username;

  }
  else
  {
    QLOG_ERROR() << "Could not delete user account";
    QMessageBox::warning(this, tr("User Account"), tr("Could not delete user account. Please try again."));
  }
}

void BoothSettingsWidget::updateUserFinished(UserAccount &userAccount, bool ok)
{
  if (ok)
  {
    QLOG_DEBUG() << QString("Updated user account: %1").arg(userAccount.getUsername());
    dbMgr->getUsers();
  }
  else
  {
    QLOG_ERROR() << "User account not updated in database";

    QMessageBox::warning(this, tr("Change Password"), tr("Could not update password of user account in database. Please try again."));
  }
}

void BoothSettingsWidget::populateProducers(QStringList producers)
{
  if (producers.length() > 0)
  {
    cmbProducer->clear();
    cmbProducer->insertItems(0, producers);
    cmbProducer->setCurrentIndex(-1);
  }
  else
  {
    QLOG_ERROR() << "Producer list is empty";
    QMessageBox::warning(this, tr("Producer List"), tr("Could not download the producer list from the server for the Custom Category setting."));
  }
}

void BoothSettingsWidget::addProducerClicked()
{
  if (cmbProducer->currentIndex() > -1)
  {
    QList<QListWidgetItem *> matches;

    matches = lstAssignedProducers->findItems(cmbProducer->currentText(), Qt::MatchExactly);

    if (matches.count() > 0)
    {
      QLOG_DEBUG() << QString("User tried to submit a producer to custom category that is already in the list");
      QMessageBox::warning(this, tr("Duplicate"), tr("The producer is already in the list."), QMessageBox::Ok);
      cmbProducer->setFocus();
    }
    else
    {
      lstAssignedProducers->insertItem(lstAssignedProducers->count(), cmbProducer->currentText());
      cmbProducer->setCurrentIndex(-1);
    }
  }
  else
  {
    QLOG_DEBUG() << QString("User tried to add a producer to custom category that is not in the list");
    QMessageBox::information(this, tr("Unknown Producer"), tr("Select a producer from the list. Only producers found in the list can be assigned to the custom category."));
  }
}

bool BoothSettingsWidget::containsExtendedChars(const QString &s)
{
   bool found = false;

   foreach (QChar c, s)
   {
     if (int(c.toAscii()) < 32 || int(c.toAscii()) > 126)
     {
       found = true;
       break;
     }
   }

   return found;
}

void BoothSettingsWidget::removeProducerClicked()
{
  if (lstAssignedProducers->currentRow() >= 0)
  {
    QListWidgetItem *item = lstAssignedProducers->takeItem(lstAssignedProducers->currentRow());
    QLOG_DEBUG() << QString("Removing producer: %1 from custom category producers.").arg(item->data(Qt::UserRole).toString());

    delete item;
  }
  else
  {
    QLOG_DEBUG() << QString("User tried to remove a producer from the custom category producers list without selecting anything");
    QMessageBox::warning(this, tr("No item"), tr("Select a producer from the list to remove."), QMessageBox::Ok);
  }

  // Return focus to the producer field
  cmbProducer->setFocus();
}

void BoothSettingsWidget::clearShiftsTable()
{
  if (shiftsModel->rowCount() > 0)
  {
    shiftsModel->removeRows(0, shiftsModel->rowCount());
  }
}

void BoothSettingsWidget::insertShiftRow(StoreShift &shift)
{
  //QLOG_DEBUG() << "shift" << shift.getShiftNum() << "start" << shift.getStartTime().toString("h:mm ap") << "end" << shift.getEndTime().toString("h:mm ap");

  QStandardItem *shiftNumField, *startTimeField, *endTimeField;

  shiftNumField = new QStandardItem(QString("%1").arg(shift.getShiftNum()));
  shiftNumField->setData(shift.getShiftNum());

  startTimeField = new QStandardItem(shift.getStartTime().toString("h:mm ap"));
  startTimeField->setData(shift.getStartTime());

  endTimeField = new QStandardItem(shift.getEndTime().toString("h:mm ap"));
  endTimeField->setData(shift.getEndTime());

  int row = shiftsModel->rowCount();

  shiftsModel->setItem(row, ShiftNum, shiftNumField);
  shiftsModel->setItem(row, StartTime, startTimeField);
  shiftsModel->setItem(row, EndTime, endTimeField);
}

QVariantList BoothSettingsWidget::getDeviceTypes()
{
  QVariantList deviceTypeList;

  const QMetaObject &mo = Settings::staticMetaObject;
  int index = mo.indexOfEnumerator("Device_Type");
  QMetaEnum metaEnum = mo.enumerator(index);

  // Populate drop down menu with all keys in the Device_Type enumerator
  for (int i = 0; i < metaEnum.keyCount(); i++)
  {
    if (metaEnum.key(i))
    {
      QVariantMap keyValue;
      keyValue[QString(metaEnum.key(i))] = metaEnum.value(i);
      deviceTypeList.append(keyValue);
    }
  }

  return deviceTypeList;
}

void BoothSettingsWidget::onSaveButtonClicked()
{
  // FIXME: Certain settings should be ignored depending on various field states. Such as when theater mode set, disable free play

  QLOG_DEBUG() << QString("User clicked the Save booth settings button");

  if (casServer->isBusy())
  {
    QLOG_DEBUG() << "casServer has another thread running, cannot start saving device settings";
    QMessageBox::warning(this, tr("Device Settings"), tr("Settings cannot be saved because the Booth Status tab is checking the status of the arcade. Please wait a moment and try again."));
  }
  else
  {
    updateDeviceSettingsSuccessList.clear();
    updateDeviceSettingsFailList.clear();
    updateDeviceSettingsInSessionList.clear();

    QString deviceAddress = deviceTree->currentItem()->data(0, Qt::UserRole).toString();

    // If All Devices, Arcade Booths, Preview Booths, Clerkstations or Theaters then the settings will be saved to a group of booths
    if (deviceAddress.toInt() == 0)
    {
      QLOG_DEBUG() << QString("User clicked Save booth settings button with \"%1\" selected").arg(deviceAddress);

      QString applicableDevices = "";

      if (deviceAddress == ALL_DEVICES_NODE_NAME)
      {
        applicableDevices = "<b>all arcade/preview booths</b>";
      }
      else if (deviceAddress == ARCADE_BOOTHS_NODE_NAME)
      {
        applicableDevices = "<b>all arcade booths</b>";
      }
      else if (deviceAddress == PREVIEW_BOOTHS_NODE_NAME)
      {
        applicableDevices = "<b>all preview booths</b>";
      }
      else if (deviceAddress == CLERKSTATIONS_NODE_NAME)
      {
        applicableDevices = "<b>all clerkstations</b>";
      }
      else
      {
        applicableDevices = "<b>all theaters</b>";
      }

      if (QMessageBox::question(this, tr("Save Settings"), tr("The settings you apply when the \"<b>%1</b>\" item is selected in the device tree will apply to %2. Are you sure this is what you want to do?").arg(deviceAddress).arg(applicableDevices), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
      {
        QLOG_DEBUG() << "User confirmed wanting to apply global settings";

        updateAllBooths();
      }
      else
      {
        QLOG_DEBUG() << "User canceled applying global settings";
      }
    }
    else
    {
      // if any value has changed then
      if (boothSettingsChanged(deviceAddress))
      {
        //   save values to casplayer_<address>.sqlite where address is the current node selected in the tree view
        //   send message to booth to restart
        //   if booth responded to command then
        //     tell user the settings were saved and booth is restarting
        //   else
        //     tell user the settings were saved but the booth did not respond

        if (validateInput())
        {
          emit updatingBooth(true);

          // Process events so the tabs widget gets disabled
          QCoreApplication::processEvents();

          saveBoothSettings(deviceAddress);

          totalBoothsToUpdate = 1;
          emit initializeSettingsUpdateProgress(totalBoothsToUpdate);
          updateProgress();
          casServer->startUpdateSettings(QStringList() << settings->getValue(ARCADE_SUBNET).toString() + deviceAddress);
        }
      }
      else
      {
        QLOG_DEBUG() << QString("User clicked Save booth settings button but no settings were changed for device address: %1").arg(deviceAddress);

        QMessageBox::information(this, tr("Save"), tr("You did not make any changes to this device, there is nothing to save."));
      }
    }
  }
}

void BoothSettingsWidget::updateAllBooths()
{
  if (validateInput(true))
  {
    emit updatingBooth(true);

    // Process events so the tabs widget gets disabled
    QCoreApplication::processEvents();

    totalBoothsToUpdate = settings->getValue(DEVICE_LIST).toStringList().count();
    emit initializeSettingsUpdateProgress(totalBoothsToUpdate);
    updateProgress();

    foreach (QString deviceNum, casServer->getDeviceAddresses())
    {
      saveBoothSettings(deviceNum);

      // TODO: This is done right now since it takes so long to update so many records in each database
      // once the settings are all in one record it will be much quicker
      QCoreApplication::processEvents();
    }
    // Save the change to the tip of the day index and possibly the show changelog flag if it changed
    dbMgr->setValue("all_settings", settings->getSettings());

    casServer->startUpdateSettings(settings->getValue(DEVICE_LIST).toStringList());
  }
}

void BoothSettingsWidget::onUndoButtonClicked()
{
  QLOG_DEBUG() << QString("User clicked the Undo Changes to booth settings button");

  QString deviceAddress = deviceTree->currentItem()->data(0, Qt::UserRole).toString();

  if (deviceAddress.toInt() != 0)
  {
    // if any value has changed then
    if (boothSettingsChanged(deviceAddress))
    {
      // ask user if changes should be undone
      if (QMessageBox::question(this, tr("Undo Changes"), tr("Settings for device address %1 were changed but not saved, are you sure you want to lose them?").arg(deviceAddress), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
      {
        QLOG_DEBUG() << QString("User chose to undo changes to booth: %1").arg(deviceAddress);
        populateForm(deviceAddress);
        updateBoothSettingFieldStates();
      }
      else
      {
        // discard changes
        QLOG_DEBUG() << QString("User chose not to undo changes to booth: %1").arg(deviceAddress);
      }
    }
    else
    {
      QLOG_DEBUG() << QString("User clicked Undo changes button but no settings were changed for device address: %1").arg(deviceAddress);

      QMessageBox::information(this, tr("Undo"), tr("You did not make any changes to this device, there is nothing to undo."));
    }
  }
  else
  {
    QLOG_DEBUG() << "Cannot undo changes when \"All Devices\", \"Arcade Booths\", \"Preview Booths\" or \"Theaters\" is selected";

    QMessageBox::information(this, tr("Undo"), tr("Changes made when \"All Devices\", \"Arcade Booths\", \"Preview Booths\" or \"Theaters\" is selected cannot be undone. Just click an individual device in the list to reset the changes."));
  }
}

void BoothSettingsWidget::populateDeviceTree()
{
  clearChildNodes();

  QVariantList boothList = dbMgr->getBoothInfo(casServer->getDeviceAddresses());

  foreach (QVariant v, boothList)
  {
    QVariantMap booth = v.toMap();

    QString parentNode = "";

    switch (booth["device_type_id"].toInt())
    {
      case BoothSettings::Arcade:
        parentNode = ARCADE_BOOTHS_NODE_NAME;
        break;

      case BoothSettings::Preview:
        parentNode = PREVIEW_BOOTHS_NODE_NAME;
        break;

      case BoothSettings::Theater:
        parentNode = THEATERS_NODE_NAME;
        break;

      case BoothSettings::ClerkStation:
        parentNode = CLERKSTATIONS_NODE_NAME;
        break;

      default:
        QLOG_ERROR() << QString("Device type: %1 is not recognized, cannot add device address: %2 to device tree").arg(booth["device_type"].toString()).arg(booth["device_num"].toString());
    }

    if (!parentNode.isEmpty())
    {
      QString label = booth["device_alias"].toString();
      if (label.isEmpty())
        label = booth["device_num"].toString();

      addTreeNode(label, booth["device_num"].toString(), parentNode);
    }
  }

  deviceTree->expandAll();
  deviceTree->setCurrentItem(deviceTree->topLevelItem(0));
}

void BoothSettingsWidget::onDeviceTypeChanged()
{
  //QLOG_DEBUG() << "onDeviceTypeChanged";

  switch ((BoothSettings::Device_Type)cmbDeviceTypes->itemData(cmbDeviceTypes->currentIndex()).toInt())
  {
    case BoothSettings::Arcade:
      //QLOG_DEBUG() << "case BoothSettings::Arcade";

      // Remove other tabs and add the arcade settings tab
      boothSettingsTabs->clear();
      boothSettingsTabs->insertTab(0, arcadeSettingsWidget, arcadeSettingsWidget->windowTitle());

      break;

    case BoothSettings::Preview:
      //QLOG_DEBUG() << "case BoothSettings::Preview";

      // Remove other tabs and add the preview settings tab
      boothSettingsTabs->clear();
      boothSettingsTabs->insertTab(0, previewSettingsWidget, previewSettingsWidget->windowTitle());

      break;

    case BoothSettings::Theater:
      //QLOG_DEBUG() << "case BoothSettings::Theater";

      // Remove other tabs and add the theater settings tab
      boothSettingsTabs->clear();
      boothSettingsTabs->insertTab(0, theaterSettingsWidget, theaterSettingsWidget->windowTitle());

      break;

    case BoothSettings::ClerkStation:
      QLOG_DEBUG() << "case BoothSettings::ClerkStation";
      boothSettingsTabs->clear();

      break;
  }

  updateBoothSettingFieldStates();
}

void BoothSettingsWidget::onEnableBuddyBoothChanged()
{
  if (chkBuddyBooth->isChecked())
  {
    spnBuddyAddress->setEnabled(true);
    arcadeSettingsLayout->itemAtPosition(BUDDY_ADDRESS_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(true);

    chkUsePhysicalBuddyButton->setEnabled(true);
    arcadeSettingsLayout->itemAtPosition(USE_PHYSICAL_BUDDY_BUTTON_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(true);

    onUsePhysicalBuddyButtonChanged();
  }
  else
  {
    spnBuddyAddress->setEnabled(false);
    arcadeSettingsLayout->itemAtPosition(BUDDY_ADDRESS_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(false);

    if (chkUsePhysicalBuddyButton->isChecked())
      chkUsePhysicalBuddyButton->setChecked(false);

    chkUsePhysicalBuddyButton->setEnabled(false);
    arcadeSettingsLayout->itemAtPosition(USE_PHYSICAL_BUDDY_BUTTON_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(false);
  }
}

void BoothSettingsWidget::onUsePhysicalBuddyButtonChanged()
{
  if (chkUsePhysicalBuddyButton->isChecked())
  {
    // Use Physical Buddy Button is checked
    spnBuddyAddress->setEnabled(false);
    arcadeSettingsLayout->itemAtPosition(BUDDY_ADDRESS_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(false);
  }
  else
  {
    // Use Physical Buddy Button is unchecked
    spnBuddyAddress->setEnabled(true);
    arcadeSettingsLayout->itemAtPosition(BUDDY_ADDRESS_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(true);
  }
}

void BoothSettingsWidget::onEnableBillAcceptorChanged()
{
  if (chkEnableBillAcceptorDoor->isChecked())
  {
    cmbBillAcceptorInterface->setEnabled(true);
    //cmbPulsesPerDollar->setEnabled(true);
    chkCashOnly->setEnabled(true);

    generalSettingsLayout->itemAtPosition(CASH_ONLY_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(true);
    generalSettingsLayout->itemAtPosition(ACCEPTOR_INTERFACE_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(true);
    generalSettingsLayout->itemAtPosition(USE_CVS_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(true);
  }
  else
  {
    cmbBillAcceptorInterface->setEnabled(false);
    //cmbPulsesPerDollar->setEnabled(false);
    chkCashOnly->setChecked(false);
    chkCashOnly->setEnabled(false);

    generalSettingsLayout->itemAtPosition(CASH_ONLY_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(false);
    generalSettingsLayout->itemAtPosition(ACCEPTOR_INTERFACE_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(false);
    generalSettingsLayout->itemAtPosition(USE_CVS_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(false);
  }

  onCashOnlyChanged();
}

void BoothSettingsWidget::onAllowCreditCardChanged()
{
  if (chkAllowCreditCards->isChecked())
  {
    chkCcDevelopmentMode->setEnabled(true);
    spnCreditChargeAmount->setEnabled(true);

    // If the All Devices node is selected then enable Payment Gateway button
    if (deviceTree->currentItem()->data(0, Qt::UserRole).toString() == ALL_DEVICES_NODE_NAME)
      btnPaymentGateway->setEnabled(true);
    else
      btnPaymentGateway->setEnabled(false);

    generalSettingsLayout->itemAtPosition(ENABLE_CC_TEST_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(true);
    arcadeSettingsLayout->itemAtPosition(CC_CHARGE_AMOUNT_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(true);
  }
  else
  {
    chkCcDevelopmentMode->setChecked(false);
    chkCcDevelopmentMode->setEnabled(false);
    spnCreditChargeAmount->setEnabled(false);
    btnPaymentGateway->setEnabled(false);

    generalSettingsLayout->itemAtPosition(ENABLE_CC_TEST_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(false);
    arcadeSettingsLayout->itemAtPosition(CC_CHARGE_AMOUNT_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(false);
  }
}

void BoothSettingsWidget::onEnableCardSwipe()
{
  if (chkUseCardSwipe->isChecked())
  {
    chkAllowCreditCards->setEnabled(true);

    generalSettingsLayout->itemAtPosition(ALLOW_CREDIT_CARDS_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(true);
  }
  else
  {
    chkAllowCreditCards->setChecked(false);
    chkAllowCreditCards->setEnabled(false);

    generalSettingsLayout->itemAtPosition(ALLOW_CREDIT_CARDS_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(false);
  }

  onAllowCreditCardChanged();
}

void BoothSettingsWidget::onCashOnlyChanged()
{
  if (chkCashOnly->isChecked())
  {
    // Card swipe cannot be used when Cash Only enabled
    if (chkUseCardSwipe->isChecked())
    {
      chkUseCardSwipe->setChecked(false);
    }

    chkUseCardSwipe->setEnabled(false);
    generalSettingsLayout->itemAtPosition(ENABLE_CARD_READER_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(false);

    // Use CVS cannot be checked when Cash Only enabled
    if (chkUseCVS->isChecked())
    {
      chkUseCVS->setChecked(false);
    }

    chkUseCVS->setEnabled(false);
    generalSettingsLayout->itemAtPosition(USE_CVS_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(false);

    chkAllowCreditCards->setChecked(false);
    chkAllowCreditCards->setEnabled(false);
  }
  else
  {
    chkUseCardSwipe->setEnabled(true);
    generalSettingsLayout->itemAtPosition(ENABLE_CARD_READER_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(true);

    chkUseCVS->setEnabled(true);
    generalSettingsLayout->itemAtPosition(USE_CVS_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(true);
  }

  onEnableCardSwipe();
}

/*
void BoothSettingsWidget::onEnableStreamingPreviewChanged()
{
  if (chkEnableStreamingPreview->isChecked())
  {
    spnPreviewSessionMinutes->setEnabled(true);

    spnThreeVideoPreviewCost->setEnabled(false);
    spnFiveVideoPreviewCost->setEnabled(false);
    spnVideoBrowsingMinutes->setEnabled(false);
    spnAdditionalPreviewMinutes->setEnabled(false);
    spnAdditionalPreviewMinutesCost->setEnabled(false);
    chkAllowAdditionalPreviewMinutesSinglePreview->setEnabled(false);

    previewSettingsLayout->itemAtPosition(6, 0)->widget()->setEnabled(true);
    previewSettingsLayout->itemAtPosition(2, 0)->widget()->setEnabled(false);
    previewSettingsLayout->itemAtPosition(3, 0)->widget()->setEnabled(false);
    previewSettingsLayout->itemAtPosition(4, 0)->widget()->setEnabled(false);
    previewSettingsLayout->itemAtPosition(1, 3)->widget()->setEnabled(false);
    previewSettingsLayout->itemAtPosition(2, 3)->widget()->setEnabled(false);
    previewSettingsLayout->itemAtPosition(3, 3)->widget()->setEnabled(false);
  }
  else
  {
    spnPreviewSessionMinutes->setEnabled(false);

    spnThreeVideoPreviewCost->setEnabled(true);
    spnFiveVideoPreviewCost->setEnabled(true);
    spnVideoBrowsingMinutes->setEnabled(true);
    spnAdditionalPreviewMinutes->setEnabled(true);
    spnAdditionalPreviewMinutesCost->setEnabled(true);
    chkAllowAdditionalPreviewMinutesSinglePreview->setEnabled(true);

    previewSettingsLayout->itemAtPosition(6, 0)->widget()->setEnabled(false);
    previewSettingsLayout->itemAtPosition(2, 0)->widget()->setEnabled(true);
    previewSettingsLayout->itemAtPosition(3, 0)->widget()->setEnabled(true);
    previewSettingsLayout->itemAtPosition(4, 0)->widget()->setEnabled(true);
    previewSettingsLayout->itemAtPosition(1, 3)->widget()->setEnabled(true);
    previewSettingsLayout->itemAtPosition(2, 3)->widget()->setEnabled(true);
    previewSettingsLayout->itemAtPosition(3, 3)->widget()->setEnabled(true);
  }
}
*/

void BoothSettingsWidget::onDemoChangedChanged()
{
  if (chkDemoMode->isChecked())
  {
    chkUseCVS->setChecked(false);
    chkUseCVS->setEnabled(false);
    generalSettingsLayout->itemAtPosition(USE_CVS_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(false);
  }
  else
  {
    chkUseCVS->setEnabled(true);
    generalSettingsLayout->itemAtPosition(USE_CVS_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(true);
  }
}

void BoothSettingsWidget::onEnableMetersChanged()
{
  if (chkEnableMeters->isChecked())
  {
    QString selectedNodeData = deviceTree->currentItem()->data(0, Qt::UserRole).toString();

    // Only enable the Enable NR Meters checkbox and field label if the All Devices node is selected
    if (selectedNodeData == ALL_DEVICES_NODE_NAME)
    {
      generalSettingsLayout->itemAtPosition(ENABLE_NR_METERS_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(true);
      chkEnableNrMeters->setEnabled(true);
    }
    else
    {
      generalSettingsLayout->itemAtPosition(ENABLE_NR_METERS_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(false);
      chkEnableNrMeters->setEnabled(false);

    }
  }
  else
  {
    generalSettingsLayout->itemAtPosition(ENABLE_NR_METERS_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(false);
    chkEnableNrMeters->setChecked(false);
    chkEnableNrMeters->setEnabled(false);
  }
}

void BoothSettingsWidget::onEnableFreePlay()
{
  /*
   *Free Play checked
        Disable: Device Type, Bill Acceptor, Acceptor Interface, Card Reader, Allow Credit Cards, Keypad, Keypad Help, Use CVS, Cash Only, Coin Slot
        Seconds/Credit, Remaining Credits Warning,

    Free Play unchecked
            Enable: Device Type, Bill Acceptor, Acceptor Interface, Card Reader, Allow Credit Cards, Keypad, Keypad Help, Use CVS, Cash Only, Coin Slot
            Seconds/Credit, Remaining Credits Warning,

    Device type changed to not Arcade
            Disable Free Play checkbox
   */

  if (chkEnableFreePlay->isChecked())
  {
    // Enable Free Play is checked
    cmbDeviceTypes->setEnabled(false);
    generalSettingsLayout->itemAtPosition(DEVICE_TYPE_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(false);

    if (chkEnableBillAcceptorDoor->isChecked())
    {
      chkEnableBillAcceptorDoor->setChecked(false);
      onEnableBillAcceptorChanged();
    }

    //QLOG_DEBUG() << "onEnableFreePlay - disabling chkEnableBillAcceptorDoor";
    chkEnableBillAcceptorDoor->setEnabled(false);
    generalSettingsLayout->itemAtPosition(ENABLE_BILL_ACCEPTOR_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(false);

    if (chkUseCardSwipe->isChecked())
    {
      chkUseCardSwipe->setChecked(false);
      onEnableCardSwipe();
    }

    chkUseCardSwipe->setEnabled(false);
    generalSettingsLayout->itemAtPosition(ENABLE_CARD_READER_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(false);

    if (chkEnableOnscreenKeypad->isChecked())
    {
      chkEnableOnscreenKeypad->setChecked(false);
    }

    chkEnableOnscreenKeypad->setEnabled(false);
    generalSettingsLayout->itemAtPosition(ENABLE_KEYPAD_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(false);

    if (chkShowKeypadHelp->isChecked())
    {
      chkShowKeypadHelp->setChecked(false);
    }

    chkShowKeypadHelp->setEnabled(false);
    generalSettingsLayout->itemAtPosition(SHOW_KEYPAD_HELP_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(false);

    if (chkUseCVS->isChecked())
    {
      chkUseCVS->setChecked(false);
    }

    chkUseCVS->setEnabled(false);
    generalSettingsLayout->itemAtPosition(USE_CVS_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(false);

    if (chkCoinSlot->isChecked())
      chkCoinSlot->setChecked(false);

    chkCoinSlot->setEnabled(false);
    generalSettingsLayout->itemAtPosition(ENABLE_COIN_SLOT_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(false);

    spnSecondsPerCredit->setEnabled(false);
    arcadeSettingsLayout->itemAtPosition(SECONDS_PER_CREDIT_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(false);

    spnLowCreditThreshold->setEnabled(false);
    arcadeSettingsLayout->itemAtPosition(REM_CREDITS_WARN_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(false);

    spnFreePlayInactivity->setEnabled(true);
    arcadeSettingsLayout->itemAtPosition(FREE_PLAY_INACTIVITY_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(true);

    spnResumeFreePlaySession->setEnabled(true);
    arcadeSettingsLayout->itemAtPosition(FREE_PLAY_RESUME_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(true);
  }
  else
  {
    QString selectedNodeData = deviceTree->currentItem()->data(0, Qt::UserRole).toString();

    // Enable Free Play is unchecked

    // Only enable device types field if the All Devices node is not selected
    if (selectedNodeData != ALL_DEVICES_NODE_NAME)
    {
      cmbDeviceTypes->setEnabled(true);
      generalSettingsLayout->itemAtPosition(DEVICE_TYPE_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(true);
    }

    //QLOG_DEBUG() << "onEnableFreePlay - enabling chkEnableBillAcceptorDoor";
    chkEnableBillAcceptorDoor->setEnabled(true);
    generalSettingsLayout->itemAtPosition(ENABLE_BILL_ACCEPTOR_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(true);

    chkUseCardSwipe->setEnabled(true);
    generalSettingsLayout->itemAtPosition(ENABLE_CARD_READER_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(true);

    chkEnableOnscreenKeypad->setEnabled(true);
    generalSettingsLayout->itemAtPosition(ENABLE_KEYPAD_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(true);

    chkShowKeypadHelp->setEnabled(true);
    generalSettingsLayout->itemAtPosition(SHOW_KEYPAD_HELP_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(true);

    chkUseCVS->setEnabled(true);
    generalSettingsLayout->itemAtPosition(USE_CVS_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(true);

    chkCoinSlot->setEnabled(true);
    generalSettingsLayout->itemAtPosition(ENABLE_COIN_SLOT_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(true);

    spnSecondsPerCredit->setEnabled(true);
    arcadeSettingsLayout->itemAtPosition(SECONDS_PER_CREDIT_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(true);

    spnLowCreditThreshold->setEnabled(true);
    arcadeSettingsLayout->itemAtPosition(REM_CREDITS_WARN_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(true);

    spnFreePlayInactivity->setEnabled(false);
    arcadeSettingsLayout->itemAtPosition(FREE_PLAY_INACTIVITY_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(false);

    spnResumeFreePlaySession->setEnabled(false);
    arcadeSettingsLayout->itemAtPosition(FREE_PLAY_RESUME_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(false);
  }
}

void BoothSettingsWidget::onPaymentGatewayButtonClicked()
{
  QLOG_DEBUG() << "User clicked Payment Gateway button";

  // Show widget to allow selecting payment gateway, specifying credentials and testing settings
  PaymentGatewayWidget paymentWidget;

  // Set the current payment gateway, username and password
  paymentWidget.setPaymentGateway("Authorize.net"); // Currently supports only one payment gateway
  paymentWidget.setUsername(ccClientID);
  paymentWidget.setPassword(ccMerchantAccountID);
  paymentWidget.setAuthorizeNetMarketTypes(settings->getValue(AUTHORIZE_NET_MARKET_TYPES).toStringList());
  paymentWidget.setAuthorizeNetDeviceTypes(settings->getValue(AUTHORIZE_NET_DEVICE_TYPES).toStringList());
  paymentWidget.setAuthorizeNetMarketType(authorizeNetMarketType);
  paymentWidget.setAuthorizeNetDeviceType(authorizeNetDeviceType);  

  // If user clicked OK then update settings
  if (paymentWidget.exec() == QDialog::Accepted)
  {
    QLOG_DEBUG() << "User updated payment gateway settings";

    ccClientID = paymentWidget.getUsername();
    ccMerchantAccountID = paymentWidget.getPassword();
    authorizeNetMarketType = paymentWidget.getAuthorizeNetMarketType();
    authorizeNetDeviceType = paymentWidget.getAuthorizeNetDeviceType();
  }
  else
  {
    QLOG_DEBUG() << "User canceled updating payment gateway settings";
  }
}

void BoothSettingsWidget::updateSettingsSuccess(QString ipAddress)
{
  QLOG_DEBUG() << QString("Device %1 received settings update and is rebooting").arg(ipAddress);
  updateDeviceSettingsSuccessList.append(ipAddress);
  updateProgress();
  //QMessageBox::information(this, tr("Finished"), tr("The device received the new settings and is currently restarting."));
}

void BoothSettingsWidget::updateSettingsInSession(QString ipAddress)
{
  QLOG_DEBUG() << QString("Device %1 received settings update but is in session").arg(ipAddress);
  updateDeviceSettingsInSessionList.append(ipAddress);
  updateProgress();
  //QMessageBox::information(this, tr("Finished"), tr("The device received the new settings, but it is currently in session. It will restart when the session ends."));
}

void BoothSettingsWidget::updateSettingsFailed(QString ipAddress)
{
  QLOG_ERROR() << QString("Device %1 could not be contacted to apply new settings").arg(ipAddress);
  updateDeviceSettingsFailList.append(ipAddress);
  updateProgress();
  //QMessageBox::warning(this, tr("Save Error"), tr("Could not contact device: %1. Make sure the device is powered on and connected to the network.").arg(ipAddress));
}

void BoothSettingsWidget::updateSettingsFinished()
{
  if (updateDeviceSettingsFailList.count() > 0 || updateDeviceSettingsInSessionList.count() > 0)
    QMessageBox::warning(this, tr("Finished with Warning/Error"), tr("Not all devices received the new settings. %1%2")
                         .arg(updateDeviceSettingsInSessionList.count() > 0 ? QString("The following are currently in session or performing a movie change and will apply settings when finished: %1").arg(updateDeviceSettingsInSessionList.join(", ")) : "")
                         .arg(updateDeviceSettingsFailList.count() > 0 ? QString("The following failed to respond: %1").arg(updateDeviceSettingsFailList.join(", ")) : ""));
  else
  {
    if (updateDeviceSettingsSuccessList.count() == 1)
      QMessageBox::information(this, tr("Finished"), tr("The device received the new settings and is currently restarting."));
    else
      QMessageBox::information(this, tr("Finished"), tr("All devices received the new settings and are currently restarting."));
  }

  if (reloadDeviceTree)
  {
    populateDeviceTree();
    reloadDeviceTree = false;
  }

  emit updatingBooth(false);
  emit cleanupSettingsUpdateProgress();
}

void BoothSettingsWidget::updateProgress()
{
  int numBooths = updateDeviceSettingsSuccessList.count() + updateDeviceSettingsInSessionList.count() + updateDeviceSettingsFailList.count();
  emit updateSettingsUpdateProgress(numBooths, QString("Updated settings on %1 of %2 booth(s)").arg(numBooths).arg(totalBoothsToUpdate));
}

void BoothSettingsWidget::updateBoothSettingFieldStates()
{
  // FIXME: Need to still do testing with these various states, not sure if this is working correctly. Ex. changing device type doesn't enable/disable other fields that are needed/not needed
  // Enable/disable applicable fields depending on the current node selected in the tree and the current device type selected

  // if All Device selected then
  if (deviceTree->currentItem()->data(0, Qt::UserRole).toString() == ALL_DEVICES_NODE_NAME)
  {
    containerWidget->setEnabled(true);

    // Disable: device address, device alias, device type, free play, buddy booth
    generalSettingsLayout->itemAtPosition(DEVICE_ADDRESS_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(false);

    generalSettingsLayout->itemAtPosition(DEVICE_NAME_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(false);
    txtDeviceAlias->setEnabled(false);

    generalSettingsLayout->itemAtPosition(DEVICE_TYPE_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(false);
    cmbDeviceTypes->setEnabled(false);

    generalSettingsLayout->itemAtPosition(ENABLE_FREE_PLAY_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(false);
    chkEnableFreePlay->setEnabled(false);

    arcadeSettingsLayout->itemAtPosition(ENABLE_BUDDY_BOOTH_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(false);
    chkBuddyBooth->setEnabled(false);

    // Enable: bill acceptor, bill acceptor alarm, card reader, demo mode, keypad, keypad help, use cvs, coin slot, store #, area #, meters
    generalSettingsLayout->itemAtPosition(ENABLE_BILL_ACCEPTOR_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(true);
    chkEnableBillAcceptorDoor->setEnabled(true);

    generalSettingsLayout->itemAtPosition(ENABLE_BILL_ACCEPTOR_ALARM_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(true);
    chkEnableBillAcceptorDoorAlarm->setEnabled(true);

    generalSettingsLayout->itemAtPosition(ENABLE_CARD_READER_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(true);
    chkUseCardSwipe->setEnabled(true);

    generalSettingsLayout->itemAtPosition(ENABLE_DEMO_MODE_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(true);
    chkDemoMode->setEnabled(true);

    generalSettingsLayout->itemAtPosition(ENABLE_KEYPAD_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(true);
    chkEnableOnscreenKeypad->setEnabled(true);

    generalSettingsLayout->itemAtPosition(SHOW_KEYPAD_HELP_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(true);
    chkShowKeypadHelp->setEnabled(true);

    generalSettingsLayout->itemAtPosition(USE_CVS_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(true);
    chkUseCVS->setEnabled(true);

    generalSettingsLayout->itemAtPosition(ENABLE_COIN_SLOT_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(true);
    chkCoinSlot->setEnabled(true);

    generalSettingsLayout->itemAtPosition(STORE_NUM_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(true);
    spnStoreNum->setEnabled(true);

    generalSettingsLayout->itemAtPosition(AREA_NUM_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(true);
    spnAreaNum->setEnabled(true);

    generalSettingsLayout->itemAtPosition(ENABLE_METERS_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(true);
    chkEnableMeters->setEnabled(true);

    customCategoryWidget->setEnabled(true);
  }
  // else if Theaters selected then
  else if (deviceTree->currentItem()->data(0, Qt::UserRole).toString() == THEATERS_NODE_NAME)
  {
    containerWidget->setEnabled(true);

    // Disable: device address, device alias, free play, buddy booth, bill acceptor, bill acceptor alarm, card reader, demo mode, keypad, keypad help, use cvs, coin slot, store #, area #, meters
    generalSettingsLayout->itemAtPosition(DEVICE_ADDRESS_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(false);

    generalSettingsLayout->itemAtPosition(DEVICE_NAME_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(false);
    txtDeviceAlias->setEnabled(false);

    generalSettingsLayout->itemAtPosition(ENABLE_FREE_PLAY_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(false);
    chkEnableFreePlay->setEnabled(false);

    arcadeSettingsLayout->itemAtPosition(ENABLE_BUDDY_BOOTH_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(false);
    chkBuddyBooth->setEnabled(false);

    generalSettingsLayout->itemAtPosition(ENABLE_BILL_ACCEPTOR_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(false);
    chkEnableBillAcceptorDoor->setEnabled(false);

    generalSettingsLayout->itemAtPosition(ENABLE_BILL_ACCEPTOR_ALARM_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(false);
    chkEnableBillAcceptorDoorAlarm->setEnabled(false);

    generalSettingsLayout->itemAtPosition(ENABLE_CARD_READER_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(false);
    chkUseCardSwipe->setEnabled(false);

    generalSettingsLayout->itemAtPosition(ENABLE_DEMO_MODE_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(false);
    chkDemoMode->setEnabled(false);

    generalSettingsLayout->itemAtPosition(ENABLE_KEYPAD_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(false);
    chkEnableOnscreenKeypad->setEnabled(false);

    generalSettingsLayout->itemAtPosition(SHOW_KEYPAD_HELP_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(false);
    chkShowKeypadHelp->setEnabled(false);

    generalSettingsLayout->itemAtPosition(USE_CVS_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(false);
    chkUseCVS->setEnabled(false);

    generalSettingsLayout->itemAtPosition(ENABLE_COIN_SLOT_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(false);
    chkCoinSlot->setEnabled(false);

    generalSettingsLayout->itemAtPosition(STORE_NUM_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(false);
    spnStoreNum->setEnabled(false);

    generalSettingsLayout->itemAtPosition(AREA_NUM_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(false);
    spnAreaNum->setEnabled(false);

    generalSettingsLayout->itemAtPosition(ENABLE_METERS_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(false);
    chkEnableMeters->setEnabled(false);

    customCategoryWidget->setEnabled(false);

    // Enable: device type
    generalSettingsLayout->itemAtPosition(DEVICE_TYPE_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(true);
    cmbDeviceTypes->setEnabled(true);
  }
  // else if Arcade Booths selected then
  else if (deviceTree->currentItem()->data(0, Qt::UserRole).toString() == ARCADE_BOOTHS_NODE_NAME)
  {
    containerWidget->setEnabled(true);

    // Disabled: device address, device alias, store #, area #, meters
    generalSettingsLayout->itemAtPosition(DEVICE_ADDRESS_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(false);

    generalSettingsLayout->itemAtPosition(DEVICE_NAME_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(false);
    txtDeviceAlias->setEnabled(false);

    generalSettingsLayout->itemAtPosition(STORE_NUM_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(false);
    spnStoreNum->setEnabled(false);

    generalSettingsLayout->itemAtPosition(AREA_NUM_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(false);
    spnAreaNum->setEnabled(false);

    generalSettingsLayout->itemAtPosition(ENABLE_METERS_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(false);
    chkEnableMeters->setEnabled(false);

    customCategoryWidget->setEnabled(false);

    // Enabled: device type, bill acceptor, bill acceptor alarm, card reader, free play, demo mode, keypad, keypad help, use cvs, coin slot, buddy booth
    generalSettingsLayout->itemAtPosition(DEVICE_TYPE_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(true);
    cmbDeviceTypes->setEnabled(true);

    generalSettingsLayout->itemAtPosition(ENABLE_BILL_ACCEPTOR_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(true);
    chkEnableBillAcceptorDoor->setEnabled(true);

    generalSettingsLayout->itemAtPosition(ENABLE_BILL_ACCEPTOR_ALARM_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(true);
    chkEnableBillAcceptorDoorAlarm->setEnabled(true);

    generalSettingsLayout->itemAtPosition(ENABLE_CARD_READER_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(true);
    chkUseCardSwipe->setEnabled(true);

    generalSettingsLayout->itemAtPosition(ENABLE_FREE_PLAY_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(true);
    chkEnableFreePlay->setEnabled(true);

    generalSettingsLayout->itemAtPosition(ENABLE_DEMO_MODE_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(true);
    chkDemoMode->setEnabled(true);

    generalSettingsLayout->itemAtPosition(ENABLE_KEYPAD_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(true);
    chkEnableOnscreenKeypad->setEnabled(true);

    generalSettingsLayout->itemAtPosition(SHOW_KEYPAD_HELP_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(true);
    chkShowKeypadHelp->setEnabled(true);

    generalSettingsLayout->itemAtPosition(USE_CVS_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(true);
    chkUseCVS->setEnabled(true);

    generalSettingsLayout->itemAtPosition(ENABLE_COIN_SLOT_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(true);
    chkCoinSlot->setEnabled(true);

    arcadeSettingsLayout->itemAtPosition(ENABLE_BUDDY_BOOTH_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(true);
    chkBuddyBooth->setEnabled(true);
  }
  // else if Preview Booths selected then
  else if (deviceTree->currentItem()->data(0, Qt::UserRole).toString() == PREVIEW_BOOTHS_NODE_NAME)
  {
    containerWidget->setEnabled(true);

    // Disabled: device address, device alias, free play, store #, area #, meters, buddy booth
    generalSettingsLayout->itemAtPosition(DEVICE_ADDRESS_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(false);

    generalSettingsLayout->itemAtPosition(DEVICE_NAME_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(false);
    txtDeviceAlias->setEnabled(false);

    generalSettingsLayout->itemAtPosition(ENABLE_FREE_PLAY_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(false);
    chkEnableFreePlay->setEnabled(false);

    generalSettingsLayout->itemAtPosition(STORE_NUM_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(false);
    spnStoreNum->setEnabled(false);

    generalSettingsLayout->itemAtPosition(AREA_NUM_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(false);
    spnAreaNum->setEnabled(false);

    generalSettingsLayout->itemAtPosition(ENABLE_METERS_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(false);
    chkEnableMeters->setEnabled(false);

    arcadeSettingsLayout->itemAtPosition(ENABLE_BUDDY_BOOTH_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(false);
    chkBuddyBooth->setEnabled(false);

    customCategoryWidget->setEnabled(false);

    // Enabled: device type, bill acceptor, bill acceptor alarm, card reader, demo mode, keypad, keypad help, use cvs, coin slot
    generalSettingsLayout->itemAtPosition(DEVICE_TYPE_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(true);
    cmbDeviceTypes->setEnabled(true);

    generalSettingsLayout->itemAtPosition(ENABLE_BILL_ACCEPTOR_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(true);
    chkEnableBillAcceptorDoor->setEnabled(true);

    generalSettingsLayout->itemAtPosition(ENABLE_BILL_ACCEPTOR_ALARM_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(true);
    chkEnableBillAcceptorDoorAlarm->setEnabled(true);

    generalSettingsLayout->itemAtPosition(ENABLE_CARD_READER_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(true);
    chkUseCardSwipe->setEnabled(true);

    generalSettingsLayout->itemAtPosition(ENABLE_DEMO_MODE_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(true);
    chkDemoMode->setEnabled(true);

    generalSettingsLayout->itemAtPosition(ENABLE_KEYPAD_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(true);
    chkEnableOnscreenKeypad->setEnabled(true);

    generalSettingsLayout->itemAtPosition(SHOW_KEYPAD_HELP_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(true);
    chkShowKeypadHelp->setEnabled(true);

    generalSettingsLayout->itemAtPosition(USE_CVS_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(true);
    chkUseCVS->setEnabled(true);

    generalSettingsLayout->itemAtPosition(ENABLE_COIN_SLOT_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(true);
    chkCoinSlot->setEnabled(true);
  }
  // else if Clerk Stations selected then
  else if (deviceTree->currentItem()->data(0, Qt::UserRole).toString() == CLERKSTATIONS_NODE_NAME)
  {
    containerWidget->setEnabled(true);

    // Disabled: device address, device alias, store #, area #, meters, bill acceptor, bill acceptor alarm, card reader, free play, demo mode, keypad, keypad help, use cvs, coin slot, buddy booth
    generalSettingsLayout->itemAtPosition(DEVICE_ADDRESS_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(false);

    generalSettingsLayout->itemAtPosition(DEVICE_NAME_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(false);
    txtDeviceAlias->setEnabled(false);

    generalSettingsLayout->itemAtPosition(STORE_NUM_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(false);
    spnStoreNum->setEnabled(false);

    generalSettingsLayout->itemAtPosition(AREA_NUM_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(false);
    spnAreaNum->setEnabled(false);

    generalSettingsLayout->itemAtPosition(ENABLE_METERS_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(false);
    chkEnableMeters->setEnabled(false);

    customCategoryWidget->setEnabled(false);

    // Enabled: device type
    generalSettingsLayout->itemAtPosition(DEVICE_TYPE_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(true);
    cmbDeviceTypes->setEnabled(true);

    generalSettingsLayout->itemAtPosition(ENABLE_BILL_ACCEPTOR_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(false);
    chkEnableBillAcceptorDoor->setEnabled(false);

    generalSettingsLayout->itemAtPosition(ENABLE_BILL_ACCEPTOR_ALARM_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(false);
    chkEnableBillAcceptorDoorAlarm->setEnabled(false);

    generalSettingsLayout->itemAtPosition(ENABLE_CARD_READER_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(false);
    chkUseCardSwipe->setEnabled(false);

    generalSettingsLayout->itemAtPosition(ENABLE_FREE_PLAY_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(false);
    chkEnableFreePlay->setEnabled(false);

    generalSettingsLayout->itemAtPosition(ENABLE_DEMO_MODE_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(false);
    chkDemoMode->setEnabled(false);

    generalSettingsLayout->itemAtPosition(ENABLE_KEYPAD_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(false);
    chkEnableOnscreenKeypad->setEnabled(false);

    generalSettingsLayout->itemAtPosition(SHOW_KEYPAD_HELP_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(false);
    chkShowKeypadHelp->setEnabled(false);

    generalSettingsLayout->itemAtPosition(USE_CVS_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(false);
    chkUseCVS->setEnabled(false);

    generalSettingsLayout->itemAtPosition(ENABLE_COIN_SLOT_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(false);
    chkCoinSlot->setEnabled(false);

    arcadeSettingsLayout->itemAtPosition(ENABLE_BUDDY_BOOTH_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(false);
    chkBuddyBooth->setEnabled(false);
  }
  // else if individual device selected then
  else if (deviceTree->currentItem()->data(0, Qt::UserRole).toString().toInt() > 0)
  {
    containerWidget->setEnabled(true);
    customCategoryWidget->setEnabled(false);

    BoothSettings booth;

    if (booth.load(QString("%1/casplayer_%2.sqlite").arg(settings->getValue(DATA_PATH).toString()).arg(deviceTree->currentItem()->data(0, Qt::UserRole).toString())))
    {
      switch (booth.DeviceType())
      {
        case Settings::Theater:
          // Disable: bill acceptor, bill acceptor alarm, card reader, free play, demo mode, keypad, keypad help, use cvs, coin slot, store #, area #, meters, buddy booth
          generalSettingsLayout->itemAtPosition(ENABLE_BILL_ACCEPTOR_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(false);
          chkEnableBillAcceptorDoor->setEnabled(false);

          generalSettingsLayout->itemAtPosition(ENABLE_BILL_ACCEPTOR_ALARM_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(false);
          chkEnableBillAcceptorDoorAlarm->setEnabled(false);

          generalSettingsLayout->itemAtPosition(ENABLE_CARD_READER_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(false);
          chkUseCardSwipe->setEnabled(false);

          generalSettingsLayout->itemAtPosition(ENABLE_FREE_PLAY_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(false);
          chkEnableFreePlay->setEnabled(false);

          generalSettingsLayout->itemAtPosition(ENABLE_DEMO_MODE_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(false);
          chkDemoMode->setEnabled(false);

          generalSettingsLayout->itemAtPosition(ENABLE_KEYPAD_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(false);
          chkEnableOnscreenKeypad->setEnabled(false);

          generalSettingsLayout->itemAtPosition(SHOW_KEYPAD_HELP_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(false);
          chkShowKeypadHelp->setEnabled(false);

          generalSettingsLayout->itemAtPosition(USE_CVS_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(false);
          chkUseCVS->setEnabled(false);

          generalSettingsLayout->itemAtPosition(ENABLE_COIN_SLOT_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(false);
          chkCoinSlot->setEnabled(false);

          generalSettingsLayout->itemAtPosition(STORE_NUM_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(false);
          spnStoreNum->setEnabled(false);

          generalSettingsLayout->itemAtPosition(AREA_NUM_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(false);
          spnAreaNum->setEnabled(false);

          generalSettingsLayout->itemAtPosition(ENABLE_METERS_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(false);
          chkEnableMeters->setEnabled(false);

          arcadeSettingsLayout->itemAtPosition(ENABLE_BUDDY_BOOTH_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(false);
          chkBuddyBooth->setEnabled(false);

          // Enable: device address, device alias, device type
          generalSettingsLayout->itemAtPosition(DEVICE_ADDRESS_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(true);

          generalSettingsLayout->itemAtPosition(DEVICE_NAME_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(true);
          txtDeviceAlias->setEnabled(true);

          generalSettingsLayout->itemAtPosition(DEVICE_TYPE_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(true);
          cmbDeviceTypes->setEnabled(true);

          break;

        case Settings::Arcade:
          // Disabled: store #, area #, meters
          generalSettingsLayout->itemAtPosition(STORE_NUM_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(false);
          spnStoreNum->setEnabled(false);

          generalSettingsLayout->itemAtPosition(AREA_NUM_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(false);
          spnAreaNum->setEnabled(false);

          generalSettingsLayout->itemAtPosition(ENABLE_METERS_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(false);
          chkEnableMeters->setEnabled(false);

          // Enabled: device address, device alias, device type, bill acceptor, bill acceptor alarm, card reader, free play, demo mode, keypad, keypad help, use cvs, coin slot, buddy booth
          generalSettingsLayout->itemAtPosition(DEVICE_ADDRESS_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(true);

          generalSettingsLayout->itemAtPosition(DEVICE_NAME_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(true);
          txtDeviceAlias->setEnabled(true);

          generalSettingsLayout->itemAtPosition(DEVICE_TYPE_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(true);
          cmbDeviceTypes->setEnabled(true);

          generalSettingsLayout->itemAtPosition(ENABLE_BILL_ACCEPTOR_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(true);
          chkEnableBillAcceptorDoor->setEnabled(true);

          generalSettingsLayout->itemAtPosition(ENABLE_BILL_ACCEPTOR_ALARM_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(true);
          chkEnableBillAcceptorDoorAlarm->setEnabled(true);

          generalSettingsLayout->itemAtPosition(ENABLE_CARD_READER_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(true);
          chkUseCardSwipe->setEnabled(true);

          generalSettingsLayout->itemAtPosition(ENABLE_FREE_PLAY_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(true);
          chkEnableFreePlay->setEnabled(true);

          generalSettingsLayout->itemAtPosition(ENABLE_DEMO_MODE_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(true);
          chkDemoMode->setEnabled(true);

          generalSettingsLayout->itemAtPosition(ENABLE_KEYPAD_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(true);
          chkEnableOnscreenKeypad->setEnabled(true);

          generalSettingsLayout->itemAtPosition(SHOW_KEYPAD_HELP_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(true);
          chkShowKeypadHelp->setEnabled(true);

          generalSettingsLayout->itemAtPosition(USE_CVS_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(true);
          chkUseCVS->setEnabled(true);

          generalSettingsLayout->itemAtPosition(ENABLE_COIN_SLOT_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(true);
          chkCoinSlot->setEnabled(true);

          arcadeSettingsLayout->itemAtPosition(ENABLE_BUDDY_BOOTH_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(true);
          chkBuddyBooth->setEnabled(true);

          break;

        case Settings::Preview:
          // Disabled: free play, store #, area #, meters, buddy booth
          generalSettingsLayout->itemAtPosition(ENABLE_FREE_PLAY_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(false);
          chkEnableFreePlay->setEnabled(false);

          generalSettingsLayout->itemAtPosition(STORE_NUM_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(false);
          spnStoreNum->setEnabled(false);

          generalSettingsLayout->itemAtPosition(AREA_NUM_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(false);
          spnAreaNum->setEnabled(false);

          generalSettingsLayout->itemAtPosition(ENABLE_METERS_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(false);
          chkEnableMeters->setEnabled(false);

          arcadeSettingsLayout->itemAtPosition(ENABLE_BUDDY_BOOTH_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(false);
          chkBuddyBooth->setEnabled(false);

          // Enabled: device address, device alias, device type, bill acceptor, bill acceptor alarm, card reader, demo mode, keypad, keypad help, use cvs, coin slot
          generalSettingsLayout->itemAtPosition(DEVICE_ADDRESS_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(true);

          generalSettingsLayout->itemAtPosition(DEVICE_NAME_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(true);
          txtDeviceAlias->setEnabled(true);

          generalSettingsLayout->itemAtPosition(DEVICE_TYPE_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(true);
          cmbDeviceTypes->setEnabled(true);

          generalSettingsLayout->itemAtPosition(ENABLE_BILL_ACCEPTOR_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(true);
          chkEnableBillAcceptorDoor->setEnabled(true);

          generalSettingsLayout->itemAtPosition(ENABLE_BILL_ACCEPTOR_ALARM_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(true);
          chkEnableBillAcceptorDoorAlarm->setEnabled(true);

          generalSettingsLayout->itemAtPosition(ENABLE_CARD_READER_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(true);
          chkUseCardSwipe->setEnabled(true);

          generalSettingsLayout->itemAtPosition(ENABLE_DEMO_MODE_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(true);
          chkDemoMode->setEnabled(true);

          generalSettingsLayout->itemAtPosition(ENABLE_KEYPAD_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(true);
          chkEnableOnscreenKeypad->setEnabled(true);

          generalSettingsLayout->itemAtPosition(SHOW_KEYPAD_HELP_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(true);
          chkShowKeypadHelp->setEnabled(true);

          generalSettingsLayout->itemAtPosition(USE_CVS_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(true);
          chkUseCVS->setEnabled(true);

          generalSettingsLayout->itemAtPosition(ENABLE_COIN_SLOT_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(true);
          chkCoinSlot->setEnabled(true);

          break;

        case Settings::ClerkStation:
          // Disabled: store #, area #, meters, bill acceptor, bill acceptor alarm, card reader, free play, demo mode, keypad, keypad help, use cvs, coin slot, buddy booth
          generalSettingsLayout->itemAtPosition(STORE_NUM_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(false);
          spnStoreNum->setEnabled(false);

          generalSettingsLayout->itemAtPosition(AREA_NUM_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(false);
          spnAreaNum->setEnabled(false);

          generalSettingsLayout->itemAtPosition(ENABLE_METERS_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(false);
          chkEnableMeters->setEnabled(false);

          // Enabled: device address, device alias, device type
          generalSettingsLayout->itemAtPosition(DEVICE_ADDRESS_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(true);

          generalSettingsLayout->itemAtPosition(DEVICE_NAME_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(true);
          txtDeviceAlias->setEnabled(true);

          generalSettingsLayout->itemAtPosition(DEVICE_TYPE_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(true);
          cmbDeviceTypes->setEnabled(true);

          generalSettingsLayout->itemAtPosition(ENABLE_BILL_ACCEPTOR_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(false);
          chkEnableBillAcceptorDoor->setEnabled(false);

          generalSettingsLayout->itemAtPosition(ENABLE_BILL_ACCEPTOR_ALARM_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(false);
          chkEnableBillAcceptorDoorAlarm->setEnabled(false);

          generalSettingsLayout->itemAtPosition(ENABLE_CARD_READER_ROW, SETTINGS_COLUMN_0)->widget()->setEnabled(false);
          chkUseCardSwipe->setEnabled(false);

          generalSettingsLayout->itemAtPosition(ENABLE_FREE_PLAY_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(false);
          chkEnableFreePlay->setEnabled(false);

          generalSettingsLayout->itemAtPosition(ENABLE_DEMO_MODE_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(false);
          chkDemoMode->setEnabled(false);

          generalSettingsLayout->itemAtPosition(ENABLE_KEYPAD_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(false);
          chkEnableOnscreenKeypad->setEnabled(false);

          generalSettingsLayout->itemAtPosition(SHOW_KEYPAD_HELP_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(false);
          chkShowKeypadHelp->setEnabled(false);

          generalSettingsLayout->itemAtPosition(USE_CVS_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(false);
          chkUseCVS->setEnabled(false);

          generalSettingsLayout->itemAtPosition(ENABLE_COIN_SLOT_ROW, SETTINGS_COLUMN_6)->widget()->setEnabled(false);
          chkCoinSlot->setEnabled(false);

          arcadeSettingsLayout->itemAtPosition(ENABLE_BUDDY_BOOTH_ROW, SETTINGS_COLUMN_3)->widget()->setEnabled(false);
          chkBuddyBooth->setEnabled(false);

          break;

        default:
          //     Unknown device selected
          //     Disable all widgets
          QLOG_ERROR() << QString("Unknown device type: %1, cannot update field states").arg(booth.DeviceType());
          containerWidget->setEnabled(false);
          break;
      }
    }
    else
    {
      // error could not load data for selected node, disable all widgets
      QLOG_ERROR() << QString("Could not load device settings for updating field states. Database: %1").arg(QString("%1/casplayer_%2.sqlite").arg(settings->getValue(DATA_PATH).toString()).arg(deviceTree->currentItem()->data(0, Qt::UserRole).toString()));
      containerWidget->setEnabled(false);
    }
  }
  else
  {
    //   Nothing selected, disable all widgets
    containerWidget->setEnabled(false);
  }
}

void BoothSettingsWidget::onEditShiftsClicked()
{
  StoreShiftsWidget w(settings);
  w.populateForm(storeShifts);

  if (w.exec())
  {
    storeShifts = w.getShifts();

    clearShiftsTable();

    foreach (StoreShift shift, storeShifts)
    {
      insertShiftRow(shift);
    }

    QLOG_DEBUG() << "Saving store shifts to all booths...";

    // FIXME: If there is an invalid setting in the device settings section then it stops this from being saved. Ideally just the shifts should be saved to each booth database
    updateAllBooths();
  }
}

void BoothSettingsWidget::onAddUserClicked()
{
  UserAccountWidget w(settings);

  if (w.exec())
  {
    UserAccount userAccount = w.getUserAccount();

    QLOG_DEBUG() << "User submitted new user account:" << userAccount.getUsername();

    if (usersModel->findItems(userAccount.getUsername(), Qt::MatchExactly, Username).length() > 0)
    {
      QLOG_DEBUG() << "User submitted a user account that already exists";
      QMessageBox::warning(this, tr("User Account"), tr("Username already exists, specify a different username."));
    }
    else
    {
      dbMgr->addUser(userAccount);
    }
  }
  else
  {
    QLOG_DEBUG() << "User canceled adding new user account";
  }
}

void BoothSettingsWidget::onDeleteUserClicked()
{
  // get current rev id of selected username
  // delete user

  QLOG_DEBUG() << "User clicked Delete User button";

  QModelIndexList selectedRows = usersTable->selectionModel()->selectedRows();

  if (selectedRows.count() == 0)
  {
    QLOG_DEBUG() << "No row selected to delete";
    QMessageBox::warning(this, tr("Delete User"), tr("Select a username from the table above to delete."));
  }
  else
  {
    if (QMessageBox::question(this, tr("Delete User"), tr("Are you sure you want to delete user: '%1'?").arg(selectedRows.at(0).data().toString()), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
    {
      QModelIndex index = selectedRows.at(0);
      QModelIndex revField = usersModel->index(index.row(), RevID);

      UserAccount userAccount;
      userAccount.setUsername(index.data().toString());
      userAccount.setRevID(revField.data().toString());

      dbMgr->deleteUser(userAccount);

      usersModel->removeRow(index.row());
    }
    else
    {
      QLOG_DEBUG() << "User canceled deleting user account";
    }
  }
}

void BoothSettingsWidget::onChangePasswordClicked()
{
  // get current rev id of selected username
  // update user
  UserAccountWidget w(settings);

  QModelIndexList selectedRows = usersTable->selectionModel()->selectedRows();

  if (selectedRows.count() == 0)
  {
    QLOG_DEBUG() << "No row selected to edit";
    QMessageBox::warning(this, tr("Change Password"), tr("Select a username from the table above to change the password."));
  }
  else
  {
    QModelIndex index = selectedRows.at(0);
    QModelIndex revField = usersModel->index(index.row(), RevID);

    UserAccount selectedUserAccount;
    selectedUserAccount.setUsername(index.data().toString());
    selectedUserAccount.setRevID(revField.data().toString());

    w.setEditMode(UserAccountWidget::UPDATE);
    w.populateForm(selectedUserAccount);

    if (w.exec())
    {
      UserAccount changedUserAccount = w.getUserAccount();

      QLOG_DEBUG() << "User changed password for user account:" << selectedUserAccount.getUsername();

      changedUserAccount.setPassword(changedUserAccount.getPassword());

      dbMgr->updateUser(changedUserAccount);
    }
    else
    {
      QLOG_DEBUG() << "User canceled changing user account password";
    }
  }
}

bool BoothSettingsWidget::boothSettingsChanged(QString deviceAddress)
{
  // Compare form fields to database

  BoothSettings booth;

  if (booth.load(QString("%1/casplayer_%2.sqlite").arg(settings->getValue(DATA_PATH).toString()).arg(deviceAddress)))
  {
    if (booth.DeviceType() != cmbDeviceTypes->itemData(cmbDeviceTypes->currentIndex()).toInt())
      return true;

    if (booth.UiType() != cmbArcadeUiType->itemData(cmbArcadeUiType->currentIndex()).toInt())
      return true;

    if (booth.BillAcceptorInterface() != cmbBillAcceptorInterface->itemData(cmbBillAcceptorInterface->currentIndex()).toInt())
      return true;

    /*
    if (booth.PulsesPerDollar() != cmbPulsesPerDollar->itemData(cmbPulsesPerDollar->currentIndex()).toInt())
      return true;
    */

    txtDeviceAlias->setText(txtDeviceAlias->text().trimmed());
    if (booth.DeviceAlias() != txtDeviceAlias->text())
      return true;

    /*
    QString selectedCategoryFilter = cmbCategoryFilter->itemData(cmbCategoryFilter->currentIndex()).toString();
    if (selectedCategoryFilter == "None")
      selectedCategoryFilter.clear();

    if (booth.CategoryFilter() != selectedCategoryFilter)
      return true;
    */
    //txtDeviceAddress->text() booth.DeviceAddress()

    /*
    if (booth.StreamingPreview() != chkEnableStreamingPreview->isChecked())
      return true;
    */

    if (spnStoreNum->value() != booth.StoreNum())
      return true;

    if (spnAreaNum->value() != booth.AreaNum())
      return true;

    if (chkUseCVS->isChecked() != booth.UseCVS())
      return true;

    if (chkBuddyBooth->isChecked() != booth.BuddyBooth())
      return true;

    // Only look at buddy address if buddy booth is enabled
    if (chkBuddyBooth->isChecked())
    {
      if (spnBuddyAddress->value() != booth.BuddyAddress())
        return true;
    }

    if (chkUseCardSwipe->isChecked() != booth.EnableCardSwipe())
      return true;

    if (chkEnableOnscreenKeypad->isChecked() != booth.EnableOnscreenKeypad())
      return true;

    if (chkShowKeypadHelp->isChecked() != booth.ShowKeyPadInfo())
      return true;

    if (chkAllowCreditCards->isChecked() != booth.AllowCreditCards())
      return true;

    if (spnCreditChargeAmount->value() != booth.CreditChargeAmount())
      return true;

    if (chkCashOnly->isChecked() != booth.CashOnly())
      return true;

    if (chkCoinSlot->isChecked() != booth.CoinSlot())
      return true;

    if (spnPreviewCost->value() != booth.PreviewCost())
      return true;

    if (spnThreeVideoPreviewCost->value() != booth.ThreeVideoPreviewCost())
      return true;

    if (spnFiveVideoPreviewCost->value() != booth.FiveVideoPreviewCost())
      return true;

    if (spnSecondsPerCredit->value() != booth.SecondsPerCredit())
      return true;

    //if (spnPreviewSessionMinutes->value() != booth.PreviewSessionMinutes())
    //  return true;

    if (spnVideoBrowsingMinutes->value() != booth.VideoBrowsingInactivityMinutes())
      return true;

    if (spnAdditionalPreviewMinutesCost->value() != booth.AdditionalPreviewMinutesCost())
      return true;

    if (spnAdditionalPreviewMinutes->value() != booth.AdditionalPreviewMinutes())
      return true;

    if (chkAllowAdditionalPreviewMinutesSinglePreview->isChecked() != booth.AllowAdditionalPreviewMinutesSinglePreview())
      return true;

    if (spnMaxTimePreviewCardProgram->value() != booth.MaxTimePreviewCardProgram())
      return true;

    if (spnLowCreditThreshold->value() != booth.LowCreditThreshold())
      return true;

    if (spnLowPreviewMinuteThreshold->value() != booth.LowPreviewMinuteThreshold())
      return true;

    if (chkCcDevelopmentMode->isChecked() != booth.CcDevelopmentMode())
      return true;

    if (chkDemoMode->isChecked() != booth.DemoMode())
      return true;

    if (chkEnableBillAcceptorDoor->isChecked() != booth.EnableBillAcceptor())
      return true;

    if (chkEnableMeters->isChecked() != booth.EnableMeters())
      return true;

    if (chkEnableNrMeters->isChecked() != booth.EnableNrMeters())
      return true;

    if (chkEnableFreePlay->isChecked() != booth.EnableFreePlay())
      return true;

    if (settings->getValue(USE_SHARE_CARDS).toBool() != booth.SharingCards())
      return true;

    if (spnFreePlayInactivity->value() != booth.FreePlayInactivityTimeout())
      return true;

    if (spnResumeFreePlaySession->value() != booth.ResumeFreePlaySessionTimeout())
      return true;

    if (chkEnableBillAcceptorDoorAlarm->isChecked() != booth.EnableBillAcceptorDoorAlarm())
      return true;

    // Because of needing to be backwards compatible with CAS I, convert Transsexual -> She-Male for the CASplayer and She-Male -> Transsexual for CAS Mgr
    // I can't even remember why at this point...
    QString theaterCategory = cmbTheaterCategory->currentText();
    if (theaterCategory == "Transsexual")
      theaterCategory = "She-Male";

    if (theaterCategory != booth.TheaterCategory())
      return true;

    if (booth.StoreName() != settings->getValue(STORE_NAME))
      return true;

//    QStringList producersList;
//    for (int i = 0; i < lstAssignedProducers->count(); ++i)
//    {
//      QListWidgetItem *item = lstAssignedProducers->item(i);
//      producersList.append(item->text());
//    }

//    QSet<QString> diff = producersList.toSet().subtract(booth.CustomMenuCategory().producers.toSet());

//    if (booth.CustomMenuCategory().name != txtCustomCategory->text() || diff.count() > 0)
//    {
//      return true;
//    }
  }

  return false;
}

void BoothSettingsWidget::saveBoothSettings(QString deviceAddress)
{
  QString selectedNodeData = deviceTree->currentItem()->data(0, Qt::UserRole).toString();

  // Save the values in the form to the booth's database currently selected in the tree view
  BoothSettings booth;

  if (booth.load(QString("%1/casplayer_%2.sqlite").arg(settings->getValue(DATA_PATH).toString()).arg(deviceAddress)))
  {
    QLOG_DEBUG() << QString("Updating settings in %1/casplayer_%2.sqlite").arg(settings->getValue(DATA_PATH).toString()).arg(deviceAddress);

    // Only update device type if the selected tree node is not All Devices
    if (selectedNodeData != ALL_DEVICES_NODE_NAME)
    {
      QLOG_DEBUG() << QString("Updating device type because current node is: %1").arg(selectedNodeData);

      // If the device type was changed then rebuild the device tree
      if (booth.DeviceType() != (BoothSettings::Device_Type)cmbDeviceTypes->itemData(cmbDeviceTypes->currentIndex()).toInt())
        reloadDeviceTree = true;

      booth.setDeviceType((BoothSettings::Device_Type)cmbDeviceTypes->itemData(cmbDeviceTypes->currentIndex()).toInt());
    }
    else
    {
      QLOG_DEBUG() << QString("Skipped updating device type because the current node is: %1").arg(selectedNodeData);
    }

    booth.setUiType((BoothSettings::UI_Type)cmbArcadeUiType->itemData(cmbArcadeUiType->currentIndex()).toInt());
    booth.setBillAcceptorInterface((BoothSettings::InterfaceType)cmbBillAcceptorInterface->itemData(cmbBillAcceptorInterface->currentIndex()).toInt());
    //booth.setPulsesPerDollar(cmbPulsesPerDollar->itemData(cmbPulsesPerDollar->currentIndex()).toInt());

    /*
    QString selectedCategoryFilter = cmbCategoryFilter->itemData(cmbCategoryFilter->currentIndex()).toString();
    if (selectedCategoryFilter == "None")
      selectedCategoryFilter.clear();

    booth.setCategoryFilter(selectedCategoryFilter);
    */

    // TODO: Allow device address to be changed
    //txtDeviceAddress->text() booth.DeviceAddress()

    // Only update if the selected tree node is not All Devices, Theaters, Arcade Booths, Clerk Stations or Preview Booths
    if (selectedNodeData.toInt() != 0)
    {
      QLOG_DEBUG() << QString("Updating device alias because current node is: %1").arg(selectedNodeData);

      // If the device alias was changed then rebuild the device tree
      if (booth.DeviceAlias() != txtDeviceAlias->text())
        reloadDeviceTree = true;

      booth.setDeviceAlias(txtDeviceAlias->text());
    }
    // Only update if current node is All Devices
    else if (selectedNodeData == ALL_DEVICES_NODE_NAME)
    {
      QLOG_DEBUG() << QString("Updating store, area, meters, NR meters settings because the current node is: %1").arg(selectedNodeData);

      booth.setStoreNum(spnStoreNum->value());
      booth.setAreaNum(spnAreaNum->value());
      booth.setEnableNrMeters(chkEnableNrMeters->isChecked());
      booth.setEnableMeters(chkEnableMeters->isChecked());

      QStringList producersList;
      for (int i = 0; i < lstAssignedProducers->count(); ++i)
      {
        QListWidgetItem *item = lstAssignedProducers->item(i);
        producersList.append(item->text());
      }

      booth.setCustomMenuCategory({txtCustomCategory->text(), producersList});
    }
    else
    {
      QLOG_DEBUG() << QString("Skipped updating device alias, store, area, meters, NR meters settings because the current node is: %1").arg(selectedNodeData);
    }

    //booth.setStreamingPreview(chkEnableStreamingPreview->isChecked());

    booth.setUseCVS(chkUseCVS->isChecked());

    // Only update if the selected tree node is not All Devices, Theaters, Arcade Booths, Clerk Stations or Preview Booths
    if (selectedNodeData.toInt() != 0)
    {
      booth.setBuddyBooth(chkBuddyBooth->isChecked());

      // Only update buddy address if buddy booth is enabled
      if (chkBuddyBooth->isChecked())
      {
        booth.setBuddyAddress(spnBuddyAddress->value());
      }
    }
    else
    {
      QLOG_DEBUG() << QString("Skipped updating buddy booth settings because the current node is: %1").arg(selectedNodeData);
    }

    booth.setEnableCardSwipe(chkUseCardSwipe->isChecked());
    booth.setEnableOnscreenKeypad(chkEnableOnscreenKeypad->isChecked());
    booth.setShowKeyPadInfo(chkShowKeypadHelp->isChecked());
    booth.setAllowCreditCards(chkAllowCreditCards->isChecked());
    booth.setCreditChargeAmount(spnCreditChargeAmount->value());
    booth.setCashOnly(chkCashOnly->isChecked());
    booth.setCoinSlot(chkCoinSlot->isChecked());
    booth.setPreviewCost(spnPreviewCost->value());
    booth.setThreeVideoPreviewCost(spnThreeVideoPreviewCost->value());
    booth.setFiveVideoPreviewCost(spnFiveVideoPreviewCost->value());
    booth.setSecondsPerCredit(spnSecondsPerCredit->value());
    //booth.setPreviewSessionMinutes(spnPreviewSessionMinutes->value());
    booth.setVideoBrowsingInactivityMinutes(spnVideoBrowsingMinutes->value());
    booth.setAdditionalPreviewMinutesCost(spnAdditionalPreviewMinutesCost->value());
    booth.setAdditionalPreviewMinutes(spnAdditionalPreviewMinutes->value());
    booth.setAllowAdditionalPreviewMinutesSinglePreview(chkAllowAdditionalPreviewMinutesSinglePreview->isChecked());
    booth.setMaxTimePreviewCardProgram(spnMaxTimePreviewCardProgram->value());
    booth.setLowCreditThreshold(spnLowCreditThreshold->value());
    booth.setLowPreviewMinuteThreshold(spnLowPreviewMinuteThreshold->value());
    booth.setCcDevelopmentMode(chkCcDevelopmentMode->isChecked());
    booth.setDemoMode(chkDemoMode->isChecked());
    booth.setEnableBillAcceptor(chkEnableBillAcceptorDoor->isChecked());
    booth.setEnableFreePlay(chkEnableFreePlay->isChecked());
    booth.setFreePlayInactivityTimeout(spnFreePlayInactivity->value());
    booth.setResumeFreePlaySessionTimeout(spnResumeFreePlaySession->value());
    booth.setEnableBillAcceptorDoorAlarm(chkEnableBillAcceptorDoorAlarm->isChecked());
    booth.setCcClientID(ccClientID);
    booth.setCcMerchantAccountID(ccMerchantAccountID);
    booth.setAuthorizeNetMarketType(authorizeNetMarketType);
    booth.setAuthorizeNetDeviceType(authorizeNetDeviceType);

    // Because of needing to be backwards compatible with CAS I, convert Transsexual -> She-Male for the CASplayer and She-Male -> Transsexual for CAS Mgr
    // I can't even remember why at this point...
    QString theaterCategory = cmbTheaterCategory->currentText();
    if (theaterCategory == "Transsexual")
      theaterCategory = "She-Male";

    booth.setTheaterCategory(theaterCategory);

    booth.setShifts(storeShifts);

    booth.setStoreName(settings->getValue(STORE_NAME).toString());

    booth.setSharingCards(settings->getValue(USE_SHARE_CARDS).toBool());

    if (booth.save(QString("%1/casplayer_%2.sqlite").arg(settings->getValue(DATA_PATH).toString()).arg(deviceAddress)))
      QLOG_DEBUG() << QString("Finished updating settings in database");
    else
      QLOG_ERROR() << QString("Could not update settings in database");
  }
}

void BoothSettingsWidget::clearChildNodes()
{
  QTreeWidgetItem *root = deviceTree->topLevelItem(0);

  for (int i = 0; i < root->childCount(); ++i)
  {
    QTreeWidgetItem *parentNode = root->child(i);

    if (parentNode->childCount() > 0)
    {
      qDeleteAll(parentNode->takeChildren());
    }
  }

  qDeleteAll(root->takeChildren());
}

bool BoothSettingsWidget::validateInput(bool globalSettings)
{
  bool valid = true;
  QStringList errors;

  if (globalSettings)
  {
    if (spnPreviewCost->value() >= spnThreeVideoPreviewCost->value())
      errors.append(tr("- Preview Price must be less than the 3 Video Deluxe Package."));

    if (spnPreviewCost->value() >= spnFiveVideoPreviewCost->value())
      errors.append(tr("- Preview Price must be less than the 5 Video Deluxe Package."));

    if (spnThreeVideoPreviewCost->value() >= spnFiveVideoPreviewCost->value())
      errors.append(tr("- 3 Video Deluxe Package must be less than the 5 Video Deluxe Package."));
  }
  else
  {
    // If device type is Arcade then check if Buddy Booth is checked and that the Buddy Address differs than this booth
    if (BoothSettings::Arcade == (BoothSettings::Device_Type)cmbDeviceTypes->itemData(cmbDeviceTypes->currentIndex()).toInt())
    {
      if (chkBuddyBooth->isChecked() && spnBuddyAddress->value() == txtDeviceAddress->text().toInt())
        errors.append(tr("- The Buddy Address must be the address of the other booth, not this booth's address."));

      /*
      // Preview streaming cannot be enabled even though this is an Arcade, silently uncheck it if necessary
      if (chkEnableStreamingPreview->isChecked())
        chkEnableStreamingPreview->setChecked(false);
      */
    }
    // If device type is Preview then the Preview Cost must be < 3 Video Deluxe package and
    // 5 Video Deluxe package and 3 Video Deluxe package must be < 5 Video Deluxe package
    else if (BoothSettings::Preview == (BoothSettings::Device_Type)cmbDeviceTypes->itemData(cmbDeviceTypes->currentIndex()).toInt())
    {
      if (spnPreviewCost->value() >= spnThreeVideoPreviewCost->value())
        errors.append(tr("- Preview Price must be less than the 3 Video Deluxe Package."));

      if (spnPreviewCost->value() >= spnFiveVideoPreviewCost->value())
        errors.append(tr("- Preview Price must be less than the 5 Video Deluxe Package."));

      if (spnThreeVideoPreviewCost->value() >= spnFiveVideoPreviewCost->value())
        errors.append(tr("- 3 Video Deluxe Package must be less than the 5 Video Deluxe Package."));
    }
  }

  // If Demo Mode checked then Use CVS must be unchecked
  if (chkDemoMode->isChecked() && chkUseCVS->isChecked())
    errors.append(tr("- Use CVS cannot be checked when Demo Mode is checked."));

  // If Allow Credit Cards checked then Use Card Swipe must also be checked
  if (chkAllowCreditCards->isChecked() && !chkUseCardSwipe->isChecked())
    errors.append(tr("- Allow Credit Card cannot be checked when Enable Card Swipe is unchecked."));

  // If Cash Only checked then Allow Credit Cards must be unchecked
  if (chkCashOnly->isChecked() && chkAllowCreditCards->isChecked())
    errors.append(tr("- Allow Credit Card cannot be checked when Cash Only is checked."));


  qreal fractPart, intPart;

  // Credit card charge amount must be divisible by $0.25
  fractPart = modf(spnCreditChargeAmount->value() * 4, &intPart);
  if (fractPart != 0)
    errors.append(tr("- CC Charge Amount must be rounded to the nearest quarter (divisible by $0.25)."));

  // Preview Cost must be divisible by $0.25
  fractPart = modf(spnPreviewCost->value() * 4, &intPart);
  if (fractPart != 0)
    errors.append(tr("- Preview Cost must be rounded to the nearest quarter (divisible by $0.25)."));

  // 3 Video Deluxe Package must be divisible by $0.25
  fractPart = modf(spnThreeVideoPreviewCost->value() * 4, &intPart);
  if (fractPart != 0)
    errors.append(tr("- 3 Video Deluxe Package must be rounded to the nearest quarter (divisible by $0.25)."));

  // 5 Video Deluxe Package must be divisible by $0.25
  fractPart = modf(spnFiveVideoPreviewCost->value() * 4, &intPart);
  if (fractPart != 0)
    errors.append(tr("- 5 Video Deluxe Package must be rounded to the nearest quarter (divisible by $0.25)."));

  // Extended Preview Price must be divisible by $0.25
  fractPart = modf(spnAdditionalPreviewMinutesCost->value() * 4, &intPart);
  if (fractPart != 0)
    errors.append(tr("- Extended Preview Price must be rounded to the nearest quarter (divisible by $0.25)."));

  txtCustomCategory->setText(txtCustomCategory->text().trimmed());
  if (!txtCustomCategory->text().isEmpty())
  {
    if (containsExtendedChars(txtCustomCategory->text()))
      errors.append(tr("- Custom Category Name cannot contain extended characters. If you cannot see any of these characters, try to clear the field and type the name instead of copying/pasting it."));
    else if (txtCustomCategory->text().contains("\""))
      errors.append("- Custom Category Name cannot contain the double-qoute (\") character.");
    else if (txtCustomCategory->text().contains("  "))
      errors.append("- Words in the Custom Category Name cannot be separated by more than one space.");
    else if (lstAssignedProducers->count() == 0)
      errors.append("- If specifying a Custom Category Name you must add one or more producers to the list. The custom category will include any movie by the producer(s) in the list.");
  }
  else
  {
    if (lstAssignedProducers->count() > 0)
      errors.append("- One or more producers are in the Custom Category list but the custom category name is blank. Specify a custom category name or remove the producers from the list.");
  }

  if (errors.count() > 0)
  {
    valid = false;
    QLOG_DEBUG() << tr("Invalid device settings: %1").arg(errors.join("\n"));

    QMessageBox::warning(this, tr("Invalid Settings"), tr("Please correct the following problems:\n\n%1").arg(errors.join("\n")));
  }

  return valid;
}

QString BoothSettingsWidget::getFirstDevice(QString treeNodeName)
{
  // If deviceType empty then just return first device address
  // else return first device address of specified device type
  // return empty string if nothing found
  QTreeWidgetItem *root = deviceTree->topLevelItem(0);
  QString deviceAddress = "";

  bool found = false;
  for (int i = 0; i < root->childCount() && !found; ++i)
  {
    if (root->child(i)->childCount() > 0)
    {
      if (treeNodeName.isEmpty() || (!treeNodeName.isEmpty() && root->child(i)->data(0, Qt::UserRole).toString() == treeNodeName))
      {
        // Last octet of IP address
        deviceAddress = root->child(i)->child(0)->data(0, Qt::UserRole).toString();
        found = true;
      }
    }
  }

  return deviceAddress;
}

QVariantList BoothSettingsWidget::getBillInterfaceTypes()
{
  QVariantList interfaceList;

  const QMetaObject &mo = BoothSettings::staticMetaObject;
  int index = mo.indexOfEnumerator("InterfaceType");
  QMetaEnum metaEnum = mo.enumerator(index);

  // Populate drop down menu with all keys in the InterfaceType enumerator
  for (int i = 0; i < metaEnum.keyCount(); i++)
  {
    if (metaEnum.key(i))
    {
      QVariantMap keyValue;
      keyValue[QString(metaEnum.key(i))] = metaEnum.value(i);
      interfaceList.append(keyValue);
    }
  }

  return interfaceList;
}

QVariantList BoothSettingsWidget::getUiTypes()
{
  QVariantList uiTypeList;

  const QMetaObject &mo = Settings::staticMetaObject;
  int index = mo.indexOfEnumerator("UI_Type");
  QMetaEnum metaEnum = mo.enumerator(index);

  // Populate drop down menu with all keys in the UI_Type enumerator
  for (int i = 0; i < metaEnum.keyCount(); i++)
  {
    if (metaEnum.key(i))
    {
      QVariantMap keyValue;
      keyValue[QString(metaEnum.key(i))] = metaEnum.value(i);
      uiTypeList.append(keyValue);
    }
  }

  return uiTypeList;
}

/*QVariantList BoothSettingsWidget::getCategoryFilters()
{
  QVariantList categoryFilterList;

  QVariantMap keyValue;
  keyValue["None"] = "None";
  categoryFilterList.append(keyValue);

  keyValue.clear();
  keyValue["Gay"] = "Gay";
  categoryFilterList.append(keyValue);

  keyValue.clear();
  keyValue["Straight"] = "Straight";
  categoryFilterList.append(keyValue);

  return categoryFilterList;
}*/
