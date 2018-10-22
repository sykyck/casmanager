#ifndef PAYMENTGATEWAYWIDGET_H
#define PAYMENTGATEWAYWIDGET_H

#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include "authorizenetservice.h"

class PaymentGatewayWidget : public QDialog
{
  Q_OBJECT
public:
  explicit PaymentGatewayWidget(QWidget *parent = 0);
  ~PaymentGatewayWidget();
  QString getPaymentGateway();
  QString getUsername();
  QString getPassword();
  qint8 getAuthorizeNetMarketType();
  qint8 getAuthorizeNetDeviceType();
  void setUsername(QString username);
  void setPassword(QString password);
  void setPaymentGateway(QString gatewayName);
  void setAuthorizeNetMarketType(qint8 marketType);
  void setAuthorizeNetDeviceType(qint8 deviceType);
  void setAuthorizeNetMarketTypes(QStringList marketTypeOptions);
  void setAuthorizeNetDeviceTypes(QStringList deviceTypeOptions);

protected:
  void showEvent(QShowEvent *);

signals:
  
public slots:

private slots:
  void onPaymentGatewayChanged(int index);
  void onMarketTypeChanged(int index);
  void onDeviceTypeChanged(int index);
  void onTestSettingsButtonClicked();
  void onOkButtonClicked();
  void onTestFinished(CcChargeResult result);
  
private:
  bool isValid();

  QFormLayout *formLayout;
  QVBoxLayout *containerLayout;
  QDialogButtonBox *buttonLayout;

  QLabel *lblDirections;
  QComboBox *paymentGateways;
  QLineEdit *txtUsername;
  QLineEdit *txtPassword;
  QComboBox *marketTypes;
  QComboBox *deviceTypes;

  QPushButton *btnTestSettings;
  QPushButton *btnOK;
  QPushButton *btnCancel;

  AuthorizeNetService *authorizeNetService;
};

#endif // PAYMENTGATEWAYWIDGET_H
