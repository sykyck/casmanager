#ifndef CCCHARGERESULT_H
#define CCCHARGERESULT_H

#include <QString>

class CcChargeResult
{
public:
  CcChargeResult();
  CcChargeResult(const CcChargeResult &other);
  CcChargeResult(bool charged, qreal amount, QString errorMessage, QString authCode, QString customerCode, QString orderNum, QString seqNum, QString maskedCardNum);

  bool isCharged() const;
  void setCharged(bool charged);

  QString getErrorMessage() const;
  void setErrorMessage(QString errorMessage);

  QString getAuthCode() const;
  void setAuthCode(QString authCode);

  QString getCustomerCode() const;
  void setCustomerCode(QString customerCode);

  QString getOrderNum() const;
  void setOrderNum(QString orderNum);

  QString getSeqNum() const;
  void setSeqNum(QString seqNum);

  QString getMaskedCardNum() const;
  void setMaskedCardNum(QString maskedCardNum);

  qreal getAmount() const;
  void setAmount(const qreal &value);

  QString TransactionID() const;
  void setTransactionID(const QString &value);

  QString CardType() const;
  void setCardType(const QString &value);

private:
  // Set if the credit card was successfully charged
  bool charged;

  // Amount that was charged to credit card
  qreal amount;

  // Error message describing why the card could not be charged
  QString errorMessage;

  QString authCode;
  QString customerCode;
  QString orderNum;
  QString seqNum;
  QString maskedCardNum;
  QString transactionID;
  QString cardType;
};

#endif // CCCHARGERESULT_H
