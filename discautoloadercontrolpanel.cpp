#include "discautoloadercontrolpanel.h"
#include <QMessageBox>

DiscAutoloaderControlPanel::DiscAutoloaderControlPanel(Settings *settings, QWidget *parent) : QDialog(parent)
{
  discAutoloader = new DiscAutoLoader(settings->getValue(AUTOLOADER_DVD_DRIVE_MOUNT).toString(),
                                      settings->getValue(AUTOLOADER_PROG_FILE).toString(),
                                      settings->getValue(AUTOLOADER_RESPONSE_FILE).toString(),
                                      settings->getValue(AUTOLOADER_DVD_DRIVE_STATUS_FILE).toString(),
                                      settings->getValue(DVD_MOUNT_TIMEOUT).toInt());
  connect(discAutoloader, SIGNAL(finishedLoadingDisc(bool,QString)), this, SLOT(loadDiscFinished(bool,QString)));
  connect(discAutoloader, SIGNAL(finishedUnloadingDisc(bool,QString)), this, SLOT(unloadDiscFinished(bool,QString)));

  lblInstructions = new QLabel(tr("Provides direct control over the disc autoloader for testing the various functions.\n\nThe Load Disc button will open the DVD drive tray, puts a disc in the tray and closes it.\n\nThe Unload Disc button will open the DVD drive tray, pick the disc out of the tray, close the DVD drive tray and then drops the disc out the front chute.\n\nThe Reject Disc is the same as Unload Disc except that the disc is dropped down a different chute under the autoloader.\n\n"));
  lblInstructions->setWordWrap(true);

  btnLoadDisc = new QPushButton(tr("Load Disc"));
  connect(btnLoadDisc, SIGNAL(clicked()), this, SLOT(loadDiscClicked()));

  btnUnloadDisc = new QPushButton(tr("Unload Disc"));
  connect(btnUnloadDisc, SIGNAL(clicked()), this, SLOT(unloadDiscClicked()));

  btnRejectDisc = new QPushButton(tr("Reject Disc"));
  connect(btnRejectDisc, SIGNAL(clicked()), this, SLOT(rejectDiscCicked()));

  btnClose = new QPushButton(tr("Close Window"));
  connect(btnClose, SIGNAL(clicked()), this, SLOT(accept()));

  buttonLayout = new QHBoxLayout;
  buttonLayout->addWidget(btnLoadDisc, 0, Qt::AlignCenter);
  buttonLayout->addWidget(btnUnloadDisc, 0, Qt::AlignCenter);
  buttonLayout->addWidget(btnRejectDisc, 0, Qt::AlignCenter);
  buttonLayout->addWidget(btnClose, 0, Qt::AlignCenter);

  formLayout = new QFormLayout;
  formLayout->addWidget(lblInstructions);
  formLayout->addItem(buttonLayout);

  this->setWindowTitle(tr("Disc Autoloader Control Panel"));
  this->setLayout(formLayout);
  // Disable maximize,minimize and resizing of window
  this->setFixedSize(this->sizeHint());
}

DiscAutoloaderControlPanel::~DiscAutoloaderControlPanel()
{
  discAutoloader->deleteLater();
}

void DiscAutoloaderControlPanel::loadDiscClicked()
{
  discAutoloader->loadDisc();

  this->setEnabled(false);
}

void DiscAutoloaderControlPanel::unloadDiscClicked()
{
  discAutoloader->unloadDisc();

  this->setEnabled(false);
}

void DiscAutoloaderControlPanel::rejectDiscCicked()
{
  discAutoloader->unloadDisc(true);

  this->setEnabled(false);
}

void DiscAutoloaderControlPanel::loadDiscFinished(bool success, QString message)
{
  this->setEnabled(true);

  if (success)
    QMessageBox::information(this, "Success", "Disc loaded successfully.");
  else
    QMessageBox::warning(this, "Error", message);
}

void DiscAutoloaderControlPanel::unloadDiscFinished(bool success, QString message)
{
  this->setEnabled(true);

  if (success)
    QMessageBox::information(this, "Success", "Disc unloaded/rejected successfully.");
  else
    QMessageBox::warning(this, "Error", message);
}
