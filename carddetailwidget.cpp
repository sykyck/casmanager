#include "carddetailwidget.h"
#include "qslog/QsLog.h"
#include <QMessageBox>
#include <math.h>

CardDetailWidget::CardDetailWidget(Settings *settings, QWidget *parent) : QDialog(parent)
{
  // Disable maximize,minimize and resizing of window
  this->setFixedSize(320, 200);
  this->settings = settings;

  editMode = ADD;

  lblCardNum = new QLabel("Card #");
  lblValue = new QLabel("Value");
  lblCardType = new QLabel("Type");

  txtCardNum = new QLineEdit;
  txtCardNum->setMaxLength(512);
  txtCardNum->installEventFilter(this);

  spnValue = new QDoubleSpinBox;
  spnValue->setMinimum(0);
  spnValue->setMaximum(999.00);
  spnValue->setDecimals(2);
  spnValue->setSingleStep(0.25);
  spnValue->setPrefix("$");
  spnValue->setValue(0);

  cmbCardType = new QComboBox;
  foreach (QVariant item, getCardTypes())
  {
    QVariantMap keyValue = item.toMap();

    foreach (QString key, keyValue.keys())
    {
      cmbCardType->addItem(key, keyValue[key]);
    }
  }

  btnSave = new QPushButton(tr("Save"));
  btnCancel = new QPushButton(tr("Cancel"));
  btnSave->setDefault(false);
  btnSave->setAutoDefault(false);
  btnCancel->setDefault(false);
  btnCancel->setAutoDefault(false);
  connect(btnSave, SIGNAL(clicked()), this, SLOT(accept()));
  connect(btnCancel, SIGNAL(clicked()), this, SLOT(reject()));

  btnGenerateCardNum = new QToolButton;
  btnGenerateCardNum->setToolButtonStyle(Qt::ToolButtonTextOnly);
  btnGenerateCardNum->setText("Random");

  connect(btnGenerateCardNum, SIGNAL(clicked()), this, SLOT(generateCardNumClicked()));


  buttonLayout = new QHBoxLayout;
  frontCoverButtonLayout = new QHBoxLayout;
  backCoverButtonLayout = new QHBoxLayout;

  gridLayout = new QGridLayout;
  gridLayout->addWidget(lblCardNum, 0, 0);
  gridLayout->addWidget(txtCardNum, 0, 1, 1, 2);
  gridLayout->addWidget(btnGenerateCardNum, 0, 3);

  gridLayout->addWidget(lblValue, 1, 0);
  gridLayout->addWidget(spnValue, 1, 1, 1, 3);

  gridLayout->addWidget(lblCardType, 2, 0);
  gridLayout->addWidget(cmbCardType, 2, 1, 1, 3);

  buttonLayout->addWidget(btnSave);
  buttonLayout->addWidget(btnCancel);
  gridLayout->addLayout(buttonLayout, 4, 0, 1, 4, Qt::AlignCenter);

  this->setLayout(gridLayout);

  setEditMode(editMode);
}

ArcadeCard CardDetailWidget::getCard()
{
  return card;
}

bool CardDetailWidget::eventFilter(QObject *obj, QEvent *event)
{
  // If TAB key pressed while in the card number field then ignore
  if (event->type() == QEvent::KeyPress)
  {
    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

    if (keyEvent->key() == Qt::Key_Tab)
    {
      txtCardNum->setText(txtCardNum->text() + "\t");
      event->accept();
      return true;
    }
    else
    {
      // standard event processing
      return QWidget::eventFilter(obj, event);
    }
  }
  else
  {
    // standard event processing
    return QWidget::eventFilter(obj, event);
  }
}

void CardDetailWidget::showEvent(QShowEvent *)
{
  if (editMode == ADD)
  {
    txtCardNum->setFocus();
    txtCardNum->selectAll();
  }
  else
  {
    spnValue->setFocus();
    spnValue->selectAll();
  }
}

void CardDetailWidget::generateCardNumClicked()
{
  txtCardNum->setText(generateCardNum());
}

void CardDetailWidget::clearForm()
{
  txtCardNum->clear();
  spnValue->setValue(spnValue->minimum());

  if (cmbCardType->count() > 0)
    cmbCardType->setCurrentIndex(0);
}

QVariantList CardDetailWidget::getCardTypes()
{
  QVariantList cardTypeList;

  const QMetaObject &mo = ArcadeCard::staticMetaObject;
  int index = mo.indexOfEnumerator("CvsCardTypes");
  QMetaEnum metaEnum = mo.enumerator(index);

  // Build list of all card types in the CvsCardTypes enumerator
  for (int i = 0; i < metaEnum.keyCount(); i++)
  {
    if (metaEnum.key(i))
    {
      // Key = enum string name, value = enum value
      QVariantMap keyValue;
      keyValue[QString(metaEnum.key(i))] = metaEnum.value(i);
      cardTypeList.append(keyValue);
    }
  }

  return cardTypeList;
}

QString CardDetailWidget::generateCardNum()
{
  // TODO: Move max value and number length to settings
  return QString("%1").arg(qrand() % 1000000, 6, 10, QChar('0'));
}

void CardDetailWidget::accept()
{
  QString errors = isValidInput();

  //dataChanged = false;
  if (errors.isEmpty())
  {
    card.setCardNum(txtCardNum->text());
    card.setCredits(spnValue->value() * 4);
    card.setMadeBy("CAS Manager");
    card.setCardType(ArcadeCard::CvsCardTypes(cmbCardType->itemData(cmbCardType->currentIndex()).toInt()));

    QDialog::accept();
  }
  else
  {
    QLOG_DEBUG() << QString("User tried to submit invalid data for card. Error: %1").arg(errors);
    QMessageBox::warning(this, tr("Invalid"), tr("Correct the following problems:\n%1").arg(errors));
  }
}

void CardDetailWidget::reject()
{
  bool continueReject = true;

  if (dataChanged() && editMode == UPDATE)
  {
    if (QMessageBox::question(this, tr("Changes made"), tr("Changes were made to the card, are you sure you want to lose these?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
      continueReject = false;
  }

  if (continueReject)
  {
    QLOG_DEBUG() << QString("User canceled adding/updating card");
    QDialog::reject();
  }
}

void CardDetailWidget::populateForm(ArcadeCard &card)
{
  this->card = card;

  txtCardNum->setText(card.getCardNum());
  spnValue->setValue(card.getCashValue());

  // Set current card type selected in combo box
  for (int i = 0; i < cmbCardType->count(); i++)
  {
    if (cmbCardType->itemData(i).toInt() == int(card.getCardType()))
    {
      cmbCardType->setCurrentIndex(i);
      break;
    }
  }
}

void CardDetailWidget::setEditMode(CardDetailWidget::EditMode mode)
{
  editMode = mode;

  if (editMode == ADD)
  {
    this->setWindowTitle(tr("New Card"));
    txtCardNum->setReadOnly(false);
    txtCardNum->setEnabled(true);
    btnGenerateCardNum->setEnabled(true);
  }
  else if (editMode == UPDATE)
  {
    this->setWindowTitle(tr("Update Card"));
    txtCardNum->setReadOnly(true);
    txtCardNum->setEnabled(false);
    btnGenerateCardNum->setEnabled(false);
  }
}

QString CardDetailWidget::isValidInput()
{
  QLOG_DEBUG() << QString("Validating card num: %1").arg(txtCardNum->text());

  QStringList errorList;

  // If the string format looks like it came from the card reader then extract the card data and replace what's in the field
  QString cardNum = txtCardNum->text().trimmed();
  if (cardNum.lastIndexOf("\t") > -1 && cardNum.lastIndexOf("\t") < cardNum.length() - 1 &&
      cardNum.at(cardNum.lastIndexOf("\t") + 1) == QChar('%') &&
      cardNum.indexOf("?") > -1)
  {
    cardNum = cardNum.mid(cardNum.lastIndexOf("\t") + 1).remove(QRegExp("[^a-zA-Z\\d\\s]"));
  }

  txtCardNum->setText(cardNum);

  QLOG_DEBUG() << QString("After cleaning up card num: %1").arg(txtCardNum->text());

  // Card # must be 4 - 12 characters
  // TODO: Move these to settings
  if (cardNum.length() < 4 || cardNum.length() > 12)
  {
    errorList.append(QString("- Card # must be between 4 - 12 characters."));
    txtCardNum->setFocus();
    txtCardNum->selectAll();

    // Verify the card # only contains digits
    //QRegExp re("\\d*");  // a digit (\d), zero or more times (*)
    /*if (!re.exactMatch(cardNum))
    {
      errorList.append("- Card # can only contain numbers.");
      txtCardNum->setFocus();
      txtCardNum->selectAll();
    }*/
  }
  /*else
  {
    // TODO: Move card # length to settings
    errorList.append(QString("- Card # must be %1 digits.").arg(6));
    txtCardNum->setFocus();
    txtCardNum->selectAll();
  }*/

  qreal cardValue = spnValue->value();

  qreal fractPart, intPart;

  // Card value must be divisible by $0.25
  fractPart = modf(cardValue * 4, &intPart);

  if (fractPart != 0)
    errorList.append("- Card value must be rounded to the nearest quarter (divisible by $0.25).");
  else if (cardValue > 0)
  {
    // if card type is TECH and card value greater than zero then warn user that a card value cannot be set for tech cards
    if (ArcadeCard::CvsCardTypes(cmbCardType->itemData(cmbCardType->currentIndex()).toInt()) == ArcadeCard::TECH)
    {
      errorList.append("- Cannot set card value when card type is TECH. Set card value to $0.");
    }
  }

  return errorList.join("\n");
}

bool CardDetailWidget::dataChanged()
{
  return (card.getCardNum() != txtCardNum->text() ||
          card.getCashValue() != spnValue->value() ||
          card.getCardType() != ArcadeCard::CvsCardTypes(cmbCardType->itemData(cmbCardType->currentIndex()).toInt()));
}
