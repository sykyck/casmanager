#include "ccchargeresult.h"

CcChargeResult::CcChargeResult()
{
  charged = false;
  amount = 0;
  errorMessage.clear();
  authCode.clear();
  customerCode.clear();
  orderNum.clear();
  seqNum.clear();
  maskedCardNum.clear();
  transactionID.clear();
  cardType.clear();
}

CcChargeResult::CcChargeResult(const CcChargeResult &other)
{
  this->charged = other.isCharged();
  this->amount = other.getAmount();
  this->errorMessage = other.getErrorMessage();
  this->authCode = other.getAuthCode();
  this->customerCode = other.getCustomerCode();
  this->orderNum = other.getOrderNum();
  this->seqNum = other.getSeqNum();
  this->maskedCardNum = other.getMaskedCardNum();
  this->transactionID = other.TransactionID();
  this->cardType = other.CardType();
}

CcChargeResult::CcChargeResult(bool charged, qreal amount, QString errorMessage, QString authCode, QString customerCode, QString orderNum, QString seqNum, QString maskedCardNum)
{
  this->charged = charged;
  this->amount = amount;
  this->errorMessage = errorMessage;
  this->authCode = authCode;
  this->customerCode = customerCode;
  this->orderNum = orderNum;
  this->seqNum = seqNum;
  this->maskedCardNum = maskedCardNum;
}

bool CcChargeResult::isCharged() const
{
  return charged;
}

void CcChargeResult::setCharged(bool charged)
{
  this->charged = charged;
}

QString CcChargeResult::getErrorMessage() const
{
  return errorMessage;
}

void CcChargeResult::setErrorMessage(QString errorMessage)
{
  this->errorMessage = errorMessage;
}

QString CcChargeResult::getAuthCode() const
{
  return authCode;
}

void CcChargeResult::setAuthCode(QString authCode)
{
  this->authCode = authCode;
}

QString CcChargeResult::getCustomerCode() const
{
  return customerCode;
}

void CcChargeResult::setCustomerCode(QString customerCode)
{
  this->customerCode = customerCode;
}

QString CcChargeResult::getOrderNum() const
{
  return orderNum;
}

void CcChargeResult::setOrderNum(QString orderNum)
{
  this->orderNum = orderNum;
}

QString CcChargeResult::getSeqNum() const
{
  return seqNum;
}

void CcChargeResult::setSeqNum(QString seqNum)
{
  this->seqNum = seqNum;
}

QString CcChargeResult::getMaskedCardNum() const
{
  return maskedCardNum;
}

void CcChargeResult::setMaskedCardNum(QString maskedCardNum)
{
  this->maskedCardNum = maskedCardNum;
}

qreal CcChargeResult::getAmount() const
{
    return amount;
}

void CcChargeResult::setAmount(const qreal &value)
{
    amount = value;
}

QString CcChargeResult::CardType() const
{
  return cardType;
}

void CcChargeResult::setCardType(const QString &value)
{
  cardType = value;
}

QString CcChargeResult::TransactionID() const
{
  return transactionID;
}

void CcChargeResult::setTransactionID(const QString &value)
{
  transactionID = value;
}
