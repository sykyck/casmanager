#include "appsettingswidget.h"
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include "global.h"

AppSettingsWidget::AppSettingsWidget(DatabaseMgr *dbMgr, CasServer *casServer, Settings *settings, SharedCardServices *sharedCardService, bool firstRun, QWidget *parent) : QDialog(parent)
{
  this->dbMgr = dbMgr;
  this->casServer = casServer;
  this->settings = settings;
  this->sharedCardService = sharedCardService;

  connect(this->sharedCardService, SIGNAL(oneWayReplicated()), this, SLOT(onOneWayReplication()));
  connect(this->sharedCardService, SIGNAL(twoWayReplicated()), this, SLOT(onTwoWayReplication()));
  connect(this->sharedCardService, SIGNAL(errorInReplication()), this, SLOT(onReplicationError()));

  formLayout = new QFormLayout;
  buttonLayout = new QDialogButtonBox;
  verticalLayout = new QVBoxLayout;

  appModeChanged = false;
  addressRangeChanged = false;

  cmbAppModes = new QComboBox;
  cmbAppModes->addItem("All Functions", Settings::All_Functions);
  cmbAppModes->addItem("DVD Copying Only", Settings::DVD_Copying_Only);
  cmbAppModes->addItem("No DVD Copying", Settings::No_DVD_Copying);
  connect(cmbAppModes, SIGNAL(currentIndexChanged(int)), this, SLOT(onAppModeChanged(int)));

  for (int i = 0; i < cmbAppModes->count(); ++i)
  {
    if (cmbAppModes->itemData(i).toInt() == settings->getValue(APP_MODE).toInt())
    {
      // Block signals while setting initial selection so it doesn't call
      // onAppModeChanged() which expects the other widgets to already exist
      cmbAppModes->blockSignals(true);
      cmbAppModes->setCurrentIndex(i);
      cmbAppModes->blockSignals(false);
      break;
    }
  }

  int minAddress = settings->getValue(MIN_ADDRESS).toString().mid(settings->getValue(MIN_ADDRESS).toString().lastIndexOf(".") + 1).toInt();
  int maxAddress = settings->getValue(MAX_ADDRESS).toString().mid(settings->getValue(MAX_ADDRESS).toString().lastIndexOf(".") + 1).toInt();

  spnMinimumBoothAddress = new QSpinBox;
  spnMinimumBoothAddress->setValue(minAddress);
  spnMinimumBoothAddress->setMinimum(21);
  spnMinimumBoothAddress->setMaximum(254);

  spnMaximumBoothAddress = new QSpinBox;
  spnMaximumBoothAddress->setValue(maxAddress);
  spnMaximumBoothAddress->setMinimum(21);
  spnMaximumBoothAddress->setMaximum(254);

  txtStoreName = new QLineEdit(settings->getValue(STORE_NAME).toString());
  txtStoreName->setMaxLength(100);
  txtStoreName->setMinimumWidth(400);

  chkShareCards = new QCheckBox;
  chkShareCards->setChecked(settings->getValue(USE_SHARE_CARDS).toBool());
  connect(chkShareCards, SIGNAL(clicked()), this, SLOT(onShareCardsClick()));

  txtRemoteDatabaseName = new QLineEdit();
  if(!settings->getValue(USE_SHARE_CARDS).toBool())
  {
     txtRemoteDatabaseName->setDisabled(true);
     chkShareCards->setChecked(false);
  }
  else
  {
     txtRemoteDatabaseName->setDisabled(false);
     txtRemoteDatabaseName->insert(settings->getValue(REMOTE_COUCHDB,DEFAULT_REMOTE_COUCHDB).toString());
     chkShareCards->setChecked(true);
  }

  chkSendCollectionReport = new QCheckBox;
  chkSendCollectionReport->setChecked(settings->getValue(ENABLE_COLLECTION_REPORT).toBool());
  connect(chkSendCollectionReport, SIGNAL(clicked()), this, SLOT(onSendCollectionReportClicked()));

  txtCollectionReportRecipients = new QLineEdit(settings->getValue(COLLECTION_REPORT_RECIPIENTS).toString());

  chkSendDailyMeterReport = new QCheckBox;
  chkSendDailyMeterReport->setChecked(settings->getValue(ENABLE_DAILY_METERS).toBool());
  connect(chkSendDailyMeterReport, SIGNAL(clicked()), this, SLOT(onSendDailyMeterReportClicked()));

  txtDailyMeterReportRecipients = new QLineEdit(settings->getValue(DAILY_METERS_REPORT_RECIPIENTS).toString());

  dailyMeterSendTime = new QTimeEdit(QTime::fromString(settings->getValue(DAILY_METERS_TIME).toString(), "hh:mm"));
  dailyMeterSendTime->setDisplayFormat("h:mm AP");

  chkSendCollectionSnapshotReport = new QCheckBox;
  chkSendCollectionSnapshotReport->setChecked(settings->getValue(ENABLE_COLLECTION_SNAPSHOT).toBool());
  connect(chkSendCollectionSnapshotReport, SIGNAL(clicked()), this, SLOT(onSendCollectionSnapshotReportClicked()));

  spnCollectionSnapshotInterval = new QSpinBox;
  spnCollectionSnapshotInterval->setValue(settings->getValue(COLLECTION_SNAPSHOT_INTERVAL).toInt());
  spnCollectionSnapshotInterval->setMinimum(5);
  spnCollectionSnapshotInterval->setMaximum(31);

  txtCollectionSnapshotReportRecipients = new QLineEdit(settings->getValue(COLLECTION_SNAPSHOT_REPORT_RECIPIENTS).toString());

  QDate firstCollectionDate;
  if (settings->getValue(FIRST_COLLECTION_SNAPSHOT_DATE).toDateTime().isValid())
    firstCollectionDate = settings->getValue(FIRST_COLLECTION_SNAPSHOT_DATE).toDateTime().date();
  else
    firstCollectionDate = QDate::currentDate();

  dtFirstCollectionDate = new QDateEdit(firstCollectionDate);
//  dtFirstCollectionDate->setMinimumDate(QDate::currentDate());
  dtFirstCollectionDate->setDisplayFormat("M/d/yyyy");
  dtFirstCollectionDate->setCalendarPopup(true);

  /*txtDataPath = new QLineEdit(settings->getValue(DATA_PATH).toString());
  txtDataPath->setMinimumWidth(400);
  txtVideoPath = new QLineEdit(settings->getValue(VIDEO_PATH).toString());
  txtMetadataPath = new QLineEdit(settings->getValue(VIDEO_METADATA_PATH).toString());
  txtDvdCopyProg = new QLineEdit(settings->DvdCopyProgFile());
  txtCustomerPassword = new QLineEdit(settings->getValue(CUSTOMER_PASSWORD, "", true).toString());
  txtCustomerPassword->setEchoMode(QLineEdit::Password);
  txtVideoLookupURL = new QLineEdit(settings->VideoLookupUrl());
  txtVideoUpdateURL = new QLineEdit(settings->VideoUpdateUrl());
  txtDvdCopyingDrive = new QLineEdit(settings->getValue(WINE_DVD_DRIVE_LETTER).toString());
  txtDvdCopyingDestPath = new QLineEdit(settings->WineDvdCopyDestDriveLetter());

  chkUploadViewTimes = new QCheckBox;
  chkUploadViewTimes->setChecked(settings->getValue(UPLOAD_VIEW_TIMES).toBool());

  chkTestMode = new QCheckBox;

  txtViewTimesServerURL = new QLineEdit(settings->ViewtimesServerUrl());*/

  btnSave = new QPushButton(tr("Save"));
  connect(btnSave, SIGNAL(clicked()), this, SLOT(onSaveClicked()));

  btnCancel = new QPushButton(tr("Cancel"));
  connect(btnCancel, SIGNAL(clicked()), this, SLOT(reject()));

  formLayout->addRow(tr("Application Mode"), cmbAppModes);
  formLayout->addRow(tr("First Booth Address"), spnMinimumBoothAddress);
  formLayout->addRow(tr("Last Booth Address"), spnMaximumBoothAddress);
  formLayout->addRow(tr("Send Collection Reports"), chkSendCollectionReport);
  formLayout->addRow(tr("Collection Report Recipients"), txtCollectionReportRecipients);
  formLayout->addRow(tr("Send Daily Meter Reports"), chkSendDailyMeterReport);
  formLayout->addRow(tr("Daily Meter Report Recipients"), txtDailyMeterReportRecipients);
  formLayout->addRow(tr("Daily Meter Report Time"), dailyMeterSendTime);
  formLayout->addRow(tr("Send Collection Snapshot Reports"), chkSendCollectionSnapshotReport);
  formLayout->addRow(tr("Collection Snapshot Report Recipients"), txtCollectionSnapshotReportRecipients);
  formLayout->addRow(tr("First Collection Snapshot Date"), dtFirstCollectionDate);
  formLayout->addRow(tr("Collection Snapshot Interval (days)"), spnCollectionSnapshotInterval);
  formLayout->addRow(tr("Share Cards "), chkShareCards);
  formLayout->addRow(tr("Database Name "), txtRemoteDatabaseName);

//  formLayout->addRow(tr("Data Path"), txtDataPath);
//  formLayout->addRow(tr("Video Path"), txtVideoPath);
//  formLayout->addRow(tr("DVD Copy Drive"), txtDvdCopyingDrive);
//  formLayout->addRow(tr("DVD Copy Path"), txtDvdCopyingDestPath);
//  formLayout->addRow(tr("Metadata Download Path"), txtMetadataPath);
//  formLayout->addRow(tr("DVD Copy Program"), txtDvdCopyProg);
//  formLayout->addRow(tr("Customer Password"), txtCustomerPassword);
//  formLayout->addRow(tr("Video Lookup URL"), txtVideoLookupURL);
//  formLayout->addRow(tr("Video Update URL"), txtVideoUpdateURL);
//  formLayout->addRow(tr("Upload View Times"), chkUploadViewTimes);
//  formLayout->addRow(tr("View Times Upload URL"), txtViewTimesServerURL);
//  formLayout->addRow(tr("Test Mode"), chkTestMode);

  buttonLayout->addButton(btnSave, QDialogButtonBox::AcceptRole);

  // Only show Cancel button if this is not the first run
  if (!firstRun)
    buttonLayout->addButton(btnCancel, QDialogButtonBox::RejectRole);

  verticalLayout->addLayout(formLayout);
  verticalLayout->addWidget(buttonLayout);

  this->setLayout(verticalLayout);

  this->setWindowTitle(tr("Settings"));

  // Disable maximize,minimize and resizing of window
  this->setFixedSize(this->sizeHint());
}

bool AppSettingsWidget::restartRequired()
{
  // Returns true if setting changes were made that require the software to be restarted
  // App mode or address range are the only settings that require a restart
  return appModeChanged || addressRangeChanged;
}

void AppSettingsWidget::onShareCardsClick()
{
    if(chkShareCards->isChecked())
    {
        txtRemoteDatabaseName->setEnabled(true);
        txtRemoteDatabaseName->clear();
    }
    else
    {
        txtRemoteDatabaseName->setEnabled(false);
        txtRemoteDatabaseName->clear();
    }
}

void AppSettingsWidget::showEvent(QShowEvent *)
{
  onSendCollectionReportClicked();
  onSendDailyMeterReportClicked();
  onSendCollectionSnapshotReportClicked();
  onAppModeChanged(0);
}

void AppSettingsWidget::onSaveClicked()
{
  if (validateInput())
  {
    if (settings->getValue(APP_MODE).toInt() != cmbAppModes->itemData(cmbAppModes->currentIndex()).toInt())
    {
      appModeChanged = true;
    }

    settings->setValue(APP_MODE, cmbAppModes->itemData(cmbAppModes->currentIndex()).toInt());

    if (settings->getValue(APP_MODE).toInt() != Settings::DVD_Copying_Only)
    {
      int minAddress = settings->getValue(MIN_ADDRESS).toString().mid(settings->getValue(MIN_ADDRESS).toString().lastIndexOf(".") + 1).toInt();
      int maxAddress = settings->getValue(MAX_ADDRESS).toString().mid(settings->getValue(MAX_ADDRESS).toString().lastIndexOf(".") + 1).toInt();

      if (minAddress != spnMinimumBoothAddress->value() ||
          maxAddress != spnMaximumBoothAddress->value())
      {
        addressRangeChanged = true;

        // Clear current device list so devices are rescanned
        settings->setValue(DEVICE_LIST, QStringList());
      }

      settings->setValue(MIN_ADDRESS, QString("%1%2").arg(settings->getValue(ARCADE_SUBNET).toString()).arg(spnMinimumBoothAddress->value()));
      settings->setValue(MAX_ADDRESS, QString("%1%2").arg(settings->getValue(ARCADE_SUBNET).toString()).arg(spnMaximumBoothAddress->value()));
    }

    settings->setValue(ENABLE_COLLECTION_REPORT, chkSendCollectionReport->isChecked());

    if (chkSendCollectionReport->isChecked())
      settings->setValue(COLLECTION_REPORT_RECIPIENTS, txtCollectionReportRecipients->text());
    else
      settings->setValue(COLLECTION_REPORT_RECIPIENTS, "");

    settings->setValue(ENABLE_DAILY_METERS, chkSendDailyMeterReport->isChecked());

    if (chkSendDailyMeterReport->isChecked())
    {
      settings->setValue(DAILY_METERS_REPORT_RECIPIENTS, txtDailyMeterReportRecipients->text());
      settings->setValue(DAILY_METERS_TIME, dailyMeterSendTime->time().toString("hh:mm"));
    }
    else
    {
      settings->setValue(DAILY_METERS_REPORT_RECIPIENTS, "");
      settings->setValue(DAILY_METERS_TIME, DEFAULT_DAILY_METERS_TIME);
    }

    settings->setValue(ENABLE_COLLECTION_SNAPSHOT, chkSendCollectionSnapshotReport->isChecked());

    if (chkSendCollectionSnapshotReport->isChecked())
    {
      settings->setValue(COLLECTION_SNAPSHOT_REPORT_RECIPIENTS, txtCollectionSnapshotReportRecipients->text());
      settings->setValue(COLLECTION_SNAPSHOT_INTERVAL, spnCollectionSnapshotInterval->value());
      settings->setValue(FIRST_COLLECTION_SNAPSHOT_DATE, dtFirstCollectionDate->date());
      settings->setValue(COLLECTION_SNAPSHOT_SCHEDULE_DAY, dtFirstCollectionDate->date().dayOfWeek());
    }
    else
    {
      settings->setValue(COLLECTION_SNAPSHOT_REPORT_RECIPIENTS, "");
      settings->setValue(COLLECTION_SNAPSHOT_INTERVAL, DEFAULT_COLLECTION_SNAPSHOT_INTERVAL);
      settings->setValue(FIRST_COLLECTION_SNAPSHOT_DATE, QDate());
      settings->setValue(COLLECTION_SNAPSHOT_SCHEDULE_DAY, 1);
    }

    settings->setValue(USE_SHARE_CARDS, chkShareCards->isChecked());
    settings->setValue(REMOTE_COUCHDB, txtRemoteDatabaseName->text());

    /*settings->setDataPath(txtDataPath->text());
    settings->setVideoPath(txtVideoPath->text());
    settings->setVideoMetadataPath(txtMetadataPath->text());    
    settings->setDvdCopyProgFile(txtDvdCopyProg->text());
    settings->setWineDvdDriveLetter(txtDvdCopyingDrive->text());
    settings->setWineDvdCopyDestDriveLetter(txtDvdCopyingDestPath->text());
    settings->setCustomerPassword(txtCustomerPassword->text());
    settings->setVideoLookupUrl(txtVideoLookupURL->text());
    settings->setVideoUpdateUrl(txtVideoUpdateURL->text());
    settings->setUploadViewTimes(chkUploadViewTimes->isChecked());
    settings->setViewtimesServerUrl(txtViewTimesServerURL->text());
    settings->setTestMode(chkTestMode->isChecked());*/

    // Save settings
    dbMgr->setValue("all_settings", settings->getSettings());
    if(chkShareCards->isChecked())
    {
       sharedCardService->replicateDatabase(QString("cas"), QString("https://cas_admin:7B445jh8axVFL2tAMoQtLBlg@ec2-34-211-226-10.us-west-2.compute.amazonaws.com:6984/%1").arg(txtRemoteDatabaseName->text()), false);
    }
    else
    {
       accept();
    }
  }
}

void AppSettingsWidget::onTwoWayReplication()
{
   accept();
}

void AppSettingsWidget::onOneWayReplication()
{
   sharedCardService->replicateDatabase(QString("https://cas_admin:7B445jh8axVFL2tAMoQtLBlg@ec2-34-211-226-10.us-west-2.compute.amazonaws.com:6984/%1").arg(txtRemoteDatabaseName->text()), QString("cas"), true);
}

void AppSettingsWidget::onReplicationError()
{
   QMessageBox::warning(this, tr("Could Not connect To Remote Database"), tr("Could Not Share Cards Right Now!"));
   chkShareCards->setChecked(false);
   txtRemoteDatabaseName->clear();
   txtRemoteDatabaseName->setDisabled(true);
}

void AppSettingsWidget::onSendCollectionReportClicked()
{
  if (chkSendCollectionReport->isChecked())
  {
    txtCollectionReportRecipients->setEnabled(true);
    formLayout->labelForField(txtCollectionReportRecipients)->setEnabled(true);
  }
  else
  {
    txtCollectionReportRecipients->setEnabled(false);
    formLayout->labelForField(txtCollectionReportRecipients)->setEnabled(false);
  }
}

void AppSettingsWidget::onSendCollectionSnapshotReportClicked()
{
  if (chkSendCollectionSnapshotReport->isChecked())
  {
    txtCollectionSnapshotReportRecipients->setEnabled(true);
    formLayout->labelForField(txtCollectionSnapshotReportRecipients)->setEnabled(true);

    spnCollectionSnapshotInterval->setEnabled(true);
    formLayout->labelForField(spnCollectionSnapshotInterval)->setEnabled(true);

    dtFirstCollectionDate->setEnabled(true);
    formLayout->labelForField(dtFirstCollectionDate)->setEnabled(true);
  }
  else
  {
    txtCollectionSnapshotReportRecipients->setEnabled(false);
    formLayout->labelForField(txtCollectionSnapshotReportRecipients)->setEnabled(false);

    spnCollectionSnapshotInterval->setEnabled(false);
    formLayout->labelForField(spnCollectionSnapshotInterval)->setEnabled(false);

    dtFirstCollectionDate->setEnabled(false);
    formLayout->labelForField(dtFirstCollectionDate)->setEnabled(false);
  }
}

void AppSettingsWidget::onSendDailyMeterReportClicked()
{
  if (chkSendDailyMeterReport->isChecked())
  {
    txtDailyMeterReportRecipients->setEnabled(true);
    dailyMeterSendTime->setEnabled(true);
    formLayout->labelForField(txtDailyMeterReportRecipients)->setEnabled(true);
    formLayout->labelForField(dailyMeterSendTime)->setEnabled(true);
  }
  else
  {
    txtDailyMeterReportRecipients->setEnabled(false);
    dailyMeterSendTime->setEnabled(false);
    formLayout->labelForField(txtDailyMeterReportRecipients)->setEnabled(false);
    formLayout->labelForField(dailyMeterSendTime)->setEnabled(false);
  }
}

void AppSettingsWidget::onAppModeChanged(int index)
{
  Q_UNUSED(index)

  if (cmbAppModes->itemData(cmbAppModes->currentIndex()).toInt() != Settings::DVD_Copying_Only)
  {
      formLayout->labelForField(spnMinimumBoothAddress)->setEnabled(true);
      formLayout->labelForField(spnMaximumBoothAddress)->setEnabled(true);
      spnMinimumBoothAddress->setEnabled(true);
      spnMaximumBoothAddress->setEnabled(true);

      formLayout->labelForField(chkSendCollectionReport)->setEnabled(true);
      chkSendCollectionReport->setEnabled(true);

      formLayout->labelForField(chkSendDailyMeterReport)->setEnabled(true);
      chkSendDailyMeterReport->setEnabled(true);
  }
  else
  {
      formLayout->labelForField(spnMinimumBoothAddress)->setEnabled(false);
      formLayout->labelForField(spnMaximumBoothAddress)->setEnabled(false);
      spnMinimumBoothAddress->setEnabled(false);
      spnMaximumBoothAddress->setEnabled(false);

      if (chkSendCollectionReport->isChecked())
      {
        chkSendCollectionReport->setChecked(false);
        onSendCollectionReportClicked();
      }

      formLayout->labelForField(chkSendCollectionReport)->setEnabled(false);
      chkSendCollectionReport->setEnabled(false);

      if (chkSendDailyMeterReport->isChecked())
      {
        chkSendDailyMeterReport->setChecked(false);
        onSendDailyMeterReportClicked();
      }

      formLayout->labelForField(chkSendDailyMeterReport)->setEnabled(false);
      chkSendDailyMeterReport->setEnabled(false);
  }
}

bool AppSettingsWidget::validateInput()
{
  QStringList errors;

  if (cmbAppModes->itemData(cmbAppModes->currentIndex()).toInt() != Settings::DVD_Copying_Only)
  {
    if (spnMinimumBoothAddress->value() > spnMaximumBoothAddress->value())
      errors.append(tr("- First Booth Address must be less than or equal to Last Booth Address."));
  }

  txtStoreName->setText(txtStoreName->text().trimmed());
  if (txtStoreName->text().length() == 0)
    errors.append(tr("- Specify a store name."));

  if (chkSendCollectionReport->isChecked())
  {
    txtCollectionReportRecipients->setText(txtCollectionReportRecipients->text().trimmed());

    if (txtCollectionReportRecipients->text().length() == 0)
      errors.append(tr("- Provide one or more email addresses to send the Collection Report to. When entering multiple email addresses, separate each with a comma."));
    else
    {
      QStringList emailAddresses = txtCollectionReportRecipients->text().split(",", QString::SkipEmptyParts);
      QStringList invalidAddresses;
      QStringList validAddresses;

      foreach (QString address, emailAddresses)
      {
        address = address.trimmed();

        if (!Global::isValidEmail(address))
          invalidAddresses.append(address);
        else
          validAddresses.append(address);
      }

      if (invalidAddresses.length() > 0)
      {
        errors.append(tr("- The following email address(s) in the Collection Report Recipients field are invalid: %1").arg(invalidAddresses.join("\n")));
      }
      else
      {
        // Replace current text with trimmed addresses
        txtCollectionReportRecipients->setText(validAddresses.join(","));
      }
    }
  }

  if (chkSendDailyMeterReport->isChecked())
  {
    txtDailyMeterReportRecipients->setText(txtDailyMeterReportRecipients->text().trimmed());

    if (txtDailyMeterReportRecipients->text().length() == 0)
      errors.append(tr("- Provide one or more email addresses to send the Daily Meter Report to. When entering multiple email addresses, separate each with a comma."));
    else
    {
      QStringList emailAddresses = txtDailyMeterReportRecipients->text().split(",", QString::SkipEmptyParts);
      QStringList invalidAddresses;
      QStringList validAddresses;

      foreach (QString address, emailAddresses)
      {
        address = address.trimmed();

        if (!Global::isValidEmail(address))
          invalidAddresses.append(address);
        else
          validAddresses.append(address);
      }

      if (invalidAddresses.length() > 0)
      {
        errors.append(tr("- The following email address(s) in the Daily Meter Report Recipients field are invalid: %1").arg(invalidAddresses.join("\n")));
      }
      else
      {
        // Replace current text with trimmed addresses
        txtDailyMeterReportRecipients->setText(validAddresses.join(","));
      }
    }
  }

  if (chkSendCollectionSnapshotReport->isChecked())
  {
    txtCollectionSnapshotReportRecipients->setText(txtCollectionSnapshotReportRecipients->text().trimmed());

    if (txtCollectionSnapshotReportRecipients->text().length() == 0)
      errors.append(tr("- Provide one or more email addresses to send the Collection Snapshot Report to. When entering multiple email addresses, separate each with a comma."));
    else
    {
      QStringList emailAddresses = txtCollectionSnapshotReportRecipients->text().split(",", QString::SkipEmptyParts);
      QStringList invalidAddresses;
      QStringList validAddresses;

      foreach (QString address, emailAddresses)
      {
        address = address.trimmed();

        if (!Global::isValidEmail(address))
          invalidAddresses.append(address);
        else
          validAddresses.append(address);
      }

      if (invalidAddresses.length() > 0)
      {
        errors.append(tr("- The following email address(s) in the Collection Snapshot Report Recipients field are invalid: %1").arg(invalidAddresses.join("\n")));
      }
      else
      {
        // Replace current text with trimmed addresses
        txtCollectionSnapshotReportRecipients->setText(validAddresses.join(","));
      }
    }

    if (!dtFirstCollectionDate->date().isValid())
    {
      errors.append(tr("- Set a valid date for the first collection snapshot report."));
    }
  }

 /* txtDataPath->setText(txtDataPath->text().trimmed());
  if (txtDataPath->text().length() > 0)
  {
    QDir dir(txtDataPath->text());
    if (!dir.exists())
    {
      if (!dir.mkpath(txtDataPath->text()))
        errors.append(tr("- Data Path does not exist and/or could not be created."));
    }
  }
  else
    errors.append(tr("- Data Path cannot be empty."));

  txtVideoPath->setText(txtVideoPath->text().trimmed());
  if (txtVideoPath->text().length() > 0)
  {
    QDir dir(txtVideoPath->text());
    if (!dir.exists())
    {
      if (!dir.mkpath(txtVideoPath->text()))
        errors.append(tr("- Video Path does not exist and/or could not be created."));
    }
  }
  else
    errors.append(tr("- Video Path cannot be empty."));

  txtDvdCopyingDrive->setText(txtDvdCopyingDrive->text().trimmed().toUpper());
  if (txtDvdCopyingDrive->text().length() > 0)
  {
    // Drive letter should be D: - Z: and end with colon
    if (txtDvdCopyingDrive->text().length() == 2)
    {
      if (txtDvdCopyingDrive->text()[0] < QChar('D') || txtDvdCopyingDrive->text()[0] > QChar('Z'))
        errors.append(tr("- DVD Copy Drive must be D: through Z:."));
      else if (txtDvdCopyingDrive->text()[1] != QChar(':'))
        errors.append(tr("- DVD Copy Drive must end with a colon."));
    }
    else
      errors.append(tr("- DVD Copy Drive must be D: through Z:."));
  }
  else
    errors.append(tr("- DVD Copy Drive cannot be empty."));

  txtDvdCopyingDestPath->setText(txtDvdCopyingDestPath->text().trimmed());
  if (txtDvdCopyingDestPath->text().length() == 0)
    errors.append(tr("- DVD Copy Path cannot be empty."));

  txtMetadataPath->setText(txtMetadataPath->text().trimmed());
  if (txtMetadataPath->text().length() > 0)
  {
    QDir dir(txtMetadataPath->text());
    if (!dir.exists())
    {
      if (!dir.mkpath(txtMetadataPath->text()))
        errors.append(tr("- Metadata Download Path does not exist and/or could not be created."));
    }
  }
  else
    errors.append(tr("- Metadata Download Path cannot be empty."));  

  txtDvdCopyProg->setText(txtDvdCopyProg->text().trimmed());
  QFile file(txtDvdCopyProg->text());
  if (!file.exists())
    errors.append(tr("- DVD Copy Program does not exist."));

  txtCustomerPassword->setText(txtCustomerPassword->text().trimmed());
  txtVideoLookupURL->setText(txtVideoLookupURL->text().trimmed());
  txtVideoUpdateURL->setText(txtVideoUpdateURL->text().trimmed());
  txtViewTimesServerURL->setText(txtViewTimesServerURL->text().trimmed());
*/


  if (errors.count() > 0)
  {
    QMessageBox::warning(this, tr("Invalid"), tr("Please correct the following problems:\n\n%1").arg(errors.join("\n")));
    return false;
  }
  else
    return true;
}
