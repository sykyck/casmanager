#ifndef BOOTHSETTINGS_H
#define BOOTHSETTINGS_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QMetaEnum>
#include "storeshift.h"

struct CustomCategory {
  QString name;
  QStringList producers;
};

class BoothSettings : public QObject
{
  Q_OBJECT
public:
  enum Device_Type
  {
    Arcade = 1,
    Preview,
    Theater,
    ClerkStation
  };

  enum UI_Type
  {
    Simple = 1,
    Categories,
    Full
  };

  enum InterfaceType
  {
    PULSE = 1,
    RS232
  };

  Q_ENUMS(InterfaceType)
  Q_ENUMS(Device_Type)
  Q_ENUMS(UI_Type)

  explicit BoothSettings(QObject *parent = 0);
  bool load(QString dbFile);
  bool save(QString dbFile);
  int DeviceAddress();
  QString DeviceAlias() const;
  BoothSettings::Device_Type DeviceType();
  QString DeviceTypeToString();
  qreal CreditChargeAmount();
  int SecondsPerCredit();
  int LowCreditThreshold();  
  int StoreNum();
  int AreaNum();
  bool UseCVS();
  bool AllowCreditCards();  
  qreal PreviewCost();
  qreal ThreeVideoPreviewCost();
  qreal FiveVideoPreviewCost();  
  int PreviewSessionMinutes();
  int VideoBrowsingInactivityMinutes();  
  int LowPreviewMinuteThreshold();  
  int AdditionalPreviewMinutes();
  bool AllowAdditionalPreviewMinutesSinglePreview();
  qreal AdditionalPreviewMinutesCost();
  int MaxTimePreviewCardProgram();  
  bool CcDevelopmentMode();
  bool DemoMode();
  BoothSettings::InterfaceType BillAcceptorInterface();
  QString BillAcceptorInterfaceToString();
  bool BuddyBooth();
  int BuddyAddress();
  QString BuddyAddressToIP();
  bool EnableBillAcceptor();
  bool EnableCardSwipe();
  BoothSettings::UI_Type UiType();
  QString UiTypeToString();
  //bool StreamingPreview();
  bool CashOnly() const;
  bool CoinSlot() const;
  bool ShowKeyPadInfo() const;
  //QString CategoryFilter() const;
  //int PulsesPerDollar() const;
  bool EnableNrMeters() const;
  bool EnableMeters() const;
  bool EnableOnscreenKeypad() const;

  void setDeviceAddress(int value);
  void setDeviceType(const Device_Type &value);
  void setCreditChargeAmount(const qreal &value);
  void setSecondsPerCredit(int value);
  void setLowCreditThreshold(int value);
  void setStoreNum(int value);
  void setAreaNum(int value);
  void setUseCVS(bool value);
  void setAllowCreditCards(bool value);
  void setPreviewCost(const qreal &value);
  void setThreeVideoPreviewCost(const qreal &value);
  void setFiveVideoPreviewCost(const qreal &value);
  //void setPreviewSessionMinutes(int value);
  void setVideoBrowsingInactivityMinutes(int value);
  void setLowPreviewMinuteThreshold(int value);
  void setAdditionalPreviewMinutes(int value);
  void setAllowAdditionalPreviewMinutesSinglePreview(bool value);
  void setAdditionalPreviewMinutesCost(const qreal &value);
  void setMaxTimePreviewCardProgram(int value);
  void setCcDevelopmentMode(bool value);
  void setDemoMode(bool value);
  void setBillAcceptorInterface(const BoothSettings::InterfaceType &value);
  void setBuddyBooth(bool value);
  void setBuddyAddress(int value);
  void setEnableBillAcceptor(bool value);
  void setEnableCardSwipe(bool value);
  void setUiType(const UI_Type &value);
  //void setStreamingPreview(bool value);
  void setCashOnly(bool value);
  void setCoinSlot(bool value);
  void setShowKeyPadInfo(bool value);
  //void setCategoryFilter(const QString &value);
  void setDeviceAlias(const QString &value);
  //void setPulsesPerDollar(int value);
  void setEnableNrMeters(bool value);
  void setEnableMeters(bool value);
  void setEnableOnscreenKeypad(bool value);

  bool EnableFreePlay() const;
  void setEnableFreePlay(bool value);

  bool EnablePhysicalBuddyButton() const;
  void setEnablePhysicalBuddyButton(bool value);

  int ResumeFreePlaySessionTimeout() const;
  void setResumeFreePlaySessionTimeout(int value);

  int FreePlayInactivityTimeout() const;
  void setFreePlayInactivityTimeout(int value);

  bool EnableBillAcceptorDoorAlarm() const;
  void setEnableBillAcceptorDoorAlarm(bool value);

  QString CcClientID() const;
  void setCcClientID(const QString &value);

  QString CcMerchantAccountID() const;
  void setCcMerchantAccountID(const QString &value);

  QString TheaterCategory() const;
  void setTheaterCategory(const QString &value);

  QList<StoreShift> Shifts() const;
  void setShifts(const QList<StoreShift> &value);
  
  QString StoreName() const;
  void setStoreName(const QString &value);

  CustomCategory CustomMenuCategory() const;
  void setCustomMenuCategory(const CustomCategory &value);

  qint8 AuthorizeNetMarketType() const;
  void setAuthorizeNetMarketType(const qint8 &value);

  qint8 AuthorizeNetDeviceType() const;
  void setAuthorizeNetDeviceType(const qint8 &value);

  bool SharingCards() const;
  void setSharingCards(const bool value);

private:
  QString getValue(QSqlDatabase db, QString keyName, QVariant defaultValue, bool *ok);
  bool setValue(QSqlDatabase db, QString keyName, QVariant value);

  // Address of the device
  int deviceAddress;

  // User-friendly name of the device
  QString deviceAlias;

  // Type of the debice
  Device_Type deviceType;

  // Amount to charge a customer's credit/debit card
  qreal creditChargeAmount;

  // Number of seconds per credit
  int secondsPerCredit;

  // When session credits are <= this threshold then the customer
  // will see an alert on both screens
  int lowCreditThreshold;

  // Store number this device is located in
  int storeNum;

  // Area number this store is located in
  int areaNum;

  // Flag set when arcade cards should be looked up and programmed
  // using the old CVS network
  bool useCVS;

  // Flag set when credit/debit cards can be used
  bool allowCreditCards;

  // Cost of 1 preview
  qreal previewCost;

  // Cost of deluxe preview package with 3 videos
  qreal threeVideoPreviewCost;

  // Cost of deluxe preview package with 5 videos
  qreal fiveVideoPreviewCost;

  // Number of minutes in a preview session
  //int previewSessionMinutes;

  // Number of minutes for no activity while browsing videos when in preview mode
  int videoBrowsingInactivityMinutes;

  // When preview session minutes are <= this threshold then the customer
  // will see an alert on both screens
  int lowPreviewMinuteThreshold;

  // Number of minutes to add to preview session if user purchases additional time
  // If this value is zero then the feature is disabled
  int additionalPreviewMinutes;

  // Flag set when user is allowed to purchase additional preview time when in a single preview session
  bool allowAdditionalPreviewMinutesSinglePreview;

  // Cost of additional preview time. If this value is zero then the feature is disabled
  qreal additionalPreviewMinutesCost;

  // Maximum time in minutes that a preview card can be programmed from the last time it was used
  int maxTimePreviewCardProgram;

  // Flag set when credit card charging is in development mode, meaning development credentials and
  // test credit card number are used with the credit card processing service
  bool ccDevelopmentMode;

  // Flag set when device should operate independently without a network connection, useful for demonstrating hardware.
  // Software updates aren't checked, collections are saved locally and not sent to web service, credit card charges
  // are simulated (waits 5 seconds to simulate communication with credit card processor).
  // When the demo mode is enabled in the SettingsWindow class, it does the following:
  // - Server path is changed from /media/casserver to /media/cas
  // - /etc/init.d/S99-cas file is written without commands for syncing to a time server and connecting to the NFS share
  // - Creates a viewtimes.sqlite database in /media/cas with random Top 5 videos
  // - Sets 10 random Straight and Gay with a more recent date than the others to designate as Just In videos
  // When the demo is disabled in the SettingsWindow class it just reverts back to the default settings
  bool demoMode;

  // Type of interface to use with bill acceptor
  BoothSettings::InterfaceType billAcceptorInterface;

  // Flag set when booth is connected to a window that when electrified by 2 booths,
  // the window becomes transparent
  bool buddyBooth;

  // IP address of the buddy booth that this one is paired with
  int buddyAddress;

  // Flag set if a physical buddy button should be monitored
  bool enablePhysicalBuddyButton;

  // Flag set when bill acceptor should be used
  bool enableBillAcceptor;

  // Flag set when the switch on the bill acceptor door should be monitored
  bool enableBillAcceptorDoorAlarm;

  // Flag set when the card swipe should be used, when cleared the UI provides a keypad to enter card number
  bool enableCardSwipe;

  // Type of User Interface
  UI_Type uiType;

  // Flag set when streaming preview mode is enabled. This is only applicable if the device type is Preview
  //bool streamingPreview;

  // Flag set if booth only accepts cash
  bool cashOnly;

  // Flag set if booth has coin slot
  bool coinSlot;

  // Flag set if message should be shown on idle screen to "Touch to Enter Code" when the card swipe is disabled
  bool showKeyPadInfo;

  // If set then only videos matching category are loaded
  //QString categoryFilter;
  
  // When bill acceptor is set to Pulse mode, this is used
  // for determining how many pulses are sent from the bill
  // acceptor per dollar
  //int pulsesPerDollar;

  // Flag set when non-resettable meters should be used. We have a client that doesn't want NR meters
  bool enableNrMeters;

  // Flag set when meters should be used to track money, credits and credit cards. If this is cleared then enableNrMeters is irrelevant
  bool enableMeters;

  // Flag set when onscreen keypad should be used
  bool enableOnscreenKeypad;

  // Flag set when free play mode should be used
  bool enableFreePlay;

  //Flag whether we are using api service to update,add,delete or getCards
  bool isSharingCards;

  // Number of minutes the customer has to resume the free play session from the time it ends
  // This value is in minutes instead of a smaller unit so it doesn't have to be converted to minutes
  // when displaying in the message window
  int resumeFreePlaySessionTimeout;

  // Number of minutes of no interaction with the system from the customer before the
  // free play session ends
  int freePlayInactivityTimeout;

  // Client ID/API Login ID used for the credit card payment gateway
  QString ccClientID;

  // Merchant account ID/Transaction Key used for the credit card payment gateway
  QString ccMerchantAccountID;

  // Authorize.net x_market_type field
  qint8 authorizeNetMarketType;

  // Authorize.net x_device_type field
  qint8 authorizeNetDeviceType;

  // Category of movies to cycle through when device type is theater
  QString theaterCategory;

  // List of store shifts describing shift #, start and end times
  QList<StoreShift> shifts;

  QString storeName;

  CustomCategory customCategory;
};

#endif // BOOTHSETTINGS_H
