#include "paymentgatewaywidget.h"
#include "qslog/QsLog.h"
#include <QMessageBox>

const QString AUTHORIZE_NET_URL = "https://secure2.authorize.net/gateway/transact.dll";
const QString TEST_MAGSTRIPE_DATA = "%B4111111111111111^DOE/JOHN           ^400550506070202203000110163000000?;4111111111111111=4005010087012181?";
const qreal CHARGE_AMOUNT = 0.01;

PaymentGatewayWidget::PaymentGatewayWidget(QWidget *parent) : QDialog(parent)
{
  this->setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint);

  authorizeNetService = 0;

  formLayout = new QFormLayout;
  containerLayout = new QVBoxLayout;
  buttonLayout = new QDialogButtonBox;

  lblDirections = new QLabel(tr("Select the payment gateway that you have an account with and enter your credentials. Click the Test Settings button to verify the settings are correct."));
  lblDirections->setWordWrap(true);

  paymentGateways = new QComboBox;  
  paymentGateways->addItem("Authorize.net");
  connect(paymentGateways, SIGNAL(currentIndexChanged(int)), this, SLOT(onPaymentGatewayChanged(int)));

  marketTypes = new QComboBox;
  marketTypes->setToolTip(tr("If your merchant account type is Card Present then this must be set to Retail."));
  connect(marketTypes, SIGNAL(currentIndexChanged(int)), this, SLOT(onMarketTypeChanged(int)));

  deviceTypes = new QComboBox;
  deviceTypes->setToolTip(tr("If your merchant account type is Card Present then the recommended device type is Self Service Terminal."));
  connect(deviceTypes, SIGNAL(currentIndexChanged(int)), this, SLOT(onDeviceTypeChanged(int)));

  txtUsername = new QLineEdit;
  txtUsername->setMaxLength(128);
  txtUsername->setEchoMode(QLineEdit::PasswordEchoOnEdit);

  txtPassword = new QLineEdit;
  txtPassword->setMaxLength(128);
  txtPassword->setEchoMode(QLineEdit::PasswordEchoOnEdit);

  btnTestSettings = new QPushButton(tr("Test Settings"));
  connect(btnTestSettings, SIGNAL(clicked()), this, SLOT(onTestSettingsButtonClicked()));

  btnOK = new QPushButton(tr("OK"));
  connect(btnOK, SIGNAL(clicked()), this, SLOT(onOkButtonClicked()));

  btnCancel = new QPushButton(tr("Cancel"));
  connect(btnCancel, SIGNAL(clicked()), this, SLOT(reject()));

  buttonLayout->addButton(btnOK, QDialogButtonBox::AcceptRole);
  buttonLayout->addButton(btnCancel, QDialogButtonBox::RejectRole);

  formLayout->addRow(tr("Gateway"), paymentGateways);
  formLayout->addRow(tr("Username"), txtUsername);
  formLayout->addRow(tr("Password"), txtPassword);
  formLayout->addRow(tr("Market Type"), marketTypes);
  formLayout->addRow(tr("Device Type"), deviceTypes);

  containerLayout->addWidget(lblDirections);
  containerLayout->addLayout(formLayout);
  containerLayout->addWidget(btnTestSettings);
  containerLayout->addWidget(buttonLayout);

  this->setLayout(containerLayout);

  this->setWindowTitle(tr("Payment Gateway Setup"));
}

PaymentGatewayWidget::~PaymentGatewayWidget()
{
  if (authorizeNetService)
    authorizeNetService->deleteLater();
}

void PaymentGatewayWidget::showEvent(QShowEvent *)
{
  onPaymentGatewayChanged(0);

  this->setFixedSize(this->size());
}

QString PaymentGatewayWidget::getPaymentGateway()
{
  return paymentGateways->currentText();
}

QString PaymentGatewayWidget::getUsername()
{
  return txtUsername->text();
}

QString PaymentGatewayWidget::getPassword()
{
  return txtPassword->text();
}

qint8 PaymentGatewayWidget::getAuthorizeNetMarketType()
{
  return marketTypes->currentIndex();
}

qint8 PaymentGatewayWidget::getAuthorizeNetDeviceType()
{
  return deviceTypes->currentIndex();
}

void PaymentGatewayWidget::setUsername(QString username)
{
  txtUsername->setText(username);
}

void PaymentGatewayWidget::setPassword(QString password)
{
  txtPassword->setText(password);
}

void PaymentGatewayWidget::setPaymentGateway(QString gatewayName)
{
  bool foundGateway = false;

  for (int i = 0; i < paymentGateways->count(); ++i)
  {
    if (paymentGateways->itemText(i) == gatewayName)
    {
      paymentGateways->setCurrentIndex(i);
      foundGateway = true;
      break;
    }
  }

  if (!foundGateway)
    paymentGateways->setCurrentIndex(-1);
}

void PaymentGatewayWidget::setAuthorizeNetMarketType(qint8 marketType)
{
  if (marketType >= 0 && marketType < marketTypes->count())
    marketTypes->setCurrentIndex(marketType);
  else
    marketTypes->setCurrentIndex(-1);
}

void PaymentGatewayWidget::setAuthorizeNetDeviceType(qint8 deviceType)
{
  if (deviceType >= 0 && deviceType < deviceTypes->count())
    deviceTypes->setCurrentIndex(deviceType);
  else
    deviceTypes->setCurrentIndex(-1);
}

void PaymentGatewayWidget::setAuthorizeNetMarketTypes(QStringList marketTypeOptions)
{
  marketTypes->clear();
  marketTypes->addItems(marketTypeOptions);
}

void PaymentGatewayWidget::setAuthorizeNetDeviceTypes(QStringList deviceTypeOptions)
{
  deviceTypes->clear();
  deviceTypes->addItems(deviceTypeOptions);
}

void PaymentGatewayWidget::onPaymentGatewayChanged(int index)
{
  Q_UNUSED(index)

  if (paymentGateways->currentText() == "Authorize.net")
  {
    // change both fields to password fields to hide values
    txtUsername->setEchoMode(QLineEdit::PasswordEchoOnEdit);
    txtPassword->setEchoMode(QLineEdit::PasswordEchoOnEdit);

    ((QLabel *)formLayout->labelForField(txtUsername))->setText("API Login ID");
    ((QLabel *)formLayout->labelForField(txtPassword))->setText("Transaction Key");

    marketTypes->setEnabled(true);
    deviceTypes->setEnabled(true);
  }
}

void PaymentGatewayWidget::onMarketTypeChanged(int index)
{
  Q_UNUSED(index)
}

void PaymentGatewayWidget::onDeviceTypeChanged(int index)
{
  Q_UNUSED(index)
}

void PaymentGatewayWidget::onTestSettingsButtonClicked()
{
  QLOG_DEBUG() << QString("User clicked Test Settings button");

  if (isValid())
  {
    // Contact gateway and show response
    if (paymentGateways->currentText() == "Authorize.net")
    {
      QLOG_DEBUG() << QString("Now testing payment gateway: %1").arg(paymentGateways->currentText());

      if (authorizeNetService)
      {
        authorizeNetService->deleteLater();
        disconnect(authorizeNetService, 0, 0, 0);
      }

      authorizeNetService = new AuthorizeNetService(AUTHORIZE_NET_URL, txtUsername->text(), txtPassword->text());
      authorizeNetService->setMarketType(marketTypes->currentIndex());
      authorizeNetService->setDeviceType(deviceTypes->currentIndex());

      connect(authorizeNetService, SIGNAL(finishedCharge(CcChargeResult)), this, SLOT(onTestFinished(CcChargeResult)));

      this->setEnabled(false);

      authorizeNetService->charge(TEST_MAGSTRIPE_DATA, CHARGE_AMOUNT, true);
    }
  }
  else
  {
    QLOG_DEBUG() << QString("User did not provide valid input");
  }
}

void PaymentGatewayWidget::onOkButtonClicked()
{
  QLOG_DEBUG() << QString("User clicked OK button");

  if (isValid())
  {
    QMessageBox::information(this, this->windowTitle(), tr("If you've changed these settings, make sure to click the Save button in the Device Settings tab so all booths get updated."));

    accept();
  }
}

void PaymentGatewayWidget::onTestFinished(CcChargeResult result)
{
  if (result.isCharged())
  {
    QLOG_DEBUG() << QString("Payment gateway success: %1").arg(result.getErrorMessage());
    QMessageBox::information(this, tr("Test Settings"), tr("Successfully authenticated with payment gateway."));
  }
  else
  {
    QLOG_ERROR() << QString("Payment gateway error: %1").arg(result.getErrorMessage());
    QMessageBox::warning(this, tr("Test Settings"), tr("Error: %1").arg(result.getErrorMessage()));
  }

  this->setEnabled(true);
}

bool PaymentGatewayWidget::isValid()
{
  QString errorMessage = "";

  if (paymentGateways->currentIndex() < 0)
    errorMessage.append("- Select a payment gateway.\n");

  if (txtUsername->text().length() < 2)
    errorMessage.append(tr("- %1 cannot be blank.\n").arg(((QLabel *)formLayout->labelForField(txtUsername))->text()));

  if (txtPassword->text().length() < 2)
    errorMessage.append(tr("- %1 cannot be blank.\n").arg(((QLabel *)formLayout->labelForField(txtPassword))->text()));

  if (!errorMessage.isEmpty())
  {
    QMessageBox::warning(this, this->windowTitle(), tr("Please correct the following problem(s):\n%1").arg(errorMessage));
    return false;
  }
  else
    return true;
}
