#include "messagewidget.h"

MessageWidget::MessageWidget(QString message,
                             QString title,
                             QString button1Label,
                             QString button2Label,
                             QString button3Label,
                             QString button4Label,
                             QString checkboxLabel,
                             int timeout,
                             QWidget *parent) : QDialog(parent)
{
  btn1 = 0;
  btn2 = 0;
  btn3 = 0;
  btn4 = 0;
  chk1 = 0;
  timer = 0;

  this->setFixedSize(400, 300);

  verticalLayout = new QVBoxLayout;
  buttonBox = new QDialogButtonBox;

  lblMessage = new QLabel(message);
  lblMessage->setWordWrap(true);

  if (!button1Label.isEmpty())
  {
    btn1 = buttonBox->addButton(QDialogButtonBox::Ok);
    connect(btn1, SIGNAL(clicked()), this, SIGNAL(button1Clicked()));
    connect(btn1, SIGNAL(clicked()), this, SLOT(close()));
  }

  if (!button2Label.isEmpty())
  {
    btn2 = buttonBox->addButton(QDialogButtonBox::Cancel);
    connect(btn2, SIGNAL(clicked()), this, SIGNAL(button2Clicked()));
    connect(btn2, SIGNAL(clicked()), this, SLOT(close()));
  }

  if (!button3Label.isEmpty())
  {
    btn3 = buttonBox->addButton(QDialogButtonBox::Close);
    connect(btn3, SIGNAL(clicked()), this, SIGNAL(button3Clicked()));
    connect(btn3, SIGNAL(clicked()), this, SLOT(close()));
  }

  if (!button4Label.isEmpty())
  {
    btn4 = buttonBox->addButton(QDialogButtonBox::No);
    connect(btn4, SIGNAL(clicked()), this, SIGNAL(button4Clicked()));
    connect(btn4, SIGNAL(clicked()), this, SLOT(close()));
  }

  if (!checkboxLabel.isEmpty())
  {
    chk1 = new QCheckBox(checkboxLabel);
    chk1->setChecked(false);
  }

  if (timeout > 0)
  {
    timer = new QTimer;
    timer->setInterval(1000);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateCountdown()));
  }

  verticalLayout->addWidget(lblMessage);
  verticalLayout->addStretch(2);
  verticalLayout->addWidget(buttonBox);

  if (chk1)
    verticalLayout->addWidget(chk1);

  this->setWindowTitle(title);
  this->setLayout(verticalLayout);
}

MessageWidget::~MessageWidget()
{
  if (timer)
  {
    timer->stop();
    timer->deleteLater();
  }
}

void MessageWidget::setChecked(bool checked)
{
  if (chk1)
  {
    chk1->setChecked(checked);
  }
}

bool MessageWidget::isChecked()
{
  if (chk1)
  {
    return chk1->isChecked();
  }
  else
    return false;
}

void MessageWidget::updateCountdown()
{
  // TODO: Update counter
  // If counter reaches zero then close widget
}
