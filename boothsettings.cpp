#include <QFile>
#include <QTextStream>
#include <QDebug>
#include "qjson/parser.h"
#include "boothsettings.h"
#include "qslog/QsLog.h"
#include "qjson-backport/qjsondocument.h"
#include "qjson-backport/qjsonobject.h"
#include "qjson-backport/qjsonvalue.h"
#include "qjson-backport/qjsonarray.h"

const int DEFAULT_ALLOW_CREDIT_CARDS = 1;
const int DEFAULT_STORE_NUM = 0;
const int DEFAULT_AREA_NUM = 0;
const int DEFAULT_USE_CVS = 1;
const int DEFAULT_DEVICE_TYPE = 1; // Arcade
const QString DEFAULT_PREVIEW_COST = "7.0";
const QString DEFAULT_THREE_VIDEO_PREVIEW_COST = "12.0";
const QString DEFAULT_FIVE_VIDEO_PREVIEW_COST = "17.0";
const int DEFAULT_VIDEO_BROWSING_MINUTES = 15; // Max minutes of inactivity in preview mode (before video is chosen)
const int DEFAULT_LOW_PREVIEW_MINUTE_THRESHOLD = 10;
const int DEFAULT_ADDITIONAL_PREVIEW_MINUTES = 60;
const int DEFAULT_ALLOW_ADDITIONAL_MINUTES_SINGLE_PREVIEW = 0;
const QString DEFAULT_ADDITIONAL_PREVIEW_MINUTES_COST = "5.0";
const int DEFAULT_MAX_TIME_PREVIEW_CARD_PROGRAM = 15;
const int DEFAULT_CC_DEVELOPMENT_MODE = 0;
const int DEFAULT_DEMO_MODE = 0;
const int DEFAULT_BILL_ACCEPTOR_INTERFACE = 1; // 1 = Pulse interface
const int DEFAULT_BUDDY_BOOTH = 0;
const int DEFAULT_BUDDY_ADDRESS = 0;
const int DEFAULT_ENABLE_BILL_ACCEPTOR = 1;
const int DEFAULT_ENABLE_CARD_SWIPE = 1;
const int DEFAULT_UI_TYPE = 3; // Full UI
const int DEFAULT_STREAMING_PREVIEW = 0; // Disable streaming preview mode
const int DEFAULT_PREVIEW_SESSION_MINUTES = 120;
const int DEFAULT_CASH_ONLY = 0;
const int DEFAULT_COIN_SLOT = 0;
const int DEFAULT_SHOW_KEYPAD_INFO = 1;
const QString DEFAULT_CATEGORY_FILTER = "";
const int DEFAULT_PULSES_PER_DOLLAR = 1;
const int DEFAULT_ENABLE_PHYSICAL_BUDDY_BUTTON = 0;
const int DEFAULT_ENABLE_NR_METERS = 1;
const int DEFAULT_ENABLE_METERS = 1;
const int DEFAULT_ENABLE_ONSCREEN_KEYPAD = 1;
const int DEFAULT_ENABLE_FREE_PLAY = 0;
const int DEFAULT_RESUME_FREE_PLAY_SESSION_TIMEOUT = 5;
const int DEFAULT_FREE_PLAY_INACTIVITY_TIMEOUT = 20;
const int DEFAULT_ENABLE_BILL_ACCEPTOR_DOOR_ALARM = 1;
const QString DEFAULT_THEATER_CATEGORY = "All Titles";
const QString DEFAULT_SHIFT_TIMES = "{\"shifts\":[]}"; // JSON array of shift objects: {shift_num: number, start_time: string, end_time: string}
const QString DEFAULT_CUSTOM_CATEGORY = "{}";
const qint8 DEFAULT_AUTHORIZE_NET_MARKET_TYPE = 2; // 2 = Retail
const qint8 DEFAULT_AUTHORIZE_NET_DEVICE_TYPE = 0; // 0 = ignore field
const bool DEFAULT_USE_SHARE_CARDS = false;

const QString STORE_NUM = "store_num";
const QString STORE_AREA = "area_num";
const QString CREDIT_CHARGE_AMOUNT = "credit_charge_amount";
const QString ALLOW_CREDIT_CARDS = "allow_credit_cards";
const QString DEVICE_ADDRESS = "device_address";
const QString DEVICE_TYPE = "device_type";
const QString LOW_CREDIT_THRESHOLD = "low_credit_threshold";
const QString SECONDS_PER_CREDIT = "seconds_per_credit";
const QString USE_CVS = "use_cvs";
const QString PREVIEW_COST = "preview_cost";
const QString THREE_VIDEO_PREVIEW_COST = "three_video_preview_cost";
const QString FIVE_VIDEO_PREVIEW_COST = "five_video_preview_cost";
const QString VIDEO_BROWSING_MINUTES = "video_browsing_minutes";
const QString LOW_PREVIEW_MINUTE_THRESHOLD = "low_preview_minute_threshold";
const QString ADDITIONAL_PREVIEW_MINUTES = "additional_preview_minutes";
const QString ALLOW_ADDITIONAL_MINUTES_SINGLE_PREVIEW = "allow_additional_minutes_single_preview";
const QString ADDITIONAL_PREVIEW_MINUTES_COST = "additional_preview_minutes_cost";
const QString MAX_TIME_PREVIEW_CARD_PROGRAM = "max_time_preview_card_program";
const QString CC_DEVELOPMENT_MODE = "cc_development_mode";
const QString DEMO_MODE = "demo_mode";
const QString BILL_ACCEPTOR_INTERFACE = "bill_acceptor_interface";
const QString BUDDY_BOOTH = "buddy_booth";
const QString BUDDY_ADDRESS = "buddy_address";
const QString ENABLE_BILL_ACCEPTOR = "enable_bill_acceptor";
const QString ENABLE_CARD_SWIPE = "enable_card_swipe";
const QString UI_TYPE = "ui_type";
const QString STREAMING_PREVIEW = "streaming_preview";
const QString PREVIEW_SESSION_MINUTES = "preview_session_minutes";
const QString CASH_ONLY = "cash_only";
const QString COIN_SLOT = "coin_slot";
const QString SHOW_KEYPAD_INFO = "show_keypad_info";
const QString CATEGORY_FILTER = "category_filter";
const QString DEVICE_ALIAS = "device_alias";
const QString PULSES_PER_DOLLAR = "pulses_per_dollar";
const QString ENABLE_PHYSICAL_BUDDY_BUTTON = "enable_physical_buddy_button";
const QString ENABLE_NR_METERS = "enable_nr_meters";
const QString ENABLE_METERS = "enable_meters";
const QString ENABLE_ONSCREEN_KEYPAD = "enable_onscreen_keypad";
const QString ENABLE_FREE_PLAY = "enable_free_play";
const QString RESUME_FREE_PLAY_SESSION_TIMEOUT = "resume_free_play_session_timeout";
const QString FREE_PLAY_INACTIVITY_TIMEOUT = "free_play_inactivity_timeout";
const QString ENABLE_BILL_ACCEPTOR_DOOR_ALARM = "enable_bill_acceptor_door_alarm";
const QString CC_CLIENT_ID = "cc_client_id";
const QString CC_MERCHANT_ACCOUNT_ID = "cc_merchant_account_id";
const QString THEATER_CATEGORY = "theater_category";
const QString SHIFT_TIMES = "shift_times";
const QString STORE_NAME = "store_name";
const QString CUSTOM_CATEGORY = "custom_category";
const QString AUTHORIZE_NET_MARKET_TYPE = "authorize_net_market_type";
const QString AUTHORIZE_NET_DEVICE_TYPE = "authorize_net_device_type";
const QString USE_SHARE_CARDS = "use_share_cards";

BoothSettings::BoothSettings(QObject *parent) : QObject(parent)
{
  deviceAddress = 0;
  deviceType = BoothSettings::Arcade;
  creditChargeAmount = 0;
  secondsPerCredit = 0;  
  lowCreditThreshold = 0;  
  storeNum = 0;
  areaNum = 0;
  useCVS = false;
  allowCreditCards = false;
  previewCost = 0;
  videoBrowsingInactivityMinutes = 0;
  lowPreviewMinuteThreshold = 0;
  additionalPreviewMinutes = 0;
  allowAdditionalPreviewMinutesSinglePreview = false;
  additionalPreviewMinutesCost = 0;
  demoMode = false;
  billAcceptorInterface = BoothSettings::PULSE;
  buddyBooth = false;
  buddyAddress = 0;
  enablePhysicalBuddyButton = (DEFAULT_ENABLE_PHYSICAL_BUDDY_BUTTON == 1);
  uiType = BoothSettings::Full;
  cashOnly = false;
  coinSlot = false;
  showKeyPadInfo = true;
  //categoryFilter.clear();
  enableMeters = true;
  enableNrMeters = true;
  enableOnscreenKeypad = true;
  enableFreePlay = false;
  resumeFreePlaySessionTimeout = DEFAULT_RESUME_FREE_PLAY_SESSION_TIMEOUT;
  freePlayInactivityTimeout = DEFAULT_FREE_PLAY_INACTIVITY_TIMEOUT;
  enableBillAcceptorDoorAlarm = (DEFAULT_ENABLE_BILL_ACCEPTOR_DOOR_ALARM == 1);
  ccClientID = "";
  ccMerchantAccountID = "";
  theaterCategory = DEFAULT_THEATER_CATEGORY;
  authorizeNetMarketType = DEFAULT_AUTHORIZE_NET_MARKET_TYPE;
  authorizeNetDeviceType = DEFAULT_AUTHORIZE_NET_DEVICE_TYPE;
  isSharingCards = false;
}

bool BoothSettings::load(QString dbFile)
{
  bool ok = true;

  // Find QSLite driver
  QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "casplayer-db");

  db.setDatabaseName(dbFile);

  if (db.open())
  {
    qDebug() << "Opened:" << dbFile;

    deviceAddress = getValue(db, DEVICE_ADDRESS, 0, &ok).toInt();

    deviceAlias = getValue(db, DEVICE_ALIAS, "", &ok);

    int deviceTypeID = getValue(db, DEVICE_TYPE, DEFAULT_DEVICE_TYPE, &ok).toInt();

    // Default to arcade
    deviceType = BoothSettings::Arcade;

    QMetaObject mo = BoothSettings::staticMetaObject;
    int index = mo.indexOfEnumerator("Device_Type");
    QMetaEnum metaEnum = mo.enumerator(index);

    const char *key = metaEnum.valueToKey(deviceTypeID);
    if (key)
      deviceType = (Device_Type)deviceTypeID;

    creditChargeAmount = getValue(db, CREDIT_CHARGE_AMOUNT, 0, &ok).toDouble();

    secondsPerCredit = getValue(db, SECONDS_PER_CREDIT, 0, &ok).toInt();

    lowCreditThreshold = getValue(db, LOW_CREDIT_THRESHOLD, 0, &ok).toInt();

    storeNum = getValue(db, STORE_NUM, DEFAULT_STORE_NUM, &ok).toInt();

    areaNum = getValue(db, STORE_AREA, DEFAULT_AREA_NUM, &ok).toInt();

    allowCreditCards = (getValue(db, ALLOW_CREDIT_CARDS, DEFAULT_ALLOW_CREDIT_CARDS, &ok).toInt() ? true : false);

    useCVS = (getValue(db, USE_CVS, DEFAULT_USE_CVS, &ok).toInt() ? true : false);

    previewCost = getValue(db, PREVIEW_COST, DEFAULT_PREVIEW_COST, &ok).toDouble();

    threeVideoPreviewCost = getValue(db, THREE_VIDEO_PREVIEW_COST, DEFAULT_THREE_VIDEO_PREVIEW_COST, &ok).toDouble();

    fiveVideoPreviewCost = getValue(db, FIVE_VIDEO_PREVIEW_COST, DEFAULT_FIVE_VIDEO_PREVIEW_COST, &ok).toDouble();

    //previewSessionMinutes = getValue(db, PREVIEW_SESSION_MINUTES, 0, &ok).toInt();

    videoBrowsingInactivityMinutes = getValue(db, VIDEO_BROWSING_MINUTES, DEFAULT_VIDEO_BROWSING_MINUTES, &ok).toInt();

    lowPreviewMinuteThreshold = getValue(db, LOW_PREVIEW_MINUTE_THRESHOLD, DEFAULT_LOW_PREVIEW_MINUTE_THRESHOLD, &ok).toInt();

    additionalPreviewMinutes = getValue(db, ADDITIONAL_PREVIEW_MINUTES, DEFAULT_ADDITIONAL_PREVIEW_MINUTES, &ok).toInt();

    allowAdditionalPreviewMinutesSinglePreview = (getValue(db, ALLOW_ADDITIONAL_MINUTES_SINGLE_PREVIEW, DEFAULT_ALLOW_ADDITIONAL_MINUTES_SINGLE_PREVIEW, &ok).toInt() ? true : false);

    additionalPreviewMinutesCost = getValue(db, ADDITIONAL_PREVIEW_MINUTES_COST, DEFAULT_ADDITIONAL_PREVIEW_MINUTES_COST, &ok).toDouble();

    maxTimePreviewCardProgram = getValue(db, MAX_TIME_PREVIEW_CARD_PROGRAM, DEFAULT_MAX_TIME_PREVIEW_CARD_PROGRAM, &ok).toInt();

    ccDevelopmentMode = (getValue(db, CC_DEVELOPMENT_MODE, DEFAULT_CC_DEVELOPMENT_MODE, &ok).toInt() ? true : false);

    demoMode = (getValue(db, DEMO_MODE, DEFAULT_DEMO_MODE, &ok).toInt() ? true : false);

    int billAcceptorInterfaceID = getValue(db, BILL_ACCEPTOR_INTERFACE, DEFAULT_BILL_ACCEPTOR_INTERFACE, &ok).toInt();

    // Default to pulse interface
    billAcceptorInterface = BoothSettings::PULSE;

    mo = BoothSettings::staticMetaObject;
    index = mo.indexOfEnumerator("InterfaceType");
    metaEnum = mo.enumerator(index);

    key = metaEnum.valueToKey(billAcceptorInterfaceID);
    if (key)
      billAcceptorInterface = (BoothSettings::InterfaceType)billAcceptorInterfaceID;

    //pulsesPerDollar = getValue(db, PULSES_PER_DOLLAR, DEFAULT_PULSES_PER_DOLLAR, &ok).toInt();

    buddyBooth = (getValue(db, BUDDY_BOOTH, DEFAULT_BUDDY_BOOTH, &ok).toInt() ? true : false);

    buddyAddress = getValue(db, BUDDY_ADDRESS, DEFAULT_BUDDY_ADDRESS, &ok).toInt();

    enablePhysicalBuddyButton = (getValue(db, ENABLE_PHYSICAL_BUDDY_BUTTON, DEFAULT_ENABLE_PHYSICAL_BUDDY_BUTTON, &ok).toInt() ? true : false);

    enableBillAcceptor = (getValue(db, ENABLE_BILL_ACCEPTOR, DEFAULT_ENABLE_BILL_ACCEPTOR, &ok).toInt() ? true : false);

    enableCardSwipe = (getValue(db, ENABLE_CARD_SWIPE, DEFAULT_ENABLE_CARD_SWIPE, &ok).toInt() ? true : false);

    int uiTypeID = getValue(db, UI_TYPE, DEFAULT_UI_TYPE, &ok).toInt();

    // Default to Full UI
    uiType = BoothSettings::Full;

    mo = BoothSettings::staticMetaObject;
    index = mo.indexOfEnumerator("UI_Type");
    metaEnum = mo.enumerator(index);

    key = metaEnum.valueToKey(uiTypeID);
    if (key)
      uiType = (UI_Type)uiTypeID;

    //streamingPreview = (getValue(db, STREAMING_PREVIEW, DEFAULT_STREAMING_PREVIEW, &ok).toInt() ? true : false);

    cashOnly = (getValue(db, CASH_ONLY, DEFAULT_CASH_ONLY, &ok).toInt() ? true : false);

    coinSlot = (getValue(db, COIN_SLOT, DEFAULT_COIN_SLOT, &ok).toInt() ? true : false);

    showKeyPadInfo = (getValue(db, SHOW_KEYPAD_INFO, DEFAULT_SHOW_KEYPAD_INFO, &ok).toInt() ? true : false);

    //categoryFilter = getValue(db, CATEGORY_FILTER, DEFAULT_CATEGORY_FILTER, &ok);

    enableMeters = (getValue(db, ENABLE_METERS, DEFAULT_ENABLE_METERS, &ok).toInt() ? true : false);

    enableNrMeters = (getValue(db, ENABLE_NR_METERS, DEFAULT_ENABLE_NR_METERS, &ok).toInt() ? true : false);

    enableOnscreenKeypad = (getValue(db, ENABLE_ONSCREEN_KEYPAD, DEFAULT_ENABLE_ONSCREEN_KEYPAD, &ok).toInt() ? true : false);

    enableFreePlay = (getValue(db, ENABLE_FREE_PLAY, DEFAULT_ENABLE_FREE_PLAY, &ok).toInt() ? true : false);

    resumeFreePlaySessionTimeout = getValue(db, RESUME_FREE_PLAY_SESSION_TIMEOUT, DEFAULT_RESUME_FREE_PLAY_SESSION_TIMEOUT, &ok).toInt();

    freePlayInactivityTimeout = getValue(db, FREE_PLAY_INACTIVITY_TIMEOUT, DEFAULT_FREE_PLAY_INACTIVITY_TIMEOUT, &ok).toInt();

    enableBillAcceptorDoorAlarm = (getValue(db, ENABLE_BILL_ACCEPTOR_DOOR_ALARM, DEFAULT_ENABLE_BILL_ACCEPTOR_DOOR_ALARM, &ok).toInt() ? true : false);

    ccClientID = getValue(db, CC_CLIENT_ID, "", &ok);

    ccMerchantAccountID = getValue(db, CC_MERCHANT_ACCOUNT_ID, "", &ok);

    authorizeNetMarketType = getValue(db, AUTHORIZE_NET_MARKET_TYPE, DEFAULT_AUTHORIZE_NET_MARKET_TYPE, &ok).toInt();

    authorizeNetDeviceType = getValue(db, AUTHORIZE_NET_DEVICE_TYPE, DEFAULT_AUTHORIZE_NET_DEVICE_TYPE, &ok).toInt();

    theaterCategory = getValue(db, THEATER_CATEGORY, DEFAULT_THEATER_CATEGORY, &ok);

    isSharingCards = ((getValue(db, USE_SHARE_CARDS, DEFAULT_USE_SHARE_CARDS, &ok) == "true") ? true : false);

    shifts.clear();
    QString jsonString = getValue(db, SHIFT_TIMES, DEFAULT_SHIFT_TIMES, &ok);
    QLOG_DEBUG() << "loaded shift times:" << jsonString;
    if (ok)
    {
      QJsonObject object = QJsonDocument().fromJson(jsonString.toLatin1()).object();

      QJsonArray shiftArray = object["shifts"].toArray();

      for (int i = 0; i < shiftArray.count(); ++i)
      {
        QJsonObject shiftObj = shiftArray[i].toObject();

        StoreShift shift;
        shift.readJson(shiftObj);

        shifts.append(shift);
      }
    }

    storeName = getValue(db, STORE_NAME, "", &ok);

    jsonString = getValue(db, CUSTOM_CATEGORY, DEFAULT_CUSTOM_CATEGORY, &ok);
    QLOG_DEBUG() << "loaded custom category:" << jsonString;
    if (ok)
    {
      QJsonObject object = QJsonDocument().fromJson(jsonString.toLatin1()).object();

      customCategory.name = object["name"].toString();
      foreach (QJsonValue item, object["producers"].toArray())
      {
        customCategory.producers.append(item.toString());
      }
    }

    db.close();
  }
  else
    ok = false;

  QString connectionName = db.connectionName();
  db.close();
  db = QSqlDatabase();
  QSqlDatabase::removeDatabase(connectionName);

  return ok;
}

bool BoothSettings::save(QString dbFile)
{
  bool ok = true;

  // Find QSLite driver
  QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "casplayer-db");

  db.setDatabaseName(dbFile);

  if (db.open())
  {
    qDebug() << "Opened:" << dbFile;

    ok = setValue(db, DEVICE_ADDRESS, deviceAddress);

    ok = setValue(db, DEVICE_ALIAS, deviceAlias);

    ok = setValue(db, DEVICE_TYPE, deviceType);

    ok = setValue(db, CREDIT_CHARGE_AMOUNT, creditChargeAmount);

    ok = setValue(db, SECONDS_PER_CREDIT, secondsPerCredit);

    ok = setValue(db, LOW_CREDIT_THRESHOLD, lowCreditThreshold);

    ok = setValue(db, STORE_NUM, storeNum);

    ok = setValue(db, STORE_AREA, areaNum);

    ok = setValue(db, ALLOW_CREDIT_CARDS, (allowCreditCards ? 1 : 0));

    ok = setValue(db, USE_CVS, (useCVS ? 1 : 0));

    ok = setValue(db, PREVIEW_COST, previewCost);

    ok = setValue(db, THREE_VIDEO_PREVIEW_COST, threeVideoPreviewCost);

    ok = setValue(db, FIVE_VIDEO_PREVIEW_COST, fiveVideoPreviewCost);

    //ok = setValue(db, PREVIEW_SESSION_MINUTES, previewSessionMinutes);

    ok = setValue(db, VIDEO_BROWSING_MINUTES, videoBrowsingInactivityMinutes);

    ok = setValue(db, LOW_PREVIEW_MINUTE_THRESHOLD, lowPreviewMinuteThreshold);

    ok = setValue(db, ADDITIONAL_PREVIEW_MINUTES, additionalPreviewMinutes);

    ok = setValue(db, ALLOW_ADDITIONAL_MINUTES_SINGLE_PREVIEW, (allowAdditionalPreviewMinutesSinglePreview ? 1 : 0));

    ok = setValue(db, ADDITIONAL_PREVIEW_MINUTES_COST, additionalPreviewMinutesCost);

    ok = setValue(db, MAX_TIME_PREVIEW_CARD_PROGRAM, maxTimePreviewCardProgram);

    ok = setValue(db, CC_DEVELOPMENT_MODE, (ccDevelopmentMode ? 1 : 0));

    ok = setValue(db, DEMO_MODE, (demoMode ? 1 : 0));

    ok = setValue(db, BILL_ACCEPTOR_INTERFACE, billAcceptorInterface);

    //ok = setValue(db, PULSES_PER_DOLLAR, pulsesPerDollar);

    ok = setValue(db, BUDDY_BOOTH, (buddyBooth ? 1 : 0));

    ok = setValue(db, BUDDY_ADDRESS, buddyAddress);

    ok = setValue(db, ENABLE_PHYSICAL_BUDDY_BUTTON, (enablePhysicalBuddyButton ? 1 : 0));

    ok = setValue(db, ENABLE_BILL_ACCEPTOR, (enableBillAcceptor ? 1 : 0));

    ok = setValue(db, ENABLE_CARD_SWIPE, (enableCardSwipe ? 1 : 0));

    ok = setValue(db, UI_TYPE, uiType);

    //ok = setValue(db, STREAMING_PREVIEW, (streamingPreview ? 1 : 0));

    ok = setValue(db, CASH_ONLY, (cashOnly ? 1 : 0));

    ok = setValue(db, COIN_SLOT, (coinSlot ? 1 : 0));

    ok = setValue(db, ENABLE_ONSCREEN_KEYPAD, (enableOnscreenKeypad ? 1 : 0));

    ok = setValue(db, SHOW_KEYPAD_INFO, (showKeyPadInfo ? 1 : 0));

    //ok = setValue(db, CATEGORY_FILTER, categoryFilter);

    ok = setValue(db, ENABLE_METERS, (enableMeters ? 1 : 0));

    ok = setValue(db, ENABLE_NR_METERS, (enableNrMeters ? 1 : 0));

    ok = setValue(db, ENABLE_FREE_PLAY, (enableFreePlay ? 1 : 0));

    ok = setValue(db, RESUME_FREE_PLAY_SESSION_TIMEOUT, resumeFreePlaySessionTimeout);

    ok = setValue(db, FREE_PLAY_INACTIVITY_TIMEOUT, freePlayInactivityTimeout);

    ok = setValue(db, ENABLE_BILL_ACCEPTOR_DOOR_ALARM, (enableBillAcceptorDoorAlarm ? 1 : 0));

    ok = setValue(db, CC_CLIENT_ID, ccClientID);

    ok = setValue(db, CC_MERCHANT_ACCOUNT_ID, ccMerchantAccountID);

    ok = setValue(db, AUTHORIZE_NET_MARKET_TYPE, authorizeNetMarketType);

    ok = setValue(db, AUTHORIZE_NET_DEVICE_TYPE, authorizeNetDeviceType);

    ok = setValue(db, THEATER_CATEGORY, theaterCategory);

    QLOG_DEBUG()<<"vALUE OF ISsHaringCards while saving"<<isSharingCards;
    ok = setValue(db, USE_SHARE_CARDS, isSharingCards);
    
    QJsonArray jsonArray;
    foreach (StoreShift sh, shifts)
    {
      QJsonObject obj;
      sh.writeJson(obj);
      jsonArray.append(obj);
    }

    QJsonObject shiftsObj;
    shiftsObj["shifts"] = jsonArray;
    QJsonDocument jsonDoc(shiftsObj);
    QString shiftsJson(jsonDoc.toJson(true));
    QLOG_DEBUG() << "saving shift times:" << shiftsJson;
    ok = setValue(db, SHIFT_TIMES, shiftsJson);

    ok = setValue(db, STORE_NAME, storeName);

    QJsonArray jsonArray2;
    foreach (QString p, customCategory.producers)
    {
      jsonArray2.append(QJsonValue(p));
    }

    QJsonObject customCategoryObj;
    customCategoryObj["producers"] = jsonArray2;
    customCategoryObj["name"] = customCategory.name;
    QJsonDocument jsonDoc2(customCategoryObj);
    QString customCategoryJson(jsonDoc2.toJson(true));
    QLOG_DEBUG() << "saving custom category:" << customCategoryJson;
    ok = setValue(db, CUSTOM_CATEGORY, customCategoryJson);


    db.close();
  }
  else
    ok = false;

  QString connectionName = db.connectionName();
  db.close();
  db = QSqlDatabase();
  QSqlDatabase::removeDatabase(connectionName);

  return ok;
}

QString BoothSettings::getValue(QSqlDatabase db, QString keyName, QVariant defaultValue, bool *ok)
{
  QSqlQuery query(db);
  QString value;

  *ok = false;

  query.prepare("SELECT data FROM settings WHERE key_name = :key_name");
  query.bindValue(":key_name", QVariant(keyName));

  if (query.exec())
  {
    if (query.isActive() && query.first())
    {
     // qDebug() << keyName << " = " << query.value(0).toString();
      value = query.value(0).toString();
      *ok = true;
    }
    else
    {
      //QLOG_DEBUG() << QString("'%1' not found, now adding").arg(keyName), true);

      // Looks like the key doesn't exist, if defaultValue is set then
      // try adding the record
      if (!defaultValue.isNull())
      {
        query.prepare("INSERT INTO settings (key_name, data) VALUES (:key_name, :data)");
        query.bindValue(":key_name", keyName);
        query.bindValue(":data", defaultValue);

        if (query.exec())
        {
          value = defaultValue.toString();
          *ok = true;
        }
      }
    }
  }
  else
    qDebug() << query.lastError().text();

  return value;
}

bool BoothSettings::setValue(QSqlDatabase db, QString keyName, QVariant value)
{
  QSqlQuery query(db);

  query.prepare("UPDATE settings SET data = :data WHERE key_name = :key_name");
  query.bindValue(":data", value);
  query.bindValue(":key_name", QVariant(keyName));

  if (query.exec())
  {
    //QLOG_DEBUG() << QString("Updated '%1' in settings table").arg(keyName), true);
    return true;
  }
  else
  {
    //QLOG_ERROR() << QString("Could not update '%1' in settings table").arg(keyName));
    return false;
  }
}

int BoothSettings::DeviceAddress()
{
  return deviceAddress;
}

BoothSettings::Device_Type BoothSettings::DeviceType()
{
  return deviceType;
}

QString BoothSettings::DeviceTypeToString()
{
  const QMetaObject &mo = BoothSettings::staticMetaObject;
  int index = mo.indexOfEnumerator("Device_Type");
  QMetaEnum metaEnum = mo.enumerator(index);

  return QString(metaEnum.valueToKey(deviceType));
}

qreal BoothSettings::CreditChargeAmount()
{
  return creditChargeAmount;
}

int BoothSettings::SecondsPerCredit()
{
  return secondsPerCredit;
}

int BoothSettings::LowCreditThreshold()
{
  return lowCreditThreshold;
}

int BoothSettings::StoreNum()
{
  return storeNum;
}

int BoothSettings::AreaNum()
{
  return areaNum;
}

bool BoothSettings::UseCVS()
{
  return useCVS;
}

bool BoothSettings::AllowCreditCards()
{
  return allowCreditCards;
}

qreal BoothSettings::PreviewCost()
{
  return previewCost;
}

qreal BoothSettings::ThreeVideoPreviewCost()
{
  return threeVideoPreviewCost;
}

qreal BoothSettings::FiveVideoPreviewCost()
{
  return fiveVideoPreviewCost;
}

//int BoothSettings::PreviewSessionMinutes()
//{
//  // If the value is less than 1 min then use default value
//  if (previewSessionMinutes < 1)
//    return DEFAULT_PREVIEW_SESSION_MINUTES;
//  else
//    return previewSessionMinutes;
//}

int BoothSettings::VideoBrowsingInactivityMinutes()
{
  return videoBrowsingInactivityMinutes;
}

int BoothSettings::LowPreviewMinuteThreshold()
{
  // If the value is less than 1 min then use default value
  if (lowPreviewMinuteThreshold < 1)
    return DEFAULT_LOW_PREVIEW_MINUTE_THRESHOLD;
  else
    return lowPreviewMinuteThreshold;
}

int BoothSettings::AdditionalPreviewMinutes()
{
  return additionalPreviewMinutes;
}

bool BoothSettings::AllowAdditionalPreviewMinutesSinglePreview()
{
  return allowAdditionalPreviewMinutesSinglePreview;
}

qreal BoothSettings::AdditionalPreviewMinutesCost()
{
  return additionalPreviewMinutesCost;
}

int BoothSettings::MaxTimePreviewCardProgram()
{
  return maxTimePreviewCardProgram;
}

bool BoothSettings::CcDevelopmentMode()
{
  return ccDevelopmentMode;
}

bool BoothSettings::DemoMode()
{
  return demoMode;
}

BoothSettings::InterfaceType BoothSettings::BillAcceptorInterface()
{
  return billAcceptorInterface;
}

QString BoothSettings::BillAcceptorInterfaceToString()
{
  const QMetaObject &mo = BoothSettings::staticMetaObject;
  int index = mo.indexOfEnumerator("InterfaceType");
  QMetaEnum metaEnum = mo.enumerator(index);

  return QString(metaEnum.valueToKey(billAcceptorInterface));
}

bool BoothSettings::BuddyBooth()
{
  return buddyBooth;
}

int BoothSettings::BuddyAddress()
{
  return buddyAddress;
}

QString BoothSettings::BuddyAddressToIP()
{
  return QString("10.0.0.%1").arg(buddyAddress);
}

bool BoothSettings::EnableBillAcceptor()
{
  return enableBillAcceptor;
}

bool BoothSettings::SharingCards() const
{
   return isSharingCards;
}

void BoothSettings::setSharingCards(bool value)
{
    QLOG_DEBUG()<<"Inside setSharingCards = "<<value;
    isSharingCards = value;
}

bool BoothSettings::EnableCardSwipe()
{
  return enableCardSwipe;
}

BoothSettings::UI_Type BoothSettings::UiType()
{
  return uiType;
}

QString BoothSettings::UiTypeToString()
{
  const QMetaObject &mo = BoothSettings::staticMetaObject;
  int index = mo.indexOfEnumerator("UI_Type");
  QMetaEnum metaEnum = mo.enumerator(index);

  return QString(metaEnum.valueToKey(uiType));
}

//bool BoothSettings::StreamingPreview()
//{
//  return streamingPreview;
//}

bool BoothSettings::EnableOnscreenKeypad() const
{
  return enableOnscreenKeypad;
}

bool BoothSettings::ShowKeyPadInfo() const
{
  return showKeyPadInfo;
}

//QString BoothSettings::CategoryFilter() const
//{
//  return categoryFilter;
//}

//int BoothSettings::PulsesPerDollar() const
//{
//  return pulsesPerDollar;
//}

bool BoothSettings::CoinSlot() const
{
  return coinSlot;
}

bool BoothSettings::CashOnly() const
{
  return cashOnly;
}

//void BoothSettings::setCategoryFilter(const QString &value)
//{
//  categoryFilter = value;
//}

void BoothSettings::setShowKeyPadInfo(bool value)
{
  showKeyPadInfo = value;
}

void BoothSettings::setCoinSlot(bool value)
{
  coinSlot = value;
}

void BoothSettings::setCashOnly(bool value)
{
  cashOnly = value;
}

//void BoothSettings::setStreamingPreview(bool value)
//{
//  streamingPreview = value;
//}

void BoothSettings::setUiType(const UI_Type &value)
{
  uiType = value;
}

void BoothSettings::setEnableCardSwipe(bool value)
{
  enableCardSwipe = value;
}

void BoothSettings::setEnableBillAcceptor(bool value)
{
  enableBillAcceptor = value;
}

void BoothSettings::setBuddyAddress(int value)
{
  buddyAddress = value;
}

void BoothSettings::setBuddyBooth(bool value)
{
  buddyBooth = value;
}

void BoothSettings::setBillAcceptorInterface(const BoothSettings::InterfaceType &value)
{
  billAcceptorInterface = value;
}

void BoothSettings::setDemoMode(bool value)
{
  demoMode = value;
}

void BoothSettings::setCcDevelopmentMode(bool value)
{
  ccDevelopmentMode = value;
}

void BoothSettings::setMaxTimePreviewCardProgram(int value)
{
  maxTimePreviewCardProgram = value;
}

void BoothSettings::setAdditionalPreviewMinutesCost(const qreal &value)
{
  additionalPreviewMinutesCost = value;
}

void BoothSettings::setAllowAdditionalPreviewMinutesSinglePreview(bool value)
{
  allowAdditionalPreviewMinutesSinglePreview = value;
}

void BoothSettings::setAdditionalPreviewMinutes(int value)
{
  additionalPreviewMinutes = value;
}

void BoothSettings::setLowPreviewMinuteThreshold(int value)
{
  lowPreviewMinuteThreshold = value;
}

void BoothSettings::setVideoBrowsingInactivityMinutes(int value)
{
  videoBrowsingInactivityMinutes = value;
}

//void BoothSettings::setPreviewSessionMinutes(int value)
//{
//  previewSessionMinutes = value;
//}

void BoothSettings::setFiveVideoPreviewCost(const qreal &value)
{
  fiveVideoPreviewCost = value;
}

void BoothSettings::setThreeVideoPreviewCost(const qreal &value)
{
  threeVideoPreviewCost = value;
}

void BoothSettings::setPreviewCost(const qreal &value)
{
  previewCost = value;
}

void BoothSettings::setAllowCreditCards(bool value)
{
  allowCreditCards = value;
}

void BoothSettings::setUseCVS(bool value)
{
  useCVS = value;
}

void BoothSettings::setAreaNum(int value)
{
  areaNum = value;
}

void BoothSettings::setStoreNum(int value)
{
  storeNum = value;
}

void BoothSettings::setLowCreditThreshold(int value)
{
  lowCreditThreshold = value;
}

void BoothSettings::setSecondsPerCredit(int value)
{
  secondsPerCredit = value;
}

void BoothSettings::setCreditChargeAmount(const qreal &value)
{
  creditChargeAmount = value;
}

void BoothSettings::setDeviceType(const Device_Type &value)
{
  deviceType = value;
}

void BoothSettings::setDeviceAddress(int value)
{
  deviceAddress = value;
}

QString BoothSettings::DeviceAlias() const
{
  return deviceAlias;
}

void BoothSettings::setDeviceAlias(const QString &value)
{
  deviceAlias = value;
}

//void BoothSettings::setPulsesPerDollar(int value)
//{
//  pulsesPerDollar = value;
//}

bool BoothSettings::EnableNrMeters() const
{
    return enableNrMeters;
}

void BoothSettings::setEnableNrMeters(bool value)
{
    enableNrMeters = value;
}

bool BoothSettings::EnableMeters() const
{
    return enableMeters;
}

void BoothSettings::setEnableMeters(bool value)
{
  enableMeters = value;
}

void BoothSettings::setEnableOnscreenKeypad(bool value)
{
  enableOnscreenKeypad = value;
}

bool BoothSettings::EnableFreePlay() const
{
    return enableFreePlay;
}

void BoothSettings::setEnableFreePlay(bool value)
{
    enableFreePlay = value;
}

bool BoothSettings::EnablePhysicalBuddyButton() const
{
  return enablePhysicalBuddyButton;
}

void BoothSettings::setEnablePhysicalBuddyButton(bool value)
{
  enablePhysicalBuddyButton = value;
}

int BoothSettings::FreePlayInactivityTimeout() const
{
  return freePlayInactivityTimeout;
}

void BoothSettings::setFreePlayInactivityTimeout(int value)
{
  freePlayInactivityTimeout = value;
}

int BoothSettings::ResumeFreePlaySessionTimeout() const
{
  return resumeFreePlaySessionTimeout;
}

void BoothSettings::setResumeFreePlaySessionTimeout(int value)
{
  resumeFreePlaySessionTimeout = value;
}

bool BoothSettings::EnableBillAcceptorDoorAlarm() const
{
    return enableBillAcceptorDoorAlarm;
}

void BoothSettings::setEnableBillAcceptorDoorAlarm(bool value)
{
    enableBillAcceptorDoorAlarm = value;
}

QString BoothSettings::CcClientID() const
{
    return ccClientID;
}

void BoothSettings::setCcClientID(const QString &value)
{
    ccClientID = value;
}

QString BoothSettings::CcMerchantAccountID() const
{
    return ccMerchantAccountID;
}

void BoothSettings::setCcMerchantAccountID(const QString &value)
{
    ccMerchantAccountID = value;
}

QString BoothSettings::TheaterCategory() const
{
  return theaterCategory;
}

void BoothSettings::setTheaterCategory(const QString &value)
{
  theaterCategory = value;
}

QList<StoreShift> BoothSettings::Shifts() const
{
  return shifts;
}

void BoothSettings::setShifts(const QList<StoreShift> &value)
{
  shifts = value;
}

QString BoothSettings::StoreName() const
{
  return storeName;
}

void BoothSettings::setStoreName(const QString &value)
{
  storeName = value;
}

CustomCategory BoothSettings::CustomMenuCategory() const
{
  return customCategory;
}

void BoothSettings::setCustomMenuCategory(const CustomCategory &value)
{
  customCategory = value;
}

qint8 BoothSettings::AuthorizeNetDeviceType() const
{
  return authorizeNetDeviceType;
}

void BoothSettings::setAuthorizeNetDeviceType(const qint8 &value)
{
  authorizeNetDeviceType = value;
}

qint8 BoothSettings::AuthorizeNetMarketType() const
{
  return authorizeNetMarketType;
}

void BoothSettings::setAuthorizeNetMarketType(const qint8 &value)
{
  authorizeNetMarketType = value;
}
